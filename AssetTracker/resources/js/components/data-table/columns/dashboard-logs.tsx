"use client"

import { ColumnDef } from "@tanstack/react-table"
import { Checkbox } from "@/components/ui/checkbox"
import { DataTableColumnHeader } from "@/components/data-table"
import { Badge } from "@/components/ui/badge"
import { Radio } from "lucide-react"
import { Link } from "@inertiajs/react"

export interface LocationLog {
    id: string
    asset_id: string
    asset_name: string
    location_name: string
    type: string
    status: string
    rssi: number | null
    kalman_rssi: number | null
    distance: number | null
    reader_name: string | null
    updated_at: string
}

export const columns: ColumnDef<LocationLog>[] = [
    {
        id: "select",
        header: ({ table }) => (
            <Checkbox
                checked={
                    table.getIsAllPageRowsSelected() ||
                    (table.getIsSomePageRowsSelected() && "indeterminate")
                }
                onCheckedChange={(value) => table.toggleAllPageRowsSelected(!!value)}
                aria-label="Select all"
            />
        ),
        cell: ({ row }) => (
            <Checkbox
                checked={row.getIsSelected()}
                onCheckedChange={(value) => row.toggleSelected(!!value)}
                aria-label="Select row"
            />
        ),
        enableSorting: false,
        enableHiding: false,
    },
    {
        accessorKey: "asset_name",
        header: ({ column }) => (
            <DataTableColumnHeader column={column} title="Asset" />
        ),
        cell: ({ row }) => {
            const asset_id = row.original.asset_id;
            const name = row.getValue("asset_name") as string;
            return asset_id ? (
                <Link
                    href={`/assets/${asset_id}`}
                    className="font-medium text-primary underline hover:opacity-80"
                >
                    {name}
                </Link>
            ) : (
                <span className="font-medium text-muted-foreground">{name}</span>
            );
        },
    },
    {
        accessorKey: "location_name",
        header: ({ column }) => (
            <DataTableColumnHeader column={column} title="Location" />
        ),
    },
    {
        accessorKey: "type",
        header: ({ column }) => (
            <DataTableColumnHeader column={column} title="Type" />
        ),
        cell: ({ row }) => {
            const type = row.getValue("type") as string;
            return (
                <Badge variant={type === 'alert' ? 'destructive' : 'default'} className="text-xs">
                    {type}
                </Badge>
            );
        },
    },
    {
        accessorKey: "status",
        header: ({ column }) => (
            <DataTableColumnHeader column={column} title="Status" />
        ),
        cell: ({ row }) => {
            const status = row.getValue("status") as string;
            return (
                <Badge
                    variant={
                        status === 'present' ? 'default' :
                        status === 'not_found' ? 'destructive' :
                        status === 'out_of_range' ? 'secondary' :
                        'outline'
                    }
                    className="text-xs"
                >
                    {status.replace('_', ' ')}
                </Badge>
            );
        },
    },
    {
        accessorKey: "rssi",
        header: ({ column }) => (
            <DataTableColumnHeader column={column} title="RSSI" />
        ),
        cell: ({ row }) => {
            const rssi = row.getValue("rssi") as number | null;
            return <span className="font-mono text-sm">{rssi || '-'}</span>;
        },
    },
    {
        accessorKey: "distance",
        header: ({ column }) => (
            <DataTableColumnHeader column={column} title="Distance" />
        ),
        cell: ({ row }) => {
            const distance = row.getValue("distance") as number | null;
            return <span className="text-sm">{distance ? `${distance}m` : '-'}</span>;
        },
    },
    {
        accessorKey: "reader_name",
        header: ({ column }) => (
            <DataTableColumnHeader column={column} title="Reader" />
        ),
        cell: ({ row }) => {
            const reader = row.getValue("reader_name") as string | null;
            return reader ? (
                <div className="flex items-center gap-1">
                    <Radio className="h-3 w-3 text-muted-foreground" />
                    <span className="text-sm">{reader}</span>
                </div>
            ) : (
                <span className="text-sm text-muted-foreground">-</span>
            );
        },
    },
    {
        accessorKey: "updated_at",
        header: ({ column }) => (
            <DataTableColumnHeader column={column} title="Updated" />
        ),
        cell: ({ row }) => (
            <span className="text-sm text-muted-foreground">{row.getValue("updated_at")}</span>
        ),
    },
];
