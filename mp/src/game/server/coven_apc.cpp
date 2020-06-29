#include "cbase.h"

#include "vphysics/constraints.h"
#include "coven_apc.h"
#include "hl2mp_gamerules.h"

extern ConVar bot_debug_visual;

Vector	CCoven_APC::wheelOffset[] = { Vector(80, -50, -32), Vector(-72, -50, -32), Vector(80, 50, -32), Vector(-72, 50, -32) };
QAngle	CCoven_APC::wheelOrientation[] = { QAngle(0, -90, 0), QAngle(0, -90, 0), QAngle(0, 90, 0), QAngle(0, 90, 0) };
int		CCoven_APC::wheelDirection[] = { -1, -1, 1, 1 };

float CCoven_APC::m_flWheelDiameter = 64.6f;
float CCoven_APC::m_flWheelRadius = CCoven_APC::m_flWheelDiameter / 2.0f;
float CCoven_APC::m_flWheelBase = (CCoven_APC::wheelOffset[0] - CCoven_APC::wheelOffset[1]).Length2D();
float CCoven_APC::m_flWheelTrack = (CCoven_APC::wheelOffset[0] - CCoven_APC::wheelOffset[2]).Length2D();

void VectorRotate2DPoint(const Vector &in, const Vector &point, float angle, Vector *out, bool bRadians = false)
{
	if (out)
	{
		if (!bRadians)
			angle = M_PI * angle / 180.0f;
		float cs;
		float sn;
		SinCos(angle, &sn, &cs);

		float translated_x = in.x - point.x;
		float translated_y = in.y - point.y;

		float result_x = translated_x * cs - translated_y * sn;
		float result_y = translated_x * sn + translated_y * cs;

		result_x += point.x;
		result_y += point.y;

		(*out).x = result_x;
		(*out).y = result_y;
	}
}

void VectorRotate2D(const Vector &in, float angle, Vector *out, bool bRadians = false)
{
	if (out)
	{
		if (!bRadians)
			angle = M_PI * angle / 180.0f;
		float cs;
		float sn;
		SinCos(angle, &sn, &cs);

		(*out).x = in.x * cs - in.y * sn;
		(*out).y = in.x * sn + in.y * cs;
	}
}

LINK_ENTITY_TO_CLASS(coven_prop_physics, CCoven_APCProp);

CCoven_APCProp::CCoven_APCProp()
{
	m_pParent = NULL;
}

void CCoven_APCProp::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (m_pParent)
		m_pParent->Use(pActivator, pCaller, useType, value);
}

int CCoven_APCProp::ObjectCaps(void)
{
	if (m_pParent)
		return BaseClass::ObjectCaps() | FCAP_IMPULSE_USE;
	return 0;
}

LINK_ENTITY_TO_CLASS(coven_apc, CCoven_APC);

CCoven_APC::CCoven_APC()
{
	Q_memset(m_pWheels, NULL, sizeof(m_pWheels));
	m_pBody = NULL;
	m_bPlayingSound = m_bRunning = m_bPlayingSiren = false;
	m_angTurningAngle.Init();
	m_flTurningAngle = 0.0f;
	m_iGoalNode = -1;
	m_iGoalCheckpoint = -1;
	m_flMaxSpeed = 0.0f;
	m_iFuel = m_iFuelUpAmount = 0;
	m_vecFrontAxelPosition.Init();
	m_flSoundSwitch = m_flSoundSirenSwitch = 0.0f;
	m_hLeftLampGlow = NULL;
	m_hRightLampGlow = NULL;
}

void CCoven_APC::Precache(void)
{
	PrecacheModel("models/props_junk/rock001a.mdl");
	PrecacheModel(COVEN_APC_GLOW_SPRITE);
	PrecacheScriptSound("ATV_engine_start");
	PrecacheScriptSound("ATV_engine_stop");
	PrecacheScriptSound("ATV_engine_idle");
	PrecacheScriptSound("TimerLow");

	BaseClass::Precache();
}

