"use client"

import { Head, Link, router } from '@inertiajs/react';
import { ArrowLeft, MapPin, Tag, Clock } from 'lucide-react';

import AppLayout from '@/layouts/app-layout';
import { Card, CardContent, CardDescription, CardHeader, CardTitle } from '@/components/ui/card';
import { Button } from '@/components/ui/button';
import { DataTable } from '@/components/data-table';
import { AssetLog, columns } from '@/components/data-table/columns/asset-logs';
import { type BreadcrumbItem } from '@/types';
import { ResponsiveContainer, LineChart, Line, XAxis, YAxis, Tooltip, CartesianGrid, Legend } from 'recharts';
import { useState } from 'react';
import { toast } from 'sonner';

interface AssetData {
    id: string;
    name: string;
    type: string | null;
    tag_name: string | null;
    location_name: string | null;
    location_id: string | null;
    last_seen: {
        location_name: string;
        location_id: string;
        updated_at: string;
        time_ago: string;
    } | null;
}

interface AssetShowProps {
    asset: AssetData;
    recentLogs: AssetLog[];
}

const breadcrumbs: BreadcrumbItem[] = [
    { title: 'Assets', href: '/assets' },
    { title: 'Asset Details', href: '#' },
];

export default function AssetShow({ asset, recentLogs }: AssetShowProps) {
    const [refreshing, setRefreshing] = useState(false);

    const handleRefreshLogs = () => {
        setRefreshing(true);
        setTimeout(() => {
            router.reload({
                only: ['recentLogs'],
                onFinish: () => {
                setRefreshing(false);
                toast.success('Logs refreshed successfully');
            },onError: () => {
                    setRefreshing(false);
                    toast.error('Failed to refresh logs');
                }
            });
        }, 1000);
    };

    return (
        <AppLayout breadcrumbs={breadcrumbs}>
            <Head title={`Asset: ${asset.name}`} />

            <div className="container mx-auto p-4">
                {/* Header */}
                <div className="flex items-center justify-between mb-6">
                    <div className="flex items-center gap-4">
                        <Link href="/assets">
                            <Button variant="ghost" size="sm">
                                <ArrowLeft className="h-4 w-4 mr-2" />
                                Back to Assets
                            </Button>
                        </Link>
                        <h1 className="text-lg font-bold">{asset.name}</h1>
                    </div>
                    <Link href={`/assets/${asset.id}/edit`}>
                        <Button>Edit Asset</Button>
                    </Link>
                </div>

                {/* Asset Info Cards */}
                <div className="grid gap-4 md:grid-cols-2 mb-6">
                    <Card>
                        <CardHeader>
                            <CardTitle>Asset Information</CardTitle>
                            <CardDescription>Basic details about this asset</CardDescription>
                        </CardHeader>
                        <CardContent>
                            <div className="space-y-3">
                                <div className="flex items-center justify-between">
                                    <span className="text-sm text-muted-foreground">Type</span>
                                    <span className="text-sm font-medium">{asset.type || 'Not specified'}</span>
                                </div>
                                <div className="flex items-center justify-between">
                                    <span className="text-sm text-muted-foreground">Tag</span>
                                    <div className="flex items-center gap-2">
                                        <Tag className="h-4 w-4 text-muted-foreground" />
                                        <span className="text-sm font-medium">{asset.tag_name || 'No tag assigned'}</span>
                                    </div>
                                </div>
                                <div className="flex items-center justify-between">
                                    <span className="text-sm text-muted-foreground">Current Location</span>
                                    <div className="flex items-center gap-2">
                                        <MapPin className="h-4 w-4 text-muted-foreground" />
                                        <span className="text-sm font-medium">{asset.location_name || 'Unknown'}</span>
                                    </div>
                                </div>
                            </div>
                        </CardContent>
                    </Card>

                    <Card>
                        <CardHeader>
                            <CardTitle>Last Seen</CardTitle>
                            <CardDescription>Latest tracking information</CardDescription>
                        </CardHeader>
                        <CardContent>
                            {asset.last_seen ? (
                                <div className="space-y-3">
                                    <div className="flex items-center justify-between">
                                        <span className="text-sm text-muted-foreground">Location</span>
                                        <div className="flex items-center gap-2">
                                            <MapPin className="h-4 w-4 text-muted-foreground" />
                                            <span className="text-sm font-medium">{asset.last_seen.location_name}</span>
                                        </div>
                                    </div>
                                    <div className="flex items-center justify-between">
                                        <span className="text-sm text-muted-foreground">Time</span>
                                        <div className="flex items-center gap-2">
                                            <Clock className="h-4 w-4 text-muted-foreground" />
                                            <span className="text-sm font-medium">{asset.last_seen.time_ago}</span>
                                        </div>
                                    </div>
                                    <div className="flex items-center justify-between">
                                        <span className="text-sm text-muted-foreground">Timestamp</span>
                                        <span className="text-sm font-mono">{asset.last_seen.updated_at}</span>
                                    </div>
                                </div>
                            ) : (
                                <p className="text-sm text-muted-foreground">No tracking data available</p>
                            )}
                        </CardContent>
                    </Card>
                </div>

                {/* Recent Logs */}
                <Card>
                    <CardHeader>
                        <CardTitle>Recent Location Logs</CardTitle>
                        <CardDescription>Latest tracking history for this asset</CardDescription>
                    </CardHeader>
                    <CardContent>
                        <Card className="mb-6">
                            <CardHeader>
                                <CardTitle>Recent RSSI History</CardTitle>
                                <CardDescription>
                                    Last 20 RSSI readings for this asset
                                </CardDescription>
                            </CardHeader>
                            <CardContent>
                                <div className="h-[240px] bg-background rounded-lg border p-2 flex items-center justify-center">
                                    <ResponsiveContainer width="100%" height="100%">
                                        <LineChart data={recentLogs}>
                                            <CartesianGrid strokeDasharray="4 2" strokeOpacity={0.1} />
                                            <XAxis dataKey="updated_at" fontSize={10} tickMargin={10} />
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
                                            <Legend
                                                verticalAlign="top"
                                                align="right"
                                                iconType="plainline"
                                                />
                                            <Line
                                                type="monotone"
                                                dataKey="rssi"
                                                stroke="#444"
                                                strokeWidth={2.5}
                                                dot={false}
                                                activeDot={{ r: 4 }}
                                            />
                                            <Line
                                                type="monotone"
                                                dataKey="kalman_rssi"
                                                stroke="#1e90ff"
                                                strokeWidth={2}
                                                dot={false}
                                                activeDot={{ r: 4 }}
                                            />
                                        </LineChart>
                                    </ResponsiveContainer>
                                </div>
                            </CardContent>
                        </Card>
                        <DataTable
                            columns={columns}
                            data={recentLogs}
                            searchColumn={["location_name", "status"]}
                            searchPlaceholder="Search logs..."
                            onRefresh={handleRefreshLogs}
                            isRefreshing={refreshing}
                        />
                    </CardContent>
                </Card>
            </div>
        </AppLayout>
    );
}
