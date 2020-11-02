#include "cbase.h"

#include "covenbuilding.h"
#include "player_pickup.h"
#include "physics_saverestore.h"
#include "phys_controller.h"
#include "IEffects.h"
#include "props.h"
#include "beam_flags.h"
#include "te_effect_dispatch.h"
#include "coven_parse.h"

extern short	g_sModelIndexFireball;		// (in combatweapon.cpp) holds the index for the fireball 
extern short	g_sModelIndexWExplosion;	// (in combatweapon.cpp) holds the index for the underwater explosion
extern short	g_sModelIndexSmoke;			// (in combatweapon.cpp) holds the index for the smoke cloud

LINK_ENTITY_TO_CLASS(coven_building, CCovenBuilding);

IMPLEMENT_SERVERCLASS_ST(CCovenBuilding, DT_CovenBuilding)
	SendPropInt(SENDINFO(m_BuildingType), 4, SPROP_UNSIGNED),
	SendPropInt(SENDINFO(m_iHealth), 9, SPROP_UNSIGNED | SPROP_CHANGES_OFTEN),
	SendPropInt(SENDINFO(m_iMaxHealth), 9, SPROP_UNSIGNED),
	SendPropEHandle(SENDINFO(mOwner)),
	SendPropInt(SENDINFO(m_iLevel), 3, SPROP_UNSIGNED),
END_SEND_TABLE()

BEGIN_DATADESC(CCovenBuilding)

DEFINE_OUTPUT(m_OnUsed, "OnUsed"),

DEFINE_THINKFUNC(BuildingThink),
DEFINE_OUTPUT(m_OnPhysGunPickup, "OnPhysGunPickup"),
DEFINE_OUTPUT(m_OnPhysGunDrop, "OnPhysGunDrop"),

DEFINE_INPUTFUNC(FIELD_VOID, "Kill", InputKill),

END_DATADESC()

ConVar sv_coven_building_hp_per_energy("sv_coven_building_hp_per_energy", "4", FCVAR_GAMEDLL | FCVAR_NOTIFY, "HP per energy per swing.");
ConVar sv_coven_building_max_energy_swing("sv_coven_building_max_energy_swing", "25", FCVAR_GAMEDLL | FCVAR_NOTIFY, "Max energy per category per swing.");
ConVar sv_coven_building_strength("sv_coven_building_strength", "34.0", FCVAR_GAMEDLL | FCVAR_NOTIFY, "Coven building strength for damage calcs.");

