<?php

namespace App\Http\Controllers;

use App\Models\User;
use App\Models\Admin;
use App\Models\Staff;
use Illuminate\Http\Request;
use Illuminate\Support\Facades\Auth;
use Illuminate\Support\Facades\Validator;
use Illuminate\Support\Facades\Hash;
use Spatie\Permission\Models\Role;

class UserController extends Controller
{
    public function index()
    {
        $users = User::all()
            ->reject(function ($user) {
                return $user->id === Auth::user()->id;
            })
            ->map(function ($user) {
                return [
                    'id' => $user->id,
                    'name' => $user->name,
                    'username' => $user->username,
                    'role' => $user->getRoleNames()->first() ?? 'No Role',
                    'department' => $user->staff ? $user->staff->department : null,
                    'position' => $user->staff ? $user->staff->position : null,
                    'email' => $user->email,
                ];
            })->values();

        $roles = Role::all()->map(function ($role) {
            return [
                'id' => $role->name,
                'name' => ucfirst($role->name),
            ];
        });

        return inertia('Users/index', [
            'users' => $users,
            'roles' => $roles,
        ]);
    }

    public function store(Request $request)
    {
        try {
            $validationRules = [
                'name' => 'required|string|max:255',
                'username' => 'required|string|max:255|unique:users,username',
                'email' => 'required|email|unique:users,email',
                'role' => 'required|string|exists:roles,name',
            ];

            // Add department and position validation if role is staff
            if ($request->role === 'staff') {
                $validationRules['department'] = 'required|string|max:255';
                $validationRules['position'] = 'required|string|max:255';
            }

            $validator = Validator::make($request->all(), $validationRules);

            if ($validator->fails()) {
                return back()->withErrors($validator)->withInput();
            }

            // Create user
            $user = User::create([
                'name' => $request->name,
                'username' => $request->username,
                'email' => $request->email,
                'password' => Hash::make('password'), // Default password
            ]);

            // Assign role
            $user->assignRole($request->role);

            // Create role-specific records
            if ($request->role === 'admin') {
                Admin::create([
                    'user_id' => $user->id
                ]);
            } elseif ($request->role === 'staff') {
                Staff::create([
                    'user_id' => $user->id,
                    'department' => $request->department,
                    'position' => $request->position,
                ]);
            }

            return redirect()->route('users.index')->with('success', 'User created successfully');
        } catch (\Exception $e) {
            return back()->withErrors([
                'form' => $e->getMessage() ?: 'An unexpected error occurred while creating the user.',
            ])->withInput();
        }
    }

    public function edit($id)
    {
        $user = User::with(['admin', 'staff'])->findOrFail($id);

        $roles = Role::all()->map(function ($role) {
            return [
                'id' => $role->name,
                'name' => ucfirst($role->name),
            ];
        });

        return inertia('Users/edit', [
            'user' => [
                'id' => $user->id,
                'name' => $user->name,
                'username' => $user->username,
                'email' => $user->email,
                'role' => $user->getRoleNames()->first() ?? '',
                'department' => $user->staff ? $user->staff->department : '',
                'position' => $user->staff ? $user->staff->position : '',
            ],
            'roles' => $roles,
        ]);
    }

    public function update(Request $request, $id)
    {
        try {
            $user = User::with(['admin', 'staff'])->findOrFail($id);

            $validationRules = [
                'name' => 'required|string|max:255',
                'username' => 'required|string|max:255|unique:users,username,' . $id,
                'email' => 'required|email|unique:users,email,' . $id,
                'role' => 'required|string|exists:roles,name',
            ];

            // Add department and position validation if role is staff
            if ($request->role === 'staff') {
                $validationRules['department'] = 'required|string|max:255';
                $validationRules['position'] = 'required|string|max:255';
            }

            $validator = Validator::make($request->all(), $validationRules);

            if ($validator->fails()) {
                return back()->withErrors($validator)->withInput();
            }

            // Update user basic info
            $user->update([
                'name' => $request->name,
                'username' => $request->username,
                'email' => $request->email,
            ]);

            // Handle role changes
            $currentRole = $user->getRoleNames()->first();

            if ($currentRole !== $request->role) {
                // Remove old role and related records
                $user->syncRoles([]);

                if ($currentRole === 'admin' && $user->admin) {
                    $user->admin->delete();
                } elseif ($currentRole === 'staff' && $user->staff) {
                    $user->staff->delete();
                }

                // Assign new role
                $user->assignRole($request->role);

                // Create new role-specific records
                if ($request->role === 'admin') {
                    Admin::create([
                        'user_id' => $user->id
                    ]);
                } elseif ($request->role === 'staff') {
                    Staff::create([
                        'user_id' => $user->id,
                        'department' => $request->department,
                        'position' => $request->position,
                    ]);
                }
            } else {
                // Same role, just update staff info if it's a staff user
                if ($request->role === 'staff' && $user->staff) {
                    $user->staff->update([
                        'department' => $request->department,
                        'position' => $request->position,
                    ]);
                }
            }

            return redirect()->route('users.index')->with('success', 'User updated successfully');
        } catch (\Exception $e) {
            return back()->withErrors([
                'form' => 'An unexpected error occurred while updating the user.',
            ])->withInput();
        }
    }

    public function destroy($id)
    {
        try {
            $user = User::with(['admin', 'staff'])->findOrFail($id);

            // Delete related records first
            if ($user->admin) {
                $user->admin->delete();
            }
            if ($user->staff) {
                $user->staff->delete();
            }

            $user->delete();

            return redirect()->route('users.index')->with('success', 'User deleted successfully');
        } catch (\Exception $e) {
            return back()->withErrors([
                'form' => 'An unexpected error occurred while deleting the user.',
            ])->withInput();
        }
    }

    public function bulkDestroy(Request $request)
    {
        $validator = Validator::make($request->all(), [
            'ids' => 'required|array',
            'ids.*' => 'exists:users,id',
        ]);

        if ($validator->fails()) {
            return back()->withErrors($validator)->withInput();
        }

        try {
            $users = User::with(['admin', 'staff'])->whereIn('id', $request->ids)->get();

            foreach ($users as $user) {
                // Delete related records first
                if ($user->admin) {
                    $user->admin->delete();
                }
                if ($user->staff) {
                    $user->staff->delete();
                }
                $user->delete();
            }

            return redirect()->route('users.index')->with('success', count($request->ids) . ' users deleted successfully');
        } catch (\Exception $e) {
            return back()->withErrors([
                'form' => 'An unexpected error occurred while deleting the user(s).',
            ])->withInput();
        }
    }
}
