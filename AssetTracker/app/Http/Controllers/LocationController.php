<?php

namespace App\Http\Controllers;

use App\Models\Location;
use Illuminate\Http\Request;
use Illuminate\Support\Facades\Validator;

class LocationController extends Controller
{
    public function index()
    {
        $locations = Location::all();

        return inertia('Locations/index', [
            'locations' => $locations,
        ]);
    }

    public function store(Request $request)
    {
        try {
            $validator = Validator::make($request->all(), [
                'name' => 'required|string|max:255|unique:locations,name',
                'floor' => 'nullable|string|max:255',
            ]);

            if ($validator->fails()) {
                return back()->withErrors($validator)->withInput();
            }

            Location::create([
                'name' => $request->name,
                'floor' => $request->floor,
            ]);

            return redirect()->route('locations.index')->with('success', 'Location created successfully');
        } catch (\Exception $e) {
            return back()->withErrors([
                'form' => 'An unexpected error occurred while saving the location.',
            ])->withInput();
        }
    }

    public function edit($id)
    {
        $location = Location::findOrFail($id);

        return inertia('Locations/edit', [
            'location' => [
                'id' => $location->id,
                'name' => $location->name,
                'floor' => $location->floor,
            ],
        ]);
    }

    public function update(Request $request, $id)
    {
        try {
            $location = Location::findOrFail($id);

            $validator = Validator::make($request->all(), [
                'name' => 'required|string|max:255|unique:locations,name,' . $id,
                'floor' => 'nullable|string|max:255',
            ]);

            if ($validator->fails()) {
                return back()->withErrors($validator)->withInput();
            }

            $location->update([
                'name' => $request->name,
                'floor' => $request->floor,
            ]);

            return redirect()->route('locations.index')->with('success', 'Location updated successfully');
        } catch (\Exception $e) {
            return back()->withErrors([
                'form' => 'An unexpected error occurred while updating the location.',
            ])->withInput();
        }
    }

    public function destroy($id)
    {
        $location = Location::findOrFail($id);
        $location->delete();

        return redirect()->route('locations.index')->with('success', 'Location deleted successfully');
    }

    public function bulkDestroy(Request $request)
    {
        $validator = Validator::make($request->all(), [
            'ids' => 'required|array',
            'ids.*' => 'exists:locations,id',
        ]);

        if ($validator->fails()) {
            return back()->withErrors($validator)->withInput();
        }

        try {
            Location::whereIn('id', $request->ids)->delete();

            return redirect()->route('locations.index')->with('success', count($request->ids) . ' locations deleted successfully');
        } catch (\Exception $e) {
            return back()->withErrors([
                'form' => 'An unexpected error occurred while deleting the location(s).',
            ])->withInput();
        }
    }
}
