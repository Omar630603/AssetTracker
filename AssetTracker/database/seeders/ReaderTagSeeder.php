<?php

namespace Database\Seeders;

use App\Models\Reader;
use App\Models\Tag;
use Illuminate\Database\Console\Seeds\WithoutModelEvents;
use Illuminate\Database\Seeder;

class ReaderTagSeeder extends Seeder
{
    /**
     * Run the database seeds.
     */
    public function run(): void
    {
        $reader = Reader::where('name', 'Asset_Reader_01')->first();
        $tag = Tag::where('name', 'Asset_Tag_01')->first();

        if ($reader && $tag) {
            $reader->tags()->attach($tag->id, ['created_at' => now(), 'updated_at' => now()]);
        }
    }
}
