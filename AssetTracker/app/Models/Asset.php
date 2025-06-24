<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;
use Illuminate\Database\Eloquent\Relations\BelongsTo;
use Illuminate\Database\Eloquent\Relations\HasMany;

class Asset extends Model
{
    const TYPES = [
        'stationary' => 'Stationary',
        'mobile' => 'Mobile'
    ];

    protected $fillable = [
        'location_id',
        'tag_id',
        'name',
        'type',
    ];

    public function location(): BelongsTo
    {
        return $this->belongsTo(Location::class);
    }

    public function tag(): BelongsTo
    {
        return $this->belongsTo(Tag::class);
    }

    public function locationLogs(): HasMany
    {
        return $this->hasMany(AssetLocationLog::class);
    }
}
