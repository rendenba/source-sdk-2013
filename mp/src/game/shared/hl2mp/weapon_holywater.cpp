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
#include "grenade_hh.h"
#endif

#include "weapon_ar2.h"
#include "effect_dispatch_data.h"
#include "weapon_hl2mpbasehlmpcombatweapon.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define GRENADE_TIMER	2.5f //Seconds

#define GRENADE_HH_PAUSED_NO			0
#define GRENADE_HH_PAUSED_PRIMARY		1
#define GRENADE_HH_PAUSED_SECONDARY		2

#define GRENADE_HH_RADIUS	4.0f // inches

#ifdef CLIENT_DLL
#define CWeaponHolywater C_WeaponHolywater
#endif

//-----------------------------------------------------------------------------
// Fragmentation grenades
//-----------------------------------------------------------------------------
class CWeaponHolywater : public CBaseHL2MPCombatWeapon
{
	DECLARE_CLASS(CWeaponHolywater, CBaseHL2MPCombatWeapon);
public:

	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CWeaponHolywater();

	void	Precache(void);
	void	PrimaryAttack(void);
	void	SecondaryAttack(void);
	void	DecrementAmmo(CBaseCombatCharacter *pOwner);
	void	ItemPostFrame(void);

	bool	Deploy(void);
	bool	Holster(CBaseCombatWeapon *pSwitchingTo = NULL);

	bool	Reload(void);

#ifndef CLIENT_DLL
	void Operator_HandleAnimEvent(animevent_t *pEvent, CBaseCombatCharacter *pOperator);
#endif

	void	ThrowGrenade(CBasePlayer *pPlayer);
	bool	IsPrimed(bool) { return (m_AttackPaused != 0); }

private:

	void	LobGrenade(CBasePlayer *pPlayer);
	// check a throw from vecSrc.  If not valid, move the position back along the line to vecEye
	void	CheckThrowPosition(CBasePlayer *pPlayer, const Vector &vecEye, Vector &vecSrc);

	CNetworkVar(bool, m_bRedraw);	//Draw the weapon again after throwing a grenade

	CNetworkVar(int, m_AttackPaused);
	CNetworkVar(bool, m_fDrawbackFinished);

	CWeaponHolywater(const CWeaponHolywater &);

#ifndef CLIENT_DLL
	DECLARE_ACTTABLE();
#endif
};

#ifndef CLIENT_DLL

acttable_t	CWeaponHolywater::m_acttable[] =
{
	{ ACT_HL2MP_IDLE, ACT_HL2MP_IDLE_GRENADE, false },
	{ ACT_HL2MP_RUN, ACT_HL2MP_RUN_GRENADE, false },
	{ ACT_HL2MP_IDLE_CROUCH, ACT_HL2MP_IDLE_CROUCH_GRENADE, false },
	{ ACT_HL2MP_WALK_CROUCH, ACT_HL2MP_WALK_CROUCH_GRENADE, false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK, ACT_HL2MP_GESTURE_RANGE_ATTACK_GRENADE, false },
	{ ACT_HL2MP_GESTURE_RELOAD, ACT_HL2MP_GESTURE_RELOAD_GRENADE, false },
	{ ACT_HL2MP_JUMP, ACT_HL2MP_JUMP_GRENADE, false },
};

IMPLEMENT_ACTTABLE(CWeaponHolywater);

#endif

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponHolywater, DT_WeaponHolywater)

BEGIN_NETWORK_TABLE(CWeaponHolywater, DT_WeaponHolywater)

#ifdef CLIENT_DLL
RecvPropBool(RECVINFO(m_bRedraw)),
RecvPropBool(RECVINFO(m_fDrawbackFinished)),
RecvPropInt(RECVINFO(m_AttackPaused)),
#else
SendPropBool(SENDINFO(m_bRedraw)),
SendPropBool(SENDINFO(m_fDrawbackFinished)),
SendPropInt(SENDINFO(m_AttackPaused)),
#endif

