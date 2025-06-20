"use client"

import { Head, useForm, Link, router } from '@inertiajs/react';

import AppLayout from '@/layouts/app-layout';
import { Button } from '@/components/ui/button';
import { type BreadcrumbItem } from '@/types';
import { toast } from 'sonner';
import { Label } from '@/components/ui/label';
import { Input } from '@/components/ui/input';
import { Card, CardContent, CardDescription, CardFooter, CardHeader, CardTitle } from '@/components/ui/card';

interface EditLocationPageProps {
    location: {
        id: string;
        name: string;
        floor?: string;
    };
}

export default function EditLocation({ location }: EditLocationPageProps) {
    const breadcrumbs: BreadcrumbItem[] = [
        { title: 'Locations', href: '/locations' },
        { title: `Edit ${location.name}`, href: `/locations/${location.id}/edit` },
    ];

    const { data, setData, put, processing, errors } = useForm({
        name: location.name,
        floor: location.floor || '',
    });

    const handleSubmit = (e: any) => {
        e.preventDefault();

        try {
            put(`/locations/${location.id}`, {
                onSuccess: () => {
                    toast.success('Location updated successfully');
                    router.visit('/locations');
                },
                onError: (errors) => {
                    console.error(errors);
                    toast.error('Failed to update location');
                },
            });
        } catch (e) {
            toast.error('Invalid JSON configuration');
        }
    };

    return (
        <AppLayout breadcrumbs={breadcrumbs}>
            <Head title={`Edit Location - ${location.name}`} />

            <div className="container mx-auto p-4">
                <div className="mb-6">
                    <h1 className="text-3xl font-bold tracking-tight">Edit Location</h1>
                    <p className="text-sm text-muted-foreground mt-1">
                        Update location details and configuration.
                    </p>
                </div>

                <form onSubmit={handleSubmit}>
                    <Card>
                        <CardHeader>
                            <CardTitle>Location Information</CardTitle>
                            <CardDescription>
                                Make changes to your location's details here.
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
                                <Label htmlFor="floor">Floor</Label>
                                <Input
                                    id="floor"
                                    value={data.floor}
                                    onChange={(e) => setData('floor', e.target.value)}
                                    className="mt-1"
                                />
                                {errors.floor && <p className="text-sm text-red-500 mt-1">{errors.floor}</p>}
                            </div>
                        </CardContent>
                        <CardFooter className="flex justify-end space-x-2">
                            <Link href="/locations">
                                <Button type="button" variant="outline">
                                    Cancel
                                </Button>
                            </Link>
                            <Button type="submit" disabled={processing}>
                                {processing ? 'Updating...' : 'Update Location'}
                            </Button>
                        </CardFooter>
                    </Card>
                </form>
            </div>
        </AppLayout>
    );
}