void CCoven_APC::Spawn(void)
{
	Precache();
	SetModel("models/props_junk/rock001a.mdl");
	AddEffects(EF_NODRAW);

	SetSolid(SOLID_VPHYSICS);
	AddSolidFlags(FSOLID_NOT_SOLID);

	SetAbsOrigin(GetAbsOrigin() + Vector(0, 0, CCoven_APC::m_flWheelDiameter));

	BaseClass::Spawn();

	m_iHealth = 100;
	m_iFuel = 0;

	for (int i = 0; i < 4; i++)
	{
		m_pWheels[i] = CCoven_APCPart::CreateAPCPart(this, "models/props_vehicles/apc_tire001.mdl", CCoven_APC::wheelOffset[i], CCoven_APC::wheelOrientation[i], CCoven_APC::m_flWheelRadius, COLLISION_GROUP_VEHICLE_WHEEL);
		m_pWheels[i]->Spawn();
		m_pWheels[i]->AttachPhysics();
		m_pWheels[i]->MoveToCorrectLocation();
	}

	m_pBody = CCoven_APCPart::CreateAPCPart(this, "models/props_vehicles/apc001.mdl", Vector(0, 0, 0), QAngle(0, -90, 0), CCoven_APC::m_flWheelRadius + 32, COLLISION_GROUP_VEHICLE, true);
	m_pBody->Spawn();
	m_pBody->AttachPhysics();
	m_pBody->MoveToCorrectLocation();

	m_flSpeed = 0.0f;

	Activate();

	SetThink(&CCoven_APC::MakeSolid);
	SetNextThink(gpGlobals->curtime + 0.1f);
	ToggleLights(false);
	ResetLocation();
	MoveForward();
}

void CCoven_APC::UpdateLightPosition(void)
{
	Vector forward(0, 0, 0), right(0, 0, 0), up(0, 0, 0);
	QAngle angles = m_pBody->GetAbsAngles();
	float roll = -angles.z;
	angles.z = -angles.x;
	angles.x = roll;
	angles -= m_pBody->GetOffsetAngle();
	AngleVectors(angles, &forward, &right, &up);
	float size = random->RandomFloat(1.5f, 2.0f);
	int bright = size * 127;
	m_hLeftLampGlow->SetAbsOrigin(GetAbsOrigin() + 143.0f * forward + 42.0f * right + 21.0f * up);
	m_hLeftLampGlow->SetBrightness(bright, 0.1f);
	m_hLeftLampGlow->SetScale(size, 0.1f);
	m_hRightLampGlow->SetAbsOrigin(GetAbsOrigin() + 143.0f * forward - 42.0f * right + 21.0f * up);
	m_hRightLampGlow->SetScale(size, 0.1f);
	m_hRightLampGlow->SetBrightness(bright, 0.1f);
}

void CCoven_APC::ToggleLights(bool bToggle)
{
	if (m_hLeftLampGlow == NULL)
	{
		m_hLeftLampGlow = CSprite::SpriteCreate(COVEN_APC_GLOW_SPRITE, GetLocalOrigin(), false);
		if (m_hLeftLampGlow)
		{
			m_hLeftLampGlow->SetTransparency(kRenderWorldGlow, 255, 255, 255, 128, kRenderFxNoDissipation);
			m_hLeftLampGlow->SetBrightness(0);
		}
	}
	if (m_hRightLampGlow == NULL)
	{
		m_hRightLampGlow = CSprite::SpriteCreate(COVEN_APC_GLOW_SPRITE, GetLocalOrigin(), false);
		if (m_hRightLampGlow)
		{
			m_hRightLampGlow->SetTransparency(kRenderWorldGlow, 255, 255, 255, 128, kRenderFxNoDissipation);
			m_hRightLampGlow->SetBrightness(0);
		}
	}

	if (bToggle)
	{
		m_hLeftLampGlow->SetColor(255, 255, 255);
		m_hRightLampGlow->SetColor(255, 255, 255);
		m_hLeftLampGlow->SetBrightness(180, 0.1f);
		m_hRightLampGlow->SetBrightness(180, 0.1f);
		m_hLeftLampGlow->SetScale(0.8f, 0.1f);
		m_hRightLampGlow->SetScale(0.8f, 0.1f);
	}
	else
	{
		m_hLeftLampGlow->SetColor(255, 255, 255);
		m_hRightLampGlow->SetColor(255, 255, 255);
		m_hLeftLampGlow->SetScale(0.1f, 0.1f);
		m_hRightLampGlow->SetScale(0.1f, 0.1f);
		m_hLeftLampGlow->SetBrightness(0, 0.1f);
		m_hRightLampGlow->SetBrightness(0, 0.1f);
	}
}

