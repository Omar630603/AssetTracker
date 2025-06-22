"use client"

import { Head, useForm, Link, router } from '@inertiajs/react';

import AppLayout from '@/layouts/app-layout';
import { Button } from '@/components/ui/button';
import { type BreadcrumbItem } from '@/types';
import { toast } from 'sonner';
import { Label } from '@/components/ui/label';
import { Input } from '@/components/ui/input';
import { Card, CardContent, CardDescription, CardFooter, CardHeader, CardTitle } from '@/components/ui/card';
import { Combobox } from '@/components/combobox';
import { Asset } from '@/components/data-table/columns/assets';

interface EditAssetPageProps {
    asset: Asset;
    asset_types?: { id: string; name: string }[];
    available_tags?: { id: string; name: string }[];
}

export default function EditAsset({ asset, asset_types = [], available_tags = []  }: EditAssetPageProps) {
    const breadcrumbs: BreadcrumbItem[] = [
        { title: 'Assets', href: '/assets' },
        { title: `Edit ${asset.name}`, href: `/assets/${asset.id}/edit` },
    ];

    const { data, setData, put, processing, errors } = useForm({
        name: asset.name,
        type: asset.type,
        tag_id: asset.tag_id,
    });

    const asset_types_options = asset_types.map(type => ({
        value: type.id,
        label: type.name,
    }));

    const available_tags_options = available_tags.map(tag => ({
        value: tag.id,
        label: tag.name,
    }));

    const handleSubmit = (e: any) => {
        e.preventDefault();

        try {
            put(`/assets/${asset.id}`, {
                onSuccess: () => {
                    toast.success('Asset updated successfully');
                    router.visit('/assets');
                },
                onError: (errors) => {
                    console.error(errors);
                    toast.error('Failed to update asset');
                },
            });
        } catch (e) {
            toast.error('Failed to make request');
        }
    };

    return (
        <AppLayout breadcrumbs={breadcrumbs}>
            <Head title={`Edit Asset - ${asset.name}`} />

            <div className="container mx-auto p-4">
                <div className="mb-6">
                    <h1 className="text-3xl font-bold tracking-tight">Edit Asset</h1>
                    <p className="text-sm text-muted-foreground mt-1">
                        Update asset details and configuration.
                    </p>
                </div>

                <form onSubmit={handleSubmit}>
                    <Card>
                        <CardHeader>
                            <CardTitle>Asset Information</CardTitle>
                            <CardDescription>
                                Make changes to your asset's details here.
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

                        </CardContent>
                        <CardFooter className="flex justify-end space-x-2">
                            <Link href="/assets">
                                <Button type="button" variant="outline">
                                    Cancel
                                </Button>
                            </Link>
                            <Button type="submit" disabled={processing}>
                                {processing ? 'Updating...' : 'Update Asset'}
                            </Button>
                        </CardFooter>
                    </Card>
                </form>
            </div>
        </AppLayout>
    );
}
