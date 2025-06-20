"use client"

import { Head, useForm, Link, router } from '@inertiajs/react';

import AppLayout from '@/layouts/app-layout';
import { Button } from '@/components/ui/button';
import { type BreadcrumbItem } from '@/types';
import { toast } from 'sonner';
import { Label } from '@/components/ui/label';
import { Input } from '@/components/ui/input';
import { Card, CardContent, CardDescription, CardFooter, CardHeader, CardTitle } from '@/components/ui/card';

interface EditTagPageProps {
    tag: {
        id: string;
        name: string;
    };
}

export default function EditTag({ tag }: EditTagPageProps) {
    const breadcrumbs: BreadcrumbItem[] = [
        { title: 'Tags', href: '/tags' },
        { title: `Edit ${tag.name}`, href: `/tags/${tag.id}/edit` },
    ];

    const { data, setData, put, processing, errors } = useForm({
        name: tag.name,
    });

    const handleSubmit = (e: any) => {
        e.preventDefault();

        try {
            put(`/tags/${tag.id}`, {
                onSuccess: () => {
                    toast.success('Tag updated successfully');
                    router.visit('/tags');
                },
                onError: (errors) => {
                    console.error(errors);
                    toast.error('Failed to update tag');
                },
            });
        } catch (e) {
            toast.error('Invalid JSON configuration');
        }
    };

    return (
        <AppLayout breadcrumbs={breadcrumbs}>
            <Head title={`Edit Tag - ${tag.name}`} />

            <div className="container mx-auto p-4">
                <div className="mb-6">
                    <h1 className="text-3xl font-bold tracking-tight">Edit Tag</h1>
                    <p className="text-sm text-muted-foreground mt-1">
                        Update tag details and configuration.
                    </p>
                </div>

                <form onSubmit={handleSubmit}>
                    <Card>
                        <CardHeader>
                            <CardTitle>Tag Information</CardTitle>
                            <CardDescription>
                                Make changes to your tag's details here.
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

                        </CardContent>
                        <CardFooter className="flex justify-end space-x-2">
                            <Link href="/tags">
                                <Button type="button" variant="outline">
                                    Cancel
                                </Button>
                            </Link>
                            <Button type="submit" disabled={processing}>
                                {processing ? 'Updating...' : 'Update Tag'}
                            </Button>
                        </CardFooter>
                    </Card>
                </form>
            </div>
        </AppLayout>
    );
}
