#ifndef WEAPON_FRAG_H
#define WEAPON_FRAG_H

#include "cbase.h"
#include "weapon_hl2mpbasehlmpcombatweapon.h"
#include "basegrenade_shared.h"

#ifdef CLIENT_DLL
#define CWeaponFrag C_WeaponFrag
#endif

//-----------------------------------------------------------------------------
// Fragmentation grenades
//-----------------------------------------------------------------------------
class CWeaponFrag : public CBaseHL2MPCombatWeapon
{
	DECLARE_CLASS(CWeaponFrag, CBaseHL2MPCombatWeapon);
public:

	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CWeaponFrag();

	void	Precache(void);
	void	PrimaryAttack(void);
	void	SecondaryAttack(void);
	void	DecrementAmmo(CBaseCombatCharacter *pOwner);
	virtual void	ItemPostFrame(void);
	virtual bool CanFire();

	bool	Deploy(void);
	bool	Holster(CBaseCombatWeapon *pSwitchingTo = NULL);

	virtual bool	IsGrenade() const { return true; }

	GrenadeType_t	GrenadeType() const { return m_GrenadeType; }

	bool	Reload(void);

#ifndef CLIENT_DLL
	void Operator_HandleAnimEvent(animevent_t *pEvent, CBaseCombatCharacter *pOperator);
#endif

	virtual void	ThrowGrenade(CBasePlayer *pPlayer);
	bool	IsPrimed(void) { return (m_AttackPaused != 0); }

private:

	virtual void	RollGrenade(CBasePlayer *pPlayer);
	virtual void	LobGrenade(CBasePlayer *pPlayer);
	// check a throw from vecSrc.  If not valid, move the position back along the line to vecEye
	virtual void	CheckThrowPosition(CBasePlayer *pPlayer, const Vector &vecEye, Vector &vecSrc);

	float m_flCookTime;
	float m_flNextBlipTime;
	bool m_bHasWarnedAI;
	float m_flWarnAITime;

	CWeaponFrag(const CWeaponFrag &);

#ifndef CLIENT_DLL
	DECLARE_ACTTABLE();
#endif

protected:
	GrenadeType_t m_GrenadeType;
	float	m_flGrenadeRadius;
	float	m_flGrenadeDamageRadius;

	CNetworkVar(bool, m_bRedraw);	//Draw the weapon again after throwing a grenade

	CNetworkVar(int, m_AttackPaused);
	CNetworkVar(bool, m_fDrawbackFinished);
};

#endif