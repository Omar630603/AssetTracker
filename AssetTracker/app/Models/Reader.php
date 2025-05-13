<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;
use Illuminate\Database\Eloquent\Relations\BelongsToMany;

class Reader extends Model
{
    //

    public function tags(): BelongsToMany
    {
        return $this->belongsToMany(Tag::class, 'reader_tags', 'reader_id', 'tag_id')->withTimestamps();
    }
}
