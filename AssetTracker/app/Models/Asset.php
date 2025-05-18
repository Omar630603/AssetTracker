<?php

namespace App\Models;

use Illuminate\Database\Eloquent\Model;
use Illuminate\Database\Eloquent\Relations\HasOne;

class Asset extends Model
{
    const TYPE = [
        'stationary' => 'stationary',
        'mobile' => 'mobile'
    ];

    public function tag(): HasOne
    {
        return $this->hasOne(Tag::class);
    }
}