void CCoven_APC::MakeSolid(void)
{
	for (int i = 0; i < 4; i++)
	{
		m_pWheels[i]->MakeSolid();
	}
	m_pBody->MakeSolid();
	SetThink(NULL);
}

void CCoven_APC::APCThink(void)
{
	if (HasValidGoal() && HasReachedCurrentGoal())
	{
		//Msg("Reached node: %d\n", m_iGoalNode);
		if (m_iGoalNode == m_iGoalCheckpoint)
			HL2MPRules()->ReachedCheckpoint(m_iGoalCheckpoint);
		m_iGoalNode++;
		if (m_iGoalNode >= HL2MPRules()->hAPCNet.Count())
		{
			SetThink(NULL);
			return;
		}
	}

	if (m_iFuel > 0)
	{
		if (m_flSpeed < m_flMaxSpeed)
			m_flSpeed += m_flMaxSpeed / 6.0f;
	}

	SetNextThink(gpGlobals->curtime + 0.1f);

	if (HasValidGoal())
	{
		if (bot_debug_visual.GetInt() > 0)
			NDebugOverlay::Cross3D(HL2MPRules()->hAPCNet[m_iGoalNode]->location, -Vector(12, 12, 12), Vector(12, 12, 12), 0, 255, 0, false, 0.5f);

		Vector directionDifference = HL2MPRules()->hAPCNet[m_iGoalNode]->location - GetAbsOrigin();
		QAngle ang(0, 0, 0), curAngles = GetAbsAngles();
		VectorAngles(directionDifference, ang);
		float dir = AngleNormalize(curAngles.y - ang.y);
		if (dir < -1.0f)
			TurnLeft(dir);
		else if (dir > 1.0f)
			TurnRight(dir);
		else
			m_flTurningAngle = 0.0f;
		//Msg("%f %f - %f %f - %f - %f %f - %d\n", curAngles.y, ang.y, m_angTurningAngle.y, m_flTurningAngle, dir, m_flSpeed, m_flMaxSpeed, m_iFuel);
	}
	
	//Msg("%f %f - %f %f - %d\n", m_angTurningAngle.y, m_flTurningAngle, m_flSpeed, m_flMaxSpeed, m_iFuel);

	MoveForward();

	if (m_bPlayingSiren && gpGlobals->curtime > m_flSoundSirenSwitch)
	{
		m_flSoundSirenSwitch = gpGlobals->curtime + GetSoundDuration("TimerLow", NULL);
		StopSound("TimerLow");
		EmitSound("TimerLow");
	}

	if (m_bPlayingSound && gpGlobals->curtime > m_flSoundSwitch)
	{
		m_flSoundSwitch = gpGlobals->curtime + GetSoundDuration("ATV_engine_idle", NULL);
		StopSound("ATV_engine_start");
		StopSound("ATV_engine_idle");
		EmitSound("ATV_engine_idle");
	}

	if (m_iFuel == 0)
	{
		if (m_bPlayingSound)
		{
			StopSound("ATV_engine_start");
			StopSound("ATV_engine_idle");
			EmitSound("ATV_engine_stop");
			m_bPlayingSound = m_bRunning = false;
			ToggleLights(false);
		}
		if (m_flSpeed > 0.0f)
			m_flSpeed -= m_flMaxSpeed / 14.0f;
		else
		{
			m_flSpeed = 0.0f;
			SetThink(NULL);
		}
	}
	else
		m_iFuel--;
}

void CCoven_APC::UpdateOnRemove(void)
{
	if (m_hLeftLampGlow != NULL)
	{
		UTIL_Remove(m_hLeftLampGlow);
		m_hLeftLampGlow = NULL;
	}
	if (m_hRightLampGlow != NULL)
	{
		UTIL_Remove(m_hRightLampGlow);
		m_hRightLampGlow = NULL;
	}
	BaseClass::UpdateOnRemove();
}

bool CCoven_APC::HasValidGoal(void)
{
	return m_iGoalNode < HL2MPRules()->hAPCNet.Count() && m_iGoalNode >= 0;
}

