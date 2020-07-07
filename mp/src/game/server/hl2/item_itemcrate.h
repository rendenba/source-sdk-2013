#ifndef ITEM_ITEMCRATE_H
#define ITEM_ITEMCRATE_H

#include "cbase.h"
#include "props.h"
#include "items.h"

//-----------------------------------------------------------------------------
// A breakable crate that drops items
//-----------------------------------------------------------------------------
class CItem_ItemCrate : public CPhysicsProp
{
public:
	DECLARE_CLASS(CItem_ItemCrate, CPhysicsProp);
	DECLARE_DATADESC();

	void Precache(void);
	void Spawn(void);

	void	FallThink(void);
	float	m_flNextResetCheckTime;
	float	m_flLifetime;

	virtual int	ObjectCaps() { return BaseClass::ObjectCaps() | FCAP_WCEDIT_POSITION; };

	virtual int		OnTakeDamage(const CTakeDamageInfo &info);

	void InputKill(inputdata_t &data);

	virtual void VPhysicsCollision(int index, gamevcollisionevent_t *pEvent);
	virtual void OnPhysGunPickup(CBasePlayer *pPhysGunUser, PhysGunPickup_t reason);

	void AddCovenItem(CovenItemID_t iItemType, int iCount = 1);

	void UpdateOnRemove(void);

	struct CovenItemCollection
	{
		int				iCount;
		CovenItemID_t	iItemType;
	};

	Vector	GetOriginalSpawnOrigin(void) { return m_vOriginalSpawnOrigin; }
	QAngle	GetOriginalSpawnAngles(void) { return m_vOriginalSpawnAngles; }
	void	SetOriginalSpawnOrigin(const Vector& origin) { m_vOriginalSpawnOrigin = origin; }
	void	SetOriginalSpawnAngles(const QAngle& angles) { m_vOriginalSpawnAngles = angles; }

protected:
	virtual void OnBreak(const Vector &vecVelocity, const AngularImpulse &angVel, CBaseEntity *pBreaker);

private:
	// Crate types. Add more!
	enum CrateType_t
	{
		CRATE_SPECIFIC_ITEM = 0,
		CRATE_COVEN,
		CRATE_TYPE_COUNT,
	};

	enum CrateAppearance_t
	{
		CRATE_APPEARANCE_DEFAULT = 0,
		CRATE_APPEARANCE_RADAR_BEACON,
	};

private:
	Vector				m_vOriginalSpawnOrigin;
	QAngle				m_vOriginalSpawnAngles;
	CrateType_t			m_CrateType;
	string_t			m_strItemClass;
	int					m_nItemCount;
	string_t			m_strAlternateMaster;
	CrateAppearance_t	m_CrateAppearance;
	CUtlVector<CovenItemCollection *> m_CovenItems;

	COutputEvent m_OnCacheInteraction;
};

#endif