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
        for ($i = 1; $i <= 5; $i++) {
            $location = Location::where('name', match ($i) {
                1 => 'Omar Room',
                2 => 'Aisha Lab',
                3 => 'Fatima Office',
                4 => 'Ali Hall',
                5 => 'Hassan Storage',
            })->first();

            $tag = Tag::where('name', "Asset_Tag_0$i")->first();

            Asset::firstOrCreate([
                'name' => "Asset_0$i",
            ], [
                'location_id' => $location?->id,
                'tag_id' => $tag?->id,
                'type' => 'stationary',
            ]);
        }
    }
}