bool CCoven_APC::HasReachedCurrentGoal(void)
{
	return (GetAbsOrigin() - HL2MPRules()->hAPCNet[m_iGoalNode]->location).Length2D() < 24.0f;
}

void CCoven_APC::SetGoal(int iGoalNode)
{
	m_iGoalNode = iGoalNode;
}

void CCoven_APC::SetCheckpoint(int iGoalCheckpoint)
{
	m_iGoalCheckpoint = iGoalCheckpoint;
}

void CCoven_APC::SetMaxSpeed(float flMaxSpeed)
{
	m_flMaxSpeed = flMaxSpeed;
}

void CCoven_APC::SetFuelUp(int iFuelUp)
{
	m_iFuelUpAmount = iFuelUp;
}

void CCoven_APC::StartSiren(void)
{
	if (!m_bPlayingSiren)
	{
		m_bPlayingSiren = true;
		EmitSound("TimerLow");
		m_flSoundSirenSwitch = gpGlobals->curtime + GetSoundDuration("TimerLow", NULL);
	}
}

void CCoven_APC::StopSiren(void)
{
	if (m_bPlayingSiren)
	{
		m_bPlayingSiren = false;
		StopSound("TimerLow");
		m_flSoundSirenSwitch = 0.0f;
	}
}

const Vector &CCoven_APC::GetFrontAxelPosition(void)
{
	if (gpGlobals->tickcount != m_vecTick)
	{
		m_vecTick = gpGlobals->tickcount;
		Vector forward(0, 0, 0);
		AngleVectors(GetAbsAngles(), &forward);
		m_vecFrontAxelPosition = GetAbsOrigin() + forward * CCoven_APC::wheelOffset[FRONT_PASSENGER].x;
	}
	
	return m_vecFrontAxelPosition;
}

//flMagnitude is negative
void CCoven_APC::TurnLeft(float flMagnitude)
{
	if (m_angTurningAngle.y > -COVEN_APC_MAX_TURNING_VALUE && m_angTurningAngle.y > flMagnitude)
	{
		if (m_angTurningAngle.y < -1.0f || m_angTurningAngle.y > 0.0f)
			m_flTurningAngle = max(m_flTurningAngle - 1.0f, -COVEN_APC_MAX_TURN_ACCEL);
		else
			m_flTurningAngle = -1.0f;
		m_angTurningAngle.y += m_flTurningAngle;
	}
	else
		m_flTurningAngle = 0.0f;
}

//flMagnitude is positive
void CCoven_APC::TurnRight(float flMagnitude)
{
	if (m_angTurningAngle.y < COVEN_APC_MAX_TURNING_VALUE && m_angTurningAngle.y < flMagnitude)
	{
		if (m_angTurningAngle.y > 1.0f || m_angTurningAngle.y < 0.0f)
			m_flTurningAngle = min(m_flTurningAngle + 1.0f, COVEN_APC_MAX_TURN_ACCEL);
		else
			m_flTurningAngle = 1.0f;
		m_angTurningAngle.y += m_flTurningAngle;
	}
	else
		m_flTurningAngle = 0.0f;
}

