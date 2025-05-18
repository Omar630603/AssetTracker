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
        'kalman' => [
            'P' => 1,
            'Q' => 0.01,
            'R' => 1,
            'initial' => -50
        ],
        'txPower' => -52,
        'maxDistance' => 5,
        'sampleCount' => 10,
        'sampleDelayMs' => 100,
        'pathLossExponent' => 2.5
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
