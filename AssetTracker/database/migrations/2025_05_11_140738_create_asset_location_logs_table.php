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
            $table->string('type')->default('heartbeat')->after('estimated_distance');
            $table->string('status')->default('present')->after('type');
            $table->timestamps();
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
