<?php

namespace Database\Seeders;

use App\Models\Asset;
use App\Models\Tag;
use Illuminate\Database\Console\Seeds\WithoutModelEvents;
use Illuminate\Database\Seeder;

class TagSeeder extends Seeder
{
    /**
     * Run the database seeds.
     */
    public function run(): void
    {
        $asset = Asset::where('name', 'Asset_01')->first();

        Tag::create([
            'asset_id' => $asset->id,
            'name' => 'Asset_Tag_01'
        ]);
    }
}
