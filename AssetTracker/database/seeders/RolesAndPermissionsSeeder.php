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
        $permissions = [
            'create locations',
            'read locations',
            'update locations',
            'delete locations',

            'create tags',
            'read tags',
            'update tags',
            'delete tags',

            'create readers',
            'read readers',
            'update readers',
            'delete readers',

            'create assets',
            'read assets',
            'update assets',
            'delete assets',

            'create users',
            'read users',
            'update users',
            'delete users',

            'create roles',
            'read roles',
            'update roles',
            'delete roles',

            'create permissions',
            'read permissions',
            'update permissions',
            'delete permissions',

            'view asset location logs and history'
        ];

        foreach ($permissions as $permission) {
            Permission::create(['name' => $permission]);
        }

        $adminRole = Role::create(['name' => 'admin']);
        $staffRole = Role::create(['name' => 'staff']);

        $adminRole->givePermissionTo($permissions);
        $staffRole->givePermissionTo([
            'read locations',
            'read tags',
            'read readers',
            'read assets',
            'view asset location logs and history'
        ]);
    }
}
