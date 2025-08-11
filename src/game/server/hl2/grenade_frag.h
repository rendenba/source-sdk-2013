//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef GRENADE_FRAG_H
#define GRENADE_FRAG_H
#pragma once

#include "basegrenade_shared.h"

struct edict_t;

CBaseGrenade	*Fraggrenade_Create( const Vector &position, const QAngle &angles, const Vector &velocity, const AngularImpulse &angVelocity, CBaseEntity *pOwner, float timer, bool combineSpawned, GrenadeType_t grenade_type = GRENADE_TYPE_FRAG , float flWarnAITime = 0.0f, bool bWarned = false, float flNextBlipTime = 0.0f);
bool			Fraggrenade_WasPunted( const CBaseEntity *pEntity );
bool			Fraggrenade_WasCreatedByCombine( const CBaseEntity *pEntity );

#endif // GRENADE_FRAG_H
