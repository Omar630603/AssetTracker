<?php

namespace App\Http\Controllers;

use App\Models\Tag;
use Illuminate\Http\Request;
use Illuminate\Support\Facades\Validator;

class TagController extends Controller
{
    public function index()
    {
        $tags = Tag::with('asset')->get()->map(function ($tag) {
            return [
                'id' => $tag->id,
                'name' => $tag->name,
                'asset_name' => $tag->asset ? $tag->asset->name : null,
            ];
        });

        return inertia('Tags/index', [
            'tags' => $tags,
        ]);
    }

    public function store(Request $request)
    {
        try {
            $validator = Validator::make($request->all(), [
                'name' => 'required|string|max:255|unique:tags,name',
            ]);

            if ($validator->fails()) {
                return back()->withErrors($validator)->withInput();
            }

            Tag::create([
                'name' => $request->name,
            ]);

            return redirect()->route('tags.index')->with('success', 'Tag created successfully');
        } catch (\Exception $e) {
            return back()->withErrors([
                'form' => 'An unexpected error occurred while saving the tag.',
            ])->withInput();
        }
    }

    public function edit($id)
    {
        $tag = Tag::findOrFail($id);

        return inertia('Tags/edit', [
            'tag' => [
                'id' => $tag->id,
                'name' => $tag->name,
            ],
        ]);
    }

    public function update(Request $request, $id)
    {
        try {
            $tag = Tag::findOrFail($id);

            $validator = Validator::make($request->all(), [
                'name' => 'required|string|max:255|unique:tags,name,' . $id,
            ]);

            if ($validator->fails()) {
                return back()->withErrors($validator)->withInput();
            }

            $tag->update([
                'name' => $request->name,
            ]);

            return redirect()->route('tags.index')->with('success', 'Tag updated successfully');
        } catch (\Exception $e) {
            return back()->withErrors([
                'form' => 'An unexpected error occurred while updating the tag.',
            ])->withInput();
        }
    }

    public function destroy($id)
    {
        $tag = Tag::findOrFail($id);
        $tag->delete();

        return redirect()->route('tags.index')->with('success', 'Tag deleted successfully');
    }

    public function bulkDestroy(Request $request)
    {
        $validator = Validator::make($request->all(), [
            'ids' => 'required|array',
            'ids.*' => 'exists:tags,id',
        ]);

        if ($validator->fails()) {
            return back()->withErrors($validator)->withInput();
        }

        try {
            Tag::whereIn('id', $request->ids)->delete();

            return redirect()->route('tags.index')->with('success', count($request->ids) . ' tags deleted successfully');
        } catch (\Exception $e) {
            return back()->withErrors([
                'form' => 'An unexpected error occurred while deleting the tag(s).',
            ])->withInput();
        }
    }
}
