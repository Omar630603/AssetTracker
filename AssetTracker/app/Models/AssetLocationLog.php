<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;

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
    ];
}