CCovenBuilding::CCovenBuilding()
{
	mOwner = NULL;
	m_pMotionController = NULL;
	m_bEnabled = m_bSelfDestructing = false;
	m_flTop = m_flBottom = m_flDestructTime = m_flPingTime = 0.0f;
	m_BuildingType = BUILDING_DEFAULT;
	m_flSparkTimer = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCovenBuilding::Spawn(void)
{
	Precache();

	BaseClass::Spawn();

	SetModel(STRING(GetModelName()));
	SetMoveType(MOVETYPE_NONE);
	SetSolid(SOLID_VPHYSICS);
	CreateVPhysics();

	ResetSequence(LookupSequence("Idle"));
	m_flAnimTime = gpGlobals->curtime;
	m_flPlaybackRate = 0.0;
	SetCycle(0);

	m_iXP = 0;
	m_iLevel = 0;
	m_bCarriedByPlayer = false;
	m_bUseCarryAngles = false;

	m_flPlayerDropTime = gpGlobals->curtime + 4.0f;

	CovenBuildingInfo_t *info = GetCovenBuildingData(m_BuildingType);

	m_iHealth = info->iHealths[0];
	m_iMaxHealth = info->iHealths[0];
	m_iMaxXP = info->iXPs[0];

	m_takedamage = DAMAGE_YES;
	//m_takedamage = DAMAGE_EVENTS_ONLY;

	if (mOwner.Get() != NULL)
		ChangeTeam(mOwner.Get()->GetTeamNumber());
	else
		ChangeTeam(COVEN_TEAMID_SLAYERS);

	if (HasSpawnFlags(SF_COVENBUILDING_INERT))
		m_flBuildTime = 0.0f;
	else
	{
		m_flBuildTime = gpGlobals->curtime + BuildTime();
		m_floatCloakFactor = 0.9f;

		if (m_pMotionController == NULL)
		{
			// Create the motion controller
			m_pMotionController = CCovenBuildingTipController::CreateTipController(this);

			// Enable the controller
			if (m_pMotionController != NULL)
			{
				m_pMotionController->Enable();
				m_pMotionController->Activate();
			}
		}
	}

	m_bGoneToSleep = false;

	ComputeExtents();
	
	SetThink(&CCovenBuilding::BuildingThink);

	SetNextThink(gpGlobals->curtime + 0.1f);

	SetCollisionGroup(COLLISION_GROUP_PLAYER);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCovenBuilding::Precache(void)
{
	const char *pModelName = STRING(GetModelName());
	PrecacheModel(pModelName);
	PropBreakablePrecacheAll(MAKE_STRING(pModelName));

	PrecacheScriptSound("DoSpark");
	PrecacheScriptSound("NPC_FloorTurret.Ping");
	PrecacheScriptSound("CovenBuildingLevel");
	s_nExplosionTexture = PrecacheModel("sprites/lgtning.vmt");
	//PrecacheScriptSound("Weapon_StunStick.Melee_Hit");
}

void CCovenBuilding::ComputeExtents(float scaleFactor)
{
	IPhysicsObject *pPhys = VPhysicsGetObject();
	if (pPhys)
	{
		m_flTop = scaleFactor * ((physcollision->CollideGetExtent(pPhys->GetCollide(), GetLocalOrigin(), GetLocalAngles(), Vector(0, 0, 1)) - GetLocalOrigin()).z);
		m_flBottom = scaleFactor * ((physcollision->CollideGetExtent(pPhys->GetCollide(), GetLocalOrigin(), GetLocalAngles(), Vector(0, 0, -1)) - GetLocalOrigin()).z);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &data - 
//-----------------------------------------------------------------------------
void CCovenBuilding::InputKill(inputdata_t &data)
{
	UTIL_Remove(this);
}

bool CCovenBuilding::CheckLevel(void)
{
	CovenBuildingInfo_t *info = GetCovenBuildingData(m_BuildingType);
	bool retVal = false;
	if (m_iXP >= m_iMaxXP)
	{
		m_iLevel++;
		m_iMaxHealth = info->iHealths[m_iLevel];
		m_iHealth = m_iMaxHealth;
		if (m_iLevel < (info->iMaxLevel - 1))
		{
			m_iXP -= m_iMaxXP;
			m_iMaxXP = info->iXPs[m_iLevel];
		}
		retVal = true;
	}

	if (retVal)
	{
#if !defined( TF_DLL )
		// Since this code only runs on the server, make sure it shows the tempents it creates.
		// This solves a problem with remote detonating the pipebombs (client wasn't seeing the explosion effect)
		CDisablePredictionFiltering disabler;
#endif

		//UTIL_ScreenShake(GetAbsOrigin(), 20.0f, 150.0, 1.0, 1250.0f, SHAKE_START);

		CEffectData data;

		data.m_vOrigin = EyePosition();

		DispatchEffect("cball_explode", data);

		CBroadcastRecipientFilter filter2;

		te->BeamRingPoint(filter2, 0, EyePosition(),	//origin
			50,	//start radius
			1024,		//end radius
			s_nExplosionTexture, //texture
			0,			//halo index
			0,			//start frame
			2,			//framerate
			0.2f,		//life
			64,			//width
			0,			//spread
			0,			//amplitude
			255,	//r
			255,	//g
			225,	//b
			32,		//a
			0,		//speed
			FBEAM_FADEOUT
			);

		//Shockring
		te->BeamRingPoint(filter2, 0, EyePosition(),	//origin
			50,	//start radius
			1024,		//end radius
			s_nExplosionTexture, //texture
			0,			//halo index
			0,			//start frame
			2,			//framerate
			0.5f,		//life
			64,			//width
			0,			//spread
			0,			//amplitude
			255,	//r
			255,	//g
			225,	//b
			64,		//a
			0,		//speed
			FBEAM_FADEOUT
			);

		EmitSound("CovenBuildingLevel");
	}

	return retVal;
}

void CCovenBuilding::NotifyOwner(void)
{
	if (mOwner.Get() != NULL)
	{
		CHL2_Player *pOwner = ToHL2Player(mOwner.Get());
		pOwner->UpdateBuilding(this);
	}
}

bool CCovenBuilding::PreThink(void)
{
	if (!m_bGoneToSleep)
	{
		IPhysicsObject *pObj = VPhysicsGetObject();
		if (pObj && pObj->IsAsleep())
		{
			m_bGoneToSleep = true;
			pObj->EnableMotion(false);
		}
	}

	StudioFrameAdvance();
	DispatchAnimEvents(this);
	NotifyOwner();

	if (m_iHealth <= 0.5f * m_iMaxHealth && gpGlobals->curtime > m_flSparkTimer)
	{
		float heightZ = m_flBottom + (m_flTop - m_flBottom) * 0.5f;
		Spark(random->RandomFloat(heightZ, m_flTop), 2, 1);
		m_flSparkTimer = gpGlobals->curtime + 0.1f * random->RandomInt(4, 10);
	}

	if (m_bSelfDestructing)
	{
		if (gpGlobals->curtime > m_flDestructTime)
		{
			SetThink(&CCovenBuilding::Detonate);
			SetNextThink(gpGlobals->curtime + 0.1f);
			return true;
		}
	}

	return false;
}

void CCovenBuilding::BuildingThink(void)
{
	if (PreThink())
		return;

	if (IsDoneBuilding())
		OnBuildingComplete();
	else
	{
		Build();
		SetNextThink(gpGlobals->curtime + 0.1f);
	}
}

void CCovenBuilding::OnBuildingComplete(void)
{
	m_floatCloakFactor = 0.0f;
	Enable();
}

void CCovenBuilding::Build(void)
{
	float heightZ = m_flBottom + (m_flTop - m_flBottom) * (1.0f - max(min((m_flBuildTime - gpGlobals->curtime) / BuildTime(), 1.0f), 0.0f)); //RandomFloat(bottom.z, top.z);
	Spark(heightZ, 1, 1);
	m_floatCloakFactor = max(min((m_flBuildTime - gpGlobals->curtime) / BuildTime(), 1.0f), 0.0f);
}

void CCovenBuilding::UpdateOnRemove(void)
{
	if (m_pMotionController != NULL)
	{
		UTIL_Remove(m_pMotionController);
		m_pMotionController = NULL;
	}

	BaseClass::UpdateOnRemove();
}

void CCovenBuilding::DoSelfDestructEffects(float percentage)
{
	// Figure out what our beep pitch will be
	//float flBeepPitch = COVEN_SELF_DESTRUCT_BEEP_MIN_PITCH + ((COVEN_SELF_DESTRUCT_BEEP_MAX_PITCH - COVEN_SELF_DESTRUCT_BEEP_MIN_PITCH) * percentage);

	//StopSound("NPC_FloorTurret.Ping");
	//EmitSound("NPC_FloorTurret.Ping");

	// Play the beep
	CPASAttenuationFilter filter(this, "NPC_FloorTurret.Ping");
	EmitSound_t params;
	params.m_pSoundName = "NPC_FloorTurret.Ping";
	//params.m_nPitch = floor(flBeepPitch);
	//params.m_nFlags = SND_CHANGE_PITCH;
	params.m_flVolume = 1.0f;
	params.m_SoundLevel = SNDLVL_180dB;
	EmitSound(filter, entindex(), params);
}

void CCovenBuilding::SelfDestructThink(void)
{
	if (PreThink())
		return;

	// Find out where we are in the cycle of our destruction
	float flDestructPerc = clamp(1 - ((m_flDestructTime - gpGlobals->curtime) / SelfDestructTime()), 0.0f, 1.0f);

	// Figure out when our next beep should occur
	float flBeepTime = COVEN_SELF_DESTRUCT_BEEP_MAX_DELAY + ((COVEN_SELF_DESTRUCT_BEEP_MIN_DELAY - COVEN_SELF_DESTRUCT_BEEP_MAX_DELAY) * flDestructPerc);

	// If it's time to beep again, do so
	if (gpGlobals->curtime > (m_flPingTime + flBeepTime))
	{
		DoSelfDestructEffects(flDestructPerc);
		// Save this as the last time we pinged
		m_flPingTime = gpGlobals->curtime;
		
	}
	float heightZ = m_flBottom + (m_flTop - m_flBottom) * 0.5f;

	//BB: TODO: this isn't quite right for tipped turrets...
	Spark(random->RandomFloat(heightZ, m_flTop), 2, 3);

	// If we're done, explode
	if (gpGlobals->curtime > m_flDestructTime)
	{
		SetThink(&CCovenBuilding::Detonate);
		SetNextThink(gpGlobals->curtime + 0.1f);
		return;
	}

	// Think again!
	SetNextThink(gpGlobals->curtime + 0.05f);
}

void CCovenBuilding::SelfDestruct(void)
{
	m_iHealth = 0;
	m_lifeState = LIFE_DYING;
	NotifyOwner();
	if (mOwner.Get() != NULL)
	{
		CHL2_Player *pOwner = ToHL2Player(mOwner.Get());
		if (MyType() == BUILDING_TURRET)
			pOwner->m_hTurret = NULL;
		else if (MyType() == BUILDING_AMMOCRATE)
			pOwner->m_hDispenser = NULL;
	}

	m_flPingTime = gpGlobals->curtime;

	if (!IsDoneBuilding())
		m_floatCloakFactor = 0.0f;

	m_flDestructTime = gpGlobals->curtime + SelfDestructTime();
	m_bSelfDestructing = true;
	SetThink(&CCovenBuilding::SelfDestructThink);
	SetNextThink(gpGlobals->curtime + 0.1f);
}

void CCovenBuilding::Detonate(void)
{
	Vector vecUp;
	GetVectors(NULL, NULL, &vecUp);
	Vector vecOrigin = WorldSpaceCenter() + (vecUp * 12.0f);
	float m_DmgRadius = 51.2f;
	trace_t		pTrace;
	Vector		vecSpot;// trace starts here!

	vecSpot = GetAbsOrigin() + Vector(0, 0, 8);
	UTIL_TraceLine(vecSpot, vecSpot + Vector(0, 0, -32), MASK_SHOT_HULL, this, COLLISION_GROUP_NONE, &pTrace);

	Vector vecAbsOrigin = GetAbsOrigin();
	int contents = UTIL_PointContents(vecAbsOrigin);

#if !defined( TF_DLL )
	// Since this code only runs on the server, make sure it shows the tempents it creates.
	// This solves a problem with remote detonating the pipebombs (client wasn't seeing the explosion effect)
	CDisablePredictionFiltering disabler;
#endif

	if (pTrace.fraction != 1.0)
	{
		Vector vecNormal = pTrace.plane.normal;
		surfacedata_t *pdata = physprops->GetSurfaceData(pTrace.surface.surfaceProps);
		CPASFilter filter(vecAbsOrigin);

		te->Explosion(filter, -1.0, // don't apply cl_interp delay
			&vecAbsOrigin,
			!(contents & MASK_WATER) ? g_sModelIndexFireball : g_sModelIndexWExplosion,
			m_DmgRadius,// * .03, 
			12,
			TE_EXPLFLAG_NONE,
			m_DmgRadius,
			0.0,
			&vecNormal,
			(char)pdata->game.material);
	}
	else
	{
		CPASFilter filter(vecAbsOrigin);
		te->Explosion(filter, -1.0, // don't apply cl_interp delay
			&vecAbsOrigin,
			!(contents & MASK_WATER) ? g_sModelIndexFireball : g_sModelIndexWExplosion,
			m_DmgRadius,// * .03, 
			12,
			TE_EXPLFLAG_NONE,
			m_DmgRadius,
			0.0);
	}

	// Use the thrower's position as the reported position
	Vector vecReported = GetAbsOrigin();

	int bits = DMG_BLAST | DMG_NO;
	float damn = 40.0f;
	CBaseEntity *ignore;
	ignore = this;

	CTakeDamageInfo info(this, this, vec3_origin, GetAbsOrigin(), damn, bits, 0, &vecReported);

	RadiusDamage(info, GetAbsOrigin(), 300.0f, CLASS_NONE, ignore);

	UTIL_DecalTrace(&pTrace, "Scorch");

	breakablepropparams_t params(GetAbsOrigin(), GetAbsAngles(), vec3_origin, RandomAngularImpulse(-800.0f, 800.0f));
	params.impactEnergyScale = 1.0f;
	params.defCollisionGroup = COLLISION_GROUP_INTERACTIVE;

	// no damage/damage force? set a burst of 100 for some movement
	params.defBurstScale = 100;
	PropBreakableCreateAll(GetModelIndex(), VPhysicsGetObject(), params, this, -1, true);

	// Throw out some small chunks too obscure the explosion even more
	CPVSFilter filter(vecOrigin);
	for (int i = 0; i < 6; i++)
	{
		Vector gibVelocity = RandomVector(-100, 100);
		int iModelIndex = modelinfo->GetModelIndex(g_PropDataSystem.GetRandomChunkModel("MetalChunks"));
		te->BreakModel(filter, 0.0, vecOrigin, GetAbsAngles(), Vector(40, 40, 40), gibVelocity, iModelIndex, 150, 4, 2.5, BREAK_METAL);
	}

	// We're done!
	//UTIL_Remove(this);
	SetThink(&CCovenBuilding::SUB_Remove);
	SetSolid(SOLID_NONE);
	AddEffects(EF_NODRAW);
	SetAbsVelocity(vec3_origin);

	SetNextThink(gpGlobals->curtime + 0.1f);
}

void CCovenBuilding::Spark(float height, int mag, int length)
{
	IPhysicsObject *pPhys = VPhysicsGetObject();

	Vector forward;
	QAngle angle;
	angle.y = RandomFloat(-180.0f, 180.0f);
	AngleVectors(angle, &forward);
	
	
	Vector start = GetLocalOrigin();
	start.z += height;
	Vector point = physcollision->CollideGetExtent(pPhys->GetCollide(), start, GetLocalAngles(), forward);
	float dist = (point - start).Length2D();

	trace_t trace;
	UTIL_TraceLine(start + forward * dist, start, MASK_SHOT, NULL, COLLISION_GROUP_NONE, &trace);
	if (trace.fraction != 1.0f)
	{
		//forward = -forward;
		EmitSound("DoSpark");
		g_pEffects->Sparks(trace.endpos, mag, length, &forward);
		g_pEffects->MetalSparks(trace.endpos, forward);
	}
	//g_pEffects->Sparks(point);
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
bool CCovenBuilding::CreateVPhysics(void)
{
	if (HasSpawnFlags(SF_COVENBUILDING_INERT))
		return (VPhysicsInitStatic() != NULL);

	return (VPhysicsInitNormal(SOLID_VPHYSICS, 0, false) != NULL);
}

bool CCovenBuilding::WasJustDroppedByPlayer(void)
{
	if (m_flPlayerDropTime > gpGlobals->curtime)
		return true;

	return false;
}

void CCovenBuilding::Activate(void)
{
	if (m_pMotionController != NULL)
		m_pMotionController->Activate();

	BaseClass::Activate();
}

void CCovenBuilding::WakeUp(void)
{
	m_bGoneToSleep = false;
	IPhysicsObject *pObj = VPhysicsGetObject();
	if (pObj)
		pObj->EnableMotion(true);
}

void CCovenBuilding::VPhysicsCollision(int index, gamevcollisionevent_t *pEvent)
{
	int otherIndex = !index;
	CBaseEntity *pHitEntity = pEvent->pEntities[otherIndex];
	if (pHitEntity && pHitEntity->IsPlayer())
	{
		CHL2_Player *pPlayer = ToHL2Player(pHitEntity);
		if (pPlayer && (pPlayer->GetTeamNumber() != GetTeamNumber() || mOwner.Get() == pPlayer))
			WakeUp();
	}

	BaseClass::VPhysicsCollision(index, pEvent);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pActivator - 
//			*pCaller - 
//			useType - 
//			value - 
//-----------------------------------------------------------------------------
void CCovenBuilding::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if (!IsDoneBuilding())
		return;

	if (m_bGoneToSleep)
		WakeUp();

	CBasePlayer *pPlayer = ToBasePlayer(pActivator);

	if (pPlayer == NULL)
		return;

	if (mOwner.Get() != NULL && mOwner.Get() == pPlayer && !m_bSelfDestructing)
	{
		m_OnUsed.FireOutput(pActivator, this);
		SetCollisionGroup(COLLISION_GROUP_DEBRIS_TRIGGER);
		//BB: TODO: DISABLE ON USE!!!
		PlayerPickupObject(pPlayer, this);
	}
}

void CCovenBuilding::Event_Killed(const CTakeDamageInfo &info)
{
	SelfDestruct();
	BaseClass::Event_Killed(info);
}

//-----------------------------------------------------------------------------
// Purpose: allows the crate to open up when hit by a crowbar
//-----------------------------------------------------------------------------
int CCovenBuilding::OnTakeDamage(const CTakeDamageInfo &info)
{
	if (HasSpawnFlags(SF_COVENBUILDING_INERT))
		return 0;

	if (mOwner.Get() == NULL || (info.GetAttacker() != NULL && info.GetAttacker()->GetTeamNumber() == GetTeamNumber() && !(info.GetDamageType() & DMG_SHOCK)))
		return 0;

	CHL2_Player *pAttacker = ToHL2Player(info.GetAttacker());
	if (pAttacker)
	{
		if (info.GetDamageType() & DMG_SHOCK && GetTeamNumber() == pAttacker->GetTeamNumber() && !m_bSelfDestructing)
		{
			CovenBuildingInfo_t *bldgInfo = GetCovenBuildingData(m_BuildingType);
			int hp = 0;
			int xp = 0;
			hp = min(sv_coven_building_max_energy_swing.GetInt(), ceil((m_iMaxHealth - m_iHealth) / sv_coven_building_hp_per_energy.GetFloat()));
			int maxXP = m_iMaxXP + sv_coven_building_max_energy_swing.GetInt();
			if (m_iLevel >= (bldgInfo->iMaxLevel - 2))
				maxXP = m_iMaxXP;
			xp = min(sv_coven_building_max_energy_swing.GetInt(), maxXP - m_iXP);

			int total = min(hp + xp, pAttacker->SuitPower_GetCurrentPercentage());
			int drain = min(total, hp);
			if (drain > 0)
			{
				m_iHealth = min(m_iHealth + sv_coven_building_hp_per_energy.GetInt() * drain, m_iMaxHealth);
				total -= drain;
				pAttacker->SuitPower_Drain(drain);
			}
			drain = min(total, xp);
			if (drain > 0)
			{
				m_iXP += drain;
				CheckLevel();
				total -= drain;
				pAttacker->SuitPower_Drain(drain);
			}
			//Msg("HP:%d XP:%d\n", m_iHealth, m_iXP);
			
			return info.GetDamage();
		}
	}

	if (m_bGoneToSleep)
		WakeUp();

	CTakeDamageInfo newInfo = info;
	newInfo.SetDamage((1.0f - sv_coven_building_strength.GetFloat() / 60.0f) * newInfo.GetDamage());

	return BaseClass::OnTakeDamage(info);
}

void CCovenBuilding::EnableUprightController(void)
{
	if (m_pMotionController != NULL)
		m_pMotionController->Enable();
}

void CCovenBuilding::DisableUprightController(void)
{
	if (m_pMotionController != NULL)
		m_pMotionController->Enable(false);
}

void CCovenBuilding::SuspendUprightController(float duration)
{
	if (m_pMotionController != NULL)
		m_pMotionController->Suspend(duration);
}

void CCovenBuilding::SetActivity(Activity NewActivity)
{
	if (!GetModelPtr())
		return;

	m_Activity = NewActivity;

	// Resolve to ideals and apply directly, skipping transitions.
	ResolveActivityToSequence(m_Activity, m_nSequence);

	SetActivityAndSequence(m_Activity, m_nSequence);
}

void CCovenBuilding::ResolveActivityToSequence(Activity NewActivity, int &iSequence)
{
	iSequence = SelectWeightedSequence(NewActivity);

	if (iSequence == ACT_INVALID)
	{
		// Abject failure. Use sequence zero.
		iSequence = 0;
	}
}

void CCovenBuilding::SetActivityAndSequence(Activity NewActivity, int iSequence)
{
	// Set to the desired anim, or default anim if the desired is not present
	if (iSequence > ACTIVITY_NOT_AVAILABLE)
	{
		if (GetSequence() != iSequence || !SequenceLoops())
		{
			//
			// Don't reset frame between movement phased animations
			/*if (!IsActivityMovementPhased(m_Activity) ||
				!IsActivityMovementPhased(NewActivity))
			{
				SetCycle(0);
			}*/
			SetCycle(0);
		}

		ResetSequence(iSequence);
	}
	else
	{
		// Not available try to get default anim
		ResetSequence(0);
	}
}

void CCovenBuilding::LockController(void)
{
	if (m_pMotionController != NULL)
		m_pMotionController->Lock();
}

void CCovenBuilding::UnlockController(void)
{
	if (m_pMotionController != NULL)
		m_pMotionController->Unlock();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCovenBuilding::OnPhysGunPickup(CBasePlayer *pPhysGunUser, PhysGunPickup_t reason)
{
	m_hPhysicsAttacker = pPhysGunUser;
	m_flLastPhysicsInfluenceTime = gpGlobals->curtime;

	// Drop our mass a lot so that we can be moved easily with +USE
	if (reason != PUNTED_BY_CANNON)
	{
		Assert(VPhysicsGetObject());

		m_bCarriedByPlayer = true;
		m_OnPhysGunPickup.FireOutput(this, this);

		// We want to use preferred carry angles if we're not nicely upright
		Vector vecToTurret = pPhysGunUser->GetAbsOrigin() - GetAbsOrigin();
		vecToTurret.z = 0;
		VectorNormalize(vecToTurret);

		// We want to use preferred carry angles if we're not nicely upright
		Vector	forward, up;
		GetVectors(&forward, NULL, &up);

		bool bUpright = DotProduct(up, Vector(0, 0, 1)) > 0.9f;
		bool bBehind = DotProduct(vecToTurret, forward) < 0.85f;

		// Correct our angles only if we're not upright or we're mostly behind the turret
		if (hl2_episodic.GetBool())
		{
			m_bUseCarryAngles = (bUpright == false || bBehind);
		}
		else
		{
			m_bUseCarryAngles = (bUpright == false);
		}
	}

	Disable();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCovenBuilding::OnPhysGunDrop(CBasePlayer *pPhysGunUser, PhysGunDrop_t Reason)
{
	m_hPhysicsAttacker = pPhysGunUser;
	m_flLastPhysicsInfluenceTime = gpGlobals->curtime;

	m_bCarriedByPlayer = false;
	m_bUseCarryAngles = false;
	m_OnPhysGunDrop.FireOutput(this, this);

	// If this is a friendly turret, remember that it was just dropped
	m_flPlayerDropTime = gpGlobals->curtime + 2.0;

	SetCollisionGroup(COLLISION_GROUP_PLAYER);

	// Restore our mass to the original value
	Assert(VPhysicsGetObject());

	Enable();
}

//-----------------------------------------------------------------------------
// Purpose: Whether this should return carry angles
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CCovenBuilding::HasPreferredCarryAnglesForPlayer(CBasePlayer *pPlayer)
{
	return m_bUseCarryAngles;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CCovenBuilding::OnAttemptPhysGunPickup(CBasePlayer *pPhysGunUser, PhysGunPickup_t reason)
{
	// Prevent players pulling enemy turrets from afar if they're in front of the turret
	Vector vecForward;
	GetVectors(&vecForward, NULL, NULL);
	Vector vecForce = (pPhysGunUser->GetAbsOrigin() - GetAbsOrigin());
	float flDistance = VectorNormalize(vecForce);

	// If it's over the physcannon tracelength, we're pulling it
	if (flDistance > 250.0f)
	{
		float flDot = DotProduct(vecForward, vecForce);
		if (flDot > 0.5)
			return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const QAngle
//-----------------------------------------------------------------------------
QAngle CCovenBuilding::PreferredCarryAngles(void)
{
	// FIXME: Embed this into the class
	static QAngle g_prefAngles;

	Vector vecUserForward;
	CBasePlayer *pPlayer = m_hPhysicsAttacker;
	if (pPlayer)
	{
		pPlayer->EyeVectors(&vecUserForward);

		// If we're looking up, then face directly forward
		if (vecUserForward.z >= 0.0f)
			return vec3_angle;

		// Otherwise, stay "upright"
		g_prefAngles.Init();
		g_prefAngles.x = -pPlayer->EyeAngles().x;
	}

	return g_prefAngles;
}

// 
// Tip controller
//

LINK_ENTITY_TO_CLASS(covenbuilding_tipcontroller, CCovenBuildingTipController);


//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC(CCovenBuildingTipController)

DEFINE_FIELD(m_bEnabled, FIELD_BOOLEAN),
DEFINE_FIELD(m_flSuspendTime, FIELD_TIME),
DEFINE_FIELD(m_worldGoalAxis, FIELD_VECTOR),
DEFINE_FIELD(m_localTestAxis, FIELD_VECTOR),
DEFINE_PHYSPTR(m_pController),
DEFINE_FIELD(m_angularLimit, FIELD_FLOAT),
DEFINE_FIELD(m_pParentBuilding, FIELD_CLASSPTR),

END_DATADESC()



CCovenBuildingTipController::~CCovenBuildingTipController()
{
	if (m_pController)
	{
		physenv->DestroyMotionController(m_pController);
		m_pController = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCovenBuildingTipController::Spawn(void)
{
	m_bEnabled = true;

	// align the object's local Z axis
	m_localTestAxis.Init(0, 0, 1);
	// with the world's Z axis
	m_worldGoalAxis.Init(0, 0, 1);

	// recover from up to 25 degrees / sec angular velocity
	m_angularLimit = 25.0f; //45 75
	m_flSuspendTime = 0;

	SetMoveType(MOVETYPE_NONE);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCovenBuildingTipController::Activate(void)
{
	BaseClass::Activate();

	if (m_pParentBuilding == NULL)
	{
		UTIL_Remove(this);
		return;
	}

	IPhysicsObject *pPhys = m_pParentBuilding->VPhysicsGetObject();

	if (pPhys == NULL)
	{
		UTIL_Remove(this);
		return;
	}

	//Setup the motion controller
	if (!m_pController)
	{
		m_pController = physenv->CreateMotionController((IMotionEvent *)this);
		m_pController->AttachObject(pPhys, true);
	}
	else
	{
		m_pController->SetEventHandler(this);
	}
}

void CCovenBuildingTipController::Lock(void)
{
	m_angularLimit = 250.0f;
}

void CCovenBuildingTipController::Unlock(void)
{
	m_angularLimit = 25.0f;
}


//-----------------------------------------------------------------------------
// Purpose: Actual simulation for tip controller
//-----------------------------------------------------------------------------
IMotionEvent::simresult_e CCovenBuildingTipController::Simulate(IPhysicsMotionController *pController, IPhysicsObject *pObject, float deltaTime, Vector &linear, AngularImpulse &angular)
{
	if (Enabled() == false)
		return SIM_NOTHING;

	// Don't simulate if we're being carried by the player
	if (m_pParentBuilding->IsBeingCarriedByPlayer())
		return SIM_NOTHING;

	float flAngularLimit = m_angularLimit;

	// If we were just dropped by a friendly player, stabilise better
	if (m_pParentBuilding->WasJustDroppedByPlayer())
	{
		// Increase the controller strength a little
		flAngularLimit += 5.0f;
	}
	else
	{
		// If the turret has some vertical velocity, don't simulate
		Vector vecVelocity;
		AngularImpulse angImpulse;
		pObject->GetVelocity(&vecVelocity, &angImpulse);
		if ((vecVelocity.LengthSqr() > m_pParentBuilding->MaxTipControllerVelocity()) || (angImpulse.LengthSqr() > m_pParentBuilding->MaxTipControllerAngularVelocity()))
			return SIM_NOTHING;
	}

	linear.Init();

	AngularImpulse angVel;
	pObject->GetVelocity(NULL, &angVel);

	matrix3x4_t matrix;
	// get the object's local to world transform
	pObject->GetPositionMatrix(&matrix);

	// Get the alignment axis in object space
	Vector currentLocalTargetAxis;
	VectorIRotate(m_worldGoalAxis, matrix, currentLocalTargetAxis);

	float invDeltaTime = (1 / deltaTime);
	if (m_angularLimit >= 150.0f)
	{
		angular = ComputeRotSpeedToAlignAxes(m_localTestAxis, currentLocalTargetAxis, angVel, 0, invDeltaTime, m_angularLimit);
		angular -= angVel;
		angular *= invDeltaTime;
		return SIM_LOCAL_ACCELERATION;
	}

	angular = ComputeRotSpeedToAlignAxes(m_localTestAxis, currentLocalTargetAxis, angVel, 1.0, invDeltaTime * invDeltaTime, flAngularLimit * invDeltaTime);

	return SIM_LOCAL_ACCELERATION;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCovenBuildingTipController::Enable(bool state)
{
	m_bEnabled = state;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : time - 
//-----------------------------------------------------------------------------
void CCovenBuildingTipController::Suspend(float time)
{
	m_flSuspendTime = gpGlobals->curtime + time;
}


float CCovenBuildingTipController::SuspendedTill(void)
{
	return m_flSuspendTime;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CCovenBuildingTipController::Enabled(void)
{
	if (m_flSuspendTime > gpGlobals->curtime)
		return false;

	return m_bEnabled;
}
