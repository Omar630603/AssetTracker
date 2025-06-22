"use client"

import { useState, useEffect } from 'react';
import { Head, useForm, Link, router } from '@inertiajs/react';

import AppLayout from '@/layouts/app-layout';
import { Button } from '@/components/ui/button';
import { type BreadcrumbItem } from '@/types';
import { toast } from 'sonner';
import { Label } from '@/components/ui/label';
import { Input } from '@/components/ui/input';
import { Combobox } from '@/components/combobox';
import { MultiSelect } from '@/components/multi-select';
import { JsonViewer } from '@/components/json-viewer';
import { Card, CardContent, CardDescription, CardFooter, CardHeader, CardTitle } from '@/components/ui/card';
import { Location } from '@/components/data-table/columns/locations';
import { Reader } from '@/components/data-table/columns/readers';
import { Tag } from '@/components/data-table/columns/tags';
import { Switch } from '@/components/ui/switch';
import { Alert, AlertDescription, AlertTitle } from '@/components/ui/alert';
import { Info } from 'lucide-react';

interface EditReaderPageProps {
    reader: Reader;
    locations: Location[];
    tags: Tag[];
    assetNamePattern: string;
}

export default function EditReader({ reader, locations = [], tags = [], assetNamePattern }: EditReaderPageProps) {
    const [configJson, setConfigJson] = useState('');

    const breadcrumbs: BreadcrumbItem[] = [
        { title: 'Readers', href: '/readers' },
        { title: `Edit ${reader.name}`, href: `/readers/${reader.id}/edit` },
    ];

    const { data, setData, put, processing, errors } = useForm({
        name: reader.name,
        location_id: reader.location_id,
        discovery_mode: reader.discovery_mode || 'explicit' as 'pattern' | 'explicit',
        config: reader.config,
        tag_ids: reader.tag_ids || [],
    });

    useEffect(() => {
        try {
            if (typeof data.config === 'string') {
                const parsed = JSON.parse(data.config);
                setConfigJson(parsed);
            } else {
                setConfigJson(JSON.stringify(data.config, null, 2));
            }
        } catch (e) {
            console.error("Error processing config data:", e);
            setConfigJson("{}");
        }
    }, [data.config]);

    const handleConfigUpdate = (jsonString: string) => {
        setConfigJson(jsonString);
        try {
            const parsed = JSON.parse(jsonString);
            setData('config', parsed);
        } catch (e) {
            console.error("Invalid JSON:", e);
        }
    };

    const handleSubmit = (e: any) => {
        e.preventDefault();

        try {
            const config = JSON.parse(configJson);
            setData('config', config);

            put(`/readers/${reader.id}`, {
                onSuccess: () => {
                    toast.success('Reader updated successfully');
                    router.visit('/readers');
                },
                onError: (errors) => {
                    console.error(errors);
                    toast.error('Failed to update reader');
                },
            });
        } catch (e) {
            toast.error('Failed to make request');
        }
    };

    const locationOptions = locations.map(loc => ({
        value: loc.id,
        label: loc.name
    }));

    const tagOptions = tags.map(tag => ({
        value: tag.id,
        label: `${tag.name} - ${tag.asset_name}`
    }));

    return (
        <AppLayout breadcrumbs={breadcrumbs}>
            <Head title={`Edit Reader - ${reader.name}`} />

            <div className="container mx-auto p-4">
                <div className="mb-6">
                    <h1 className="text-3xl font-bold tracking-tight">Edit Reader</h1>
                    <p className="text-sm text-muted-foreground mt-1">
                        Update reader details and configuration.
                    </p>
                </div>

                <form onSubmit={handleSubmit}>
                    <Card>
                        <CardHeader>
                            <CardTitle>Reader Information</CardTitle>
                            <CardDescription>
                                Make changes to your reader's details here.
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
                                <Label htmlFor="location_id">Location</Label>
                                <Combobox
                                    items={locationOptions}
                                    value={data.location_id}
                                    onSelect={(value) => setData('location_id', value)}
                                    placeholder="Select location"
                                    emptyMessage="No locations found"
                                    searchPlaceholder="Search locations..."
                                    className="mt-1"
                                />
                                {errors.location_id && <p className="text-sm text-red-500 mt-1">{errors.location_id}</p>}
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
                                    <Label htmlFor="tag_ids">Associated Tags</Label>
                                    <MultiSelect
                                        items={tagOptions}
                                        selectedValues={data.tag_ids}
                                        onValueChange={(values: string[]) => setData('tag_ids', values)}
                                        placeholder="Select tags"
                                        emptyMessage="No tags found"
                                        searchPlaceholder="Search tags..."
                                        className="mt-1"
                                    />
                                    {errors.tag_ids && <p className="text-sm text-red-500 mt-1">{errors.tag_ids}</p>}
                                </div>
                            )}

                            <div>
                                <Label htmlFor="config">Configuration</Label>
                                <JsonViewer
                                    data={configJson}
                                    editable={true}
                                    onChange={handleConfigUpdate}
                                    height="64"
                                    className="mt-1"
                                />
                                {errors.config && <p className="text-sm text-red-500 mt-1">{errors.config}</p>}
                            </div>
                        </CardContent>
                        <CardFooter className="flex justify-end space-x-2">
                            <Link href="/readers">
                                <Button type="button" variant="outline">
                                    Cancel
                                </Button>
                            </Link>
                            <Button type="submit" disabled={processing}>
                                {processing ? 'Updating...' : 'Update Reader'}
                            </Button>
                        </CardFooter>
                    </Card>
                </form>
            </div>
        </AppLayout>
    );
}
