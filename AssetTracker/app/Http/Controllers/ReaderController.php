<?php

namespace App\Http\Controllers;

use App\Models\Reader;
use App\Models\Location;
use App\Models\Tag;
use Illuminate\Http\Request;
use Illuminate\Support\Facades\Validator;

class ReaderController extends Controller
{
    public function index()
    {
        $readers = Reader::with('location')->get()->map(function ($reader) {
            return [
                'id' => $reader->id,
                'name' => $reader->name,
                'location' => $reader->location->name,
                'discovery_mode' => $reader->discovery_mode,
                'config' => $reader->config,
                'config_fetched_at' => $reader->config_fetched_at ? $reader->config_fetched_at : null,
                'tags' => $reader->tags->map(function ($tag) {
                    return [
                        'id' => $tag->id,
                        'name' => $tag->name,
                        'asset_name' => $tag->asset ? $tag->asset->name : 'No Asset',
                    ];
                }),
            ];
        });

        $locations = Location::select('id', 'name')->get();

        $tags = Tag::with('asset')->get()->map(function ($tag) {
            return [
                'id' => $tag->id,
                'name' => $tag->name,
                'asset_name' => $tag->asset ? $tag->asset->name : 'No Asset',
            ];
        });

        return inertia('Readers/index', [
            'readers' => $readers,
            'locations' => $locations,
            'tags' => $tags,
            'defaultConfig' => Reader::$defaultConfig,
            'assetNamePattern' => env('ASSET_NAME_PATTERN', 'ASSET_'),
        ]);
    }

    public function store(Request $request)
    {
        try {
            $validator = Validator::make($request->all(), [
                'name' => 'required|string|max:255|unique:readers,name',
                'location_id' => 'required|exists:locations,id',
                'discovery_mode' => 'required|in:pattern,explicit',
                'config' => 'required|array',
            ]);

            if ($validator->fails()) {
                return back()->withErrors($validator)->withInput();
            }

            $reader = Reader::create([
                'name' => $request->name,
                'location_id' => $request->location_id,
                'discovery_mode' => $request->discovery_mode,
                'config' => $request->config,
            ]);

            // Attach tags if provided and discovery_mode is explicit
            if ($request->discovery_mode === 'explicit' && $request->has('tag_ids') && is_array($request->tag_ids)) {
                $reader->tags()->sync($request->tag_ids);
            }

            return redirect()->route('readers.index')->with('success', 'Reader created successfully');
        } catch (\Exception $e) {
            return back()->withErrors([
                'form' => 'An unexpected error occurred while saving the reader.',
            ])->withInput();
        }
    }

    public function edit($id)
    {
        $reader = Reader::with('tags')->findOrFail($id);
        $locations = Location::select('id', 'name')->get();

        $tags = Tag::with('asset')->get()->map(function ($tag) {
            return [
                'id' => $tag->id,
                'name' => $tag->name,
                'asset_name' => $tag->asset ? $tag->asset->name : 'No Asset',
            ];
        });

        return inertia('Readers/edit', [
            'reader' => [
                'id' => $reader->id,
                'name' => $reader->name,
                'location_id' => $reader->location_id,
                'discovery_mode' => $reader->discovery_mode,
                'config' => $reader->config,
                'tag_ids' => $reader->tags->pluck('id')->toArray(),
            ],
            'locations' => $locations,
            'tags' => $tags,
            'assetNamePattern' => env('ASSET_NAME_PATTERN', 'ASSET_'),
        ]);
    }

    public function update(Request $request, $id)
    {
        try {
            $reader = Reader::findOrFail($id);

            $validator = Validator::make($request->all(), [
                'name' => 'required|string|max:255|unique:readers,name,' . $id,
                'location_id' => 'required|exists:locations,id',
                'discovery_mode' => 'required|in:pattern,explicit',
                'config' => 'required|array',
                'tag_ids' => 'nullable|array',
                'tag_ids.*' => 'exists:tags,id',
            ]);

            if ($validator->fails()) {
                return back()->withErrors($validator)->withInput();
            }

            $reader->update([
                'name' => $request->name,
                'location_id' => $request->location_id,
                'discovery_mode' => $request->discovery_mode,
                'config' => $request->config,
            ]);

            // Only update tags if discovery_mode is explicit and tag_ids are provided
            if ($request->discovery_mode === 'explicit' && $request->has('tag_ids')) {
                $reader->tags()->sync($request->tag_ids);
                $reader->touch();
            }

            return redirect()->route('readers.index')->with('success', 'Reader updated successfully');
        } catch (\Exception $e) {
            return back()->withErrors([
                'form' => 'An unexpected error occurred while updating the reader.',
            ])->withInput();
        }
    }

    public function destroy($id)
    {
        $reader = Reader::findOrFail($id);
        $reader->delete();

        return redirect()->route('readers.index')->with('success', 'Reader deleted successfully');
    }

    public function bulkDestroy(Request $request)
    {
        $validator = Validator::make($request->all(), [
            'ids' => 'required|array',
            'ids.*' => 'exists:readers,id',
        ]);

        if ($validator->fails()) {
            return back()->withErrors($validator)->withInput();
        }

        try {
            Reader::whereIn('id', $request->ids)->delete();

            return redirect()->route('readers.index')->with('success', count($request->ids) . ' readers deleted successfully');
        } catch (\Exception $e) {
            return back()->withErrors([
                'form' => 'An unexpected error occurred while deleting the reader(s).',
            ])->withInput();
        }
    }
}
