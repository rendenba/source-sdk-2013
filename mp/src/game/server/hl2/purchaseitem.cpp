#include "cbase.h"
#include "purchaseitem_shared.h"
#include "hl2mp_gamerules.h"

CPurchaseItem::CPurchaseItem()
{
	iItemType = COVEN_ITEM_NONE;
}

void CPurchaseItem::Spawn(void)
{
	AddSpawnFlags(SF_COVENBUILDING_INERT);

	BaseClass::Spawn();
	SetCollisionGroup(COLLISION_GROUP_WEAPON);

	AddEffects(EF_ITEM_BLINK);
}

void CPurchaseItem::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	CBasePlayer *pPlayer = ToBasePlayer(pActivator);

	if (pPlayer && pPlayer->GetTeamNumber() == GetTeamNumber())
		PlayerUseItem(pPlayer);
}

bool CPurchaseItem::PlayerUseItem(CBasePlayer *pPlayer)
{
	CHL2MP_Player *pHL2Player = ToHL2MPPlayer(pPlayer);
	return pHL2Player->PurchaseCovenItem(iItemType);
}

void CPurchaseItem::Precache(void)
{
	CovenBuildingInfo_t *bldgInfo = GetCovenBuildingData(m_BuildingType);
	PrecacheModel(bldgInfo->szModelName);
}

/***************************************************************************************************************/
class CPurchaseItem_Grenade : public CPurchaseItem
{
public:
	DECLARE_CLASS(CPurchaseItem_Grenade, CPurchaseItem);

	CPurchaseItem_Grenade()
	{
		iItemType = COVEN_ITEM_GRENADE;
	}
	void Spawn(void)
	{
		m_BuildingType = BUILDING_PURCHASE_GRENADE;
		Precache();
		CovenBuildingInfo_t *bldgInfo = GetCovenBuildingData(m_BuildingType);
		SetModel(bldgInfo->szModelName);
		BaseClass::Spawn();
	}
};
LINK_ENTITY_TO_CLASS(npc_depot_grenade, CPurchaseItem_Grenade);

/***************************************************************************************************************/
class CPurchaseItem_Holywater : public CPurchaseItem
{
public:
	DECLARE_CLASS(CPurchaseItem_Holywater, CPurchaseItem);

	CPurchaseItem_Holywater()
	{
		iItemType = COVEN_ITEM_HOLYWATER;
	}
	void Spawn(void)
	{
		m_BuildingType = BUILDING_PURCHASE_HOLYWATER;
		Precache();
		CovenBuildingInfo_t *bldgInfo = GetCovenBuildingData(m_BuildingType);
		SetModel(bldgInfo->szModelName);
		BaseClass::Spawn();
	}
};
LINK_ENTITY_TO_CLASS(npc_depot_holywater, CPurchaseItem_Holywater);

/***************************************************************************************************************/
class CPurchaseItem_StunGrenade : public CPurchaseItem
{
public:
	DECLARE_CLASS(CPurchaseItem_StunGrenade, CPurchaseItem);

	CPurchaseItem_StunGrenade()
	{
		iItemType = COVEN_ITEM_STUN_GRENADE;
	}
	void Spawn(void)
	{
		m_BuildingType = BUILDING_PURCHASE_STUNGRENADE;
		Precache();
		CovenBuildingInfo_t *bldgInfo = GetCovenBuildingData(m_BuildingType);
		SetModel(bldgInfo->szModelName);
		BaseClass::Spawn();
	}
};
LINK_ENTITY_TO_CLASS(npc_depot_stungrenade, CPurchaseItem_StunGrenade);

/***************************************************************************************************************/
class CPurchaseItem_StimPack : public CPurchaseItem
{
public:
	DECLARE_CLASS(CPurchaseItem_StimPack, CPurchaseItem);

	CPurchaseItem_StimPack()
	{
		iItemType = COVEN_ITEM_STIMPACK;
	}
	void Spawn(void)
	{
		m_BuildingType = BUILDING_PURCHASE_STIMPACK;
		Precache();
		CovenBuildingInfo_t *bldgInfo = GetCovenBuildingData(m_BuildingType);
		SetModel(bldgInfo->szModelName);
		BaseClass::Spawn();
	}
};
LINK_ENTITY_TO_CLASS(npc_depot_stimpack, CPurchaseItem_StimPack);

/***************************************************************************************************************/
class CPurchaseItem_Medkit : public CPurchaseItem
{
public:
	DECLARE_CLASS(CPurchaseItem_Medkit, CPurchaseItem);

	CPurchaseItem_Medkit()
	{
		iItemType = COVEN_ITEM_MEDKIT;
	}
	void Spawn(void)
	{
		m_BuildingType = BUILDING_PURCHASE_MEDKIT;
		Precache();
		CovenBuildingInfo_t *bldgInfo = GetCovenBuildingData(m_BuildingType);
		SetModel(bldgInfo->szModelName);
		BaseClass::Spawn();
	}
};
LINK_ENTITY_TO_CLASS(npc_depot_medkit, CPurchaseItem_Medkit);