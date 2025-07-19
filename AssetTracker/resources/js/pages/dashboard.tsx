"use client";

import { useState, useEffect, useRef } from "react";
import { Head, useForm, router } from "@inertiajs/react";
import {
    AlertCircle,
    Package,
    Radio,
    FileText,
    Trash2,
} from "lucide-react";
import AppLayout from "@/layouts/app-layout";
import { Button } from "@/components/ui/button";
import {
    Card,
    CardContent,
    CardDescription,
    CardHeader,
    CardTitle,
} from "@/components/ui/card";
import {
    Alert,
    AlertDescription,
    AlertTitle,
} from "@/components/ui/alert";
import { PopupModal } from "@/components/popup-modal";
import { DataTable } from "@/components/data-table";
import { LocationLog, columns } from "@/components/data-table/columns/dashboard-logs";
import { toast } from "sonner";
import {
    LineChart,
    Line,
    XAxis,
    YAxis,
    CartesianGrid,
    Tooltip,
    ResponsiveContainer,
    BarChart,
    Bar,
    Cell,
} from "recharts";
import { type BreadcrumbItem } from "@/types";

const breadcrumbs: BreadcrumbItem[] = [
    { title: "Dashboard", href: "/dashboard" },
];

interface DashboardStats {
    assets: {
        total: number;
        withTags: number;
    };
    readers: {
        total: number;
        active: number;
    };
    logs: {
        total: number;
        recent: number;
    };
}

interface ChartData {
    hour: string;
    count: number;
}

interface BarChartData {
    location: string;
    count: number;
}

interface SparklineData {
    day: string;
    count: number;
}

interface DashboardProps {
    stats: DashboardStats;
    logs: LocationLog[];
    chartData: ChartData[];
    topLocations: BarChartData[];
    sparklines: {
        logs: SparklineData[];
        assets: SparklineData[];
        readers: SparklineData[];
    };
}

