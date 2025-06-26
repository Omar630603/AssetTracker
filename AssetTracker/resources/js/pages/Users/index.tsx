"use client"

import { useState, useEffect } from 'react';
import { Head, router, useForm } from '@inertiajs/react';
import { AlertCircle, PlusCircle, Trash2 } from 'lucide-react';

import AppLayout from '@/layouts/app-layout';
import { Button } from '@/components/ui/button';
import { DataTable } from '@/components/data-table';
import { User, columns } from '@/components/data-table/columns/users';
import { type BreadcrumbItem } from '@/types';
import { PopupModal } from '@/components/popup-modal';
import { toast } from 'sonner';
import { Label } from '@/components/ui/label';
import { Input } from '@/components/ui/input';
import { Alert, AlertDescription, AlertTitle } from '@/components/ui/alert';
import { Combobox } from '@/components/combobox';

const breadcrumbs: BreadcrumbItem[] = [
    { title: 'Users', href: '/users' },
];

interface UsersPageProps {
    users: User[];
    roles?: { id: string; name: string }[];
}

export default function UsersIndex({ users, roles = [] }: UsersPageProps) {
    const [usersData, setUsersData] = useState<User[]>(users);
    const [openAddModal, setOpenAddModal] = useState(false);
    const [openDeleteSelectedModal, setOpenDeleteSelectedModal] = useState(false);
    const [rowSelection, setRowSelection] = useState({});
    const [selectedUserIds, setSelectedUserIds] = useState<string[]>([]);
    const [refreshing, setRefreshing] = useState(false);

    const { data, setData, post, processing, errors, reset } = useForm({
        name: '',
        username: '',
        email: '',
        role: '',
        department: '',
        position: '',
    });

    const { delete: deleteSelected, setData: setDeleteData, processing: deleteProcessing } = useForm({
        ids: [""]
    });

    const roles_options = roles.map(role => ({
        value: role.id,
        label: role.name,
    }));

    useEffect(() => {
        setUsersData(users);
    }, [users]);

    useEffect(() => {
        const selectedIds = Object.keys(rowSelection).map(index => {
            const numericIndex = parseInt(index);
            return usersData[numericIndex]?.id;
        }).filter(Boolean);

        setSelectedUserIds(selectedIds);
        setDeleteData('ids', selectedIds);
    }, [rowSelection, usersData]);

    const handleRefreshLogs = () => {
        setRefreshing(true);
        setTimeout(() => {
            router.reload({
                only: ['users'],
                onFinish: () => {
                    setRefreshing(false);
                    toast.success('Users refreshed successfully');
                },
                onError: () => {
                    setRefreshing(false);
                    toast.error('Failed to refresh users');
                },
            });
        }, 500);
    };

    const handleSubmit = () => {
        try {
            post('/users', {
                onSuccess: () => {
                    toast.success('User added successfully');
                    setOpenAddModal(false);
                    reset();
                },
                onError: (errors) => {
                    console.error(errors);
                    toast.error('Failed to add user');
                },
                onFinish: () => {
                    // Optional: reset any loading states here
                },
            });
        } catch (e) {
            toast.error('Failed to make request');
        }
    };

    const handleDeleteSelected = () => {
        if (selectedUserIds.length === 0) return;

        deleteSelected(route('users.bulk-destroy'), {
            onSuccess: () => {
                toast.success(`${selectedUserIds.length} user(s) deleted successfully`);
            },
            onError: () => {
                toast.error('Failed to delete selected users');
            },
            onFinish: () => {
                setOpenDeleteSelectedModal(false);
                setRowSelection({});
                setSelectedUserIds([]);
            }
        });
    };

    const resetForm = () => {
        reset();
        setOpenAddModal(false);
    };

    const hasSelections = Object.keys(rowSelection).length > 0;
    const isStaffRole = data.role === 'staff';

    return (
        <AppLayout breadcrumbs={breadcrumbs}>
            <Head title="Users Management" />

            <div className="container mx-auto p-4">
                <div className="flex items-center justify-between mb-6">
                    <div>
                        <h1 className="text-3xl font-bold tracking-tight">Users</h1>
                        <p className="text-sm text-muted-foreground mt-1">
                            Manage your system users and their configurations.
                        </p>
                    </div>

                    <div className="flex space-x-2">
                        {hasSelections && (
                            <Button
                                variant="destructive"
                                onClick={() => setOpenDeleteSelectedModal(true)}
                            >
                                <Trash2 className="h-4 w-4 mr-2" />
                                <span>Delete Selected ({Object.keys(rowSelection).length})</span>
                            </Button>
                        )}

                        <Button onClick={() => setOpenAddModal(true)}>
                            <PlusCircle className="h-4 w-4 mr-2" />
                            <span>Add User</span>
                        </Button>
                    </div>
                </div>

                <DataTable
                    columns={columns}
                    data={usersData}
                    searchColumn={["name", "username"]}
                    searchPlaceholder="Search users..."
                    onRowSelectionChange={setRowSelection}
                    onRefresh={handleRefreshLogs}
                    isRefreshing={refreshing}
                    state={{ rowSelection }}
                />

                <PopupModal
                    isOpen={openAddModal}
                    onClose={resetForm}
                    onPrimaryAction={handleSubmit}
                    title="Add New User"
                    variant="confirm"
                    size="lg"
                    primaryActionLabel="Add User"
                    isLoading={processing}
                >
                    <div className="space-y-4">
                        <div>
                            <Label htmlFor="name">Name</Label>
                            <Input
                                id="name"
                                value={data.name}
                                onChange={(e) => setData('name', e.target.value)}
                            />
                            {errors.name && <p className="text-sm text-red-500">{errors.name}</p>}
                        </div>

                        <div>
                            <Label htmlFor="username">Username</Label>
                            <Input
                                id="username"
                                value={data.username}
                                onChange={(e) => setData('username', e.target.value)}
                            />
                            {errors.username && <p className="text-sm text-red-500">{errors.username}</p>}
                        </div>

                        <div>
                            <Label htmlFor="email">Email</Label>
                            <Input
                                id="email"
                                type="email"
                                value={data.email}
                                onChange={(e) => setData('email', e.target.value)}
                            />
                            {errors.email && <p className="text-sm text-red-500">{errors.email}</p>}
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
                                    />
                                    {errors.department && <p className="text-sm text-red-500">{errors.department}</p>}
                                </div>

                                <div>
                                    <Label htmlFor="position">Position</Label>
                                    <Input
                                        id="position"
                                        value={data.position}
                                        onChange={(e) => setData('position', e.target.value)}
                                        placeholder="e.g., Nurse"
                                    />
                                    {errors.position && <p className="text-sm text-red-500">{errors.position}</p>}
                                </div>
                            </>
                        )}
                    </div>
                </PopupModal>

                <PopupModal
                    isOpen={openDeleteSelectedModal}
                    onClose={() => setOpenDeleteSelectedModal(false)}
                    onPrimaryAction={handleDeleteSelected}
                    title="Delete Selected Users"
                    variant="confirm"
                    size="md"
                    primaryActionLabel="Delete Selected"
                    isLoading={deleteProcessing}
                >
                    <Alert>
                        <AlertCircle className="h-4 w-4" />
                        <AlertTitle>Warning</AlertTitle>
                        <AlertDescription>
                            Are you sure you want to delete {selectedUserIds.length} selected user(s)?
                            This action cannot be undone.
                        </AlertDescription>
                    </Alert>
                </PopupModal>
            </div>
        </AppLayout>
    );
}
