
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
#else
	#include "hl2mp_player.h"
#endif

#include "weapon_hl2mpbasehlmpcombatweapon.h"

#ifdef CLIENT_DLL
#define CWeapon357 C_Weapon357
#endif

//-----------------------------------------------------------------------------
// CWeapon357
//-----------------------------------------------------------------------------

class CWeapon357 : public CBaseHL2MPCombatWeapon
{
	DECLARE_CLASS( CWeapon357, CBaseHL2MPCombatWeapon );
public:

	CWeapon357( void );

	void	PrimaryAttack( void );
	bool Reload(void);
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

#ifndef CLIENT_DLL
	DECLARE_ACTTABLE();
#endif

private:
	
	CWeapon357( const CWeapon357 & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( Weapon357, DT_Weapon357 )

BEGIN_NETWORK_TABLE( CWeapon357, DT_Weapon357 )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeapon357 )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_357, CWeapon357 );
PRECACHE_WEAPON_REGISTER( weapon_357 );


#ifndef CLIENT_DLL
acttable_t CWeapon357::m_acttable[] = 
{
	{ ACT_HL2MP_IDLE,					ACT_HL2MP_IDLE_PISTOL,					false },
	{ ACT_HL2MP_RUN,					ACT_HL2MP_RUN_PISTOL,					false },
	{ ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_PISTOL,			false },
	{ ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_PISTOL,			false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_PISTOL,	false },
	{ ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_PISTOL,		false },
	{ ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_PISTOL,					false },
	{ ACT_RANGE_ATTACK1,				ACT_RANGE_ATTACK_PISTOL,				false },
};



IMPLEMENT_ACTTABLE( CWeapon357 );

#endif

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeapon357::CWeapon357( void )
{
	m_bReloadsSingly	= false;
	m_bFiresUnderwater	= false;
}

bool CWeapon357::Reload(void)
{
	CBaseCombatCharacter *pOwner = GetOwner();
	if (!pOwner)
		return false;

	// If I don't have any spare ammo, I can't reload
	if (pOwner->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
		return false;

	bool bReload = false;

	// If you don't have clips, then don't try to reload them.
	if (UsesClipsForAmmo1())
	{
		// need to reload primary clip?
		int primary = MIN(GetMaxClip1() - m_iClip1, pOwner->GetAmmoCount(m_iPrimaryAmmoType));
		if (primary != 0)
		{
			bReload = true;
		}
	}

	if (UsesClipsForAmmo2())
	{
		// need to reload secondary clip?
		int secondary = MIN(GetMaxClip2() - m_iClip2, pOwner->GetAmmoCount(m_iSecondaryAmmoType));
		if (secondary != 0)
		{
			bReload = true;
		}
	}

	if (!bReload)
		return false;

	CheckDeferredAction(true);

	// Play reload
	WeaponSound(RELOAD);
	SendWeaponAnim(ACT_VM_RELOAD);

	// Play the player's reload animation
	if (pOwner->IsPlayer())
	{
		((CBasePlayer *)pOwner)->SetAnimation(PLAYER_RELOAD);
	}

	//BB: I simply do not understand the spaghetti calls to setting activities and sequences. This corrects the issue for some reason.
	float factor = 1.0f;
	CHL2MP_Player *pHL2MPPlayer = ToHL2MPPlayer(pOwner);
	if (pHL2MPPlayer->HasStatus(COVEN_STATUS_HASTE))
		factor /= 1.0f + pHL2MPPlayer->GetStatusMagnitude(COVEN_STATUS_HASTE) * 0.01f;

	if (pHL2MPPlayer->HasStatus(COVEN_STATUS_SLOW))
		factor *= 1.0f + pHL2MPPlayer->GetStatusMagnitude(COVEN_STATUS_SLOW) * 0.01f;

	m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration() * factor;

	m_bInReload = true;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeapon357::PrimaryAttack( void )
{
	//BB: this is a major hack. This will need to get put into ANY weapon that is used by a builder class since baseweapon primary attack isn't called.
	if (BuilderClassWeapon())
		return;

	// Only the player fires this way so we can cast
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if ( !pPlayer )
	{
		return;
	}

	//BB: this sucks that PrimaryAttack is redefined without inheritance in every freaking weapon!
	if (CheckDeferredAction(true))
		return;

	CHL2MP_Player *pHL2Player = ToHL2MPPlayer(pPlayer);

	if ( m_iClip1 <= 0 )
	{
		if ( !m_bFireOnEmpty )
		{
			Reload();
		}
		else
		{
			WeaponSound( EMPTY );
			m_flNextPrimaryAttack = 0.15f;
		}

		return;
	}

	WeaponSound( SINGLE );
	pPlayer->DoMuzzleFlash();

	SendWeaponAnim(ACT_VM_PRIMARYATTACK);
	pPlayer->SetAnimation(PLAYER_ATTACK1);


	m_flNextPrimaryAttack = gpGlobals->curtime + 0.75f;
	if (pHL2Player->HasStatus(COVEN_STATUS_HASTE))
		m_flNextPrimaryAttack -= pHL2Player->GetStatusMagnitude(COVEN_STATUS_HASTE) * 0.0075f;
	if (pHL2Player->HasStatus(COVEN_STATUS_SLOW))
		m_flNextPrimaryAttack += pHL2Player->GetStatusMagnitude(COVEN_STATUS_SLOW) * 0.0075f;

	m_flNextSecondaryAttack = gpGlobals->curtime + 0.75;

	m_iClip1--;

	Vector vecSrc		= pPlayer->Weapon_ShootPosition();
	Vector vecAiming	= pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );	

	FireBulletsInfo_t info( 1, vecSrc, vecAiming, vec3_origin, MAX_TRACE_LENGTH, m_iPrimaryAmmoType );
	info.m_pAttacker = pPlayer;

	// Fire the bullets, and force the first shot to be perfectly accuracy
	pPlayer->FireBullets( info );

	//Disorient the player
	QAngle angles = pPlayer->GetLocalAngles();

	angles.x += random->RandomInt( -1, 1 );
	angles.y += random->RandomInt( -1, 1 );
	angles.z = 0;

#ifndef CLIENT_DLL
	pPlayer->SnapEyeAngles( angles );
#endif

	pPlayer->ViewPunch( QAngle( -8, random->RandomFloat( -2, 2 ), 0 ) );

	if ( !m_iClip1 && pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 )
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 ); 
	}
}
