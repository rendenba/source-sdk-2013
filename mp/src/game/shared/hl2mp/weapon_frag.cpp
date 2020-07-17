//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"

#ifdef CLIENT_DLL
	#include "c_hl2mp_player.h"
	#include "c_te_effect_dispatch.h"
#else
	#include "hl2mp_player.h"
	#include "te_effect_dispatch.h"
	#include "grenade_frag.h"
#endif

#include "weapon_ar2.h"
#include "effect_dispatch_data.h"
#include "weapon_frag.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define GRENADE_TIMER	2.5f //Seconds

#define GRENADE_PAUSED_NO			0
#define GRENADE_PAUSED_PRIMARY		1
#define GRENADE_PAUSED_SECONDARY	2

#define FRAG_GRENADE_BLIP_FREQUENCY			1.0f
#define FRAG_GRENADE_BLIP_FAST_FREQUENCY	0.3f

#define FRAG_GRENADE_WARN_TIME 1.5f



#ifndef CLIENT_DLL

acttable_t	CWeaponFrag::m_acttable[] = 
{
	{ ACT_HL2MP_IDLE,					ACT_HL2MP_IDLE_GRENADE,					false },
	{ ACT_HL2MP_RUN,					ACT_HL2MP_RUN_GRENADE,					false },
	{ ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_GRENADE,			false },
	{ ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_GRENADE,			false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_GRENADE,	false },
	{ ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_GRENADE,		false },
	{ ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_GRENADE,					false },
};

IMPLEMENT_ACTTABLE(CWeaponFrag);

#endif

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponFrag, DT_WeaponFrag )

BEGIN_NETWORK_TABLE( CWeaponFrag, DT_WeaponFrag )

#ifdef CLIENT_DLL
	RecvPropBool( RECVINFO( m_bRedraw ) ),
	RecvPropBool( RECVINFO( m_fDrawbackFinished ) ),
	RecvPropInt( RECVINFO( m_AttackPaused ) ),
#else
	SendPropBool( SENDINFO( m_bRedraw ) ),
	SendPropBool( SENDINFO( m_fDrawbackFinished ) ),
	SendPropInt( SENDINFO( m_AttackPaused ) ),
#endif
	
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CWeaponFrag )
	DEFINE_PRED_FIELD( m_bRedraw, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_fDrawbackFinished, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_AttackPaused, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_frag, CWeaponFrag );
PRECACHE_WEAPON_REGISTER(weapon_frag);

CWeaponFrag::CWeaponFrag( void ) :
	CBaseHL2MPCombatWeapon()
{
	m_bRedraw = false;
	m_flCookTime = 0.0f;
	m_flWarnAITime = 0.0f;
	m_bHasWarnedAI = false;
	m_flNextBlipTime = 0.0f;
	m_flGrenadeRadius = 4.0f; // inches
	m_flGrenadeDamageRadius = 250.0f;
	m_GrenadeType = GRENADE_TYPE_FRAG;
}

bool CWeaponFrag::CanFire()
{
	return !IsPrimed();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponFrag::Precache( void )
{
	BaseClass::Precache();

#ifndef CLIENT_DLL
	UTIL_PrecacheOther( "npc_grenade_frag" );
#endif

	PrecacheScriptSound( "WeaponFrag.Throw" );
	PrecacheScriptSound( "WeaponFrag.Roll" );
}

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEvent - 
//			*pOperator - 
//-----------------------------------------------------------------------------
void CWeaponFrag::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	bool fThrewGrenade = false;

	switch( pEvent->event )
	{
		case EVENT_WEAPON_SEQUENCE_FINISHED:
			m_fDrawbackFinished = true;
			break;

		case EVENT_WEAPON_THROW:
			ThrowGrenade( pOwner );
			DecrementAmmo( pOwner );
			fThrewGrenade = true;
			break;

		case EVENT_WEAPON_THROW2:
			RollGrenade( pOwner );
			DecrementAmmo( pOwner );
			fThrewGrenade = true;
			break;

		case EVENT_WEAPON_THROW3:
			LobGrenade( pOwner );
			DecrementAmmo( pOwner );
			fThrewGrenade = true;
			break;

		default:
			BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
			break;
	}

#define RETHROW_DELAY	0.5
	if( fThrewGrenade )
	{
		m_flNextPrimaryAttack	= gpGlobals->curtime + RETHROW_DELAY;
		m_flNextSecondaryAttack	= gpGlobals->curtime + RETHROW_DELAY;
		m_flTimeWeaponIdle = FLT_MAX; //NOTE: This is set once the animation has finished up!
	}
}

