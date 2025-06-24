"use client"

import React, { useState } from "react";
import { ColumnDef } from "@tanstack/react-table"
import { MoreHorizontal, AlertCircle } from "lucide-react"
import { Button } from "@/components/ui/button"
import { Checkbox } from "@/components/ui/checkbox"
import {
    DropdownMenu,
    DropdownMenuContent,
    DropdownMenuItem,
    DropdownMenuLabel,
    DropdownMenuSeparator,
    DropdownMenuTrigger,
} from "@/components/ui/dropdown-menu"
import { DataTableColumnHeader } from "@/components/data-table"
import { PopupModal } from "@/components/popup-modal"
import { Alert, AlertDescription, AlertTitle } from "@/components/ui/alert";
import { toast } from "sonner"
import { useForm } from "@inertiajs/react";

export interface Asset {
    id: string,
    name: string,
    type: string,
    tag_id?: string,
    tag_name?: string,
    location_name?: string,
}

export const columns: ColumnDef<Asset>[] = [
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
        accessorKey: "name",
        header: ({ column }) => (
            <DataTableColumnHeader column={column} title="Name" />
        ),
        cell: ({ row }) => <div className="font-medium">{row.getValue("name")}</div>,
    },

    {
        accessorKey: "type",
        header: ({ column }) => (
            <DataTableColumnHeader column={column} title="Type" />
        ),
        cell: ({ row }) => {
            const type = row.getValue("type") as string;
            return <span className="text-muted-foreground">{type || "N/A"}</span>;
        },
    },

    {
        accessorKey: "tag_name",
        header: ({ column }) => (
            <DataTableColumnHeader column={column} title="Tag" />
        ),
        cell: ({ row }) => {
            const tag_name = row.getValue("tag_name") as string;
            return <span className="text-muted-foreground">{tag_name || "N/A"}</span>;
        },
    },

    {
        accessorKey: "location_name",
        header: ({ column }) => (
            <DataTableColumnHeader column={column} title="Location" />
        ),
        cell: ({ row }) => {
            const location_name = row.getValue("location_name") as string;
            return <span className="text-muted-foreground">{location_name || "N/A"}</span>;
        },
    },

    {
        id: "actions",
        header: ({ column }) => (
            <DataTableColumnHeader column={column} title="Actions" />
        ),
        cell: ({ row }) => {
            const asset = row.original;
            const assetName = row.getValue("name") as string;
            const [openDeleteModal, setDeleteModal] = useState(false);
            const { delete: destroy, processing } = useForm();

            const handleDelete = () => {
                destroy(`/assets/${asset.id}`, {
                    onSuccess: () => {
                        setDeleteModal(false);
                        toast.success("Deleted", {
                            description: `Asset "${asset.name}" has been deleted.`,
                            descriptionClassName: "!text-gray-800 dark:!text-gray-400"
                        });
                    },
                    onError: () => {
                        toast.error("Error", {
                            description: `Failed to delete asset "${asset.name}".`,
                            descriptionClassName: "!text-gray-800 dark:!text-gray-400"
                        });
                    },
                });
            };

            return (
                <>
                    <DropdownMenu modal={false}>
                        <DropdownMenuTrigger asChild>
                            <Button variant="ghost" className="h-8 w-8 p-0">
                                <span className="sr-only">Open menu</span>
                                <MoreHorizontal className="h-4 w-4" />
                            </Button>
                        </DropdownMenuTrigger>
                        <DropdownMenuContent align="end">
                            <DropdownMenuLabel>Actions</DropdownMenuLabel>
                            <DropdownMenuSeparator />
                            <DropdownMenuItem>
                                <a href={`/assets/${asset.id}`} className="flex w-full">
                                    View
                                </a>
                            </DropdownMenuItem>
                            <DropdownMenuItem>
                                <a href={`/assets/${asset.id}/edit`} className="flex w-full">
                                    Edit
                                </a>
                            </DropdownMenuItem>
                            <DropdownMenuItem className="text-red-500"
                                onSelect={(e) => { e.preventDefault(); setDeleteModal(true); }}
                            >
                                Delete
                            </DropdownMenuItem>
                        </DropdownMenuContent>
                    </DropdownMenu>
                    <PopupModal
                        isOpen={openDeleteModal}
                        onClose={() => setDeleteModal(false)}
                        onPrimaryAction={handleDelete}
                        title={`Delete Confirmation${assetName ? ` - ${assetName}` : ''}`}
                        variant="confirm"
                        size="md"
                        isLoading={processing}
                    >
                        <Alert>
                            <AlertCircle className="h-4 w-4" />
                            <AlertTitle>Warning</AlertTitle>
                            <AlertDescription>
                                Are you sure you want to delete this asset?
                            </AlertDescription>
                        </Alert>
                    </PopupModal>
                </>
            );
        },
    },
];
