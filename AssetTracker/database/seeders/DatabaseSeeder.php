<?php

namespace Database\Seeders;

use App\Models\Reader;
use App\Models\Staff;
use App\Models\User;
// use Illuminate\Database\Console\Seeds\WithoutModelEvents;
use Illuminate\Database\Seeder;

class DatabaseSeeder extends Seeder
{
    /**
     * Seed the application's database.
     */
    public function run(): void
    {
        $this->call([
            RolesAndPermissionsSeeder::class,
            AdminSeeder::class,
            StaffSeeder::class,
            LocationSeeder::class,
            TagSeeder::class,
            AssetSeeder::class,
            ReaderSeeder::class,
            ReaderTagSeeder::class,
            AssetLocationLogSeeder::class,
        ]);
    }
}
