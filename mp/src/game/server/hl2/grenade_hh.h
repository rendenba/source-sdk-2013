//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef GRENADE_HH_H
#define GRENADE_HH_H
#pragma once

#include "cbase.h"
#include "basegrenade_shared.h"
#include "Sprite.h"
#include "SpriteTrail.h"
#include "soundent.h"

class CBaseGrenade;
class SmokeTrail;
struct edict_t;

#define HHGRENADE_MODEL "models/weapons/w_holywater/w_grenade.mdl" //BB: changed for skoll

class CGrenadeHH : public CBaseGrenade
{
	DECLARE_CLASS( CGrenadeHH, CBaseGrenade );

#if !defined( CLIENT_DLL )
	DECLARE_DATADESC();
#endif
					
	~CGrenadeHH( void );

public:
	CHandle< SmokeTrail > m_hSmokeTrail;
	void	GrenadeHHTouch( CBaseEntity *pOther );
	void	Detonate();
	void	Spawn( void );
	void	Precache( void );
	bool	CreateVPhysics( void );
	void	SetVelocity( const Vector &velocity, const AngularImpulse &angVelocity );
	int		OnTakeDamage( const CTakeDamageInfo &inputInfo );
	void	VPhysicsUpdate( IPhysicsObject *pPhysics );
	void	SetPunted( bool punt ) { m_punted = punt; }
	bool	WasPunted( void ) const { return m_punted; }

	// this function only used in episodic.
#ifdef HL2_EPISODIC
	bool	HandleInteraction(int interactionType, void *data, CBaseCombatCharacter* sourceEnt);
#endif 

protected:

	bool	m_inSolid;
	bool	m_punted;
};

#endif // GRENADE_FRAG_H
