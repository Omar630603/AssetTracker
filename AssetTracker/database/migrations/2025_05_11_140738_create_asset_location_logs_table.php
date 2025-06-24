<?php

use Illuminate\Database\Migrations\Migration;
use Illuminate\Database\Schema\Blueprint;
use Illuminate\Support\Facades\Schema;

return new class extends Migration
{
    /**
     * Run the migrations.
     */
    public function up(): void
    {
        Schema::create('asset_location_logs', function (Blueprint $table) {
            $table->id();
            $table->foreignId('asset_id')->constrained()->onDelete('cascade');
            $table->foreignId('location_id')->constrained()->onDelete('cascade')->nullable();
            $table->float('rssi')->nullable();
            $table->float('kalman_rssi')->nullable();
            $table->float('estimated_distance')->nullable();
            $table->string('type')->default('heartbeat');
            $table->string('status')->default('present');
            $table->string('reader_name')->nullable();
            $table->timestamps();
        });

        Schema::table('asset_location_logs', function (Blueprint $table) {
            $table->index(['asset_id', 'location_id'], 'asset_location_logs_asset_location_index');
            $table->index('type', 'asset_location_logs_type_index');
            $table->index('status', 'asset_location_logs_status_index');
            $table->index('created_at', 'asset_location_logs_created_at_index');
        });
    }

    /**
     * Reverse the migrations.
     */
    public function down(): void
    {
        Schema::dropIfExists('asset_location_logs');
    }
};
