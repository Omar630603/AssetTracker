"use client"

import { Head, useForm, Link, router } from '@inertiajs/react';

import AppLayout from '@/layouts/app-layout';
import { Button } from '@/components/ui/button';
import { type BreadcrumbItem } from '@/types';
import { toast } from 'sonner';
import { Label } from '@/components/ui/label';
import { Input } from '@/components/ui/input';
import { Card, CardContent, CardDescription, CardFooter, CardHeader, CardTitle } from '@/components/ui/card';
import { User } from '@/components/data-table/columns/users';
import { Combobox } from '@/components/combobox';

interface EditUserPageProps {
    user: User;
    roles: { id: string; name: string }[];
}

export default function EditUser({ user, roles }: EditUserPageProps) {
    const breadcrumbs: BreadcrumbItem[] = [
        { title: 'Users', href: '/users' },
        { title: `Edit ${user.name}`, href: `/users/${user.id}/edit` },
    ];

    const { data, setData, put, processing, errors } = useForm({
        name: user.name,
        username: user.username,
        email: user.email,
        role: user.role,
        department: user.department || '',
        position: user.position || '',
    });

    const roles_options = roles.map(role => ({
        value: role.id,
        label: role.name,
    }));

    const handleSubmit = (e: any) => {
        e.preventDefault();

        try {
            put(`/users/${user.id}`, {
                onSuccess: () => {
                    toast.success('User updated successfully');
                    router.visit('/users');
                },
                onError: (errors) => {
                    console.error(errors);
                    toast.error('Failed to update user');
                },
            });
        } catch (e) {
            toast.error('Failed to make request');
        }
    };

    const isStaffRole = data.role === 'staff';

    return (
        <AppLayout breadcrumbs={breadcrumbs}>
            <Head title={`Edit User - ${user.name}`} />

            <div className="container mx-auto p-4">
                <div className="mb-6">
                    <h1 className="text-3xl font-bold tracking-tight">Edit User</h1>
                    <p className="text-sm text-muted-foreground mt-1">
                        Update user details and configuration.
                    </p>
                </div>

                <form onSubmit={handleSubmit}>
                    <Card>
                        <CardHeader>
                            <CardTitle>User Information</CardTitle>
                            <CardDescription>
                                Make changes to the user's details here.
                            </CardDescription>
                        </CardHeader>
                        <CardContent className="space-y-6">
                            <div>
                                <Label htmlFor="name">Name</Label>
                                <Input
                                    id="name"
                                    value={data.name}
                                    onChange={(e) => setData('name', e.target.value)}
                                    className="mt-1"
                                />
                                {errors.name && <p className="text-sm text-red-500 mt-1">{errors.name}</p>}
                            </div>

                            <div>
                                <Label htmlFor="username">Username</Label>
                                <Input
                                    id="username"
                                    value={data.username}
                                    onChange={(e) => setData('username', e.target.value)}
                                    className="mt-1"
                                />
                                {errors.username && <p className="text-sm text-red-500 mt-1">{errors.username}</p>}
                            </div>

                            <div>
                                <Label htmlFor="email">Email</Label>
                                <Input
                                    id="email"
                                    type="email"
                                    value={data.email}
                                    onChange={(e) => setData('email', e.target.value)}
                                    className="mt-1"
                                />
                                {errors.email && <p className="text-sm text-red-500 mt-1">{errors.email}</p>}
                            </div>

                            <div>
                                <Label htmlFor="role">Role</Label>
                                <Combobox
                                    items={roles_options}
                                    value={data.role}
                                    onSelect={(value) => setData('role', value)}
                                    placeholder="Select user role"
                                    emptyMessage="No roles found"
                                    searchPlaceholder="Search roles..."
                                    className="mt-1"
                                />
                                {errors.role && <p className="text-sm text-red-500 mt-1">{errors.role}</p>}
                            </div>

                            {isStaffRole && (
                                <>
                                    <div>
                                        <Label htmlFor="department">Department</Label>
                                        <Input
                                            id="department"
                                            value={data.department}
                                            onChange={(e) => setData('department', e.target.value)}
                                            placeholder="e.g., Cardiology"
                                            className="mt-1"
                                        />
                                        {errors.department && <p className="text-sm text-red-500 mt-1">{errors.department}</p>}
                                    </div>

                                    <div>
                                        <Label htmlFor="position">Position</Label>
                                        <Input
                                            id="position"
                                            value={data.position}
                                            onChange={(e) => setData('position', e.target.value)}
                                            placeholder="e.g., Nurse"
                                            className="mt-1"
                                        />
                                        {errors.position && <p className="text-sm text-red-500 mt-1">{errors.position}</p>}
                                    </div>
                                </>
                            )}
                        </CardContent>
                        <CardFooter className="flex justify-end space-x-2">
                            <Link href="/users">
                                <Button type="button" variant="outline">
                                    Cancel
                                </Button>
                            </Link>
                            <Button type="submit" disabled={processing}>
                                {processing ? 'Updating...' : 'Update User'}
                            </Button>
                        </CardFooter>
                    </Card>
                </form>
            </div>
        </AppLayout>
    );
}
