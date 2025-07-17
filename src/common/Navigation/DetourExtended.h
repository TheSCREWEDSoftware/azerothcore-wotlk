/*
 * Copyright (C) 2016+     AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license, you may redistribute it and/or modify it under version 2 of the License, or (at your option), any later version.
 */

#ifndef _ACORE_DETOUR_EXTENDED_H
#define _ACORE_DETOUR_EXTENDED_H

#include "DetourNavMeshQuery.h"

class dtQueryFilterExt: public dtQueryFilter
{
public:
    float getCost(float const* pa, float const* pb,
        dtPolyRef const prevRef, dtMeshTile const* prevTile, dtPoly const* prevPoly,
        dtPolyRef const curRef, dtMeshTile const* curTile, dtPoly const* curPoly,
        dtPolyRef const nextRef, dtMeshTile const* nextTile, dtPoly const* nextPoly) override const;
};

#endif // _ACORE_DETOUR_EXTENDED_H
