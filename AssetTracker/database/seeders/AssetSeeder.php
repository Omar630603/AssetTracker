<?php

namespace Database\Seeders;

use App\Models\Asset;
use App\Models\Location;
use App\Models\Tag;
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
        $tag = Tag::where('name', 'Asset_Tag_01')->first();

        Asset::create([
            'location_id' => $location->id,
            'tag_id' => $tag->id,
            'name' => 'Asset_01',
            'type' => 'stationary'
        ]);
    }
}
