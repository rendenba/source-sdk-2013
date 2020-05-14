#ifndef COVEN_AMMOCRATE_H
#define COVEN_AMMOCRATE_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "player.h"
#include "gamerules.h"
#include "items.h"
#include "ammodef.h"
#include "eventlist.h"
#include "npcevent.h"
#include "hl2mp_player.h"
#include "covenbuilding.h"

#define	COVEN_CRATE_CLOSE_DELAY		1.5f
#define COVEN_CRATE_MODELNAME		"models/items/ammocrate_smg1.mdl"

#define SF_COVEN_CRATE_INFINITE		(1 << 8)

class CCoven_AmmoCrate : public CCovenBuilding
{
public:
	DECLARE_CLASS(CCoven_AmmoCrate, CCovenBuilding);

	virtual void	Spawn(void);
	virtual void	Precache(void);

	virtual void	HandleAnimEvent(animevent_t *pEvent);
	virtual void	SelfDestructThink(void);
	virtual void	SelfDestruct(void);
	virtual int		GetAmmo(int index);
	virtual int		GetMaxAmmo(int index);

	void			AmmoCrateThink(void);

	float			PickupRadius() { return 100.0f; };

	virtual float	BuildTime(void) { return 4.0f; };

	virtual void	OnBuildingComplete(void);

	virtual void	Enable(void);
	virtual			BuildingType const MyType() { return BUILDING_AMMOCRATE; };

	void		AddMetal(void);
	int			GetMetal(int have);
	bool		GiveMetal(CHL2MP_Player *pPlayer);
	bool		GiveAmmo(int index, bool &gaveMetal);
	bool		Open(CBasePlayer *pPlayer);
	virtual const Vector GetPlayerMidPoint(void) const;

	int m_iMetal;
	int m_flMetalTimer;

protected:

	int		m_iAmmoType;
	int		m_iMaxMetal;

	COutputEvent m_OnPhysGunPickup;
	COutputEvent m_OnPhysGunDrop;

	float	m_flCloseTime;
	CHandle< CBasePlayer > m_hActivator;

	DECLARE_DATADESC();
};

#endif