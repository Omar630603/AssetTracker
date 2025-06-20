"use client"

import { useState, useEffect } from 'react';
import { Head, useForm } from '@inertiajs/react';
import { AlertCircle, PlusCircle, Trash2 } from 'lucide-react';

import AppLayout from '@/layouts/app-layout';
import { Button } from '@/components/ui/button';
import { DataTable } from '@/components/data-table';
import { Location, columns } from '@/components/data-table/columns/locations';
import { type BreadcrumbItem } from '@/types';
import { PopupModal } from '@/components/popup-modal';
import { toast } from 'sonner';
import { Label } from '@/components/ui/label';
import { Input } from '@/components/ui/input';
import { Alert, AlertDescription, AlertTitle } from '@/components/ui/alert';

const breadcrumbs: BreadcrumbItem[] = [
    { title: 'Locations', href: '/locations' },
];

interface LocationsPageProps {
    locations: Location[];
}

export default function LocationsIndex({ locations }: LocationsPageProps) {
    const [locationsData, setLocationsData] = useState<Location[]>(locations);
    const [openAddModal, setOpenAddModal] = useState(false);
    const [openDeleteSelectedModal, setOpenDeleteSelectedModal] = useState(false);
    const [rowSelection, setRowSelection] = useState({});
    const [selectedLocationIds, setSelectedLocationIds] = useState<string[]>([]);

    const { data, setData, post, processing, errors, reset } = useForm({
        name: '',
        floor: '',
    });

    const { delete: deleteSelected, setData: setDeleteData, processing: deleteProcessing } = useForm({
        ids: [""]
    });

    useEffect(() => {
        setLocationsData(locations);
    }, [locations]);

    useEffect(() => {
        const selectedIds = Object.keys(rowSelection).map(index => {
            const numericIndex = parseInt(index);
            return locationsData[numericIndex]?.id;
        }).filter(Boolean);

        setSelectedLocationIds(selectedIds);
        setDeleteData('ids', selectedIds);
    }, [rowSelection, locationsData]);

    const handleSubmit = () => {
        try {
            post('/locations', {
                onSuccess: () => {
                    toast.success('Location added successfully');
                    setOpenAddModal(false);
                    reset();
                },
                onError: (errors) => {
                    console.error(errors);
                    toast.error('Failed to add location');
                },
                onFinish: () => {
                    // Optional: reset any loading states here
                },
            });
        } catch (e) {
            toast.error('Invalid JSON configuration');
        }
    };

    const handleDeleteSelected = () => {
        if (selectedLocationIds.length === 0) return;

        deleteSelected(route('locations.bulk-destroy'), {
            onSuccess: () => {
                toast.success(`${selectedLocationIds.length} location(s) deleted successfully`);
                setOpenDeleteSelectedModal(false);
                setRowSelection({});
            },
            onError: () => {
                toast.error('Failed to delete selected locations');
            }
        });
    };

    const resetForm = () => {
        reset();
        setOpenAddModal(false);
    };

    const hasSelections = Object.keys(rowSelection).length > 0;

    return (
        <AppLayout breadcrumbs={breadcrumbs}>
            <Head title="Locations Management" />

            <div className="container mx-auto p-4">
                <div className="flex items-center justify-between mb-6">
                    <div>
                        <h1 className="text-3xl font-bold tracking-tight">Locations</h1>
                        <p className="text-sm text-muted-foreground mt-1">
                            Manage your readers' locations and their configurations.
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
                            <span>Add Location</span>
                        </Button>
                    </div>
                </div>

                <DataTable
                    columns={columns}
                    data={locationsData}
                    searchColumn={["name", "floor"]}
                    searchPlaceholder="Search locations..."
                    onRowSelectionChange={setRowSelection}
                    state={{ rowSelection }}
                />

                <PopupModal
                    isOpen={openAddModal}
                    onClose={resetForm}
                    onPrimaryAction={handleSubmit}
                    title="Add New Location"
                    variant="confirm"
                    size="lg"
                    primaryActionLabel="Add Location"
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
                            <Label htmlFor="floor">Floor</Label>
                            <Input
                                id="floor"
                                value={data.floor}
                                onChange={(e) => setData('floor', e.target.value)}
                            />
                            {errors.floor && <p className="text-sm text-red-500">{errors.floor}</p>}
                        </div>
                    </div>
                </PopupModal>

                <PopupModal
                    isOpen={openDeleteSelectedModal}
                    onClose={() => setOpenDeleteSelectedModal(false)}
                    onPrimaryAction={handleDeleteSelected}
                    title="Delete Selected Locations"
                    variant="confirm"
                    size="md"
                    primaryActionLabel="Delete Selected"
                    isLoading={deleteProcessing}
                >
                    <Alert>
                        <AlertCircle className="h-4 w-4" />
                        <AlertTitle>Warning</AlertTitle>
                        <AlertDescription>
                            Are you sure you want to delete {selectedLocationIds.length} selected location(s)?
                            This action cannot be undone.
                        </AlertDescription>
                    </Alert>
                </PopupModal>
            </div>
        </AppLayout>
    );
}
