<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;
use Illuminate\Database\Eloquent\Relations\BelongsTo;

class AssetLocationLog extends Model
{
    protected $fillable = [
        'asset_id',
        'location_id',
        'rssi',
        'kalman_rssi',
        'estimated_distance',
        'type',
        'status',
        'reader_name',
    ];

    protected $casts = [
        'rssi' => 'double',
        'kalman_rssi' => 'double',
        'estimated_distance' => 'double',
    ];

    public function asset(): BelongsTo
    {
        return $this->belongsTo(Asset::class);
    }

    public function location(): BelongsTo
    {
        return $this->belongsTo(Location::class);
    }
}
