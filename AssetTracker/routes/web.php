<?php

use Illuminate\Support\Facades\Route;
use Inertia\Inertia;
use App\Http\Controllers\{
    LocationController,
    TagController,
    ReaderController,
    AssetController,
    // UserController,
    // RoleController,
    // PermissionController,
    AssetLocationLogController
};

Route::get('/', function () {
    return Inertia::render('welcome');
})->name('home');

Route::middleware(['auth', 'verified'])->group(function () {

    Route::get('dashboard', function () {
        return Inertia::render('dashboard');
    })->name('dashboard');

    // Common routes for both staff and admin with 'read' access
    Route::middleware(['permission:read locations'])->get('/locations', [LocationController::class, 'index'])->name('locations.index');
    Route::middleware(['permission:read tags'])->get('/tags', [TagController::class, 'index'])->name('tags.index');
    Route::middleware(['permission:read readers'])->get('/readers', [ReaderController::class, 'index'])->name('readers.index');
    Route::middleware(['permission:read assets'])->get('/assets', [AssetController::class, 'index'])->name('assets.index');
    Route::middleware(['permission:view asset location logs and history'])->get('/logs', [AssetLocationLogController::class, 'index'])->name('logs.index');

    // Admin-only routes (full CRUD)
    Route::middleware('role:admin')->group(function () {
        Route::resource('locations', LocationController::class)->except(['index']);
        Route::resource('tags', TagController::class)->except(['index']);
        Route::resource('readers', ReaderController::class)->except(['index']);
        Route::resource('assets', AssetController::class)->except(['index']);
        // Route::resource('users', UserController::class);
        // Route::resource('roles', RoleController::class);
        // Route::resource('permissions', PermissionController::class);
    });
});

require __DIR__ . '/settings.php';
require __DIR__ . '/auth.php';
