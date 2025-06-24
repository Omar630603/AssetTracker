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
        $locationNames = [
            'Omar Room',
            'Aisha Lab',
            'Fatima Office',
            'Ali Hall',
            'Hassan Storage'
        ];

        foreach ($locationNames as $i => $locName) {
            $location = Location::where('name', $locName)->first();
            Reader::firstOrCreate([
                'name' => "Asset_Reader_0" . ($i + 1),
            ], [
                'location_id' => $location?->id,
                'config' => Reader::$defaultConfig
            ]);
        }
    }
}
