"use client"

import React, { useState } from "react";
import { ColumnDef } from "@tanstack/react-table"
import { MoreHorizontal, Eye, AlertCircle } from "lucide-react"
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
import { JsonViewer } from "@/components/json-viewer"
import { Alert, AlertDescription, AlertTitle } from "@/components/ui/alert";
import { toast } from "sonner"
import { useForm } from "@inertiajs/react";

export interface Reader {
    id: string
    name: string
    location: string
    targets: string
    config?: string
    tags?: Tag[]
}

interface Tag {
    id: string
    name: string
    asset_name: string
}

const TagsList: React.FC<{ tags?: Tag[], readerName?: string }> = ({ tags, readerName }) => {
    const [openTagsModal, setOpenTagsModal] = useState(false);

    if (!tags || tags.length === 0) {
        return <span className="text-muted-foreground">None</span>;
    }

    return (
        <>
            <div className="flex items-center space-x-2 cursor-pointer">
                <Button
                    variant="ghost"
                    size="sm"
                    className="h-6 p-0 px-1"
                    onClick={() => setOpenTagsModal(true)}
                >
                    <Eye className="h-3 w-3 mr-1" />
                    <span className="text-xs">View {tags.length} Tag(s)</span>
                </Button>
            </div>

            <PopupModal
                isOpen={openTagsModal}
                onClose={() => setOpenTagsModal(false)}
                title={`Target Tags${readerName ? ` - ${readerName}` : ''}`}
                description="Associated tags and assets"
                variant="view"
                size="md"
            >
                <div className="space-y-2">
                    {tags.map((tag) => (
                        <div key={tag.id} className="flex items-center p-2 border rounded">
                            <div>
                                <div className="font-medium">{tag.name}</div>
                                <div className="text-sm text-muted-foreground">
                                    {tag.asset_name}
                                </div>
                            </div>
                        </div>
                    ))}
                </div>
            </PopupModal>
        </>
    );
};

const JsonConfigPreview: React.FC<{ data: string, readerName?: string }> = ({ data, readerName }) => {
    const [openConfigJsonModal, setOpenConfigJsonModal] = useState(false);

    try {
        return (
            <>
                <div className="flex items-center space-x-2 cursor-pointer">
                    <Button
                        variant="ghost"
                        size="sm"
                        className="h-6 p-0 px-1"
                        onClick={() => setOpenConfigJsonModal(true)}
                    >
                        <Eye className="h-3 w-3 mr-1" />
                        <span className="text-xs">View</span>
                    </Button>
                </div>

                <PopupModal
                    isOpen={openConfigJsonModal}
                    onClose={() => setOpenConfigJsonModal(false)}
                    title={`Configuration Details${readerName ? ` - ${readerName}` : ''}`}
                    description="Full configuration properties"
                    variant="view"
                    size="md"
                >
                    <JsonViewer data={data} />
                </PopupModal>
            </>
        );
    } catch (e) {
        return <div className="text-xs text-red-500">Invalid JSON</div>;
    }
};

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
    },

    {
        accessorKey: "targets",
        header: ({ column }) => (
            <DataTableColumnHeader column={column} title="Targets" />
        ),
        cell: ({ row }) => {
            const readerName = row.getValue("name") as string;
            const tags = row.original.tags;

            return (
                <div className="flex items-center">
                    {tags && <TagsList tags={tags} readerName={readerName} />}
                </div>
            );
        },
    },

    {
        accessorKey: "config",
        header: ({ column }) => (
            <DataTableColumnHeader column={column} title="Config" />
        ),
        cell: ({ row }) => {
            const configValue = row.getValue("config");
            const readerName = row.getValue("name") as string;

            if (!configValue) return <span className="text-muted-foreground">None</span>;

            return <JsonConfigPreview data={configValue as string} readerName={readerName} />;
        },
    },

    {
        id: "actions",
        cell: ({ row }) => {
            const reader = row.original;
            const readerName = row.getValue("name") as string;
            const [openDeleteModal, setDeleteModal] = useState(false);
            const { delete: destroy, processing } = useForm();

            const handleDelete = () => {
                destroy(`/readers/${reader.id}`, {
                    onSuccess: () => {
                        setDeleteModal(false);
                        toast.success("Deleted", {
                            description: `Reader "${reader.name}" has been deleted.`,
                        });
                    },
                    onError: () => {
                        toast.error("Error", {
                            description: `Failed to delete reader "${reader.name}".`,
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
                                Are you sure you want to delete this reader?
                            </AlertDescription>
                        </Alert>
                    </PopupModal>
                </>
            );
        },
    },
];
