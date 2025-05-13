<?php

namespace Database\Seeders;

use App\Models\Location;
use App\Models\Reader;
use Illuminate\Database\Console\Seeds\WithoutModelEvents;
use Illuminate\Database\Seeder;

class ReaderSeeder extends Seeder
{
    /**
     * Run the database seeds.
     */
    public function run(): void
    {

        $location = Location::where('name', 'Omar Room')->first();

        Reader::create([
            'location_id' => $location->id,
            'name' => 'Asset_Reader_01',
            'config' => json_encode([
                "txPower" => -52,
                "pathLossExponent" => 2.5,
                "maxDistance" => 5.0,
                "sampleCount" => 10,
                "sampleDelayMs" => 100,
                "kalman" => [
                    "Q" => 0.01,
                    "R" => 1,
                    "P" => 1,
                    "initial" => -50
                ]
            ])
        ]);
    }
}
