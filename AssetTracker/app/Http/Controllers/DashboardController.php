<?php

namespace App\Http\Controllers;

use App\Models\Asset;
use App\Models\AssetLocationLog;
use App\Models\Reader;
use Carbon\Carbon;
use Illuminate\Http\Request;
use Illuminate\Support\Facades\DB;

class DashboardController extends Controller
{
    public function index()
    {
        // Get stats
        $totalAssets = Asset::count();
        $assetsWithTags = Asset::whereNotNull('tag_id')->count();

        $totalReaders = Reader::count();
        $activeReaders = Reader::whereHas('locationLogs', function ($query) {
            $query->where('updated_at', '>=', Carbon::now()->subHours(1));
        })->count();

        $totalLogs = AssetLocationLog::count();
        $recentLogs = AssetLocationLog::where('created_at', '>=', Carbon::now()->subHours(24))->count();

        // Get recent logs for the table
        $logs = AssetLocationLog::with(['asset', 'location'])
            ->orderBy('updated_at', 'desc')
            ->take(50)
            ->get()
            ->map(function ($log) {
                return [
                    'id' => $log->id,
                    'asset_id' => $log->asset->id ?? null,
                    'asset_name' => $log->asset->name ?? 'Unknown',
                    'location_name' => $log->location->name ?? 'Unknown',
                    'type' => $log->type,
                    'status' => $log->status,
                    'rssi' => $log->rssi,
                    'kalman_rssi' => $log->kalman_rssi,
                    'distance' => $log->estimated_distance,
                    'reader_name' => $log->reader_name,
                    'updated_at' => $log->updated_at->format('Y-m-d H:i:s'),
                ];
            });

        // Get chart data for LineChart (Logs per hour)
        $chartData = AssetLocationLog::select(
            DB::raw('HOUR(created_at) as hour'),
            DB::raw('COUNT(*) as count')
        )
            ->where('created_at', '>=', Carbon::now()->subHours(24))
            ->groupBy('hour')
            ->orderBy('hour')
            ->get()
            ->map(function ($item) {
                return [
                    'hour' => $item->hour . ':00',
                    'count' => $item->count
                ];
            });

        // New: Top 5 locations by log count (BarChart)
        $topLocations = AssetLocationLog::with('location')
            ->select('location_id', DB::raw('count(*) as count'))
            ->groupBy('location_id')
            ->orderByDesc('count')
            ->take(5)
            ->get()
            ->map(function ($item) {
                return [
                    'location' => $item->location->name ?? 'Unknown',
                    'count' => $item->count
                ];
            });

        // New: sparklines for stat cards (last 7 days for logs)
        $logsSparkline = AssetLocationLog::select(
            DB::raw('DATE(created_at) as day'),
            DB::raw('COUNT(*) as count')
        )
            ->where('created_at', '>=', Carbon::now()->subDays(7))
            ->groupBy('day')
            ->orderBy('day')
            ->get()
            ->map(function ($item) {
                return [
                    'day' => $item->day,
                    'count' => $item->count
                ];
            });

        // Assets sparkline: total assets per day
        $assetsSparkline = Asset::select(
            DB::raw('DATE(created_at) as day'),
            DB::raw('COUNT(*) as count')
        )
            ->where('created_at', '>=', Carbon::now()->subDays(7))
            ->groupBy('day')
            ->orderBy('day')
            ->get()
            ->map(function ($item) {
                return [
                    'day' => $item->day,
                    'count' => $item->count
                ];
            });

        // Readers sparkline: readers "active" per day (for demo, just randomize for now)
        $readerSparkline = collect(range(0, 6))->map(function ($i) use ($totalReaders) {
            return [
                'day' => Carbon::now()->subDays(6 - $i)->format('Y-m-d'),
                'count' => rand(0, $totalReaders),
            ];
        });

        return inertia('dashboard', [
            'stats' => [
                'assets' => [
                    'total' => $totalAssets,
                    'withTags' => $assetsWithTags,
                ],
                'readers' => [
                    'total' => $totalReaders,
                    'active' => $activeReaders,
                ],
                'logs' => [
                    'total' => $totalLogs,
                    'recent' => $recentLogs,
                ],
            ],
            'logs' => $logs,
            'chartData' => $chartData,
            'topLocations' => $topLocations,
            'sparklines' => [
                'logs' => $logsSparkline,
                'assets' => $assetsSparkline,
                'readers' => $readerSparkline,
            ],
        ]);
    }

    public function deleteLogs(Request $request)
    {
        $request->validate([
            'ids' => 'required|array',
            'ids.*' => 'exists:asset_location_logs,id',
        ]);

        AssetLocationLog::whereIn('id', $request->ids)->delete();

        return back()->with('success', count($request->ids) . ' log(s) deleted successfully');
    }
}
