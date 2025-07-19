<?php

namespace Database\Seeders;

use App\Models\Asset;
use App\Models\Location;
use App\Models\Reader;
use App\Models\AssetLocationLog;
use Illuminate\Database\Seeder;
use Carbon\Carbon;

class AssetLocationLogSeeder extends Seeder
{
    public function run(): void
    {
        $assets = Asset::all();
        $locations = Location::all();
        $readers = Reader::all();
        $now = Carbon::now();
        $status_options = ['present', 'not_found', 'out_of_range'];

        // For each asset, create logs across all locations and readers
        foreach ($assets as $asset) {
            for ($j = 0; $j < 40; $j++) { // 40 logs per asset
                $loc = $locations->random();
                $reader = $readers->random();
                $minutesAgo = rand($j * 10, ($j + 1) * 10);
                $createdAt = $now->copy()->subMinutes($minutesAgo);
                $rssi = rand(-75, -55);
                $kalmanRssi = $rssi + rand(-2, 2);
                $distance = round(rand(2, 10) + rand(0, 100) / 100, 2);
                $status = $status_options[array_rand($status_options)];
                $type = $status === 'present' ? 'heartbeat' : 'alert';

                AssetLocationLog::create([
                    'asset_id' => $asset->id,
                    'location_id' => $loc->id,
                    'reader_name' => $reader->name,
                    'type' => $type,
                    'status' => $status,
                    'rssi' => $rssi,
                    'kalman_rssi' => $kalmanRssi,
                    'estimated_distance' => $distance,
                    'created_at' => $createdAt,
                    'updated_at' => $createdAt,
                ]);
            }
        }
    }
}
