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

#ifdef CLIENT_DLL
#define CWeaponStunFrag C_WeaponStunFrag
#endif

//-----------------------------------------------------------------------------
// Fragmentation grenades
//-----------------------------------------------------------------------------
class CWeaponStunFrag : public CWeaponFrag
{
	DECLARE_CLASS(CWeaponStunFrag, CWeaponFrag);
public:

	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CWeaponStunFrag();

private:
	CWeaponStunFrag(const CWeaponStunFrag &);
};

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponStunFrag, DT_WeaponStunFrag)

BEGIN_NETWORK_TABLE(CWeaponStunFrag, DT_WeaponStunFrag)

END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponStunFrag)
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS(weapon_stunfrag, CWeaponStunFrag);
PRECACHE_WEAPON_REGISTER(weapon_stunfrag);

CWeaponStunFrag::CWeaponStunFrag(void) :
CWeaponFrag()
{
	m_flGrenadeRadius = 4.0f; // inches
	m_flGrenadeDamageRadius = 300.0f;
	m_GrenadeType = GRENADE_TYPE_STUN;
}

void DropPrimedStunFragGrenade(CHL2MP_Player *pPlayer, CBaseCombatWeapon *pGrenade)
{
	CWeaponStunFrag *pWeaponFrag = dynamic_cast<CWeaponStunFrag*>(pGrenade);

	if (pWeaponFrag)
	{
		pWeaponFrag->ThrowGrenade(pPlayer);
		pWeaponFrag->DecrementAmmo(pPlayer);
	}
}

