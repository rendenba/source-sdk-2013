#ifndef C_COVENBUILDING_H
#define C_COVENBUILDING_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "c_basecombatcharacter.h"

class C_CovenBuilding : public C_BaseCombatCharacter
{
public:
	DECLARE_CLASS(C_CovenBuilding, C_BaseCombatCharacter);
	DECLARE_CLIENTCLASS();

	virtual bool IsABuilding() const { return true; };
};

#endif