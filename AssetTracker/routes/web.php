<?php

use Illuminate\Support\Facades\Route;
use Inertia\Inertia;
use App\Http\Controllers\{
    DashboardController,
    LocationController,
    TagController,
    ReaderController,
    AssetController,
    UserController,
};

Route::get('/', function () {
    return Inertia::render('welcome');
})->name('home');

Route::middleware(['auth', 'verified'])->group(function () {
    // Common routes for both staff and admin with 'read' access
    Route::get('dashboard', [DashboardController::class, 'index'])->name('dashboard');
    Route::middleware(['permission:read locations'])->get('/locations', [LocationController::class, 'index'])->name('locations.index');
    Route::middleware(['permission:read tags'])->get('/tags', [TagController::class, 'index'])->name('tags.index');
    Route::middleware(['permission:read readers'])->get('/readers', [ReaderController::class, 'index'])->name('readers.index');
    Route::middleware(['permission:read assets'])->get('/assets', [AssetController::class, 'index'])->name('assets.index');

    // Admin-only routes (full CRUD)
    Route::middleware('role:admin')->group(function () {
        Route::delete('dashboard/logs/destroy/bulk', [DashboardController::class, 'bulkDestroyLogs'])->name('dashboard.logs.bulk-destroy');

        Route::resource('locations', LocationController::class)->except(['index']);
        Route::delete('locations/destroy/bulk', [LocationController::class, 'bulkDestroy'])->name('locations.bulk-destroy');

        Route::resource('assets', AssetController::class)->except(['index']);
        Route::delete('assets/destroy/bulk', [AssetController::class, 'bulkDestroy'])->name('assets.bulk-destroy');

        Route::resource('tags', TagController::class)->except(['index']);
        Route::delete('tags/destroy/bulk', [TagController::class, 'bulkDestroy'])->name('tags.bulk-destroy');

        Route::resource('readers', ReaderController::class)->except(['index']);
        Route::delete('readers/destroy/bulk', [ReaderController::class, 'bulkDestroy'])->name('readers.bulk-destroy');

        Route::resource('users', UserController::class);
        Route::delete('users/destroy/bulk', [UserController::class, 'bulkDestroy'])->name('users.bulk-destroy');
    });
});

require __DIR__ . '/settings.php';
require __DIR__ . '/auth.php';
