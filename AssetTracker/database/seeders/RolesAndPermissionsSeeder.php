<?php

namespace Database\Seeders;

use Illuminate\Database\Console\Seeds\WithoutModelEvents;
use Illuminate\Database\Seeder;
use Spatie\Permission\Models\Role;
use Spatie\Permission\Models\Permission;

class RolesAndPermissionsSeeder extends Seeder
{
    /**
     * Run the database seeds.
     */
    public function run(): void
    {
        app()[\Spatie\Permission\PermissionRegistrar::class]->forgetCachedPermissions();

        Permission::create(['name' => 'edit articles']);
        Permission::create(['name' => 'delete articles']);
        Permission::create(['name' => 'publish articles']);
        Permission::create(['name' => 'unpublish articles']);

        $role = Role::create(['name' => 'admin']);
        $role->givePermissionTo([
            'edit articles',
            'delete articles',
            'publish articles',
            'unpublish articles'
        ]);
        $role = Role::create(['name' => 'staff']);
        $role->givePermissionTo([
            'edit articles',
            'publish articles'
        ]);
    }
}
