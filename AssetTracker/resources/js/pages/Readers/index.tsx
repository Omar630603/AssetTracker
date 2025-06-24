"use client"

import { useState, useEffect } from 'react';
import { Head, router, useForm } from '@inertiajs/react';
import { AlertCircle, PlusCircle, Trash2, Info } from 'lucide-react';

import AppLayout from '@/layouts/app-layout';
import { Button } from '@/components/ui/button';
import { DataTable } from '@/components/data-table';
import { Reader, columns } from '@/components/data-table/columns/readers';
import { type BreadcrumbItem } from '@/types';
import { PopupModal } from '@/components/popup-modal';
import { toast } from 'sonner';
import { Label } from '@/components/ui/label';
import { Input } from '@/components/ui/input';
import { Combobox } from '@/components/combobox';
import { JsonViewer } from '@/components/json-viewer';
import { Alert, AlertDescription, AlertTitle } from '@/components/ui/alert';
import { Switch } from '@/components/ui/switch';
import { MultiSelect } from '@/components/multi-select';
import { Tag } from '@/components/data-table/columns/tags';

const breadcrumbs: BreadcrumbItem[] = [
    { title: 'Readers', href: '/readers' },
];

interface ReadersPageProps {
    readers: Reader[];
    locations: { id: string; name: string }[];
    tags?: Tag[];
    defaultConfig: any;
    assetNamePattern: string;
}

