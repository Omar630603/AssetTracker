<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;
use Illuminate\Database\Eloquent\Relations\BelongsToMany;
use Illuminate\Database\Eloquent\Relations\HasOne;

class Tag extends Model
{
    protected $fillable = ['name'];

    public function readers(): BelongsToMany
    {
        return $this->belongsToMany(Reader::class, 'reader_tags', 'tag_id', 'reader_id')->withTimestamps();
    }

    public function asset(): HasOne
    {
        return $this->hasOne(Asset::class);
    }
}
