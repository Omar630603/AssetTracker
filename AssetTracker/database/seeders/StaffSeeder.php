<?php

namespace Database\Seeders;

use App\Models\Staff;
use App\Models\User;
use Illuminate\Database\Console\Seeds\WithoutModelEvents;
use Illuminate\Database\Seeder;

class StaffSeeder extends Seeder
{
    /**
     * Run the database seeds.
     */
    public function run(): void
    {
        $user = User::factory(1)->create([
            'name' => 'Staff User',
            'username' => 'staff01',
            'email' => 'staff@assettracker.com',
            'password' => bcrypt('password')
        ]);

        $user = User::where('username', 'staff01')->first();

        Staff::create([
            'user_id' => $user->id,
            'position' => 'Nurse',
            'department' => 'Cardiology',
        ]);

        $user->assignRole('staff');
    }
}
