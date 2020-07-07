#include "cbase.h"
#include "covenbuilderlocaldata.h"
#include "hl2_player.h"
#include "mathlib/mathlib.h"
#include "entitylist.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_SEND_TABLE_NOBASE(CCovenBuilderLocalData, DT_CovenBuilderLocal)
SendPropInt(SENDINFO(m_iTurretHP), 9, SPROP_UNSIGNED),
SendPropInt(SENDINFO(m_iTurretMaxHP), 9, SPROP_UNSIGNED),
SendPropInt(SENDINFO(m_iTurretAmmo), 9, SPROP_UNSIGNED),
SendPropInt(SENDINFO(m_iTurretMaxAmmo), 9, SPROP_UNSIGNED),
SendPropInt(SENDINFO(m_iTurretEnergy), 9, SPROP_UNSIGNED),
SendPropInt(SENDINFO(m_iTurretMaxEnergy), 9, SPROP_UNSIGNED),
SendPropInt(SENDINFO(m_iTurretXP), 9, SPROP_UNSIGNED),
SendPropInt(SENDINFO(m_iTurretMaxXP), 9, SPROP_UNSIGNED),
SendPropInt(SENDINFO(m_iTurretLevel), 3, SPROP_UNSIGNED),
SendPropBool(SENDINFO(m_bTurretTipped)),

SendPropInt(SENDINFO(m_iDispenserHP), 9, SPROP_UNSIGNED),
SendPropInt(SENDINFO(m_iDispenserMaxHP), 9, SPROP_UNSIGNED),
SendPropInt(SENDINFO(m_iDispenserMetal), 9, SPROP_UNSIGNED),
SendPropInt(SENDINFO(m_iDispenserMaxMetal), 9, SPROP_UNSIGNED),
SendPropInt(SENDINFO(m_iDispenserXP), 9, SPROP_UNSIGNED),
SendPropInt(SENDINFO(m_iDispenserMaxXP), 9, SPROP_UNSIGNED),
SendPropInt(SENDINFO(m_iDispenserLevel), 3, SPROP_UNSIGNED),

SendPropInt(SENDINFO(m_iNumTripmines), 3, SPROP_UNSIGNED),
END_SEND_TABLE()

CCovenBuilderLocalData::CCovenBuilderLocalData()
{
	m_iTurretHP = m_iDispenserHP = 0;
	m_iTurretMaxHP = m_iDispenserMaxHP = 100;
	m_iTurretAmmo = m_iDispenserMetal = 0;
	m_iTurretMaxAmmo = m_iDispenserMaxMetal = 100;
	m_iTurretEnergy = m_iTurretMaxEnergy = 0;
	m_iTurretXP = m_iDispenserXP = 0;
	m_iTurretMaxXP = m_iDispenserMaxXP = 100;
	m_iTurretLevel = m_iDispenserLevel = 0;
	m_bTurretTipped = false;
	m_iNumTripmines = 0;
}

