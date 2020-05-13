#ifndef COVEN_BUILDERLOCALDATA_H
#define COVEN_BUILDERLOCALDATA_H
#ifdef _WIN32
#pragma once
#endif

#include "networkvar.h"

class CCovenBuilderLocalData
{
public:
	// Save/restore
	DECLARE_CLASS_NOBASE(CCovenBuilderLocalData);
	DECLARE_EMBEDDED_NETWORKVAR();

	CCovenBuilderLocalData();

	CNetworkVar(int,	m_iTurretHP);
	CNetworkVar(int,	m_iTurretMaxHP);
	CNetworkVar(int,	m_iTurretAmmo);
	CNetworkVar(int,	m_iTurretMaxAmmo);
	CNetworkVar(int,	m_iTurretEnergy);
	CNetworkVar(int,	m_iTurretMaxEnergy);
	CNetworkVar(int,	m_iTurretXP);
	CNetworkVar(int,	m_iTurretMaxXP);
	CNetworkVar(int,	m_iTurretLevel);
	CNetworkVar(bool,	m_bTurretTipped);

	CNetworkVar(int,	m_iDispenserHP);
	CNetworkVar(int,	m_iDispenserMaxHP);
	CNetworkVar(int,	m_iDispenserMetal);
	CNetworkVar(int,	m_iDispenserMaxMetal);
	CNetworkVar(int,	m_iDispenserXP);
	CNetworkVar(int,	m_iDispenserMaxXP);
	CNetworkVar(int,	m_iDispenserLevel);

	CNetworkVar(int,	m_iNumTripmines);
};

EXTERN_SEND_TABLE(DT_CovenBuilderLocal);


#endif // HL2_PLAYERLOCALDATA_H
