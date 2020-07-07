#ifndef PURCHASEITEM_SHARED_H
#define PURCHASEITEM_SHARED_H

#ifdef _WIN32
#pragma once
#endif

#include "baseentity_shared.h"

#if defined(CLIENT_DLL)

#define CPurchaseItem C_PurchaseItem
#include "c_covenbuilding.h"

#else

#include "covenbuilding.h"

#endif

#if !defined( CLIENT_DLL )
class CPurchaseItem : public CCovenBuilding
#else
class CPurchaseItem : public C_CovenBuilding
#endif
{
public:
	DECLARE_CLASS(CPurchaseItem, CCovenBuilding);

	CPurchaseItem();

#if !defined( CLIENT_DLL )
	virtual void Spawn(void);
	virtual void Precache(void);

	virtual int	ObjectCaps() { return BaseClass::ObjectCaps() | FCAP_IMPULSE_USE; };
	virtual void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);

	virtual bool PlayerUseItem(CBasePlayer *pPlayer);
#endif

	CovenItemID_t GetItemType()
	{
		return iItemType;
	}

protected:
	CovenItemID_t iItemType;
};

#endif
