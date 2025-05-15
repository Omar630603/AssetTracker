<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;
use Illuminate\Database\Eloquent\Relations\BelongsToMany;

class Reader extends Model
{
    //

    protected $fillable = [
        'location_id',
        'name',
        'config',
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