void CCoven_APC::MoveForward(void)
{
	WheelLocation_t lowestWheel = FRONT_PASSENGER;
	WheelLocation_t highestWheel = FRONT_PASSENGER;
	Vector vecFloorPosition[4];
	Vector vecDesiredPosition[5];
	QAngle angDesiredAngles[5];

	if (m_angTurningAngle.y == 0.0f)
	{
		Vector forward;
		AngleVectors(GetAbsAngles(), &forward);
		for (int i = 0; i < 4; i++)
		{
			vecDesiredPosition[i] = m_pWheels[i]->GetAbsOrigin() + forward * m_flSpeed;
			float roll = GetAbsAngles().z;
			angDesiredAngles[i] = m_pWheels[i]->GetAbsAngles();
			angDesiredAngles[i].x = 0;
			angDesiredAngles[i] += QAngle(roll * CCoven_APC::wheelDirection[i], 0, WheelDegreesTraveled(m_flSpeed) * CCoven_APC::wheelDirection[i]);
		}
		angDesiredAngles[4] = GetAbsAngles();
		vecDesiredPosition[4] = GetAbsOrigin() + forward * m_flSpeed;
	}
	else
	{
		float alpha = m_angTurningAngle.y;

		//Right turn
		WheelLocation_t steeringWheel = FRONT_PASSENGER;
		float direction = 1.0f;

		//Left turn
		if (alpha < 0.0f)
		{
			direction = -1.0f;
			steeringWheel = FRONT_DRIVER;
		}

		//float r = CCoven_APC::m_flWheelBase / tan(M_PI * alpha / 180.0f); //rear wheel radius
		float R = CCoven_APC::m_flWheelBase / sin(M_PI * alpha / 180.0f); //front wheel radius
		float beata = -m_flSpeed / R; //radians

		QAngle angleToCenter = m_pWheels[steeringWheel]->GetAbsAngles();
		angleToCenter.x = angleToCenter.z = 0;
		Vector dirToCenter(0, 0, 0);
		AngleVectors(angleToCenter, &dirToCenter);
		Vector O = m_pWheels[steeringWheel]->GetAbsOrigin() - CCoven_APC::wheelDirection[steeringWheel] * R * dirToCenter;
		//Msg("%f %f %f %f\n", beata, R, (m_pWheels[0]->GetAbsOrigin() - m_pWheels[2]->GetAbsOrigin()).Length2D(), m_pWheels[0]->GetAbsAngles().y - m_pWheels[2]->GetAbsAngles().y);

		for (int i = 0; i < 4; i++)
		{
			vecDesiredPosition[i] = m_pWheels[i]->GetAbsOrigin();
			VectorRotate2DPoint(m_pWheels[i]->GetAbsOrigin(), O, beata, &vecDesiredPosition[i], true);

			QAngle oldAng = m_pWheels[i]->GetAbsAngles();
			Vector newDir;
			newDir = direction * CCoven_APC::wheelDirection[i] * (vecDesiredPosition[i] - O);
			angDesiredAngles[i].Init();
			VectorAngles(newDir, angDesiredAngles[i]);
			float roll = GetAbsAngles().z;
			//float pitch = GetAbsAngles().x;
			angDesiredAngles[i].z = oldAng.z;
			if (i == steeringWheel)
				angDesiredAngles[i].y -= m_flTurningAngle;
			angDesiredAngles[i] += QAngle(roll * CCoven_APC::wheelDirection[i], 0, WheelDegreesTraveled(m_flSpeed) * CCoven_APC::wheelDirection[i]);
			
		}

		if (bot_debug_visual.GetInt() > 0)
			NDebugOverlay::Cross3D(O, -Vector(12, 12, 12), Vector(12, 12, 12), 0, 0, 255, true, 1.0f);

		Vector dir(0, 0, 0), newDir(0, 0, 0);
		VectorRotate2DPoint(m_pBody->GetAbsOrigin(), O, beata, &vecDesiredPosition[4], true);
		AngleVectors(GetAbsAngles(), &dir);
		VectorRotate2D(dir, beata, &newDir, true);
		VectorAngles(newDir, angDesiredAngles[4]);
		angDesiredAngles[4].x = AngleNormalize(angDesiredAngles[4].x);
		angDesiredAngles[4].y = AngleNormalize(angDesiredAngles[4].y);
		//Msg("DEBUG: %f %f %f\n", dir.z, newDir.z, beata);
	}

	for (int i = 0; i < 4; i++)
	{
		trace_t tr;
		//BB: TODO: we need to trace a hull here!
		UTIL_TraceLine(vecDesiredPosition[i], vecDesiredPosition[i] + Vector(0, 0, -500), MASK_ALL, m_pWheels[i]->GetProp(), COLLISION_GROUP_WEAPON, &tr);
		if (tr.DidHit())
			vecFloorPosition[i] = tr.endpos;
		else
			vecFloorPosition[i].Init();

		if (vecFloorPosition[i].z < vecFloorPosition[lowestWheel].z)
			lowestWheel = (WheelLocation_t)i;
		if (vecFloorPosition[i].z > vecFloorPosition[highestWheel].z)
			highestWheel = (WheelLocation_t)i;
	}

	if (lowestWheel != highestWheel)
	{
		//Pitch = highest point of highest direction - lowest point of other direction
		float highest = vecDesiredPosition[highestWheel].z;
		float lowestP = vecDesiredPosition[CCoven_APC::AdjacentWheel(highestWheel)].z;

		float pitchRadians = atan((highest - lowestP) / CCoven_APC::m_flWheelBase);

		if (highestWheel == FRONT_DRIVER || highestWheel == FRONT_PASSENGER)
			pitchRadians = -pitchRadians;

		//Roll = highest point of highest side - lowest point of other side.
		float lowestR = vecDesiredPosition[CCoven_APC::OppositeWheel(highestWheel)].z;

		float rollRadians = atan((highest - lowestR) / CCoven_APC::m_flWheelTrack);

		if (highestWheel >= FRONT_DRIVER)
			rollRadians = -rollRadians;

		angDesiredAngles[4].x = AngleNormalize(pitchRadians * 180.0f / M_PI);
		angDesiredAngles[4].z = AngleNormalize(rollRadians * 180.0f / M_PI);

		WheelLocation_t oppositeWheel = CCoven_APC::AdjacentWheel(CCoven_APC::OppositeWheel(highestWheel));
		vecDesiredPosition[oppositeWheel].z = vecDesiredPosition[highestWheel].z - lowestP - lowestR;

		//NDebugOverlay::Cross3D(vecDesiredPosition[highestWheel], -Vector(12, 12, 12), Vector(12, 12, 12), 255, 0, 0, false, 0.5f);
		//NDebugOverlay::Cross3D(vecDesiredPosition[lowestWheel], -Vector(12, 12, 12), Vector(12, 12, 12), 0, 255, 0, false, 0.5f);
	}

	//Msg("APC Pitch: %f\n", angDesiredAngles[4].x);

	for (int i = 0; i < 4; i++)
	{
		vecDesiredPosition[i].z = vecFloorPosition[i].z + m_pWheels[i]->RestHeight();
		m_pWheels[i]->MoveTo(&vecDesiredPosition[i], &angDesiredAngles[i]);
	}

	float midZ = vecDesiredPosition[lowestWheel].z + (vecDesiredPosition[highestWheel].z - vecDesiredPosition[lowestWheel].z) / 2.0f + CCoven_APC::m_flWheelRadius;
	vecDesiredPosition[4].z = midZ;

	SetAbsAngles(angDesiredAngles[4]);
	SetAbsOrigin(vecDesiredPosition[4]);

	float roll = -angDesiredAngles[4].z;
	angDesiredAngles[4].z = -angDesiredAngles[4].x + random->RandomFloat(-0.5f, 0.5f);
	angDesiredAngles[4].x = roll + random->RandomFloat(-0.5f, 0.5f);
	angDesiredAngles[4] += m_pBody->GetOffsetAngle();
	m_pBody->MoveTo(&vecDesiredPosition[4], &angDesiredAngles[4]);

	//Msg("Body Pitch: %f\n", angDesiredAngles[4].x);

	if (m_bRunning)
		UpdateLightPosition();
}

