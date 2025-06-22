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
            'config' => Reader::$defaultConfig
        ]);
    }
}
