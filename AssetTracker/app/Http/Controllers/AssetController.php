<?php

namespace App\Http\Controllers;

use App\Models\Asset;
use App\Models\AssetLocationLog;
use App\Models\Tag;
use Illuminate\Http\Request;
use Illuminate\Support\Facades\Validator;

class AssetController extends Controller
{
    public function index()
    {
        $assets = Asset::with('tag')->get()->map(function ($asset) {
            return [
                'id' => $asset->id,
                'name' => $asset->name,
                'type' => Asset::TYPES[$asset->type] ?? null,
                'tag_name' => $asset->tag ? $asset->tag->name : null,
                'location_name' => $asset->location ? $asset->location->name : null,
            ];
        });

        $asset_types = collect(Asset::TYPES)->map(function ($name, $id) {
            return [
                'id' => $id,
                'name' => $name,
            ];
        })->values();

        $availableTags = Tag::whereDoesntHave('asset')->get();

        return inertia('Assets/index', [
            'assets' => $assets,
            'available_tags' => $availableTags,
            'asset_types' => $asset_types,
        ]);
    }

    public function show($id)
    {
        $asset = Asset::with(['tag', 'location'])->findOrFail($id);

        $lastLog = AssetLocationLog::where('asset_id', $asset->id)
            ->with('location')
            ->orderBy('updated_at', 'desc')
            ->first();

        $recentLogs = AssetLocationLog::where('asset_id', $asset->id)
            ->with('location')
            ->orderBy('updated_at', 'desc')
            ->take(20)
            ->get()
            ->map(function ($log) {
                return [
                    'id' => $log->id,
                    'location_name' => $log->location->name ?? 'Unknown',
                    'type' => $log->type,
                    'status' => $log->status,
                    'rssi' => $log->rssi,
                    'kalman_rssi' => $log->kalman_rssi,
                    'distance' => $log->estimated_distance,
                    'reader_name' => $log->reader_name,
                    'updated_at' => $log->updated_at->format('Y-m-d H:i:s'),
                    'time_ago' => $log->updated_at->diffForHumans(),
                ];
            });

        return inertia('Assets/show', [
            'asset' => [
                'id' => $asset->id,
                'name' => $asset->name,
                'type' => Asset::TYPES[$asset->type] ?? null,
                'tag_name' => $asset->tag ? $asset->tag->name : null,
                'location_name' => $asset->location ? $asset->location->name : null,
                'location_id' => $asset->location_id,
                'last_seen' => $lastLog ? [
                    'location_name' => $lastLog->location->name ?? 'Unknown',
                    'location_id' => $lastLog->location_id,
                    'updated_at' => $lastLog->updated_at->format('Y-m-d H:i:s'),
                    'time_ago' => $lastLog->updated_at->diffForHumans(),
                ] : null,
            ],
            'recentLogs' => $recentLogs,
        ]);
    }

    public function logs($id)
    {
        $asset = Asset::findOrFail($id);
        $logs = AssetLocationLog::where('asset_id', $asset->id)
            ->whereNotNull('rssi')
            ->orderBy('created_at', 'desc')
            ->take(100)
            ->get()
            ->map(function ($log) {
                return [
                    'timestamp' => $log->created_at->format('H:i:s'),
                    'rssi' => $log->rssi,
                    'kalman_rssi' => $log->kalman_rssi,
                ];
            })
            ->reverse()
            ->values();

        return response()->json([
            'logs' => $logs,
        ]);
    }

    public function store(Request $request)
    {
        try {
            $validator = Validator::make($request->all(), [
                'name' => 'required|string|max:255|unique:assets,name',
                'type' => 'nullable|in:' . implode(',', array_keys(Asset::TYPES)),
                'tag_id' => [
                    'nullable',
                    'exists:tags,id',
                    function ($attribute, $value, $fail) {
                        if ($value && Asset::where('tag_id', $value)->exists()) {
                            $fail('This tag is already associated with another asset.');
                        }
                    }
                ],
            ]);
            if ($validator->fails()) {
                return back()->withErrors($validator)->withInput();
            }

            Asset::create([
                'name' => $request->name,
                'type' => $request->type,
                'tag_id' => $request->tag_id,
            ]);

            return redirect()->route('assets.index')->with('success', 'Asset created successfully');
        } catch (\Exception $e) {
            return back()->withErrors([
                'form' => $e->getMessage() ?: 'An unexpected error occurred while saving the asset.',
            ])->withInput();
        }
    }

    public function edit($id)
    {
        $asset = Asset::findOrFail($id);

        $asset_types = collect(Asset::TYPES)->map(function ($name, $id) {
            return [
                'id' => $id,
                'name' => $name,
            ];
        })->values();

        $availableTags = Tag::whereDoesntHave('asset')
            ->orWhere('id', $asset->tag_id)
            ->get();

        return inertia('Assets/edit', [
            'asset' => [
                'id' => $asset->id,
                'name' => $asset->name,
                'type' => $asset->type,
                'tag_id' => $asset->tag_id,
                'location_name' => $asset->location ? $asset->location->name : null,
            ],
            'asset_types' => $asset_types,
            'available_tags' => $availableTags,
        ]);
    }

    public function update(Request $request, $id)
    {
        try {
            $asset = Asset::findOrFail($id);

            $validator = Validator::make($request->all(), [
                'name' => 'required|string|max:255|unique:assets,name,' . $id,
                'type' => 'nullable|in:' . implode(',', array_keys(Asset::TYPES)),
                'tag_id' => [
                    'nullable',
                    'exists:tags,id',
                    function ($attribute, $value, $fail) use ($id) {
                        if ($value && Asset::where('tag_id', $value)->where('id', '!=', $id)->exists()) {
                            $fail('This tag is already associated with another asset.');
                        }
                    }
                ],
            ]);

            if ($validator->fails()) {
                return back()->withErrors($validator)->withInput();
            }

            $asset->update([
                'name' => $request->name,
                'type' => $request->type,
                'tag_id' => $request->tag_id,
            ]);

            return redirect()->route('assets.index')->with('success', 'Asset updated successfully');
        } catch (\Exception $e) {
            return back()->withErrors([
                'form' => 'An unexpected error occurred while updating the asset.',
            ])->withInput();
        }
    }

    public function destroy($id)
    {
        $asset = Asset::findOrFail($id);
        $asset->delete();

        return redirect()->route('assets.index')->with('success', 'Asset deleted successfully');
    }

    public function bulkDestroy(Request $request)
    {
        $validator = Validator::make($request->all(), [
            'ids' => 'required|array',
            'ids.*' => 'exists:assets,id',
        ]);

        if ($validator->fails()) {
            return back()->withErrors($validator)->withInput();
        }

        try {
            Asset::whereIn('id', $request->ids)->delete();

            return redirect()->route('assets.index')->with('success', count($request->ids) . ' assets deleted successfully');
        } catch (\Exception $e) {
            return back()->withErrors([
                'form' => 'An unexpected error occurred while deleting the asset(s).',
            ])->withInput();
        }
    }
}
