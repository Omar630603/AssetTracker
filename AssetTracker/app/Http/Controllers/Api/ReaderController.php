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
    public function receiveLocationLogs(Request $request)
    {
        // Validate the request
        $validator = Validator::make($request->all(), [
            'reader_name' => 'required|string|max:255',
            'devices' => 'required|array|min:1',
            'devices.*.device_name' => 'required|string|max:255',
            'devices.*.type' => 'required|string|in:heartbeat,alert',
            'devices.*.status' => 'required|string|in:present,not_found,out_of_range',
            'devices.*.rssi' => 'nullable|numeric',
            'devices.*.kalman_rssi' => 'nullable|numeric',
            'devices.*.estimated_distance' => 'nullable|numeric|min:-1',
        ]);

        if ($validator->fails()) {
            return response()->json([
                'error' => 'Validation failed',
                'details' => $validator->errors()
            ], 400);
        }

        // Look up the reader once for all devices
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

        $results = [];
        DB::beginTransaction();

        try {
            foreach ($request->devices as $deviceData) {
                // Find the asset by tag name
                $asset = Asset::whereHas('tag', function ($query) use ($deviceData) {
                    $query->where('name', $deviceData['device_name']);
                })->first();

                if (!$asset) {
                    Log::info('Unregistered device detected', [
                        'device_name' => $deviceData['device_name'],
                        'reader' => $request->reader_name,
                    ]);
                    $results[] = [
                        'device_name' => $deviceData['device_name'],
                        'status' => 'warning',
                        'message' => 'Device not registered',
                    ];
                    continue;
                }

                // Log the location data with de-duplication logic
                $existingLog = AssetLocationLog::where('asset_id', $asset->id)
                    ->where('location_id', $locationId)
                    ->where('status', $deviceData['status'])
                    ->where('type', $deviceData['type'])
                    ->where('updated_at', '>=', Carbon::now()->subSeconds(self::LOG_TIME_WINDOW))
                    ->orderBy('updated_at', 'desc')
                    ->first();

                $logData = [
                    'asset_id' => $asset->id,
                    'location_id' => $locationId,
                    'rssi' => $deviceData['rssi'] ?? null,
                    'kalman_rssi' => $deviceData['kalman_rssi'] ?? null,
                    'estimated_distance' => $deviceData['estimated_distance'] ?? null,
                    'type' => $deviceData['type'],
                    'status' => $deviceData['status'],
                ];

                $isDrasticChange = false;
                $locationLog = null;

                if ($existingLog) {
                    if (isset($deviceData['rssi']) && $existingLog->rssi !== null) {
                        $rssiDiff = abs($deviceData['rssi'] - $existingLog->rssi);
                        if ($rssiDiff > self::RSSI_THRESHOLD) {
                            $isDrasticChange = true;
                        }
                    }
                    if (
                        !$isDrasticChange &&
                        isset($deviceData['kalman_rssi']) &&
                        $existingLog->kalman_rssi !== null
                    ) {
                        $kalmanDiff = abs($deviceData['kalman_rssi'] - $existingLog->kalman_rssi);
                        if ($kalmanDiff > self::KALMAN_RSSI_THRESHOLD) {
                            $isDrasticChange = true;
                        }
                    }
                    if (
                        !$isDrasticChange &&
                        isset($deviceData['estimated_distance']) &&
                        $existingLog->estimated_distance !== null
                    ) {
                        $distanceDiff = abs($deviceData['estimated_distance'] - $existingLog->estimated_distance);
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
                            'rssi' => $deviceData['rssi'] ?? null,
                            'kalman_rssi' => $deviceData['kalman_rssi'] ?? null,
                            'estimated_distance' => $deviceData['estimated_distance'] ?? null,
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
                if (
                    $deviceData['status'] === 'present' &&
                    $asset->location_id !== $locationId
                ) {
                    $asset->update(['location_id' => $locationId]);
                }

                // Log alerts
                if ($deviceData['type'] === 'alert') {
                    Log::warning('Asset Alert', [
                        'asset_id' => $asset->id,
                        'asset_name' => $asset->name,
                        'status' => $deviceData['status'],
                        'location_id' => $locationId,
                        'distance' => $deviceData['estimated_distance'] ?? null,
                        'reader' => $request->reader_name,
                    ]);
                }

                $results[] = [
                    'device_name' => $deviceData['device_name'],
                    'status' => 'success',
                    'log_id' => $locationLog->id,
                    'action' => (!$existingLog || $isDrasticChange) ? 'created' : 'updated',
                ];
            }

            DB::commit();
            return response()->json(['results' => $results], 201);

        } catch (\Exception $e) {
            DB::rollback();
            Log::error('Failed to create/update location logs', [
                'error' => $e->getMessage(),
                'reader_name' => $request->reader_name,
            ]);
            return response()->json([
                'error' => 'Failed to record logs',
                'details' => $e->getMessage(),
            ], 500);
        }
    }
}
