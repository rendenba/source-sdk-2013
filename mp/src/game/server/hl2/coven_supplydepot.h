#ifndef COVEN_SUPPLYDEPOT_H
#define COVEN_SUPPLYDEPOT_H

#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "covenbuilding.h"

class CCoven_SupplyDepot : public CCovenBuilding
{
public:
	DECLARE_CLASS(CCoven_SupplyDepot, CCovenBuilding);

	CCoven_SupplyDepot();

	virtual void	Spawn(void);
	virtual void	Precache(void);

	int iDepotType;
};

#endif