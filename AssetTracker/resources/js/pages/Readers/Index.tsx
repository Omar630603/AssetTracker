import AppLayout from '@/layouts/app-layout';
import { type BreadcrumbItem } from '@/types';
import { Head } from '@inertiajs/react';
import { Reader, columns } from "@/components/Readers/columns";
import { DataTable } from "@/components/Readers/data-table";
import { use, useEffect, useState } from 'react';

const breadcrumbs: BreadcrumbItem[] = [
    {
        title: 'Readers',
        href: '/readers',
    },
];

export default function Readers({readers}: { readers: Reader[] }) {
    const [data, setData] = useState<Reader[]>(readers);

    useEffect(() => {
        setData(readers);
    }, [readers]);

    return (
        <AppLayout breadcrumbs={breadcrumbs}>
            <Head title="Readers" />
            <div className="container mx-auto p-4">
                <h1 className="text-2xl font-bold">Readers</h1>
                <p className="text-sm text-muted-foreground">
                    Manage your readers here.
                </p>
                <DataTable
                    columns={columns}
                    data={data}
                />
            </div>
        </AppLayout>
    );
}
