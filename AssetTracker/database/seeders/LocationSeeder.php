<?php

namespace Database\Seeders;

use App\Models\Location;
use Illuminate\Database\Console\Seeds\WithoutModelEvents;
use Illuminate\Database\Seeder;

class LocationSeeder extends Seeder
{
    /**
     * Run the database seeds.
     */
    public function run(): void
    {
        $locationNames = [
            ['name' => 'Omar Room', 'floor' => '1st Floor'],
            ['name' => 'Aisha Lab', 'floor' => '2nd Floor'],
            ['name' => 'Fatima Office', 'floor' => '3rd Floor'],
            ['name' => 'Ali Hall', 'floor' => 'Ground Floor'],
            ['name' => 'Hassan Storage', 'floor' => 'Basement'],
        ];

        foreach ($locationNames as $loc) {
            Location::firstOrCreate(['name' => $loc['name']], $loc);
        }
    }
}