export default function ReadersIndex({ readers, locations = [], tags = [], defaultConfig, assetNamePattern }: ReadersPageProps) {
    const [readersData, setReadersData] = useState<Reader[]>(readers);
    const [openAddModal, setOpenAddModal] = useState(false);
    const [openDeleteSelectedModal, setOpenDeleteSelectedModal] = useState(false);
    const [configJson, setConfigJson] = useState('');
    const [rowSelection, setRowSelection] = useState({});
    const [selectedReaderIds, setSelectedReaderIds] = useState<string[]>([]);
    const [refreshing, setRefreshing] = useState(false);

    const { data, setData, post, processing, errors, reset } = useForm({
        name: '',
        location_id: '',
        discovery_mode: 'explicit' as 'pattern' | 'explicit',
        config: defaultConfig,
        tag_ids: [] as string[],
    });

    const { delete: deleteSelected, setData: setDeleteData, processing: deleteProcessing } = useForm({
        ids: [""]
    });

    useEffect(() => {
        setReadersData(readers);
    }, [readers]);

    useEffect(() => {
        setConfigJson(JSON.stringify(data.config, null, 2));
    }, [data.config]);

    useEffect(() => {
        const selectedIds = Object.keys(rowSelection).map(index => {
            const numericIndex = parseInt(index);
            return readersData[numericIndex]?.id;
        }).filter(Boolean);

        setSelectedReaderIds(selectedIds);
        setDeleteData('ids', selectedIds);
    }, [rowSelection, readersData]);

    const handleConfigUpdate = (jsonString: string) => {
        try {
            const parsed = JSON.parse(jsonString);
            setData('config', parsed);
        } catch (e) {
            // Don't update the form data if invalid JSON
        }
        setConfigJson(jsonString);
    };

    const handleRefreshLogs = () => {
        setRefreshing(true);
        setTimeout(() => {
            router.reload({
                only: ['readers'],
                onFinish: () => {
                    setRefreshing(false);
                    toast.success('Readers refreshed successfully');
                },
                onError: () => {
                    setRefreshing(false);
                    toast.error('Failed to refresh readers');
                }
            });
        }, 500);
    };

    const handleSubmit = () => {
        try {
            const config = JSON.parse(configJson);
            setData('config', config);

            post('/readers', {
                onSuccess: () => {
                    toast.success('Reader added successfully');
                    setOpenAddModal(false);
                    reset();
                },
                onError: (errors) => {
                    console.error(errors);
                    toast.error('Failed to add reader');
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
        if (selectedReaderIds.length === 0) return;

        deleteSelected(route('readers.bulk-destroy'), {
            onSuccess: () => {
                toast.success(`${selectedReaderIds.length} reader(s) deleted successfully`);
                setOpenDeleteSelectedModal(false);
                setRowSelection({});
            },
            onError: () => {
                toast.error('Failed to delete selected readers');
            }
        });
    };

    const resetForm = () => {
        reset();
        setConfigJson(JSON.stringify(defaultConfig, null, 2));
        setOpenAddModal(false);
    };

    const locationOptions = locations.map(loc => ({
        value: loc.id,
        label: loc.name
    }));

    const tagOptions = tags.map(tag => ({
        value: tag.id,
        label: `${tag.name} - ${tag.asset_name}`
    }));

    const hasSelections = Object.keys(rowSelection).length > 0;

    return (
        <AppLayout breadcrumbs={breadcrumbs}>
            <Head title="Readers Management" />

            <div className="container mx-auto p-4">
                <div className="flex items-center justify-between mb-6">
                    <div>
                        <h1 className="text-3xl font-bold tracking-tight">Readers</h1>
                        <p className="text-sm text-muted-foreground mt-1">
                            Manage your asset tracking readers and their configurations.
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
                            <span>Add Reader</span>
                        </Button>
                    </div>
                </div>

                <DataTable
                    columns={columns}
                    data={readersData}
                    searchColumn={["name", "location"]}
                    searchPlaceholder="Search readers..."
                    onRowSelectionChange={setRowSelection}
                    onRefresh={handleRefreshLogs}
                    isRefreshing={refreshing}
                    state={{ rowSelection }}
                />

                <PopupModal
                    isOpen={openAddModal}
                    onClose={resetForm}
                    onPrimaryAction={handleSubmit}
                    title="Add New Reader"
                    variant="confirm"
                    size="lg"
                    primaryActionLabel="Add Reader"
                    isLoading={processing}
                    className="max-h-[90vh]"
                >
                    <div className="space-y-4 overflow-y-auto max-h-[60vh] pr-4">
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
                            <Label htmlFor="location_id">Location</Label>
                            <Combobox
                                items={locationOptions}
                                value={data.location_id}
                                onSelect={(value) => setData('location_id', value)}
                                placeholder="Select location"
                                emptyMessage="No locations found"
                                searchPlaceholder="Search locations..."
                            />
                            {errors.location_id && <p className="text-sm text-red-500">{errors.location_id}</p>}
                        </div>

                        <div className="flex items-center justify-between">
                            <div className="space-y-0.5">
                                <Label htmlFor="discovery_mode">Pattern Discovery Mode</Label>
                                <p className="text-sm text-muted-foreground">
                                    Enable to scan for devices with pattern prefix
                                </p>
                            </div>
                            <Switch
                                id="discovery_mode"
                                checked={data.discovery_mode === 'pattern'}
                                onCheckedChange={(checked) => setData('discovery_mode', checked ? 'pattern' : 'explicit')}
                            />
                        </div>

                        {data.discovery_mode === 'pattern' ? (
                            <Alert>
                                <Info className="h-4 w-4" />
                                <AlertTitle>Pattern Mode</AlertTitle>
                                <AlertDescription>
                                    This reader will scan for devices with names starting with "{assetNamePattern}"
                                </AlertDescription>
                            </Alert>
                        ) : (
                            <div>
                                <Label htmlFor="tag_ids">Associate Tags</Label>
                                <MultiSelect
                                    items={tagOptions}
                                    selectedValues={data.tag_ids}
                                    onValueChange={(values: string[]) => setData('tag_ids', values)}
                                    placeholder="Select tags"
                                    emptyMessage="No tags found"
                                    searchPlaceholder="Search tags..."
                                />
                                {errors.tag_ids && <p className="text-sm text-red-500">{errors.tag_ids}</p>}
                            </div>
                        )}

                        <div>
                            <Label htmlFor="config">Configuration</Label>
                            <JsonViewer
                                data={configJson}
                                editable={true}
                                onChange={handleConfigUpdate}
                                height="64"
                            />
                            {errors.config && <p className="text-sm text-red-500">{errors.config}</p>}
                        </div>
                    </div>
                </PopupModal>

                <PopupModal
                    isOpen={openDeleteSelectedModal}
                    onClose={() => setOpenDeleteSelectedModal(false)}
                    onPrimaryAction={handleDeleteSelected}
                    title="Delete Selected Readers"
                    variant="confirm"
                    size="md"
                    primaryActionLabel="Delete Selected"
                    isLoading={deleteProcessing}
                >
                    <Alert>
                        <AlertCircle className="h-4 w-4" />
                        <AlertTitle>Warning</AlertTitle>
                        <AlertDescription>
                            Are you sure you want to delete {selectedReaderIds.length} selected reader(s)?
                            This action cannot be undone.
                        </AlertDescription>
                    </Alert>
                </PopupModal>
            </div>
        </AppLayout>
    );
}