bool CCoven_APC::IsRunning(void)
{
	return m_bRunning || m_flSpeed > 0.0f;
}

void CCoven_APC::ResetLocation(void)
{
	for (int i = 0; i < 4; i++)
	{
		QAngle ang = GetAbsAngles();
		float roll = ang.z;
		//float pitch = ang.x;
		ang.x = ang.z = 0;
		if (i == FRONT_DRIVER || i == FRONT_PASSENGER)
			ang += m_angTurningAngle;
		ang += m_pWheels[i]->GetOffsetAngle() + QAngle(roll * CCoven_APC::wheelDirection[i], 0, m_pWheels[i]->GetAbsAngles().z + WheelDegreesTraveled(m_flSpeed) * CCoven_APC::wheelDirection[i]);
		Vector vec = GetAbsOrigin() + CCoven_APC::wheelOffset[i];
		Vector rotated = vec;
		VectorRotate2DPoint(vec, GetAbsOrigin(), GetAbsAngles().y, &rotated);
		m_pWheels[i]->MoveTo(&rotated, &ang);
	}

	Vector bodyLocation = GetAbsOrigin();
	QAngle bodyAngles = GetAbsAngles() + QAngle(0, -90, 0);
	m_pBody->MoveTo(&bodyLocation, &bodyAngles);
}

