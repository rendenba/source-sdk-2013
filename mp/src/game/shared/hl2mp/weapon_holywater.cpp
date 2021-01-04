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
#include "weapon_frag.h"

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
class CWeaponHolywater : public CWeaponFrag
{
	DECLARE_CLASS(CWeaponHolywater, CWeaponFrag);
public:

	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CWeaponHolywater();

	void	Precache(void);
	void	ItemPostFrame(void);

	void	ThrowGrenade(CBasePlayer *pPlayer);

private:

	void	LobGrenade(CBasePlayer *pPlayer);
	void	RollGrenade(CBasePlayer *pPlayer) { LobGrenade(pPlayer); }
	// check a throw from vecSrc.  If not valid, move the position back along the line to vecEye
	void	CheckThrowPosition(CBasePlayer *pPlayer, const Vector &vecEye, Vector &vecSrc);

	CWeaponHolywater(const CWeaponHolywater &);
};


IMPLEMENT_NETWORKCLASS_ALIASED(WeaponHolywater, DT_WeaponHolywater)

BEGIN_NETWORK_TABLE(CWeaponHolywater, DT_WeaponHolywater)

END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponHolywater)
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS(weapon_holywater, CWeaponHolywater);
PRECACHE_WEAPON_REGISTER(weapon_holywater);

CWeaponHolywater::CWeaponHolywater(void) :
CWeaponFrag()
{
	m_GrenadeType = GRENADE_TYPE_HOLYWATER;
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
}

void DropPrimedHolyWaterGrenade(CHL2MP_Player *pPlayer, CBaseCombatWeapon *pGrenade)
{
	CWeaponHolywater *pWeaponHW = dynamic_cast<CWeaponHolywater*>(pGrenade);

	if (pWeaponHW)
	{
		pWeaponHW->ThrowGrenade(pPlayer);
		pWeaponHW->DecrementAmmo(pPlayer);
	}
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
					m_AttackPaused = GRENADE_HH_PAUSED_NO;
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
					m_AttackPaused = GRENADE_HH_PAUSED_NO;
				}
				break;

			default:
				break;
			}
		}
	}

	//HACK! skip base frag grenade call
	CBaseHL2MPCombatWeapon::ItemPostFrame();

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
	pGrenade->SetDamageRadius(200.0f);
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
	pGrenade->SetDamageRadius(200.0f);
#endif

	WeaponSound(WPN_DOUBLE);

	// player "shoot" animation
	pPlayer->SetAnimation(PLAYER_ATTACK1);

	m_bRedraw = true;
}
