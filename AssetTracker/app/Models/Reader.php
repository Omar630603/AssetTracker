<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;
use Illuminate\Database\Eloquent\Relations\BelongsToMany;

class Reader extends Model
{
    protected $fillable = [
        'location_id',
        'name',
        'discovery_mode',
        'config',
    ];

    protected $casts = [
        'config' => 'array',
    ];

    public static array $defaultConfig = [
        'txPower' => -68,
        'pathLossExponent' => 2.5,
        'maxDistance' => 5.0,
        'sampleCount' => 5,
        'sampleDelayMs' => 100,
        'kalman' => [
            'P' => 1.0,
            'Q' => 0.1,
            'R' => 2.0,
            'initial' => -60.0
        ],
    ];

    public function location()
    {
        return $this->belongsTo(Location::class);
    }

    public function tags(): BelongsToMany
    {
        return $this->belongsToMany(Tag::class, 'reader_tags', 'reader_id', 'tag_id')->withTimestamps();
    }
}
