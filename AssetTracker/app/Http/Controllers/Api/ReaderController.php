<?php

namespace App\Http\Controllers\Api;

use App\Http\Controllers\Controller;
use App\Models\Reader;
use Illuminate\Http\Request;
use Illuminate\Support\Facades\Log;
use Illuminate\Support\Facades\Validator;

class ReaderController extends Controller
{
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

    public function receiveAlert(Request $request)
    {
        // Validate the request
        $validator = Validator::make($request->all(), [
            'reader_name' => 'required|string|max:255',
            'device_name' => 'required|string|max:255',
            'alert_type' => 'required|string|in:out_of_range,not_found,connection_lost',
            'distance' => 'nullable|numeric',
            'timestamp' => 'required|integer',
        ]);

        if ($validator->fails()) {
            return response()->json([
                'error' => 'Validation failed',
                'details' => $validator->errors()
            ], 400);
        }

        // Verify reader exists
        $reader = Reader::where('name', $request->reader_name)->first();
        if (!$reader) {
            return response()->json(['error' => 'Reader not found'], 404);
        }

        // Log the alert (you can later save to database)
        $alertData = [
            'reader_name' => $request->reader_name,
            'device_name' => $request->device_name,
            'alert_type' => $request->alert_type,
            'distance' => $request->distance,
            'timestamp' => $request->timestamp,
            'received_at' => now()->toISOString(),
        ];

        // Log to Laravel log for now (later implement database storage)
        Log::info('Reader Alert Received', $alertData);

        // TODO: Implement database storage
        // TODO: Implement real-time notifications
        // TODO: Implement alert escalation logic

        return response()->json([
            'status' => 'success',
            'message' => 'Alert received and logged',
            'alert_id' => uniqid('alert_', true), // Temporary ID
        ], 200);
    }

    public function receiveHeartbeat(Request $request)
    {
        // Validate the request
        $validator = Validator::make($request->all(), [
            'reader_name' => 'required|string|max:255',
            'device_name' => 'required|string|max:255',
            'distance' => 'required|numeric|min:0',
            'status' => 'required|string|in:present,absent,unknown',
            'timestamp' => 'required|integer',
        ]);

        if ($validator->fails()) {
            return response()->json([
                'error' => 'Validation failed',
                'details' => $validator->errors()
            ], 400);
        }

        // Verify reader exists
        $reader = Reader::where('name', $request->reader_name)->first();
        if (!$reader) {
            return response()->json(['error' => 'Reader not found'], 404);
        }

        // Log the heartbeat
        $heartbeatData = [
            'reader_name' => $request->reader_name,
            'device_name' => $request->device_name,
            'distance' => $request->distance,
            'status' => $request->status,
            'timestamp' => $request->timestamp,
            'received_at' => now()->toISOString(),
        ];

        // Log to Laravel log for now (later implement database storage)
        Log::info('Reader Heartbeat Received', $heartbeatData);

        // TODO: Implement database storage
        // TODO: Update device last seen timestamp
        // TODO: Implement status tracking

        return response()->json([
            'status' => 'success',
            'message' => 'Heartbeat received and logged',
            'heartbeat_id' => uniqid('heartbeat_', true), // Temporary ID
        ], 200);
    }

    public function getReaderStatus(Request $request)
    {
        $readerName = $request->input('reader_name');

        if (!$readerName) {
            return response()->json(['error' => 'Reader name is required'], 400);
        }

        $reader = Reader::where('name', $readerName)->first();
        if (!$reader) {
            return response()->json(['error' => 'Reader not found'], 404);
        }

        // TODO: Implement actual status retrieval from database
        // For now, return basic reader info
        return response()->json([
            'reader_name' => $reader->name,
            'status' => 'active', // TODO: Implement actual status tracking
            'last_seen' => now()->toISOString(), // TODO: Get from actual last heartbeat
            'connected_devices' => [], // TODO: Get from database
            'recent_alerts' => [], // TODO: Get from database
        ]);
    }
}
