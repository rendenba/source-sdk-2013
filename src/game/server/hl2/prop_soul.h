//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef PROP_SOUL_H
#define PROP_SOUL_H

#ifdef _WIN32
#pragma once
#endif

#include "prop_combine_ball.h"

#include "weapon_hl2mpbasehlmpcombatweapon.h"

#ifdef CLIENT_DLL

	#include "iviewrender_beams.h"

#endif

#ifndef CLIENT_DLL
#include "Sprite.h"
#include "npcevent.h"
#include "beam_shared.h"

class CPropSoul : public CBaseCombatCharacter
{
	DECLARE_CLASS( CPropSoul, CBaseCombatCharacter );
	//DECLARE_SERVERCLASS();


public:
	CPropSoul();
	~CPropSoul();

#ifdef HL1_DLL
	Class_T Classify( void ) { return CLASS_NONE; }
#else
	Class_T Classify( void ) { return CLASS_MISSILE; }
#endif
	
	void	Spawn( void );
	void	Precache( void );
	void	MissileTouch( CBaseEntity *pOther );
	void	Explode( void );
	void	ShotDown( void );
	void	AccelerateThink( void );
	void	AugerThink( void );
	void	IgniteThink( void );
	void	SeekThink( void );
	void	DumbFire( void );
	void	SetGracePeriod( float flGracePeriod );

	int		OnTakeDamage_Alive( const CTakeDamageInfo &info );
	void	Event_Killed( const CTakeDamageInfo &info );
	
	virtual float	GetDamage() { return m_flDamage; }
	virtual void	SetDamage(float flDamage) { m_flDamage = flDamage; }

	unsigned int PhysicsSolidMaskForEntity( void ) const;

	static CPropSoul *Create( const Vector &vecOrigin, const QAngle &vecAngles, edict_t *pentOwner );

	CHandle<CBasePlayer>		m_hOwner;

protected:
	virtual void DoExplosion();	
	virtual int AugerHealth() { return m_iMaxHealth - 20; }

	// Creates the smoke trail
	void CreateSmokeTrail( void );

	// Gets the shooting position 

	float					m_flAugerTime;		// Amount of time to auger before blowing up anyway
	float					m_flMarkDeadTime;
	float					m_flDamage;

private:
	float					m_flGracePeriodEndsAt;

	DECLARE_DATADESC();
};

#endif

#endif // PROP_COMBINE_BALL_H
