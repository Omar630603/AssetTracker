<?php

namespace App\Http\Controllers\Api;

use App\Http\Controllers\Controller;
use App\Models\Reader;
use Illuminate\Http\Request;

class ReaderController extends Controller
{
    public function getConfig(Request $request)
    {
        $readerName = $request->input('reader_name');

        $reader = Reader::where('name', $readerName)->first();

        if (!$reader) {
            return response()->json(['error' => 'Reader not found'], 404);
        }

        $targets = $reader->tags()->pluck('name')->toArray();

        return response()->json([
            'name' => $reader->name,
            'targets' => $targets,
            'config' => json_decode($reader->config),
        ]);
    }
}
