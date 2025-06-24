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
import { Badge } from "@/components/ui/badge";

export interface User {
    id: string
    name: string
    username: string
    role: string
    department?: string | null
    position?: string | null
    email: string
}

export const columns: ColumnDef<User>[] = [
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
        cell: ({ row }) =>
            <div className="font-medium">
                {row.getValue("name")}
                <br />
                <span className="text-sm text-muted-foreground">
                    {row.original.username}
                </span>
            </div>,
    },

    {
        accessorKey: "email",
        header: ({ column }) => (
            <DataTableColumnHeader column={column} title="Email" />
        ),
        cell: ({ row }) => (
            <span className="text-sm text-muted-foreground">
                {row.getValue("email")}
            </span>
        ),
    },

    {
        accessorKey: "role",
        header: ({ column }) => (
            <DataTableColumnHeader column={column} title="Role" />
        ),
        cell: ({ row }) => (
            <Badge variant="outline" className="flex flex-col items-start py-2 capitalize">
                <span>{row.original.role}</span>
                {row.original.role === "staff" && (
                    <span className="text-xs text-muted-foreground">
                        {row.original.department || "No Department"} - {row.original.position || "No Position"}
                    </span>
                )}
            </Badge>
        ),
    },

    {
        id: "actions",
        header: ({ column }) => (
            <DataTableColumnHeader column={column} title="Actions" />
        ),
        cell: ({ row }) => {
            const user = row.original;
            const userName = row.getValue("name") as string;
            const [openDeleteModal, setDeleteModal] = useState(false);
            const { delete: destroy, processing } = useForm();

            const handleDelete = () => {
                destroy(`/users/${user.id}`, {
                    onSuccess: () => {
                        setDeleteModal(false);
                        toast.success("Deleted", {
                            description: `User "${user.name}" has been deleted.`,
                            descriptionClassName: "!text-gray-800 dark:!text-gray-400"
                        });
                    },
                    onError: () => {
                        toast.error("Error", {
                            description: `Failed to delete user "${user.name}".`,
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
                                <a href={`/users/${user.id}/edit`} className="flex w-full">
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
                        title={`Delete Confirmation${userName ? ` - ${userName}` : ''}`}
                        variant="confirm"
                        size="md"
                        isLoading={processing}
                    >
                        <Alert>
                            <AlertCircle className="h-4 w-4" />
                            <AlertTitle>Warning</AlertTitle>
                            <AlertDescription>
                                Are you sure you want to delete this user?
                            </AlertDescription>
                        </Alert>
                    </PopupModal>
                </>
            );
        },
    },
];