void CCoven_APC::StartUp(void)
{
	if (!m_bRunning)
	{
		if (!m_bPlayingSound)
		{
			m_bPlayingSound = true;
			EmitSound("ATV_engine_start");
			m_flSoundSwitch = gpGlobals->curtime + GetSoundDuration("ATV_engine_Start", NULL);
			SetThink(&CCoven_APC::APCThink);
			SetNextThink(gpGlobals->curtime + 0.2f);
		}
		m_bRunning = true;
		ToggleLights(true);
	}
	m_iFuel += m_iFuelUpAmount;
}

int CCoven_APC::CurrentGoal(void)
{
	return m_iGoalNode;
}

int CCoven_APC::CurrentCheckpoint(void)
{
	return m_iGoalCheckpoint;
}

WheelLocation_t CCoven_APC::OppositeWheel(WheelLocation_t wheel)
{
	if (wheel > REAR_PASSENGER)
		return (WheelLocation_t)(wheel - 2);
	return (WheelLocation_t)(wheel + 2);
}

WheelLocation_t CCoven_APC::AdjacentWheel(WheelLocation_t wheel)
{
	if (wheel == FRONT_PASSENGER)
		return REAR_PASSENGER;
	if (wheel == REAR_PASSENGER)
		return FRONT_PASSENGER;
	if (wheel == FRONT_DRIVER)
		return REAR_DRIVER;
	return FRONT_DRIVER;
}

LINK_ENTITY_TO_CLASS(coven_apc_part, CCoven_APCPart);

CCoven_APCPart::CCoven_APCPart()
{
	m_pConstraint = NULL;
	m_hAPC = NULL;
	m_vecOffset.Init();
	m_angOffset.Init();
	m_flRestHeight = 0.0f;
}

CCoven_APCPart::~CCoven_APCPart()
{
	if (m_pConstraint)
	{
		physenv->DestroyConstraint(m_pConstraint);
		m_pConstraint = NULL;
	}
}

CCoven_APCPart *CCoven_APCPart::CreateAPCPart(CCoven_APC *pParentAPC, char *szModelName, const Vector &vecOffset, const QAngle &angOffset, float flRestHeight, Collision_Group_t collisionGroup, bool bMakeUsable)
{
	CCoven_APCPart *pPart = (CCoven_APCPart *)CBaseEntity::Create("coven_apc_part", pParentAPC->GetAbsOrigin(), pParentAPC->GetAbsAngles());
	if (!pPart)
		return NULL;

	pPart->m_hAPC = pParentAPC;
	pPart->m_vecOffset = vecOffset;
	pPart->m_angOffset = angOffset;
	pPart->m_flRestHeight = flRestHeight;

	pPart->m_pPart = dynamic_cast< CCoven_APCProp * >(CreateEntityByName("coven_prop_physics"));
	if (pPart->m_pPart)
	{
		Vector location = pParentAPC->GetAbsOrigin();
		QAngle orientation = pParentAPC->GetAbsAngles();
		char buf[512];
		// Pass in standard key values
		Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", location.x, location.y, location.z);
		pPart->m_pPart->KeyValue("origin", buf);
		Q_snprintf(buf, sizeof(buf), "%.10f %.10f %.10f", orientation.x, orientation.y, orientation.z);
		pPart->m_pPart->KeyValue("angles", buf);
		pPart->m_pPart->KeyValue("model", szModelName);
		pPart->m_pPart->KeyValue("fademindist", "-1");
		pPart->m_pPart->KeyValue("fademaxdist", "0");
		pPart->m_pPart->KeyValue("fadescale", "1");
		pPart->m_pPart->KeyValue("inertiaScale", "1.0");
		pPart->m_pPart->KeyValue("physdamagescale", "0.1");
		pPart->m_pPart->Precache();
		pPart->m_pPart->SetCollisionGroup(collisionGroup);
		pPart->m_pPart->Spawn();
		pPart->m_pPart->Activate();
		pPart->m_pPart->AddSolidFlags(FSOLID_NOT_SOLID);
	}

	if (bMakeUsable)
		pPart->m_pPart->m_pParent = pPart;

	pPart->AddSolidFlags(FSOLID_NOT_SOLID);

	// Disable movement on the root, we'll move this thing manually.
	pPart->VPhysicsInitShadow(false, false);
	pPart->SetMoveType(MOVETYPE_NONE);

	return pPart;
}

