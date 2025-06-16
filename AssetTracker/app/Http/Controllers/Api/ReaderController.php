<?php

namespace App\Http\Controllers\Api;

use App\Http\Controllers\Controller;
use App\Models\Reader;
use App\Models\Asset;
use App\Models\AssetLocationLog;
use Illuminate\Http\Request;
use Illuminate\Support\Facades\Log;
use Illuminate\Support\Facades\Validator;

class ReaderController extends Controller
{
    // Fetch reader configuration including location_id and target devices
    public function getConfig(Request $request)
    {
        $readerName = $request->input('reader_name');
        $versionCheckOnly = $request->input('version_check', false);

        $reader = Reader::where('name', $readerName)->first();

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

        $targets = $reader->tags()->pluck('name')->toArray();

        return response()->json([
            'name' => $reader->name,
            'targets' => $targets,
            'config' => $reader->config,
            'version' => $version,
            'last_updated' => $reader->updated_at->toISOString(),
        ]);
    }

    // Unified endpoint for receiving location logs (replaces separate heartbeat/alert endpoints)
    public function receiveLocationLog(Request $request)
    {
        $validator = Validator::make($request->all(), [
            'reader_name' => 'required|string|max:255',
            'device_name' => 'required|string|max:255',
            'type' => 'required|string|in:heartbeat,alert',
            'status' => 'required|string|in:present,not_found,out_of_range',
            'rssi' => 'nullable|numeric',
            'kalman_rssi' => 'nullable|numeric',
            'estimated_distance' => 'nullable|numeric|min:-1', // Allow -1 for not found
            'timestamp' => 'required|integer',
        ]);

        if ($validator->fails()) {
            return response()->json([
                'error' => 'Validation failed',
                'details' => $validator->errors()
            ], 400);
        }

        // Verify reader exists and get location_id
        $reader = Reader::where('name', $request->reader_name)->first();
        if (!$reader) {
            return response()->json(['error' => 'Reader not found'], 404);
        }

        // Get location_id from reader
        $locationId = $reader->location_id;
        if (!$locationId) {
            Log::warning('Reader has no location assigned', ['reader_name' => $request->reader_name]);
            return response()->json(['error' => 'Reader location not configured'], 400);
        }

        // Find the asset by tag name
        $asset = Asset::whereHas('tag', function ($query) use ($request) {
            $query->where('name', $request->device_name);
        })->first();

        if (!$asset) {
            // Log warning but don't fail - device might not be registered yet
            Log::warning('Asset not found for device', [
                'device_name' => $request->device_name,
                'reader_name' => $request->reader_name
            ]);

            return response()->json([
                'status' => 'warning',
                'message' => 'Device received but asset not found in system'
            ], 200);
        }

        // Create location log entry
        try {
            $locationLog = AssetLocationLog::create([
                'asset_id' => $asset->id,
                'location_id' => $locationId, // Use reader's location
                'rssi' => $request->rssi,
                'kalman_rssi' => $request->kalman_rssi,
                'estimated_distance' => $request->estimated_distance,
                'type' => $request->type,
                'status' => $request->status,
            ]);

            // Update asset's current location only if status is "present"
            if ($request->status === 'present') {
                $asset->update(['location_id' => $locationId]);

                Log::info('Asset location updated', [
                    'asset_id' => $asset->id,
                    'asset_name' => $asset->name,
                    'location_id' => $locationId,
                    'distance' => $request->estimated_distance,
                    'reader' => $request->reader_name
                ]);
            }

            // Log alerts for monitoring
            if ($request->type === 'alert') {
                Log::warning('Asset Alert', [
                    'asset_id' => $asset->id,
                    'asset_name' => $asset->name,
                    'status' => $request->status,
                    'location_id' => $locationId,
                    'distance' => $request->estimated_distance,
                    'reader' => $request->reader_name,
                    'rssi' => $request->rssi,
                    'kalman_rssi' => $request->kalman_rssi
                ]);
            } else {
                // Log heartbeats at debug level to reduce noise
                Log::debug('Asset Heartbeat', [
                    'asset_id' => $asset->id,
                    'asset_name' => $asset->name,
                    'location_id' => $locationId,
                    'distance' => $request->estimated_distance,
                    'reader' => $request->reader_name
                ]);
            }

            return response()->json([
                'status' => 'success',
                'message' => 'Location log recorded successfully',
                'log_id' => $locationLog->id,
                'type' => $request->type
            ], 201);

        } catch (\Exception $e) {
            Log::error('Failed to create location log', [
                'error' => $e->getMessage(),
                'request_data' => $request->all(),
                'reader_name' => $request->reader_name,
                'device_name' => $request->device_name
            ]);

            return response()->json([
                'error' => 'Failed to record location log',
                'message' => 'Internal server error'
            ], 500);
        }
    }
}
