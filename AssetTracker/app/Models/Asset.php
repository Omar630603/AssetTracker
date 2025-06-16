<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;
use Illuminate\Database\Eloquent\Relations\BelongsTo;

class Asset extends Model
{
    const TYPE = [
        'stationary' => 'stationary',
        'mobile' => 'mobile'
    ];

    protected $fillable = [
        'location_id',
        'tag_id',
        'name',
        'type',
    ];

    public function tag(): BelongsTo
    {
        return $this->belongsTo(Tag::class);
    }
}
