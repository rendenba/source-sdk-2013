#if !defined( C_COVEN_BUILDERLOCALDATA_H )
#define C_COVEN_BUILDERLOCALDATA_H
#ifdef _WIN32
#pragma once
#endif


#include "dt_recv.h"

EXTERN_RECV_TABLE(DT_CovenBuilderLocal);

class C_CovenBuilderLocalData
{
public:
	DECLARE_CLASS_NOBASE(C_CovenBuilderLocalData);
	DECLARE_EMBEDDED_NETWORKVAR();

	C_CovenBuilderLocalData();

	int		m_iTurretHP;
	int		m_iTurretMaxHP;
	int		m_iTurretAmmo;
	int		m_iTurretMaxAmmo;
	int		m_iTurretEnergy;
	int		m_iTurretMaxEnergy;
	int		m_iTurretXP;
	int		m_iTurretMaxXP;
	int		m_iTurretLevel;
	bool	m_bTurretTipped;

	int		m_iDispenserHP;
	int		m_iDispenserMaxHP;
	int		m_iDispenserMetal;
	int		m_iDispenserMaxMetal;
	int		m_iDispenserXP;
	int		m_iDispenserMaxXP;
	int		m_iDispenserLevel;
};


#endif
