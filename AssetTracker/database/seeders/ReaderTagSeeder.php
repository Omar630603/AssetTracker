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
        for ($i = 1; $i <= 5; $i++) {
            $reader = Reader::where('name', "Asset_Reader_0$i")->first();
            $tag = Tag::where('name', "Asset_Tag_0$i")->first();
            if ($reader && $tag) {
                $reader->tags()->syncWithoutDetaching([
                    $tag->id => ['created_at' => now(), 'updated_at' => now()]
                ]);
            }
        }
    }
}
