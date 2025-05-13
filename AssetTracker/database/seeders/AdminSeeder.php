<?php

namespace Database\Seeders;

use App\Models\Admin;
use App\Models\User;
use Illuminate\Database\Console\Seeds\WithoutModelEvents;
use Illuminate\Database\Seeder;

class AdminSeeder extends Seeder
{
    /**
     * Run the database seeds.
     */
    public function run(): void
    {
        $user = User::factory(1)->create([
            'name' => 'Admin User',
            'username' => 'admin01',
            'email' => 'admin@assettracker.com',
            'password' => bcrypt('password')
        ]);

        $user = User::where('username', 'admin01')->first();

        Admin::create([
            'user_id' => $user->id
        ]);

        $user->assignRole('admin');
    }
}
