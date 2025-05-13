<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;
use Illuminate\Database\Eloquent\Relations\BelongsToMany;

class Tag extends Model
{
    //

    public function readers(): BelongsToMany
    {
        return $this->belongsToMany(Reader::class, 'reader_tags', 'tag_id', 'reader_id')->withTimestamps();
    }
}
