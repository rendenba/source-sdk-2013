#include "cbase.h"

#include "c_covenbuilding.h"

#undef CCovenBuilding

LINK_ENTITY_TO_CLASS(coven_building, C_CovenBuilding);

IMPLEMENT_CLIENTCLASS_DT(C_CovenBuilding, DT_CovenBuilding, CCovenBuilding)
	RecvPropInt(RECVINFO(m_BuildingType)),
	RecvPropInt(RECVINFO(m_iHealth)),
	RecvPropInt(RECVINFO(m_iMaxHealth)),
	RecvPropEHandle(RECVINFO(mOwner)),
	RecvPropInt(RECVINFO(m_iLevel)),
END_RECV_TABLE()

C_CovenBuilding::C_CovenBuilding()
{
	m_BuildingType = BUILDING_DEFAULT;
	mOwner = NULL;
}