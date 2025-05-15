<?php

namespace App\Http\Controllers;

use App\Models\Reader;
use Illuminate\Http\Request;

class ReaderController extends Controller
{
    public function index()
    {
        $readers = Reader::with('location')->get()->map(function ($reader) {
            return [
                'name' => $reader->name,
                'location' => $reader->location->name,
            ];
        });

        return inertia('Readers/Index', [
            'readers' => $readers,
        ]);
    }
}
