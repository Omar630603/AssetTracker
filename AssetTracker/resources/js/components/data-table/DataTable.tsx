"use client"

import * as React from "react"
import {
    ColumnDef,
    ColumnFiltersState,
    SortingState,
    VisibilityState,
    flexRender,
    getCoreRowModel,
    getFilteredRowModel,
    getPaginationRowModel,
    getSortedRowModel,
    useReactTable,
    RowSelectionState,
} from "@tanstack/react-table"

import {
    Table,
    TableBody,
    TableCell,
    TableHead,
    TableHeader,
    TableRow,
} from "@/components/ui/table"

import { Input } from "@/components/ui/input"
import { DataTableViewOptions } from "./DataTableViewOptions"
import { DataTablePagination } from "./DataTablePagination"

import { RotateCw } from "lucide-react";

interface DataTableProps<TData, TValue> {
    columns: ColumnDef<TData, TValue>[]
    data: TData[]
    searchColumn?: string[]
    searchPlaceholder?: string
    onRowSelectionChange?: (value: RowSelectionState) => void
    onRefresh?: () => void;
    isRefreshing?: boolean;
    state?: {
        rowSelection?: RowSelectionState
    }
}

export function DataTable<TData, TValue>({
    columns,
    data,
    searchColumn = ["name"], // Default search column
    searchPlaceholder = "Search...",
    onRowSelectionChange,
    onRefresh,
    isRefreshing = false,
    state = {},
}: DataTableProps<TData, TValue>) {
    const [sorting, setSorting] = React.useState<SortingState>([])
    const [columnFilters, setColumnFilters] = React.useState<ColumnFiltersState>([])
    const [columnVisibility, setColumnVisibility] = React.useState<VisibilityState>({})
    const [internalRowSelection, setInternalRowSelection] = React.useState<RowSelectionState>({})
    const [globalFilter, setGlobalFilter] = React.useState('')

    // Use external state if provided, otherwise use internal state
    const rowSelection = state.rowSelection !== undefined ? state.rowSelection : internalRowSelection
    // Wrap the handler to accept both value and updater function as per OnChangeFn<RowSelectionState>
    const handleRowSelectionChange: (updaterOrValue: RowSelectionState | ((old: RowSelectionState) => RowSelectionState)) => void =
        (updaterOrValue) => {
            if (onRowSelectionChange) {
                if (typeof updaterOrValue === "function") {
                    // updater function
                    onRowSelectionChange(updaterOrValue(rowSelection));
                } else {
                    // direct value
                    onRowSelectionChange(updaterOrValue);
                }
            } else {
                setInternalRowSelection(updaterOrValue);
            }
        };

    const table = useReactTable({
        data,
        columns,
        getCoreRowModel: getCoreRowModel(),
        getPaginationRowModel: getPaginationRowModel(),
        onSortingChange: setSorting,
        getSortedRowModel: getSortedRowModel(),
        onColumnFiltersChange: setColumnFilters,
        getFilteredRowModel: getFilteredRowModel(),
        onColumnVisibilityChange: setColumnVisibility,
        onRowSelectionChange: handleRowSelectionChange,
        onGlobalFilterChange: setGlobalFilter,
        state: {
            sorting,
            columnFilters,
            columnVisibility,
            rowSelection,
            globalFilter,
        },
        globalFilterFn: (row, columnId, filterValue) => {
            const search = String(filterValue).toLowerCase();
            return searchColumn.some(key => String(row.getValue(key)).toLowerCase().includes(search));
        },
    })

    return (
        <div className="space-y-4">
            <div className="flex items-center justify-between">
                {onRefresh && (
                    <button
                        type="button"
                        onClick={onRefresh}
                        disabled={isRefreshing}
                        className="flex items-center gap-2 h-9 rounded-md border border-input bg-transparent px-3 py-1 text-sm shadow-xs transition-colors outline-none disabled:pointer-events-none disabled:cursor-not-allowed disabled:opacity-50 focus-visible:border-ring focus-visible:ring-ring/50 focus-visible:ring-[3px] text-foreground mr-2 cursor-pointer"
                        style={{ width: "auto" }}
                        title="Refresh"
                    >
                        <RotateCw className={`w-4 h-4 ${isRefreshing ? 'animate-spin' : ''}`} /> Refresh
                    </button>
                )}
                <Input
                    placeholder={searchPlaceholder}
                    value={globalFilter}
                    onChange={(e) => setGlobalFilter(e.target.value)}
                    className="max-w-sm"
                />

                <DataTableViewOptions table={table} />
            </div>

            <div className="rounded-md border">
                <Table>
                    <TableHeader>
                        {table.getHeaderGroups().map((headerGroup) => (
                            <TableRow key={headerGroup.id}>
                                {headerGroup.headers.map((header) => (
                                    <TableHead key={header.id}>
                                        {header.isPlaceholder
                                            ? null
                                            : flexRender(
                                                header.column.columnDef.header,
                                                header.getContext()
                                            )}
                                    </TableHead>
                                ))}
                            </TableRow>
                        ))}
                    </TableHeader>
                    <TableBody>
                        {!isRefreshing ? (
                            table.getRowModel().rows?.length ? (
                                table.getRowModel().rows.map((row) => (
                                    <TableRow
                                        key={row.id}
                                        data-state={row.getIsSelected() && "selected"}
                                    >
                                        {row.getVisibleCells().map((cell) => (
                                            <TableCell key={cell.id}>
                                                {flexRender(cell.column.columnDef.cell, cell.getContext())}
                                            </TableCell>
                                        ))}
                                    </TableRow>
                                ))
                            ) : (
                                <TableRow>
                                    <TableCell colSpan={columns.length} className="h-24 text-center">
                                        No results.
                                    </TableCell>
                                </TableRow>
                            )
                        ) : (
                            <TableRow>
                                <TableCell colSpan={columns.length} className="h-24 text-center">
                                    Loading...
                                </TableCell>
                            </TableRow>
                        )}

                    </TableBody>
                </Table>
            </div>

            <DataTablePagination table={table} />
        </div>
    )
}
