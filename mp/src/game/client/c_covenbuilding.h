#ifndef C_COVENBUILDING_H
#define C_COVENBUILDING_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "c_basecombatcharacter.h"

#define CCovenBuilding C_CovenBuilding

class C_CovenBuilding : public C_BaseCombatCharacter
{
public:
	DECLARE_CLASS(C_CovenBuilding, C_BaseCombatCharacter);
	DECLARE_CLIENTCLASS();

	C_CovenBuilding();

	virtual bool	IsABuilding() const { return true; }
	virtual int		GetHealth() { return m_iHealth; }
	virtual int		GetMaxHealth() { return m_iMaxHealth; }

	BuildingType_t m_BuildingType;
	EHANDLE mOwner;
	int		m_iLevel;
};

#endif