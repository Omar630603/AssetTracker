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

export interface Tag {
    id: string
    name: string
    asset_name?: string
}

export const columns: ColumnDef<Tag>[] = [
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
        accessorKey: "asset_name",
        header: ({ column }) => (
            <DataTableColumnHeader column={column} title="Asset" />
        ),
        cell: ({ row }) => {
            const asset_name = row.getValue("asset_name") as string;
            return <span className="text-muted-foreground">{asset_name || "N/A"}</span>;
        },
    },

    {
        id: "actions",
        cell: ({ row }) => {
            const tag = row.original;
            const tagName = row.getValue("name") as string;
            const [openDeleteModal, setDeleteModal] = useState(false);
            const { delete: destroy, processing } = useForm();

            const handleDelete = () => {
                destroy(`/tags/${tag.id}`, {
                    onSuccess: () => {
                        setDeleteModal(false);
                        toast.success("Deleted", {
                            description: `Tag "${tag.name}" has been deleted.`,
                            descriptionClassName: "!text-gray-800 dark:!text-gray-400"
                        });
                    },
                    onError: () => {
                        toast.error("Error", {
                            description: `Failed to delete tag "${tag.name}".`,
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
                                <a href={`/tags/${tag.id}/edit`} className="flex w-full">
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
                        title={`Delete Confirmation${tagName ? ` - ${tagName}` : ''}`}
                        variant="confirm"
                        size="md"
                        isLoading={processing}
                    >
                        <Alert>
                            <AlertCircle className="h-4 w-4" />
                            <AlertTitle>Warning</AlertTitle>
                            <AlertDescription>
                                Are you sure you want to delete this tag?
                            </AlertDescription>
                        </Alert>
                    </PopupModal>
                </>
            );
        },
    },
];
