"use client"

import { useState, useEffect } from 'react';
import { Head, router, useForm } from '@inertiajs/react';
import { AlertCircle, PlusCircle, Trash2 } from 'lucide-react';

import AppLayout from '@/layouts/app-layout';
import { Button } from '@/components/ui/button';
import { DataTable } from '@/components/data-table';
import { Asset, columns } from '@/components/data-table/columns/assets';
import { type BreadcrumbItem } from '@/types';
import { PopupModal } from '@/components/popup-modal';
import { toast } from 'sonner';
import { Label } from '@/components/ui/label';
import { Input } from '@/components/ui/input';
import { Alert, AlertDescription, AlertTitle } from '@/components/ui/alert';
import { Combobox } from '@/components/combobox';

const breadcrumbs: BreadcrumbItem[] = [
    { title: 'Assets', href: '/assets' },
];

interface AssetsPageProps {
    assets: Asset[];
    asset_types?: { id: string; name: string }[];
    available_tags?: { id: string; name: string }[];
}

export default function AssetsIndex({ assets, asset_types = [], available_tags = [] }: AssetsPageProps) {
    const [assetsData, setAssetsData] = useState<Asset[]>(assets);
    const [openAddModal, setOpenAddModal] = useState(false);
    const [openDeleteSelectedModal, setOpenDeleteSelectedModal] = useState(false);
    const [rowSelection, setRowSelection] = useState({});
    const [selectedAssetIds, setSelectedAssetIds] = useState<string[]>([]);
    const [refreshing, setRefreshing] = useState(false);

    const { data, setData, post, processing, errors, reset } = useForm({
        name: '',
        type: '',
        tag_id: '',
    });

    const { delete: deleteSelected, setData: setDeleteData, processing: deleteProcessing } = useForm({
        ids: [""]
    });

    const asset_types_options = asset_types.map(type => ({
        value: type.id,
        label: type.name,
    }));

    const available_tags_options = available_tags.map(tag => ({
        value: tag.id,
        label: tag.name,
    }));

    useEffect(() => {
        setAssetsData(assets);
    }, [assets]);

    useEffect(() => {
        const selectedIds = Object.keys(rowSelection).map(index => {
            const numericIndex = parseInt(index);
            return assetsData[numericIndex]?.id;
        }).filter(Boolean);

        setSelectedAssetIds(selectedIds);
        setDeleteData('ids', selectedIds);
    }, [rowSelection, assetsData]);

    const handleRefreshLogs = () => {
        setRefreshing(true);
        setTimeout(() => {
            router.reload({
                only: ['assets'],
                onFinish: () => {
                    setRefreshing(false);
                    toast.success('Assets refreshed successfully');
                },
                onError: () => {
                    setRefreshing(false);
                    toast.error('Failed to refresh assets');
                }
            });
        }, 500);
    };

    const handleSubmit = () => {
        try {
            post('/assets', {
                onSuccess: () => {
                    toast.success('Asset added successfully');
                    setOpenAddModal(false);
                    reset();
                },
                onError: (errors) => {
                    console.error(errors);
                    toast.error('Failed to add asset');
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
        if (selectedAssetIds.length === 0) return;

        deleteSelected(route('assets.bulk-destroy'), {
            onSuccess: () => {
                toast.success(`${selectedAssetIds.length} asset(s) deleted successfully`);
                setOpenDeleteSelectedModal(false);
                setRowSelection({});
            },
            onError: () => {
                toast.error('Failed to delete selected assets');
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
            <Head title="Assets Management" />

            <div className="container mx-auto p-4">
                <div className="flex items-center justify-between mb-6">
                    <div>
                        <h1 className="text-3xl font-bold tracking-tight">Assets</h1>
                        <p className="text-sm text-muted-foreground mt-1">
                            Manage your assets' assets and their configurations.
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
                            <span>Add Asset</span>
                        </Button>
                    </div>
                </div>

                <DataTable
                    columns={columns}
                    data={assetsData}
                    searchColumn={["name", "floor"]}
                    searchPlaceholder="Search assets..."
                    onRowSelectionChange={setRowSelection}
                    onRefresh={handleRefreshLogs}
                    isRefreshing={refreshing}
                    state={{ rowSelection }}
                />

                <PopupModal
                    isOpen={openAddModal}
                    onClose={resetForm}
                    onPrimaryAction={handleSubmit}
                    title="Add New Asset"
                    variant="confirm"
                    size="lg"
                    primaryActionLabel="Add Asset"
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
                            <Label htmlFor="type">Type</Label>
                            <Combobox
                                items={asset_types_options}
                                value={data.type}
                                onSelect={(value) => setData('type', value)}
                                placeholder="Select asset type"
                                emptyMessage="No types found"
                                searchPlaceholder="Search types..."
                                className="mt-1"
                            />
                            {errors.type && <p className="text-sm text-red-500 mt-1">{errors.type}</p>}
                        </div>

                        <div>
                            <Label htmlFor="tag_id">Tag</Label>
                            <Combobox
                                items={available_tags_options}
                                value={data.tag_id}
                                onSelect={(value) => setData('tag_id', value)}
                                placeholder="Select tag"
                                emptyMessage="No tags found"
                                searchPlaceholder="Search tags..."
                                className="mt-1"
                            />
                            {errors.tag_id && <p className="text-sm text-red-500 mt-1">{errors.tag_id}</p>}
                        </div>
                    </div>
                </PopupModal>

                <PopupModal
                    isOpen={openDeleteSelectedModal}
                    onClose={() => setOpenDeleteSelectedModal(false)}
                    onPrimaryAction={handleDeleteSelected}
                    title="Delete Selected Assets"
                    variant="confirm"
                    size="md"
                    primaryActionLabel="Delete Selected"
                    isLoading={deleteProcessing}
                >
                    <Alert>
                        <AlertCircle className="h-4 w-4" />
                        <AlertTitle>Warning</AlertTitle>
                        <AlertDescription>
                            Are you sure you want to delete {selectedAssetIds.length} selected asset(s)?
                            This action cannot be undone.
                        </AlertDescription>
                    </Alert>
                </PopupModal>
            </div>
        </AppLayout>
    );
}