void CCoven_APCPart::MakeSolid(void)
{
	m_pPart->RemoveSolidFlags(FSOLID_NOT_SOLID);
}

Vector CCoven_APCPart::GetOffsetPosition(void)
{
	Vector offset = m_vecOffset;
	if (m_vecOffset != vec3_origin)
	{
		float sine = 0.0f;
		float cosine = 0.0f;
		SinCos(m_hAPC->GetAbsAngles().y / 180.0f * M_PI, &sine, &cosine);
		offset.x = cosine * m_vecOffset.x - sine * m_vecOffset.y;
		offset.y = sine * m_vecOffset.x + cosine * m_vecOffset.y;
	}
	return offset;
}

QAngle CCoven_APCPart::GetOffsetAngle(void)
{
	return m_angOffset;
}

float CCoven_APCPart::RestHeight(void)
{
	return m_flRestHeight;
}

void CCoven_APCPart::MoveTo(const Vector *vec, const QAngle *ang)
{
	if (vec != NULL)
		SetAbsOrigin(*vec);
	if (ang != NULL)
		SetAbsAngles(*ang);
	if (vec != NULL || ang != NULL)
		UpdatePhysicsShadowToCurrentPosition(0.1f);
}

void CCoven_APCPart::UpdatePosition(void)
{
	if (m_hAPC != NULL)
	{
		SetAbsOrigin(m_hAPC->GetAbsOrigin() + GetOffsetPosition());
		UpdatePhysicsShadowToCurrentPosition(0.1f);
	}
}

void CCoven_APCPart::Spawn(void)
{
	Precache();
	SetModel("models/props_junk/rock001a.mdl");
	AddEffects(EF_NODRAW);

	SetSolid(SOLID_VPHYSICS);
	AddSolidFlags(FSOLID_NOT_SOLID);

	BaseClass::Spawn();
}

void CCoven_APCPart::Precache(void)
{
	PrecacheModel("models/props_junk/rock001a.mdl");

	BaseClass::Precache();
}

void CCoven_APCPart::DropToFloor(void)
{
	trace_t tr;
	UTIL_TraceLine(GetAbsOrigin(), GetAbsOrigin() + Vector(0, 0, -500), MASK_ALL, m_pPart, COLLISION_GROUP_PLAYER, &tr);
	if (tr.DidHit())
	{
		SetAbsOrigin(tr.endpos + Vector(0, 0, m_flRestHeight));
	}
}

void CCoven_APCPart::MoveToCorrectLocation(void)
{
	if (m_hAPC != NULL)
	{
		Vector location = m_hAPC->GetAbsOrigin() + GetOffsetPosition();
		QAngle angles = m_hAPC->GetAbsAngles() + m_angOffset;
		Teleport(&location, &angles, NULL);
		UpdatePhysicsShadowToCurrentPosition(0);
	}
}

void CCoven_APCPart::AttachPhysics(void)
{
	if (!m_pConstraint)
	{
		IPhysicsObject *pPartPhys = m_pPart->VPhysicsGetObject();
		IPhysicsObject *pShadowPhys = VPhysicsGetObject();

		constraint_fixedparams_t fixed;
		fixed.Defaults();
		fixed.InitWithCurrentObjectState(pShadowPhys, pPartPhys);
		fixed.constraint.Defaults();

		m_pConstraint = physenv->CreateFixedConstraint(pShadowPhys, pPartPhys, NULL, fixed);
	}
}

void CCoven_APCPart::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (pActivator->IsPlayer() && m_hAPC != NULL)
	{
		CHL2MP_Player *pHL2Player = ToHL2MPPlayer(pActivator);
		if (pHL2Player->HasStatus(COVEN_STATUS_HAS_GAS))
		{
			pHL2Player->RemoveStatus(COVEN_STATUS_HAS_GAS);
			UTIL_Remove((CBaseEntity *)pHL2Player->hCarriedItem.Get());
			m_hAPC->StartUp();
		}
		else
			pHL2Player->EmitSound("HL2Player.UseDeny");
	}
}