export default function Dashboard({
    stats,
    logs,
    chartData,
    topLocations,
    sparklines,
}: DashboardProps) {
    const [logsData, setLogsData] = useState<LocationLog[]>(logs);
    const [openDeleteModal, setOpenDeleteModal] = useState(false);
    const [rowSelection, setRowSelection] = useState({});
    const [selectedLogIds, setSelectedLogIds] = useState<string[]>([]);
    const [refreshing, setRefreshing] = useState(false);
    const [autoRefresh, setAutoRefresh] = useState(true);
    const intervalRef = useRef<NodeJS.Timeout | null>(null);

    const { setData: setDeleteData, delete: deleteSelected, processing } = useForm({
        ids: [] as string[],
    });

    useEffect(() => {
        setLogsData(logs);
    }, [logs]);

    useEffect(() => {
        const selectedIds = Object.keys(rowSelection)
            .map((index) => {
                const numericIndex = parseInt(index);
                return logsData[numericIndex]?.id;
            })
            .filter(Boolean);

        setSelectedLogIds(selectedIds);
        setDeleteData('ids', selectedIds);
    }, [rowSelection, logsData]);

    useEffect(() => {
        if (intervalRef.current) {
            clearInterval(intervalRef.current);
            intervalRef.current = null;
        }

        if (!autoRefresh || refreshing) return;

        intervalRef.current = setInterval(() => {
            if (refreshing) return;
            router.reload({
                only: ["logs"],
                onSuccess: () => {
                    console.log('Auto-refresh completed');
                },
                onError: () => {
                    console.error('Auto-refresh failed');
                }
            });
        }, 10000);

        return () => {
            if (intervalRef.current) {
                console.log('Cleaning up auto-refresh interval');
                clearInterval(intervalRef.current);
                intervalRef.current = null;
            }
        };
    }, [autoRefresh]);

    useEffect(() => {
        return () => {
            if (intervalRef.current) {
                clearInterval(intervalRef.current);
            }
        };
    }, []);

    const handleRefreshLogs = () => {
        setRefreshing(true);
        setTimeout(() => {
            router.reload({
                only: ['logs'],
                onFinish: () => {
                    setRefreshing(false);
                    toast.success('Logs refreshed successfully');
                },
                onError: () => {
                    setRefreshing(false);
                    toast.error('Failed to refresh logs');
                },
            });
        }, 500);
    };

    const handleDeleteSelected = () => {
        if (selectedLogIds.length === 0) return;

        deleteSelected(`dashboard/logs/destroy/bulk`, {
            onSuccess: () => {
                toast.success("Selected logs deleted successfully");
            },
            onError: () => {
                toast.error("Failed to delete selected logs");
            },
            onFinish: () => {
                setOpenDeleteModal(false);
                setRowSelection({});
                setSelectedLogIds([]);
            }
        });
    };

    const hasSelections = Object.keys(rowSelection).length > 0;

    // Colors for bar chart
    const barColors = ["#6366F1", "#0EA5E9", "#10B981", "#F59E42", "#EF4444"];

    // Mini sparkline chart component
    const Sparkline = ({ data }: { data: SparklineData[] }) => (
        <ResponsiveContainer width="100%" height={30}>
            <LineChart data={data}>
                <Line
                    type="monotone"
                    dataKey="count"
                    stroke="#6366F1"
                    strokeWidth={2}
                    dot={false}
                />
            </LineChart>
        </ResponsiveContainer>
    );

    return (
        <AppLayout breadcrumbs={breadcrumbs}>
            <Head title="Dashboard" />
            <div className="container mx-auto p-4">
                {/* Stat Cards with Sparklines */}
                <div className="grid gap-4 md:grid-cols-3 mb-6">
                    <Card>
                        <CardHeader className="flex flex-row items-center justify-between space-y-0 pb-2">
                            <CardTitle className="text-sm font-medium">Tags/Assets</CardTitle>
                            <Package className="h-4 w-4 text-muted-foreground" />
                        </CardHeader>
                        <CardContent>
                            <div className="text-2xl font-bold">{stats.assets.total}</div>
                            <p className="text-xs text-muted-foreground">
                                {stats.assets.withTags} assets with tags assigned
                            </p>
                            <div className="mt-2 h-6">
                                <Sparkline data={sparklines.assets} />
                            </div>
                        </CardContent>
                    </Card>

                    <Card>
                        <CardHeader className="flex flex-row items-center justify-between space-y-0 pb-2">
                            <CardTitle className="text-sm font-medium">Readers</CardTitle>
                            <Radio className="h-4 w-4 text-muted-foreground" />
                        </CardHeader>
                        <CardContent>
                            <div className="text-2xl font-bold">{stats.readers.total}</div>
                            <p className="text-xs text-muted-foreground">
                                {stats.readers.active} active in last hour
                            </p>
                            <div className="mt-2 h-6">
                                <Sparkline data={sparklines.readers} />
                            </div>
                        </CardContent>
                    </Card>

                    <Card>
                        <CardHeader className="flex flex-row items-center justify-between space-y-0 pb-2">
                            <CardTitle className="text-sm font-medium">Location Logs</CardTitle>
                            <FileText className="h-4 w-4 text-muted-foreground" />
                        </CardHeader>
                        <CardContent>
                            <div className="text-2xl font-bold">{stats.logs.total}</div>
                            <p className="text-xs text-muted-foreground">
                                {stats.logs.recent} logs in last 24 hours
                            </p>
                            <div className="mt-2 h-6">
                                <Sparkline data={sparklines.logs} />
                            </div>
                        </CardContent>
                    </Card>
                </div>

                {/* Two big charts side by side */}
                <div className="grid gap-4 md:grid-cols-2 mb-6">
                    {/* Left: Logs per Hour */}
                    <Card>
                        <CardHeader>
                            <CardTitle>Logs per Hour</CardTitle>
                            <CardDescription>
                                Location logs in the last 24 hours
                            </CardDescription>
                        </CardHeader>
                        <CardContent>
                            <div className="h-[260px]">
                                <ResponsiveContainer width="100%" height="100%">
                                    <LineChart data={chartData}>
                                        <CartesianGrid strokeDasharray="4 2" strokeOpacity={0.12} />
                                        <XAxis dataKey="hour" fontSize={10} tickMargin={10} />
                                        <YAxis fontSize={10} tickMargin={10} />
                                        <Tooltip
                                            contentStyle={{
                                                background: "rgba(34, 39, 45, 0.1)",
                                                backdropFilter: "blur(10px)",
                                                WebkitBackdropFilter: "blur(10px)",
                                                borderRadius: "0.75rem",
                                                border: "1px solid rgba(180,180,200,0.22)",
                                                fontSize: 12,
                                                boxShadow: "0 4px 24px 0 rgba(30,30,30,0.07)",
                                                minWidth: 80,
                                            }}
                                            labelStyle={{
                                                color: "#8e8e93",
                                            }}
                                        />
                                        <Line
                                            type="monotone"
                                            dataKey="count"
                                            stroke="#6366F1"
                                            strokeWidth={2.5}
                                            dot={false}
                                            activeDot={{ r: 4 }}
                                        />
                                    </LineChart>
                                </ResponsiveContainer>
                            </div>
                        </CardContent>
                    </Card>

                    {/* Right: Top 5 Locations by Log Count */}
                    <Card>
                        <CardHeader>
                            <CardTitle>Top 5 Locations by Logs</CardTitle>
                            <CardDescription>
                                Most active locations (all time)
                            </CardDescription>
                        </CardHeader>
                        <CardContent>
                            <div className="h-[260px] flex items-center">
                                <ResponsiveContainer width="100%" height="100%">
                                    <BarChart
                                        data={topLocations}
                                        layout="vertical"
                                        margin={{ top: 5, right: 30, left: 5, bottom: 5 }}
                                    >
                                        <CartesianGrid strokeDasharray="3 3" strokeOpacity={0.12} />
                                        <XAxis type="number" fontSize={10} tickMargin={10} />
                                        <YAxis dataKey="location" type="category" fontSize={12} width={90} tickMargin={10} />
                                        <Tooltip
                                            cursor={false}
                                            contentStyle={{
                                                background: "rgba(34, 39, 45, 0.1)",
                                                backdropFilter: "blur(10px)",
                                                WebkitBackdropFilter: "blur(10px)",
                                                borderRadius: "0.75rem",
                                                border: "1px solid rgba(180,180,200,0.22)",
                                                fontSize: 12,
                                                boxShadow: "0 4px 24px 0 rgba(30,30,30,0.07)",
                                                minWidth: 80,
                                            }}
                                            labelStyle={{
                                                color: "#fff",
                                            }}
                                        />
                                        <Bar dataKey="count">
                                            {topLocations.map((entry, index) => (
                                                <Cell key={`cell-${index}`} fill={barColors[index % barColors.length]} />
                                            ))}
                                        </Bar>
                                    </BarChart>
                                </ResponsiveContainer>
                            </div>
                        </CardContent>
                    </Card>
                </div>

                {/* Table */}
                <Card>
                    <CardHeader>
                        <div className="flex items-center justify-between">
                            <div>
                                <CardTitle>Recent Location Logs</CardTitle>
                                <CardDescription>Latest asset location updates</CardDescription>
                            </div>
                            {hasSelections && (
                                <Button
                                    variant="destructive"
                                    size="sm"
                                    onClick={() => setOpenDeleteModal(true)}
                                >
                                    <Trash2 className="h-4 w-4 mr-2" />
                                    Delete Selected ({selectedLogIds.length})
                                </Button>
                            )}
                        </div>
                    </CardHeader>
                    <CardContent>
                        <DataTable
                            columns={columns}
                            data={logsData}
                            searchColumn={["asset_name", "location_name", "type", "status", "reader_name"]}
                            searchPlaceholder="Search logs..."
                            onRowSelectionChange={setRowSelection}
                            onRefresh={handleRefreshLogs}
                            isRefreshing={refreshing}
                            setAutoRefresh={setAutoRefresh}
                            autoRefresh={autoRefresh}
                            state={{ rowSelection }}
                        />
                    </CardContent>
                </Card>

                {/* Delete Modal */}
                <PopupModal
                    isOpen={openDeleteModal}
                    onClose={() => setOpenDeleteModal(false)}
                    onPrimaryAction={handleDeleteSelected}
                    title="Delete Selected Logs"
                    variant="confirm"
                    size="md"
                    primaryActionLabel="Delete"
                    isLoading={processing}
                >
                    <Alert>
                        <AlertCircle className="h-4 w-4" />
                        <AlertTitle>Warning</AlertTitle>
                        <AlertDescription>
                            Are you sure you want to delete {selectedLogIds.length} selected log(s)?
                            This action cannot be undone.
                        </AlertDescription>
                    </Alert>
                </PopupModal>
            </div>
        </AppLayout>
    );
}
