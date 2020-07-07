#include "cbase.h"

#include "c_covenbuilding.h"

#undef CCovenBuilding

LINK_ENTITY_TO_CLASS(coven_building, C_CovenBuilding);

IMPLEMENT_CLIENTCLASS_DT(C_CovenBuilding, DT_CovenBuilding, CCovenBuilding)
	RecvPropInt(RECVINFO(m_BuildingType)),
END_RECV_TABLE()

C_CovenBuilding::C_CovenBuilding()
{
	m_BuildingType = BUILDING_DEFAULT;
}