#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponFrag::Deploy( void )
{
	m_bRedraw = false;
	m_fDrawbackFinished = false;
	m_flCookTime = 0.0f;
	m_flNextBlipTime = 0.0f;
	m_flWarnAITime = 0.0f;
	m_bHasWarnedAI = false;
	m_AttackPaused = GRENADE_PAUSED_NO;

	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponFrag::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	if (m_flCookTime > 0.0f)
	{
		CBaseCombatCharacter *pOwner = GetOwner();
		if (pOwner != NULL)
			pOwner->EmitSound("Player.DenyWeaponSelection");
		return false;
	}
	m_fDrawbackFinished = false;
	m_bRedraw = false;

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponFrag::Reload( void )
{
	if ( !HasPrimaryAmmo() )
		return false;

	if ( ( m_bRedraw ) && ( m_flNextPrimaryAttack <= gpGlobals->curtime ) && ( m_flNextSecondaryAttack <= gpGlobals->curtime ) )
	{
		//Redraw the weapon
		SendWeaponAnim( ACT_VM_DRAW );

		//Update our times
		m_flNextPrimaryAttack	= gpGlobals->curtime + SequenceDuration();
		m_flNextSecondaryAttack	= gpGlobals->curtime + SequenceDuration();
		m_flTimeWeaponIdle = gpGlobals->curtime + SequenceDuration();

		//Mark this as done
		m_bRedraw = false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponFrag::SecondaryAttack( void )
{
	if ( m_bRedraw )
		return;

	if ( !HasPrimaryAmmo() )
		return;

	CBaseCombatCharacter *pOwner  = GetOwner();

	if ( pOwner == NULL )
		return;

	CBasePlayer *pPlayer = ToBasePlayer( pOwner );
	
	if ( pPlayer == NULL )
		return;

	// Note that this is a secondary attack and prepare the grenade attack to pause.
	m_AttackPaused = GRENADE_PAUSED_SECONDARY;
	SendWeaponAnim( ACT_VM_PULLBACK_LOW );

	// Don't let weapon idle interfere in the middle of a throw!
	m_flTimeWeaponIdle = FLT_MAX;
	m_flNextSecondaryAttack	= FLT_MAX;

	// If I'm now out of ammo, switch away
	if ( !HasPrimaryAmmo() )
	{
		pPlayer->SwitchToNextBestWeapon( this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponFrag::PrimaryAttack( void )
{
	if ( m_bRedraw )
		return;

	CBaseCombatCharacter *pOwner = GetOwner();

	if (pOwner == NULL)
	{
		return;
	}

	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());

	if ( !pPlayer )
		return;

	// Note that this is a primary attack and prepare the grenade attack to pause.
	m_AttackPaused = GRENADE_PAUSED_PRIMARY;
	SendWeaponAnim( ACT_VM_PULLBACK_HIGH );
	
	// Put both of these off indefinitely. We do not know how long
	// the player will hold the grenade.
	m_flTimeWeaponIdle = FLT_MAX;
	m_flNextPrimaryAttack = FLT_MAX;

	// If I'm now out of ammo, switch away
	if ( !HasPrimaryAmmo() )
	{
		pPlayer->SwitchToNextBestWeapon( this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOwner - 
//-----------------------------------------------------------------------------
void CWeaponFrag::DecrementAmmo( CBaseCombatCharacter *pOwner )
{
	pOwner->RemoveAmmo( 1, m_iPrimaryAmmoType );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponFrag::ItemPostFrame( void )
{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());

	if (pOwner && IsPrimed())
	{
		if (pOwner->m_nButtons & IN_RELOAD && m_flCookTime == 0.0f)
		{
			m_flCookTime = gpGlobals->curtime;
			WeaponSound(SPECIAL2);
			WeaponSound(SPECIAL3);
			m_flNextBlipTime = gpGlobals->curtime + FRAG_GRENADE_BLIP_FREQUENCY;
			m_flWarnAITime = gpGlobals->curtime + GRENADE_TIMER - FRAG_GRENADE_WARN_TIME;
		}
	}

	if (m_fDrawbackFinished)
	{
		CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

		if (pOwner)
		{
			switch (m_AttackPaused)
			{
			case GRENADE_PAUSED_PRIMARY:
				if (!(pOwner->m_nButtons & IN_ATTACK) || (m_flCookTime > 0.0f && gpGlobals->curtime > m_flCookTime + GRENADE_TIMER))
				{
					SendWeaponAnim(ACT_VM_THROW);
					m_fDrawbackFinished = false;
					m_AttackPaused = GRENADE_PAUSED_NO;
				}
				break;

			case GRENADE_PAUSED_SECONDARY:
				if (!(pOwner->m_nButtons & IN_ATTACK2) || (m_flCookTime > 0.0f && gpGlobals->curtime > m_flCookTime + GRENADE_TIMER))
				{
					//See if we're ducking
					if (((pOwner->m_nButtons & IN_DUCK) == 0) || (m_flCookTime > 0.0f && gpGlobals->curtime > m_flCookTime + GRENADE_TIMER))
					{
						//Send the weapon animation
						SendWeaponAnim(ACT_VM_HAULBACK);
					}
					else
					{
						//Send the weapon animation
						SendWeaponAnim( ACT_VM_SECONDARYATTACK );
					}


					m_fDrawbackFinished = false;
					m_AttackPaused = GRENADE_PAUSED_NO;
				}
				break;

			default:
				break;
			}
		}
	}

	BaseClass::ItemPostFrame();

	if (m_bRedraw)
	{
		m_flCookTime = 0.0f;
		m_flNextBlipTime = 0.0f;
		m_bHasWarnedAI = false;
		m_flWarnAITime = 0.0f;

		if (IsViewModelSequenceFinished())
		{
			Reload();
		}
	}

	if (m_flCookTime > 0.0f)
	{
		if (!m_bHasWarnedAI && gpGlobals->curtime >= m_flWarnAITime)
		{
			m_bHasWarnedAI = true;
		}

		if (gpGlobals->curtime > m_flNextBlipTime)
		{
			WeaponSound(SPECIAL3);

			if (m_bHasWarnedAI)
			{
				m_flNextBlipTime = gpGlobals->curtime + FRAG_GRENADE_BLIP_FAST_FREQUENCY;
			}
			else
			{
				m_flNextBlipTime = gpGlobals->curtime + FRAG_GRENADE_BLIP_FREQUENCY;
			}
		}
	}
}

	// check a throw from vecSrc.  If not valid, move the position back along the line to vecEye
void CWeaponFrag::CheckThrowPosition( CBasePlayer *pPlayer, const Vector &vecEye, Vector &vecSrc )
{
	trace_t tr;

	UTIL_TraceHull(vecEye, vecSrc, -Vector(m_flGrenadeRadius + 2, m_flGrenadeRadius + 2, m_flGrenadeRadius + 2), Vector(m_flGrenadeRadius + 2, m_flGrenadeRadius + 2, m_flGrenadeRadius + 2),
		pPlayer->PhysicsSolidMaskForEntity(), pPlayer, pPlayer->GetCollisionGroup(), &tr );
	
	if ( tr.DidHit() )
	{
		vecSrc = tr.endpos;
	}
}

void DropPrimedFragGrenade( CHL2MP_Player *pPlayer, CBaseCombatWeapon *pGrenade )
{
	CWeaponFrag *pWeaponFrag = dynamic_cast<CWeaponFrag*>( pGrenade );

	if ( pWeaponFrag )
	{
		pWeaponFrag->ThrowGrenade( pPlayer );
		pWeaponFrag->DecrementAmmo( pPlayer );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//-----------------------------------------------------------------------------
void CWeaponFrag::ThrowGrenade( CBasePlayer *pPlayer )
{
#ifndef CLIENT_DLL
	Vector	vecEye = pPlayer->EyePosition();
	Vector	vForward, vRight;

	pPlayer->EyeVectors(&vForward, &vRight, NULL);
	Vector vecSrc = vecEye + vForward * 18.0f + vRight * 8.0f;
	CheckThrowPosition(pPlayer, vecEye, vecSrc);
	//	vForward[0] += 0.1f;

	float timer = GRENADE_TIMER;
	if (m_flCookTime > 0.0f)
		timer = max(0.0f, timer - gpGlobals->curtime + m_flCookTime);
	Vector vecThrow;
	pPlayer->GetVelocity(&vecThrow, NULL);
	if (timer > 0.0f)
	{
		vForward[2] += 0.1f;
		vecThrow += vForward * 900;
	}
	
	CBaseGrenade *pGrenade = Fraggrenade_Create(vecSrc, vec3_angle, vecThrow, AngularImpulse(600, random->RandomInt(-1200, 1200), 0), pPlayer, timer, false, m_GrenadeType, m_flWarnAITime, m_bHasWarnedAI, m_flNextBlipTime);

	if ( pGrenade )
	{
		if ( pPlayer && pPlayer->m_lifeState != LIFE_ALIVE )
		{
			pPlayer->GetVelocity( &vecThrow, NULL );

			IPhysicsObject *pPhysicsObject = pGrenade->VPhysicsGetObject();
			if ( pPhysicsObject )
			{
				pPhysicsObject->SetVelocity( &vecThrow, NULL );
			}
		}
		
		pGrenade->SetDamage( GetHL2MPWpnData().m_iPlayerDamage );
		pGrenade->SetDamageRadius(m_flGrenadeDamageRadius);
	}
#endif

	m_bRedraw = true;

	WeaponSound( SINGLE );
	
	// player "shoot" animation
	pPlayer->SetAnimation( PLAYER_ATTACK1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//-----------------------------------------------------------------------------
void CWeaponFrag::LobGrenade( CBasePlayer *pPlayer )
{
#ifndef CLIENT_DLL
	Vector	vecEye = pPlayer->EyePosition();
	Vector	vForward, vRight;

	pPlayer->EyeVectors( &vForward, &vRight, NULL );
	Vector vecSrc = vecEye + vForward * 18.0f + vRight * 8.0f + Vector( 0, 0, -8 );
	CheckThrowPosition( pPlayer, vecEye, vecSrc );
	
	float timer = GRENADE_TIMER;
	if (m_flCookTime > 0.0f)
		timer = max(0.0f, timer - gpGlobals->curtime + m_flCookTime);
	Vector vecThrow;
	pPlayer->GetVelocity( &vecThrow, NULL );
	if (timer > 0.0f)
		vecThrow += vForward * 350 + Vector( 0, 0, 50 );

	CBaseGrenade *pGrenade = Fraggrenade_Create( vecSrc, vec3_angle, vecThrow, AngularImpulse(200,random->RandomInt(-600,600),0), pPlayer, timer, false, m_GrenadeType, m_flWarnAITime, m_bHasWarnedAI, m_flNextBlipTime );

	if ( pGrenade )
	{
		pGrenade->SetDamage( GetHL2MPWpnData().m_iPlayerDamage );
		pGrenade->SetDamageRadius(m_flGrenadeDamageRadius);
	}
#endif

	WeaponSound( WPN_DOUBLE );

	// player "shoot" animation
	pPlayer->SetAnimation( PLAYER_ATTACK1 );

	m_bRedraw = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//-----------------------------------------------------------------------------
void CWeaponFrag::RollGrenade( CBasePlayer *pPlayer )
{
#ifndef CLIENT_DLL
	// BUGBUG: Hardcoded grenade width of 4 - better not change the model :)
	Vector vecSrc;
	pPlayer->CollisionProp()->NormalizedToWorldSpace( Vector( 0.5f, 0.5f, 0.0f ), &vecSrc );
	vecSrc.z += m_flGrenadeRadius;

	float timer = GRENADE_TIMER;
	if (m_flCookTime > 0.0f)
		timer = max(0.0f, timer - gpGlobals->curtime + m_flCookTime);

	Vector vecFacing = pPlayer->BodyDirection2D( );
	// no up/down direction
	vecFacing.z = 0;
	VectorNormalize( vecFacing );
	trace_t tr;
	UTIL_TraceLine( vecSrc, vecSrc - Vector(0,0,16), MASK_PLAYERSOLID, pPlayer, COLLISION_GROUP_NONE, &tr );
	if ( tr.fraction != 1.0 )
	{
		// compute forward vec parallel to floor plane and roll grenade along that
		Vector tangent;
		CrossProduct( vecFacing, tr.plane.normal, tangent );
		CrossProduct( tr.plane.normal, tangent, vecFacing );
	}
	vecSrc += (vecFacing * 18.0);
	CheckThrowPosition( pPlayer, pPlayer->WorldSpaceCenter(), vecSrc );

	Vector vecThrow;
	pPlayer->GetVelocity( &vecThrow, NULL );
	vecThrow += vecFacing * 700;
	// put it on its side
	QAngle orientation(0,pPlayer->GetLocalAngles().y,-90);
	// roll it
	AngularImpulse rotSpeed(0,0,720);
	CBaseGrenade *pGrenade = Fraggrenade_Create( vecSrc, orientation, vecThrow, rotSpeed, pPlayer, timer, false, m_GrenadeType, m_flWarnAITime, m_bHasWarnedAI, m_flNextBlipTime );

	if ( pGrenade )
	{
		pGrenade->SetDamage( GetHL2MPWpnData().m_iPlayerDamage );
		pGrenade->SetDamageRadius(m_flGrenadeDamageRadius);
	}

#endif

	WeaponSound( SPECIAL1 );

	// player "shoot" animation
	pPlayer->SetAnimation( PLAYER_ATTACK1 );

	m_bRedraw = true;
}

