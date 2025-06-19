<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;
use Illuminate\Database\Eloquent\Relations\BelongsToMany;

class Reader extends Model
{
    protected $fillable = [
        'location_id',
        'name',
        'config',
    ];

    protected $casts = [
        'config' => 'array',
    ];

    public static array $defaultConfig = [
        'txPower' => -52,
        'maxDistance' => 5.0,
        'sampleCount' => 10,
        'sampleDelayMs' => 100,
        'pathLossExponent' => 2.5,
        'kalman' => [
            'Q' => 0.001,
            'R' => 0.5,
            'P' => 1.0,
            'initial' => -70.0
        ]
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
