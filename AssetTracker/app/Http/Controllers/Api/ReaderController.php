<?php

namespace App\Http\Controllers\Api;

use App\Http\Controllers\Controller;
use App\Models\Reader;
use App\Models\Asset;
use App\Models\AssetLocationLog;
use Illuminate\Http\Request;
use Illuminate\Support\Facades\Log;
use Illuminate\Support\Facades\Validator;
use Illuminate\Support\Facades\DB;
use Carbon\Carbon;

class ReaderController extends Controller
{
    // Drastic change thresholds - can be moved to config later
    const RSSI_THRESHOLD = 10;
    const KALMAN_RSSI_THRESHOLD = 10;
    const DISTANCE_THRESHOLD = 2;
    const LOG_TIME_WINDOW = 300; // 5 minutes in seconds

    // Fetch reader configuration
    public function getConfig(Request $request)
    {
        $readerName = $request->input('reader_name');
        $reader = Reader::where('name', $readerName)->first();

        if (!$reader) {
            return response()->json(['error' => 'Reader not found'], 404);
        }

        if (!$reader->is_active) {
            return response()->json(['error' => 'Reader is inactive'], 403);
        }

        $version = $reader->updated_at->timestamp;

        // Get config - use reader config if exists, otherwise default
        $config = $reader->config ?? Reader::$defaultConfig;

        // Always add assetNamePattern from env
        $config['assetNamePattern'] = env('ASSET_NAME_PATTERN', 'Asset_');

        $response = [
            'name' => $reader->name,
            'discovery_mode' => $reader->discovery_mode ?? 'pattern',
            'config' => $config,
            'version' => $version,
        ];

        // Only include targets for explicit mode
        if ($reader->discovery_mode === 'explicit') {
            $targets = $reader->tags()->pluck('name')->toArray();
            $response['targets'] = $targets;
        }

        $reader->config_fetched_at = Carbon::now();
        $reader->save();

        return response()->json($response);
    }

    // Receive location logs from readers
    public function receiveLocationLog(Request $request)
    {
        $validator = Validator::make($request->all(), [
            'reader_name' => 'required|string|max:255',
            'device_name' => 'required|string|max:255',
            'type' => 'required|string|in:heartbeat,alert',
            'status' => 'required|string|in:present,not_found,out_of_range',
            'rssi' => 'nullable|numeric',
            'kalman_rssi' => 'nullable|numeric',
            'estimated_distance' => 'nullable|numeric|min:-1',
        ]);

        if ($validator->fails()) {
            return response()->json([
                'error' => 'Validation failed',
                'details' => $validator->errors()
            ], 400);
        }

        $reader = Reader::where('name', $request->reader_name)->first();
        if (!$reader) {
            return response()->json(['error' => 'Reader not found'], 404);
        }

        if (!$reader->is_active) {
            return response()->json(['error' => 'Reader is inactive'], 403);
        }

        $locationId = $reader->location_id;
        if (!$locationId) {
            Log::warning('Reader has no location assigned', ['reader_name' => $request->reader_name]);
            return response()->json(['error' => 'Reader location not configured'], 400);
        }

        // Look up asset
        $asset = Asset::whereHas('tag', function ($query) use ($request) {
            $query->where('name', $request->device_name);
        })->first();

        if (!$asset) {
            // Device not registered - just log and return
            Log::info('Unregistered device detected', [
                'device_name' => $request->device_name,
                'reader' => $request->reader_name,
            ]);

            return response()->json([
                'status' => 'warning',
                'message' => 'Device not registered'
            ], 200);
        }

        // Log the location data with de-duplication logic
        DB::beginTransaction();

        try {
            // Check for existing log within time window
            $existingLog = AssetLocationLog::where('asset_id', $asset->id)
                ->where('location_id', $locationId)
                ->where('status', $request->status)
                ->where('type', $request->type)
                ->where('updated_at', '>=', Carbon::now()->subSeconds(self::LOG_TIME_WINDOW))
                ->orderBy('updated_at', 'desc')
                ->first();

            $logData = [
                'asset_id' => $asset->id,
                'location_id' => $locationId,
                'rssi' => $request->rssi,
                'kalman_rssi' => $request->kalman_rssi,
                'estimated_distance' => $request->estimated_distance,
                'type' => $request->type,
                'status' => $request->status,
            ];

            $isDrasticChange = false;
            $locationLog = null;

            if ($existingLog) {
                if ($request->rssi !== null && $existingLog->rssi !== null) {
                    $rssiDiff = abs($request->rssi - $existingLog->rssi);
                    if ($rssiDiff > self::RSSI_THRESHOLD) {
                        $isDrasticChange = true;
                    }
                }

                if (!$isDrasticChange && $request->kalman_rssi !== null && $existingLog->kalman_rssi !== null) {
                    $kalmanDiff = abs($request->kalman_rssi - $existingLog->kalman_rssi);
                    if ($kalmanDiff > self::KALMAN_RSSI_THRESHOLD) {
                        $isDrasticChange = true;
                    }
                }

                if (!$isDrasticChange && $request->estimated_distance !== null && $existingLog->estimated_distance !== null) {
                    $distanceDiff = abs($request->estimated_distance - $existingLog->estimated_distance);
                    if ($distanceDiff > self::DISTANCE_THRESHOLD) {
                        $isDrasticChange = true;
                    }
                }

                $lastReaderName = $existingLog->reader_name ?? null;
                if ($lastReaderName && $lastReaderName !== $request->reader_name) {
                    $isDrasticChange = true;
                }

                if (!$isDrasticChange) {
                    $existingLog->update([
                        'rssi' => $request->rssi,
                        'kalman_rssi' => $request->kalman_rssi,
                        'estimated_distance' => $request->estimated_distance,
                        'updated_at' => Carbon::now(),
                    ]);
                    $locationLog = $existingLog;

                    Log::info('Updated existing location log', [
                        'log_id' => $existingLog->id,
                        'asset_id' => $asset->id,
                        'reader' => $request->reader_name,
                    ]);
                }
            }

            // Create new log if no existing log or drastic change detected
            if (!$existingLog || $isDrasticChange) {
                // Add reader_name to track which reader detected the asset
                $logData['reader_name'] = $request->reader_name;
                $locationLog = AssetLocationLog::create($logData);

                Log::info('Created new location log', [
                    'log_id' => $locationLog->id,
                    'asset_id' => $asset->id,
                    'reader' => $request->reader_name,
                    'drastic_change' => $isDrasticChange,
                ]);
            }

            // Update asset location if present
            if ($request->status === 'present' && $asset->location_id !== $locationId) {
                $asset->update(['location_id' => $locationId]);
            }

            DB::commit();

            // Log alerts
            if ($request->type === 'alert') {
                Log::warning('Asset Alert', [
                    'asset_id' => $asset->id,
                    'asset_name' => $asset->name,
                    'status' => $request->status,
                    'location_id' => $locationId,
                    'distance' => $request->estimated_distance,
                    'reader' => $request->reader_name,
                ]);
            }

            return response()->json([
                'status' => 'success',
                'log_id' => $locationLog->id,
                'action' => (!$existingLog || $isDrasticChange) ? 'created' : 'updated'
            ], 201);

        } catch (\Exception $e) {
            DB::rollback();

            Log::error('Failed to create/update location log', [
                'error' => $e->getMessage(),
                'reader_name' => $request->reader_name,
                'device_name' => $request->device_name
            ]);

            return response()->json([
                'error' => 'Failed to record log'
            ], 500);
        }
    }
}
