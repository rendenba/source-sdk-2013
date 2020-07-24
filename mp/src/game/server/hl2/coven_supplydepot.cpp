#include "cbase.h"
#include "coven_supplydepot.h"
#include "covenlib.h"
#include "coven_parse.h"

LINK_ENTITY_TO_CLASS(coven_supplydepot, CCoven_SupplyDepot);

CCoven_SupplyDepot::CCoven_SupplyDepot()
{
	iDepotType = 0;
}

void CCoven_SupplyDepot::Spawn(void)
{
	if (iDepotType == 0)
		return;

	AddSpawnFlags(SF_COVENBUILDING_INERT);

	if (iDepotType >= CovenSupplyDepotDataLength())
		return;

	CovenSupplyDepotInfo_t *info = GetCovenSupplyDepotData(iDepotType);

	QAngle angles = GetAbsAngles() + info->qOffsetAngle;
	Vector shift = info->vOffsetPosition;
	VectorRotate2D(shift, angles.y, &shift);
	Vector origin = GetAbsOrigin() + shift;
	SetAbsAngles(angles);
	SetAbsOrigin(origin);
	SetModel(info->szModelName);

	for (int i = 0; i < info->ItemInfo.Count(); i++)
	{
		CBaseEntity *pEnt = CreateEntityByName(info->ItemInfo[i]->szItemName);
		Vector src = origin + info->ItemInfo[i]->vRelPosition;
		VectorRotate2DPoint(src, origin, angles.y, &src);
		pEnt->SetAbsAngles(angles + info->ItemInfo[i]->qRelAngle);
		pEnt->SetAbsOrigin(src);
		pEnt->Spawn();
		pEnt->ChangeTeam(COVEN_TEAMID_SLAYERS);
	}

	/*switch (iDepotType)
	{
		case COVEN_SUPPLYDEPOT_TYPE_ONE:
		{
			Vector shift;
			QAngle angles = GetAbsAngles();
			shift.Init(0, -9, 0);
			VectorRotate2D(shift, angles.y, &shift);
			Vector origin = GetAbsOrigin() + shift;
			SetAbsOrigin(origin);
			SetModel("models/props_wasteland/prison_shelf001a.mdl");

			CBaseEntity *pEnt = CreateEntityByName("npc_depot_holywater");
			Vector src = origin + Vector(2.872559, -1.312744, -11.4);
			VectorRotate2DPoint(src, origin, angles.y, &src);
			pEnt->SetAbsAngles(angles + QAngle(0, -70, 0));
			pEnt->SetAbsOrigin(src);
			pEnt->Spawn();
			pEnt->ChangeTeam(COVEN_TEAMID_SLAYERS);

			pEnt = CreateEntityByName("npc_depot_stimpack");
			src = origin + Vector(-9.272559, -1.312744, -5.5);
			VectorRotate2DPoint(src, origin, angles.y, &src);
			pEnt->SetAbsAngles(angles + QAngle(10, -80, 90));
			pEnt->SetAbsOrigin(src);
			pEnt->Spawn();
			pEnt->ChangeTeam(COVEN_TEAMID_SLAYERS);

			pEnt = CreateEntityByName("npc_depot_pills");
			src = origin + Vector(12.272559, -3.312744, -7.5);
			VectorRotate2DPoint(src, origin, angles.y, &src);
			pEnt->SetAbsAngles(angles + QAngle(0, 80, 0));
			pEnt->SetAbsOrigin(src);
			pEnt->Spawn();
			pEnt->ChangeTeam(COVEN_TEAMID_SLAYERS);
			break;
		}
		case COVEN_SUPPLYDEPOT_TYPE_TWO:
		{
			SetAbsOrigin(GetAbsOrigin() + Vector(0, 0, 16));
			Vector origin = GetAbsOrigin();
			QAngle angles = GetAbsAngles();
			SetModel("models/props_wasteland/cafeteria_table001a.mdl");

			CBaseEntity *pEnt = CreateEntityByName("npc_depot_grenade");
			Vector src = origin + Vector(-1.097168, -10.170532, 16.68);
			VectorRotate2DPoint(src, origin, angles.y, &src);
			pEnt->SetAbsAngles(angles + QAngle(40, 10, 90));
			pEnt->SetAbsOrigin(src);
			pEnt->Spawn();
			pEnt->ChangeTeam(COVEN_TEAMID_SLAYERS);

			pEnt = CreateEntityByName("npc_depot_stungrenade");
			src = origin + Vector(-6.097168, -40.170532, 16.68);
			VectorRotate2DPoint(src, origin, angles.y, &src);
			pEnt->SetAbsAngles(angles + QAngle(70, -60, 90));
			pEnt->SetAbsOrigin(src);
			pEnt->Spawn();
			pEnt->ChangeTeam(COVEN_TEAMID_SLAYERS);

			pEnt = CreateEntityByName("npc_depot_medkit");
			src = origin + Vector(2.097168, 22.170532, 15.2);
			VectorRotate2DPoint(src, origin, angles.y, &src);
			pEnt->SetAbsAngles(angles + QAngle(0, -80, 0));
			pEnt->SetAbsOrigin(src);
			pEnt->Spawn();
			pEnt->ChangeTeam(COVEN_TEAMID_SLAYERS);
			break;
		}
		case COVEN_SUPPLYDEPOT_TYPE_FOUR:
		{
			/*Vector(18.611817, -1.845764, -55.3)*/
			/*Vector origin = GetAbsOrigin();
			QAngle angles = GetAbsAngles();
			SetModel("models/props_lab/workspace004.mdl");

			CBaseEntity *pEnt = CreateEntityByName("npc_depot_grenade");
			Vector src = origin - Vector(-48.018798, 11.116821, -31.9);
			VectorRotate2DPoint(src, origin, angles.y, &src);
			pEnt->SetAbsAngles(angles + QAngle(40, 40, 90));
			pEnt->SetAbsOrigin(src);
			pEnt->Spawn();
			pEnt->ChangeTeam(COVEN_TEAMID_SLAYERS);
			break;
		}
		default:
		{
			break;
		}
	}*/
	m_BuildingType = BUILDING_SUPPLYDEPOT;
	BaseClass::Spawn();
}

void CCoven_SupplyDepot::Precache(void)
{
	for (int i = 1; i < CovenSupplyDepotDataLength(); i++)
	{
		CovenSupplyDepotInfo_t *info = GetCovenSupplyDepotData(i);
		PrecacheModel(info->szModelName);
	}
	BaseClass::Precache();
}