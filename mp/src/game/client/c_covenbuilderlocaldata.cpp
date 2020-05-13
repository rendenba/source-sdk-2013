#include "cbase.h"
#include "c_covenbuilderlocaldata.h"
#include "dt_recv.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_RECV_TABLE_NOBASE(C_CovenBuilderLocalData, DT_CovenBuilderLocal)
RecvPropInt(RECVINFO(m_iTurretHP)),
RecvPropInt(RECVINFO(m_iTurretMaxHP)),
RecvPropInt(RECVINFO(m_iTurretAmmo)),
RecvPropInt(RECVINFO(m_iTurretMaxAmmo)),
RecvPropInt(RECVINFO(m_iTurretEnergy)),
RecvPropInt(RECVINFO(m_iTurretMaxEnergy)),
RecvPropInt(RECVINFO(m_iTurretXP)),
RecvPropInt(RECVINFO(m_iTurretMaxXP)),
RecvPropInt(RECVINFO(m_iTurretLevel)),
RecvPropBool(RECVINFO(m_bTurretTipped)),

RecvPropInt(RECVINFO(m_iDispenserHP)),
RecvPropInt(RECVINFO(m_iDispenserMaxHP)),
RecvPropInt(RECVINFO(m_iDispenserMetal)),
RecvPropInt(RECVINFO(m_iDispenserMaxMetal)),
RecvPropInt(RECVINFO(m_iDispenserXP)),
RecvPropInt(RECVINFO(m_iDispenserMaxXP)),
RecvPropInt(RECVINFO(m_iDispenserLevel)),

RecvPropInt(RECVINFO(m_iNumTripmines)),
END_RECV_TABLE()

C_CovenBuilderLocalData::C_CovenBuilderLocalData()
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

