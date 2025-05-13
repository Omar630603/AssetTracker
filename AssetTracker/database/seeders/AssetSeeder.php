<?php

namespace Database\Seeders;

use App\Models\Asset;
use App\Models\Location;
use Illuminate\Database\Console\Seeds\WithoutModelEvents;
use Illuminate\Database\Seeder;

class AssetSeeder extends Seeder
{
    /**
     * Run the database seeds.
     */
    public function run(): void
    {
        $location = Location::where('name', 'Omar Room')->first();

        Asset::create([
            'location_id' => $location->id,
            'name' => 'Asset_01',
            'type' => Asset::TYPE['stationary']
        ]);
    }
}
