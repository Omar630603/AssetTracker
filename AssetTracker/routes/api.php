<?php

use Illuminate\Support\Facades\Route;
use App\Http\Controllers\Api\ReaderController;

Route::get('/test', function () {
    return response()->json(['message' => 'API is working']);
});
Route::get('/reader-config', [ReaderController::class, 'getConfig']);
Route::post('/reader-alert', [ReaderController::class, 'receiveAlert']);
Route::post('/reader-heartbeat', [ReaderController::class, 'receiveHeartbeat']);
Route::get('/reader-status', [ReaderController::class, 'getReaderStatus']);
