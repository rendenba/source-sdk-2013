#ifndef COVEN_SUPPLYDEPOT_H
#define COVEN_SUPPLYDEPOT_H

#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "covenbuilding.h"

typedef enum
{
	COVEN_SUPPLYDEPOT_TYPE_NONE,
	COVEN_SUPPLYDEPOT_TYPE_ONE,
	COVEN_SUPPLYDEPOT_TYPE_TWO,
	COVEN_SUPPLYDEPOT_TYPE_THREE,
	COVEN_SUPPLYDEPOT_TYPE_FOUR,
	COVEN_SUPPLYDEPOT_TYPE_MAX
} CovenSupplyDepotType_t;

class CCoven_SupplyDepot : public CCovenBuilding
{
public:
	DECLARE_CLASS(CCoven_SupplyDepot, CCovenBuilding);

	CCoven_SupplyDepot();

	virtual void	Spawn(void);
	virtual void	Precache(void);

	CovenSupplyDepotType_t iDepotType;
};

#endif