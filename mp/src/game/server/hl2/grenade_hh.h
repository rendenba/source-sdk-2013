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
#define HHGRENADE_MODEL "models/Weapons/w_grenade.mdl"
//#define HHGRENADE_MODEL "models/weapons/w_holywater/w_grenade.mdl" //BB: changed for skoll

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
	void	GrenadeHHThink( void );
	void	Detonate();
	void	Spawn( void );
	void	OnRestore( void );
	void	Precache( void );
	bool	CreateVPhysics( void );
	void	CreateEffects( void );
	void	SetTimer( float detonateDelay, float warnDelay );
	void	SetVelocity( const Vector &velocity, const AngularImpulse &angVelocity );
	int		OnTakeDamage( const CTakeDamageInfo &inputInfo );
	void	BlipSound() { EmitSound( "Grenade.Blip" ); }
	void	DelayThink();
	void	VPhysicsUpdate( IPhysicsObject *pPhysics );
	void	OnPhysGunPickup( CBasePlayer *pPhysGunUser, PhysGunPickup_t reason );
	void	SetCombineSpawned( bool combineSpawned ) { m_combineSpawned = combineSpawned; }
	bool	IsCombineSpawned( void ) const { return m_combineSpawned; }
	void	SetPunted( bool punt ) { m_punted = punt; }
	bool	WasPunted( void ) const { return m_punted; }

	// this function only used in episodic.
#ifdef HL2_EPISODIC
	bool	HandleInteraction(int interactionType, void *data, CBaseCombatCharacter* sourceEnt);
#endif 

	void	InputSetTimer( inputdata_t &inputdata );

protected:
	CHandle<CSprite>		m_pMainGlow;
	CHandle<CSpriteTrail>	m_pGlowTrail;

	float	m_flNextBlipTime;
	bool	m_inSolid;
	bool	m_combineSpawned;
	bool	m_punted;
};

#endif // GRENADE_FRAG_H
