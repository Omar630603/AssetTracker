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
        Schema::create('readers', function (Blueprint $table) {
            $table->id();
            $table->foreignId('location_id')->constrained()->onDelete('cascade');
            $table->string('name')->unique();
            $table->enum('discovery_mode', ['pattern', 'explicit'])->default('explicit');
            $table->json('config')->nullable();
            $table->timestamps();
        });

        Schema::table('readers', function (Blueprint $table) {
            $table->index(['location_id', 'name'], 'readers_location_name_index');
        });
    }

    /**
     * Reverse the migrations.
     */
    public function down(): void
    {
        Schema::dropIfExists('readers');
    }
};
