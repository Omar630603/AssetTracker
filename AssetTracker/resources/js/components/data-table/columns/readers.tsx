"use client"

import React, { useState } from "react";
import { ColumnDef } from "@tanstack/react-table"
import { MoreHorizontal, AlertCircle, Eye } from "lucide-react"
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
import { JsonViewer } from "@/components/json-viewer";

export interface Reader {
    id: string,
    name: string,
    location: string,
    location_id?: string,
    discovery_mode?: 'pattern' | 'explicit',
    config: any,
    config_fetched_at?: string,
    is_active?: boolean,
    tags?: {
        id: string,
        name: string,
        asset_name: string,
    }[],
    tag_ids?: string[],
}

export const columns: ColumnDef<Reader>[] = [
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
        accessorKey: "location",
        header: ({ column }) => (
            <DataTableColumnHeader column={column} title="Location" />
        ),
        cell: ({ row }) => {
            const location = row.getValue("location") as string;
            return <span className="text-muted-foreground">{location || "N/A"}</span>;
        },
    },

    {
        accessorKey: "discovery_mode",
        header: ({ column }) => (
            <DataTableColumnHeader column={column} title="Discovery Mode" />
        ),
        cell: ({ row }) => {
            const mode = row.getValue("discovery_mode") as string;
            const reader = row.original;
            const tags = reader.tags || [];
            const readerName = row.getValue("name") as string;
            const [openTagsModal, setOpenTagsModal] = useState(false);

            return (
                <>
                    <Badge
                        variant={mode === 'pattern' ? 'secondary' : 'outline'}
                        className={mode === 'explicit' && tags && tags.length > 0 ? 'cursor-pointer' : ''}
                        onClick={() => {
                            if (mode === 'explicit' && tags && tags.length > 0) {
                                setOpenTagsModal(true);
                            }
                        }}
                    >
                        {mode === 'pattern' ? 'Pattern' : 'Explicit'}

                        {mode === 'explicit' && tags && tags.length > 0 && (
                            <span className="ml-1">/ {tags.length} Tag(s)</span>
                        )}
                    </Badge>

                    {mode === 'explicit' && (
                        <PopupModal
                            isOpen={openTagsModal}
                            onClose={() => setOpenTagsModal(false)}
                            title={`Associated Tags - ${readerName}`}
                            size="md"
                        >
                            <div className="space-y-2">
                                {tags && tags.length > 0 ? (
                                    <div className="space-y-2">
                                        {tags.map((tag) => (
                                            <div key={tag.id} className="flex items-center justify-between p-2 rounded-lg border">
                                                <div>
                                                    <span className="font-medium">{tag.name}</span>
                                                    <span className="text-sm text-muted-foreground ml-2">
                                                        {tag.asset_name}
                                                    </span>
                                                </div>
                                            </div>
                                        ))}
                                    </div>
                                ) : (
                                    <p className="text-muted-foreground">No tags associated</p>
                                )}
                            </div>
                        </PopupModal>
                    )}
                </>
            );
        },
    },

    {
        accessorKey: "config",
        header: ({ column }) => (
            <DataTableColumnHeader column={column} title="Configuration" />
        ),
        cell: ({ row }) => {
            const config = row.getValue("config") as string;
            const readerName = row.getValue("name") as string;
            const [openConfigModal, setOpenConfigModal] = useState(false);
            const isActive = row.original.is_active ?? false;
            const last_time_config_fetched = row.original.config_fetched_at as string | undefined;

            return (
                <>
                    <Button
                        variant="ghost"
                        size="sm"
                        onClick={() => setOpenConfigModal(true)}
                        className="h-8 px-2 text-xs cursor-pointer"
                    >
                        View <Eye />
                    </Button>
                    <Badge
                        variant={isActive ? "default" : "destructive"}
                        className="ml-2"
                    >
                        {isActive ? "Active" : "Inactive"}
                    </Badge>
                    {last_time_config_fetched && (
                        <span className="ml-2 text-xs text-muted-foreground">
                            Last fetched: {new Date(last_time_config_fetched).toLocaleString()}
                        </span>
                    )}
                    <PopupModal
                        isOpen={openConfigModal}
                        onClose={() => setOpenConfigModal(false)}
                        title={`Configuration - ${readerName}`}
                        size="lg"
                    >
                        <div className="overflow-auto max-h-[60vh]">
                            <JsonViewer
                                data={config}
                                editable={false}
                                height="64"
                                className="mt-1"
                            />
                        </div>
                    </PopupModal>
                </>
            );
        },
    },

    {
        id: "actions",
        header: ({ column }) => (
            <DataTableColumnHeader column={column} title="Actions" />
        ),
        cell: ({ row }) => {
            const reader = row.original;
            const readerName = row.getValue("name") as string;
            const [openDeleteModal, setDeleteModal] = useState(false);
            const { delete: destroy, processing } = useForm();

            const [openSwitchModal, setOpenSwitchModal] = useState(false);
            const {post: switchPost, processing: processingSwitch} = useForm();

            const handleDelete = () => {
                destroy(`/readers/${reader.id}`, {
                    onSuccess: () => {
                        setDeleteModal(false);
                        toast.success("Deleted", {
                            description: `Reader "${reader.name}" has been deleted.`,
                            descriptionClassName: "!text-gray-800 dark:!text-gray-400"
                        });
                    },
                    onError: () => {
                        toast.error("Error", {
                            description: `Failed to delete reader "${reader.name}".`,
                            descriptionClassName: "!text-gray-800 dark:!text-gray-400"
                        });
                    },
                });
            };

            const handleSwitch = (action: 'enable' | 'disable') => {
                switchPost(`/readers/${reader.id}/switch`, {
                    onSuccess: () => {
                        setOpenSwitchModal(false);
                        toast.success(`Reader ${action === 'enable' ? 'enabled' : 'disabled'} successfully`, {
                            description: `Reader "${reader.name}" has been ${action === 'enable' ? 'enabled' : 'disabled'}.`,
                            descriptionClassName: "!text-gray-800 dark:!text-gray-400"
                        });
                    },
                    onError: () => {
                        toast.error(`Failed to ${action} reader "${reader.name}"`, {
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
                                <a href={`/readers/${reader.id}/edit`} className="flex w-full">
                                    Edit
                                </a>
                            </DropdownMenuItem>
                            <DropdownMenuItem
                                onSelect={(e) => {
                                    e.preventDefault();
                                    setOpenSwitchModal(true);
                                }}
                            >
                                {reader.is_active ? 'Disable' : 'Enable'}
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
                        title={`Delete Confirmation${readerName ? ` - ${readerName}` : ''}`}
                        variant="confirm"
                        size="md"
                        isLoading={processing}
                    >
                        <Alert>
                            <AlertCircle className="h-4 w-4" />
                            <AlertTitle>Warning</AlertTitle>
                            <AlertDescription>
                                Are you sure you want to delete this reader? This action cannot be undone.
                            </AlertDescription>
                        </Alert>
                    </PopupModal>
                    <PopupModal
                        isOpen={openSwitchModal}
                        onClose={() => setOpenSwitchModal(false)}
                        title={`${reader.is_active ? 'Disable' : 'Enable'} Reader - ${readerName}`}
                        size="md"
                        isLoading={processingSwitch}
                        onPrimaryAction={() => handleSwitch(reader.is_active ? 'disable' : 'enable')}
                        variant="confirm"
                    >
                        <Alert>
                            <AlertCircle className="h-4 w-4" />
                            <AlertTitle>{reader.is_active ? 'Disable' : 'Enable'} Reader</AlertTitle>
                            <AlertDescription>
                                Are you sure you want to {reader.is_active ? 'disable' : 'enable'} this reader? This will {reader.is_active ? 'stop' : 'start'} its operations.
                            </AlertDescription>
                        </Alert>
                    </PopupModal>
                </>
            );
        },
    },
];
