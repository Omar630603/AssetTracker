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
use Illuminate\Support\Facades\Cache;

class ReaderController extends Controller
{
    // Cache readers for 5 minutes to reduce database queries
    private function getReaderCached($readerName)
    {
        return Cache::remember("reader_{$readerName}", 300, function () use ($readerName) {
            return Reader::where('name', $readerName)->first();
        });
    }

    // Cache asset lookups for 10 minutes
    private function getAssetByTagNameCached($deviceName)
    {
        return Cache::remember("asset_tag_{$deviceName}", 600, function () use ($deviceName) {
            return Asset::whereHas('tag', function ($query) use ($deviceName) {
                $query->where('name', $deviceName);
            })->first();
        });
    }

    // Fetch reader configuration including location_id and target devices
    public function getConfig(Request $request)
    {
        $readerName = $request->input('reader_name');
        $versionCheckOnly = $request->input('version_check', false);

        // Use cached reader lookup
        $reader = $this->getReaderCached($readerName);

        if (!$reader) {
            return response()->json(['error' => 'Reader not found'], 404);
        }

        $version = $reader->updated_at->timestamp;

        if ($versionCheckOnly) {
            return response()->json([
                'version' => $version,
                'reader_name' => $reader->name,
                'last_updated' => $reader->updated_at->toISOString(),
            ]);
        }

        // Eager load tags to reduce queries
        $targets = Reader::with('tags:name')
            ->where('id', $reader->id)
            ->first()
            ->tags
            ->pluck('name')
            ->toArray();

        return response()->json([
            'name' => $reader->name,
            'targets' => $targets,
            'config' => $reader->config ?? [
                'txPower' => -52,
                'maxDistance' => 5.0,
                'sampleCount' => 10,
                'sampleDelayMs' => 100,
                'pathLossExponent' => 2.5,
                'kalman' => [
                    'Q' => 0.001,
                    'R' => 0.5,
                    'P' => 1.0,
                    'initial' => -70.0
                ]
            ],
            'version' => $version,
            'last_updated' => $reader->updated_at->toISOString(),
        ]);
    }

    // Unified endpoint for receiving location logs
    public function receiveLocationLog(Request $request)
    {
        // Quick validation - fail fast
        $validator = Validator::make($request->all(), [
            'reader_name' => 'required|string|max:255',
            'device_name' => 'required|string|max:255',
            'type' => 'required|string|in:heartbeat,alert',
            'status' => 'required|string|in:present,not_found,out_of_range',
            'rssi' => 'nullable|numeric',
            'kalman_rssi' => 'nullable|numeric',
            'estimated_distance' => 'nullable|numeric|min:-1',
            'timestamp' => 'nullable|integer', // Made optional
        ]);

        if ($validator->fails()) {
            return response()->json([
                'error' => 'Validation failed',
                'details' => $validator->errors()
            ], 400);
        }

        // Use cached reader lookup
        $reader = $this->getReaderCached($request->reader_name);
        if (!$reader) {
            return response()->json(['error' => 'Reader not found'], 404);
        }

        $locationId = $reader->location_id;
        if (!$locationId) {
            Log::warning('Reader has no location assigned', ['reader_name' => $request->reader_name]);
            return response()->json(['error' => 'Reader location not configured'], 400);
        }

        // Use cached asset lookup
        $asset = $this->getAssetByTagNameCached($request->device_name);

        if (!$asset) {
            // For unknown devices, return quickly without logging to reduce latency
            return response()->json([
                'status' => 'warning',
                'message' => 'Device not registered'
            ], 200);
        }

        // Use database transaction for consistency and potential rollback
        DB::beginTransaction();

        try {
            // Prepare data
            $logData = [
                'asset_id' => $asset->id,
                'location_id' => $locationId,
                'rssi' => $request->rssi,
                'kalman_rssi' => $request->kalman_rssi,
                'estimated_distance' => $request->estimated_distance,
                'type' => $request->type,
                'status' => $request->status,
            ];

            // Insert location log
            $locationLog = AssetLocationLog::create($logData);

            // Update asset's current location only if status is "present"
            if ($request->status === 'present') {
                // Only update if location actually changed
                if ($asset->location_id !== $locationId) {
                    $asset->update(['location_id' => $locationId]);
                    // Clear cache for this asset
                    Cache::forget("asset_tag_{$request->device_name}");
                }
            }

            DB::commit();

            // Async logging to reduce response time
            if ($request->type === 'alert') {
                // Queue alert logs for async processing if you have queues set up
                Log::warning('Asset Alert', [
                    'asset_id' => $asset->id,
                    'asset_name' => $asset->name,
                    'status' => $request->status,
                    'location_id' => $locationId,
                    'distance' => $request->estimated_distance,
                    'reader' => $request->reader_name,
                ]);
            }

            // Return minimal response to reduce transfer time
            return response()->json([
                'status' => 'success',
                'log_id' => $locationLog->id
            ], 201);

        } catch (\Exception $e) {
            DB::rollback();

            Log::error('Failed to create location log', [
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