END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponHolywater)
DEFINE_PRED_FIELD(m_bRedraw, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_FIELD(m_fDrawbackFinished, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_FIELD(m_AttackPaused, FIELD_INTEGER, FTYPEDESC_INSENDTABLE),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS(weapon_holywater, CWeaponHolywater);
PRECACHE_WEAPON_REGISTER(weapon_holywater);

CWeaponHolywater::CWeaponHolywater(void) :
CBaseHL2MPCombatWeapon()
{
	m_bRedraw = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponHolywater::Precache(void)
{
	BaseClass::Precache();

#ifndef CLIENT_DLL
	UTIL_PrecacheOther("grenade_hh");
#endif

	PrecacheScriptSound("WeaponFrag.Throw");
	PrecacheScriptSound("WeaponFrag.Roll");
}

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEvent - 
//			*pOperator - 
//-----------------------------------------------------------------------------
void CWeaponHolywater::Operator_HandleAnimEvent(animevent_t *pEvent, CBaseCombatCharacter *pOperator)
{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());
	bool fThrewGrenade = false;

	switch (pEvent->event)
	{
	case EVENT_WEAPON_SEQUENCE_FINISHED:
		m_fDrawbackFinished = true;
		break;

	case EVENT_WEAPON_THROW:
		ThrowGrenade(pOwner);
		DecrementAmmo(pOwner);
		fThrewGrenade = true;
		break;

	case EVENT_WEAPON_THROW2:
		LobGrenade(pOwner);
		DecrementAmmo(pOwner);
		fThrewGrenade = true;
		break;

	case EVENT_WEAPON_THROW3:
		LobGrenade(pOwner);
		DecrementAmmo(pOwner);
		fThrewGrenade = true;
		break;

	default:
		BaseClass::Operator_HandleAnimEvent(pEvent, pOperator);
		break;
	}

#define RETHROW_DELAY	0.5
	if (fThrewGrenade)
	{
		m_flNextPrimaryAttack = gpGlobals->curtime + RETHROW_DELAY;
		m_flNextSecondaryAttack = gpGlobals->curtime + RETHROW_DELAY;
		m_flTimeWeaponIdle = FLT_MAX; //NOTE: This is set once the animation has finished up!
	}
}

#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CWeaponHolywater::Deploy(void)
{
	m_bRedraw = false;
	m_fDrawbackFinished = false;

	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponHolywater::Holster(CBaseCombatWeapon *pSwitchingTo)
{
	m_bRedraw = false;
	m_fDrawbackFinished = false;

	return BaseClass::Holster(pSwitchingTo);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponHolywater::Reload(void)
{
	if (!HasPrimaryAmmo())
		return false;

	if ((m_bRedraw) && (m_flNextPrimaryAttack <= gpGlobals->curtime) && (m_flNextSecondaryAttack <= gpGlobals->curtime))
	{
		//Redraw the weapon
		SendWeaponAnim(ACT_VM_DRAW);

		//Update our times
		m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
		m_flNextSecondaryAttack = gpGlobals->curtime + SequenceDuration();
		m_flTimeWeaponIdle = gpGlobals->curtime + SequenceDuration();

		//Mark this as done
		m_bRedraw = false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponHolywater::SecondaryAttack(void)
{
	if (m_bRedraw)
		return;

	if (!HasPrimaryAmmo())
		return;

	CBaseCombatCharacter *pOwner = GetOwner();

	if (pOwner == NULL)
		return;

	CBasePlayer *pPlayer = ToBasePlayer(pOwner);

	if (pPlayer == NULL)
		return;

	// Note that this is a secondary attack and prepare the grenade attack to pause.
	m_AttackPaused = GRENADE_HH_PAUSED_SECONDARY;
	SendWeaponAnim(ACT_VM_PULLBACK_LOW);

	// Don't let weapon idle interfere in the middle of a throw!
	m_flTimeWeaponIdle = FLT_MAX;
	m_flNextSecondaryAttack = FLT_MAX;

	// If I'm now out of ammo, switch away
	if (!HasPrimaryAmmo())
	{
		pPlayer->SwitchToNextBestWeapon(this);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponHolywater::PrimaryAttack(void)
{
	if (m_bRedraw)
		return;

	CBaseCombatCharacter *pOwner = GetOwner();

	if (pOwner == NULL)
	{
		return;
	}

	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());;

	if (!pPlayer)
		return;

	// Note that this is a primary attack and prepare the grenade attack to pause.
	m_AttackPaused = GRENADE_HH_PAUSED_PRIMARY;
	SendWeaponAnim(ACT_VM_PULLBACK_HIGH);

	// Put both of these off indefinitely. We do not know how long
	// the player will hold the grenade.
	m_flTimeWeaponIdle = FLT_MAX;
	m_flNextPrimaryAttack = FLT_MAX;

	// If I'm now out of ammo, switch away
	if (!HasPrimaryAmmo())
	{
		pPlayer->SwitchToNextBestWeapon(this);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOwner - 
//-----------------------------------------------------------------------------
void CWeaponHolywater::DecrementAmmo(CBaseCombatCharacter *pOwner)
{
	pOwner->RemoveAmmo(1, m_iPrimaryAmmoType);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponHolywater::ItemPostFrame(void)
{
	if (m_fDrawbackFinished)
	{
		CBasePlayer *pOwner = ToBasePlayer(GetOwner());

		if (pOwner)
		{
			switch (m_AttackPaused)
			{
			case GRENADE_HH_PAUSED_PRIMARY:
				if (!(pOwner->m_nButtons & IN_ATTACK))
				{
					SendWeaponAnim(ACT_VM_THROW);
					m_fDrawbackFinished = false;
				}
				break;

			case GRENADE_HH_PAUSED_SECONDARY:
				if (!(pOwner->m_nButtons & IN_ATTACK2))
				{
					//See if we're ducking
					if (pOwner->m_nButtons & IN_DUCK)
					{
						//Send the weapon animation
						SendWeaponAnim(ACT_VM_SECONDARYATTACK);
					}
					else
					{
						//Send the weapon animation
						SendWeaponAnim(ACT_VM_HAULBACK);
					}

					m_fDrawbackFinished = false;
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
		if (IsViewModelSequenceFinished())
		{
			Reload();
		}
	}
}

// check a throw from vecSrc.  If not valid, move the position back along the line to vecEye
void CWeaponHolywater::CheckThrowPosition(CBasePlayer *pPlayer, const Vector &vecEye, Vector &vecSrc)
{
	trace_t tr;

	UTIL_TraceHull(vecEye, vecSrc, -Vector(GRENADE_HH_RADIUS + 2, GRENADE_HH_RADIUS + 2, GRENADE_HH_RADIUS + 2), Vector(GRENADE_HH_RADIUS + 2, GRENADE_HH_RADIUS + 2, GRENADE_HH_RADIUS + 2),
		pPlayer->PhysicsSolidMaskForEntity(), pPlayer, pPlayer->GetCollisionGroup(), &tr);

	if (tr.DidHit())
	{
		vecSrc = tr.endpos;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//-----------------------------------------------------------------------------
void CWeaponHolywater::ThrowGrenade(CBasePlayer *pPlayer)
{
#ifndef CLIENT_DLL
	Vector	vecEye = pPlayer->EyePosition();
	Vector	vForward, vRight;

	pPlayer->EyeVectors(&vForward, &vRight, NULL);
	Vector vecSrc = vecEye + vForward * 18.0f + vRight * 8.0f + Vector(0, 0, -8);
	CheckThrowPosition(pPlayer, vecEye, vecSrc);

	Vector vecThrow;
	GetVelocity(&vecThrow, NULL);
	vecThrow = vecThrow * 0.3f;
	vecThrow += vForward * 500 + Vector(0, 0, 150);
	CGrenadeHH *pGrenade = (CGrenadeHH*)Create("grenade_HH", vecSrc, vec3_angle, pPlayer);
	pGrenade->SetAbsVelocity(vecThrow);
	pGrenade->SetLocalAngularVelocity(RandomAngle(-200, 200));
	pGrenade->SetMoveType(MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE);
	pGrenade->SetThrower(pPlayer);
	pGrenade->SetDamage(GetHL2MPWpnData().m_iPlayerDamage);
	pGrenade->SetDamageRadius(150.0f);
#endif

	m_bRedraw = true;

	WeaponSound(SINGLE);

	// player "shoot" animation
	pPlayer->SetAnimation(PLAYER_ATTACK1);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//-----------------------------------------------------------------------------
void CWeaponHolywater::LobGrenade(CBasePlayer *pPlayer)
{
#ifndef CLIENT_DLL
	Vector	vecEye = pPlayer->EyePosition();
	Vector	vForward, vRight;

	pPlayer->EyeVectors(&vForward, &vRight, NULL);
	Vector vecSrc = vecEye + vForward * 18.0f + vRight * 8.0f + Vector(0, 0, -8);
	CheckThrowPosition(pPlayer, vecEye, vecSrc);

	Vector vecThrow;
	GetVelocity(&vecThrow, NULL);
	vecThrow = vecThrow * 0.3f;
	vecThrow += vForward * 250 + Vector(0, 0, 150);
	CGrenadeHH *pGrenade = (CGrenadeHH*)Create("grenade_HH", vecSrc, vec3_angle, pPlayer);
	pGrenade->SetAbsVelocity(vecThrow);
	pGrenade->SetLocalAngularVelocity(RandomAngle(-200, 200));
	pGrenade->SetMoveType(MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE);
	pGrenade->SetThrower(pPlayer);
	pGrenade->SetDamage(GetHL2MPWpnData().m_iPlayerDamage);
	pGrenade->SetDamageRadius(150.0f);
#endif

	WeaponSound(WPN_DOUBLE);

	// player "shoot" animation
	pPlayer->SetAnimation(PLAYER_ATTACK1);

	m_bRedraw = true;
}
