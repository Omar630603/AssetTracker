"use client"

import { ColumnDef } from "@tanstack/react-table"
import { DataTableColumnHeader } from "@/components/data-table"
import { Badge } from "@/components/ui/badge"
import { Radio } from "lucide-react"

export interface AssetLog {
    id: string
    location_name: string
    type: string
    status: string
    rssi: number | null
    kalman_rssi: number | null
    distance: number | null
    reader_name: string | null
    updated_at: string
    time_ago: string
}

export const columns: ColumnDef<AssetLog>[] = [
    {
        accessorKey: "location_name",
        header: ({ column }) => (
            <DataTableColumnHeader column={column} title="Location" />
        ),
        cell: ({ row }) => <div className="font-medium">{row.getValue("location_name")}</div>,
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
                    {status.replace(/_/g, ' ')}
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
            return <span className="font-mono text-sm">{`${rssi} dBm` || '-'}</span>;
        },
    },
    {
        accessorKey: "kalman_rssi",
        header: ({ column }) => (
            <DataTableColumnHeader column={column} title="Kalman RSSI" />
        ),
        cell: ({ row }) => {
            const kalman_rssi = row.getValue("kalman_rssi") as number | null;
            return <span className="font-mono text-sm">{`${kalman_rssi} dBm` || '-'}</span>;
        },
    },
    {
        accessorKey: "distance",
        header: ({ column }) => (
            <DataTableColumnHeader column={column} title="Distance" />
        ),
        cell: ({ row }) => {
            const distance = row.getValue("distance") as number | null;
            return <span className="text-sm">{distance ? `${distance} m` : '-'}</span>;
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
        accessorKey: "time_ago",
        header: ({ column }) => (
            <DataTableColumnHeader column={column} title="Time" />
        ),
        cell: ({ row }) => (
            <span className="text-sm text-muted-foreground">{row.getValue("time_ago")}</span>
        ),
    },
];
