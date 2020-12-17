#include "cbase.h"

#include "Sprite.h"
#include "beam_shared.h"
#include "ammodef.h"
#include "props.h"
//#include "dlight.h"

#include "coven_turret.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//Activities
extern int ACT_FLOOR_TURRET_OPEN;
extern int ACT_FLOOR_TURRET_CLOSE;
extern int ACT_FLOOR_TURRET_OPEN_IDLE;
extern int ACT_FLOOR_TURRET_CLOSED_IDLE;
extern int ACT_FLOOR_TURRET_FIRE;

LINK_ENTITY_TO_CLASS(coven_turret, CCoven_Turret);

//Datatable
BEGIN_DATADESC(CCoven_Turret)

DEFINE_FIELD(m_iAmmoType, FIELD_INTEGER),
DEFINE_FIELD(m_bBlinkState, FIELD_BOOLEAN),
DEFINE_FIELD(m_bNoAlarmSounds, FIELD_BOOLEAN),

DEFINE_FIELD(m_flShotTime, FIELD_TIME),
DEFINE_FIELD(m_flLastSight, FIELD_TIME),
DEFINE_FIELD(m_flThrashTime, FIELD_TIME),
DEFINE_FIELD(m_flNextActivateSoundTime, FIELD_TIME),
DEFINE_FIELD(m_bCarriedByPlayer, FIELD_BOOLEAN),
DEFINE_FIELD(m_bUseCarryAngles, FIELD_BOOLEAN),
DEFINE_FIELD(m_flPlayerDropTime, FIELD_TIME),

DEFINE_FIELD(m_vecGoalAngles, FIELD_VECTOR),
DEFINE_FIELD(m_iEyeAttachment, FIELD_INTEGER),
DEFINE_FIELD(m_iMuzzleAttachment, FIELD_INTEGER),
DEFINE_FIELD(m_iEyeState, FIELD_INTEGER),
DEFINE_FIELD(m_vecEnemyLKP, FIELD_VECTOR),

DEFINE_FIELD(m_hPhysicsAttacker, FIELD_EHANDLE),
DEFINE_FIELD(m_flLastPhysicsInfluenceTime, FIELD_TIME),

DEFINE_KEYFIELD(m_iKeySkin, FIELD_INTEGER, "SkinNumber"),

DEFINE_THINKFUNC(Retire),
DEFINE_THINKFUNC(Deploy),
DEFINE_THINKFUNC(ActiveThink),
DEFINE_THINKFUNC(SearchThink),
DEFINE_THINKFUNC(AutoSearchThink),
DEFINE_THINKFUNC(TippedThink),
DEFINE_THINKFUNC(InactiveThink),
DEFINE_THINKFUNC(SuppressThink),
DEFINE_THINKFUNC(DisabledThink),
DEFINE_THINKFUNC(SelfDestructThink),

// Inputs
DEFINE_INPUTFUNC(FIELD_VOID, "Enable", InputEnable),
DEFINE_INPUTFUNC(FIELD_VOID, "Disable", InputDisable),
DEFINE_INPUTFUNC(FIELD_VOID, "DepleteAmmo", InputDepleteAmmo),
DEFINE_INPUTFUNC(FIELD_VOID, "RestoreAmmo", InputRestoreAmmo),

DEFINE_OUTPUT(m_OnDeploy, "OnDeploy"),
DEFINE_OUTPUT(m_OnRetire, "OnRetire"),
DEFINE_OUTPUT(m_OnTipped, "OnTipped"),

END_DATADESC()

extern ConVar sv_coven_building_hp_per_energy;
extern ConVar sv_coven_building_max_energy_swing;
extern ConVar sv_coven_building_strength;
ConVar sv_coven_building_ammo_per_energy("sv_coven_building_ammo_per_energy", "2", FCVAR_GAMEDLL | FCVAR_NOTIFY, "Ammo per energy per swing.");
ConVar sv_coven_building_energy_per_energy("sv_coven_building_energy_per_energy", "2", FCVAR_GAMEDLL | FCVAR_NOTIFY, "Energy ammo per energy per swing.");

//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CCoven_Turret::CCoven_Turret(void) :
m_bActive(false),
m_hEyeGlow(NULL),
m_hLightGlow(NULL),
m_pFlashlightBeam(NULL),
m_iAmmoType(-1),
m_flNextActivateSoundTime(0.0f),
m_bCarriedByPlayer(false),
m_bUseCarryAngles(false),
m_flPlayerDropTime(0.0f),
m_flShotTime(0.0f),
m_flLastSight(0.0f),
m_bBlinkState(false),
m_flThrashTime(0.0f)
{
	m_vecGoalAngles.Init();
	m_iLastPlayerCheck = -1;
	mOwner = NULL;
	m_iAmmo = 0;
	m_iMaxAmmo = 100;
	m_iMaxEnergy = 50;
	bTipped = false;

	m_vecEyePosition = vec3_invalid;
	m_vecEnemyLKP = vec3_invalid;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCoven_Turret::UpdateOnRemove(void)
{
	if (m_pFlashlightBeam != NULL)
	{
		UTIL_Remove(m_pFlashlightBeam);
		m_pFlashlightBeam = NULL;
	}

	if (m_hEyeGlow != NULL)
	{
		UTIL_Remove(m_hEyeGlow);
		m_hEyeGlow = NULL;
	}

	if (m_hLightGlow != NULL)
	{
		UTIL_Remove(m_hLightGlow);
		m_hLightGlow = NULL;
	}

	StopSound("NPC_FloorTurret.Alarm");
	StopSound("NPC_FloorTurret.Move");

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: Precache
//-----------------------------------------------------------------------------
void CCoven_Turret::Precache(void)
{
	//const char *pModelName = STRING(GetModelName());
	//pModelName = (pModelName && pModelName[0] != '\0') ? pModelName : COVEN_TURRET_MODEL;
	//PrecacheModel(pModelName);
	CovenBuildingInfo_t *bldgInfo = GetCovenBuildingData(BUILDING_TURRET);
	SetModelName(AllocPooledString(bldgInfo->szModelName));
	PrecacheModel(COVEN_TURRET_GLOW_SPRITE);
	PrecacheModel(LASER_BEAM_SPRITE);
	m_nHaloSprite = PrecacheModel(LASER_BEAM_SPRITE);
	//PrecacheScriptSound("NPC_FloorTurret.AlarmPing");

	// Activities
	ADD_CUSTOM_ACTIVITY(CCoven_Turret, ACT_FLOOR_TURRET_OPEN);
	ADD_CUSTOM_ACTIVITY(CCoven_Turret, ACT_FLOOR_TURRET_CLOSE);
	ADD_CUSTOM_ACTIVITY(CCoven_Turret, ACT_FLOOR_TURRET_CLOSED_IDLE);
	ADD_CUSTOM_ACTIVITY(CCoven_Turret, ACT_FLOOR_TURRET_OPEN_IDLE);
	ADD_CUSTOM_ACTIVITY(CCoven_Turret, ACT_FLOOR_TURRET_FIRE);

	PrecacheScriptSound("NPC_FloorTurret.Retire");
	PrecacheScriptSound("NPC_FloorTurret.Deploy");
	PrecacheScriptSound("NPC_FloorTurret.Move");
	PrecacheScriptSound("NPC_Combine.WeaponBash");
	PrecacheScriptSound("NPC_FloorTurret.Activate");
	PrecacheScriptSound("NPC_FloorTurret.Alert");
	m_ShotSounds = PrecacheScriptSound("NPC_FloorTurret.ShotSounds");
	PrecacheScriptSound("NPC_FloorTurret.Die");
	PrecacheScriptSound("NPC_FloorTurret.Retract");
	PrecacheScriptSound("NPC_FloorTurret.Alarm");
	PrecacheScriptSound("NPC_FloorTurret.Ping");
	PrecacheScriptSound("NPC_FloorTurret.DryFire");
	PrecacheScriptSound("NPC_FloorTurret.Destruct");

	PrecacheScriptSound("Weapon_Shotgun.Empty");
	PrecacheScriptSound("HL2Player.FlashLightOn");
	PrecacheScriptSound("HL2Player.FlashLightOff");

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: Spawn the entity
//-----------------------------------------------------------------------------
void CCoven_Turret::Spawn(void)
{
	Precache();

	m_BuildingType = BUILDING_TURRET;
	CovenBuildingInfo_t *bldgInfo = GetCovenBuildingData(m_BuildingType);
	const char *pModelName = STRING(GetModelName());
	SetModel((pModelName && pModelName[0] != '\0') ? pModelName : bldgInfo->szModelName);

	// If we're a citizen turret, we use a different skin
	if (IsCitizenTurret())
	{
		if (m_iKeySkin == 0)
		{	// select a "random" skin -- rather than being truly random, use a static variable
			// to cycle through them evenly. The static won't be saved across save/load, but 
			// frankly I don't care so much about that.
			// m_nSkin = random->RandomInt( 1, 4 );
			static unsigned int nextSkin = 0;
			m_nSkin = nextSkin + 1;

			// add one mod 4
			nextSkin = (nextSkin + 1) & 0x03;
		}
		else
		{	// at least make sure that it's in the right range
			m_nSkin = clamp(m_iKeySkin, 1, 4);
		}
	}

	BaseClass::Spawn();

	SetBlocksLOS(false);

	m_HackedGunPos = Vector(0, 0, 12.75);
	SetViewOffset(EyeOffset(ACT_IDLE));
	m_flFieldOfView = 0.2f; // 78 degrees

	m_iAmmo = m_iMaxAmmo = bldgInfo->iAmmo1[m_iLevel];
	m_iEnergy = m_iMaxEnergy = bldgInfo->iAmmo2[m_iLevel];

	AddEFlags(EFL_NO_DISSOLVE);

	SetPoseParameter(m_poseAim_Yaw, 0);
	SetPoseParameter(m_poseAim_Pitch, 0);

	m_iAmmoType = GetAmmoDef()->Index("Pistol");

	m_iMuzzleAttachment = LookupAttachment("eyes");
	m_iEyeAttachment = LookupAttachment("light");

	// FIXME: Do we ever need m_bAutoStart? (Sawyer)
	m_spawnflags |= SF_COVEN_TURRET_AUTOACTIVATE;


	SetEyeState(TURRET_EYE_DISABLED);

	// Don't allow us to skip animation setup because our attachments are critical to us!
	SetBoneCacheFlags(BCF_NO_ANIMATION_SKIP);

	CreateEffects();

	SetEnemy(NULL);
}

void CCoven_Turret::UpdateEyeVector(void)
{
	if (UpdateMuzzleMatrix())
	{
		Vector vecOrigin;
		MatrixGetColumn(m_muzzleToWorld, 3, vecOrigin);

		Vector vecForward;
		MatrixGetColumn(m_muzzleToWorld, 0, vecForward);

		// Note: We back up into the model to avoid an edge case where the eyes clip out of the world and
		//		 cause problems with the PVS calculations -- jdw

		m_vecEyePosition = vecOrigin - vecForward * 8.0f;
	}
}

const Vector &CCoven_Turret::EyePosition(void) const
{
	const_cast<CCoven_Turret *>(this)->UpdateEyeVector();
	
	return m_vecEyePosition;
}

int CCoven_Turret::GetAmmo(int index)
{
	if (index == 1)
		return m_iAmmo;
	else if (index == 2)
		return m_iEnergy;
	else if (index == 3)
		return m_iAmmo + m_iEnergy;

	return -1;
}

int CCoven_Turret::GetMaxAmmo(int index)
{
	if (index == 1)
		return m_iMaxAmmo;
	else if (index == 2)
		return m_iMaxEnergy;

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCoven_Turret::Activate(void)
{
	// Force the eye state to the current state so that our glows are recreated after transitions
	SetEyeState(m_iEyeState);
	
	BaseClass::Activate();
}

//-----------------------------------------------------------------------------
// Purpose: Retract and stop attacking
//-----------------------------------------------------------------------------
void CCoven_Turret::Retire(void)
{
	if (PreThink(TURRET_RETIRING))
		return;

	//Level out the turret
	m_vecGoalAngles = GetAbsAngles();
	SetNextThink(gpGlobals->curtime + 0.05f);

	TurnOffLight();

	//Set ourselves to close
	if (GetActivity() != ACT_FLOOR_TURRET_CLOSE && GetActivity() != ACT_FLOOR_TURRET_CLOSED_IDLE)
	{
		//Set our visible state to dormant
		SetEyeState(TURRET_EYE_DORMANT);

		SetActivity((Activity)ACT_FLOOR_TURRET_OPEN_IDLE);

		//If we're done moving to our desired facing, close up
		if (UpdateFacing() == false)
		{
			SetActivity((Activity)ACT_FLOOR_TURRET_CLOSE);
			EmitSound("NPC_FloorTurret.Retire");

			//Notify of the retraction
			m_OnRetire.FireOutput(NULL, this);
		}
	}
	else if (IsActivityFinished())
	{
		m_bActive = false;
		m_flLastSight = 0;

		SetActivity((Activity)ACT_FLOOR_TURRET_CLOSED_IDLE);

		if (m_bEnabled)
		{
			//Go back to auto searching
			SetThink(&CCoven_Turret::AutoSearchThink);
			SetNextThink(gpGlobals->curtime + 0.05f);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Deploy and start attacking
//-----------------------------------------------------------------------------
void CCoven_Turret::Deploy(void)
{
	if (PreThink(TURRET_DEPLOYING))
		return;

	m_vecGoalAngles = GetAbsAngles();

	SetNextThink(gpGlobals->curtime + 0.05f);

	//Show we've seen a target
	SetEyeState(TURRET_EYE_SEE_TARGET);

	//Open if we're not already
	if (GetActivity() != ACT_FLOOR_TURRET_OPEN)
	{
		m_bActive = true;
		SetActivity((Activity)ACT_FLOOR_TURRET_OPEN);
		EmitSound("NPC_FloorTurret.Deploy");

		//Notify we're deploying
		m_OnDeploy.FireOutput(NULL, this);
	}

	//If we're done, then start searching
	if (IsActivityFinished())
	{
		SetActivity((Activity)ACT_FLOOR_TURRET_OPEN_IDLE);

		m_flShotTime = gpGlobals->curtime + 1.0f;

		m_flPlaybackRate = 0;
		SetThink(&CCoven_Turret::SearchThink);

		EmitSound("NPC_FloorTurret.Move");
	}

	m_flLastSight = gpGlobals->curtime + COVEN_TURRET_MAX_WAIT;
}

void CCoven_Turret::SetEnemy(CBaseCombatCharacter *pEnemy)
{
	m_hEnemy = pEnemy;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CCoven_Turret::HandleInteraction(int interactionType, void *data, CBaseCombatCharacter *sourceEnt)
{
	/*if (interactionType == g_interactionCombineBash)
	{
		// We've been bashed by a combine soldier. Remember who it was, if we haven't got an active kicker
		if (!m_hLastNPCToKickMe)
		{
			m_hLastNPCToKickMe = sourceEnt;
			m_flKnockOverFailedTime = gpGlobals->curtime + 3.0;
		}

		// Get knocked away
		Vector forward, up;
		AngleVectors(sourceEnt->GetLocalAngles(), &forward, NULL, &up);
		ApplyAbsVelocityImpulse(forward * 100 + up * 50);
		CTakeDamageInfo info(sourceEnt, sourceEnt, 30, DMG_CLUB);
		CalculateMeleeDamageForce(&info, forward, GetAbsOrigin());
		TakeDamage(info);

		EmitSound("NPC_Combine.WeaponBash");
		return true;
	}*/

	return BaseClass::HandleInteraction(interactionType, data, sourceEnt);
}

//-----------------------------------------------------------------------------
// Purpose: Returns the speed at which the turret can face a target
//-----------------------------------------------------------------------------
float CCoven_Turret::MaxYawSpeed(void)
{
	//TODO: Scale by difficulty?
	return 200.0f + 80.0f * m_iLevel;//360.0f;// +40.0f * m_iLevel;
}

void CCoven_Turret::ShootTimerUpdate()
{
	m_flShotTime = 0.0f;//gpGlobals->curtime + 0.15f - 0.025f * m_iLevel;
}

//-----------------------------------------------------------------------------
// Purpose: Causes the turret to face its desired angles
//-----------------------------------------------------------------------------
bool CCoven_Turret::UpdateFacing(void)
{
	bool  bMoved = false;
	UpdateMuzzleMatrix();

	Vector vecGoalDir;
	AngleVectors(m_vecGoalAngles, &vecGoalDir);

	Vector vecGoalLocalDir;
	VectorIRotate(vecGoalDir, m_muzzleToWorld, vecGoalLocalDir);

	QAngle vecGoalLocalAngles;
	VectorAngles(vecGoalLocalDir, vecGoalLocalAngles);

	// Update pitch
	float flDiff = AngleNormalize(UTIL_ApproachAngle(vecGoalLocalAngles.x, 0.0, 0.05f * MaxYawSpeed()));

	SetPoseParameter(m_poseAim_Pitch, GetPoseParameter(m_poseAim_Pitch) + (flDiff / 1.5f));

	if (fabs(flDiff) > 0.1f)
	{
		bMoved = true;
	}

	// Update yaw
	flDiff = AngleNormalize(UTIL_ApproachAngle(vecGoalLocalAngles.y, 0.0, 0.05f * MaxYawSpeed()));

	SetPoseParameter(m_poseAim_Yaw, GetPoseParameter(m_poseAim_Yaw) + (flDiff / 1.5f));

	if (fabs(flDiff) > 0.1f)
	{
		bMoved = true;
	}

	// You're going to make decisions based on this info.  So bump the bone cache after you calculate everything
	InvalidateBoneCache();

	return bMoved;
}

void CCoven_Turret::DryFire(void)
{
	EmitSound("NPC_FloorTurret.DryFire");
	EmitSound("NPC_FloorTurret.Activate");
	EmitSound("Weapon_Shotgun.Empty");
	if (RandomFloat(0, 1) > 0.5)
	{
		m_flShotTime = gpGlobals->curtime + random->RandomFloat(1, 2.5);
	}
	else
	{
		m_flShotTime = gpGlobals->curtime;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Turret will continue to fire on a target's position when it loses sight of it
//-----------------------------------------------------------------------------
void CCoven_Turret::SuppressThink(void)
{
	//Allow descended classes a chance to do something before the think function
	if (PreThink(TURRET_SUPPRESSING))
		return;

	//Update our think time
	SetNextThink(gpGlobals->curtime + 0.1f);

	// Look for a new enemy
	HackFindEnemy();

	//If we've acquired an enemy, start firing at it
	if (GetEnemy())
	{
		SetThink(&CCoven_Turret::ActiveThink);
		return;
	}

	TurnOffLight();

	//See if we're done suppressing
	if (gpGlobals->curtime > m_flLastSight)
	{
		// Should we look for a new target?
		//ClearEnemyMemory();
		RemoveEnemy();
		SetThink(&CCoven_Turret::SearchThink);
		m_vecGoalAngles = GetAbsAngles();

		SpinDown();

		if (HasSpawnFlags(SF_COVEN_TURRET_FASTRETIRE))
		{
			// Retire quickly in this case. (The case where we saw the player, but he hid again).
			m_flLastSight = gpGlobals->curtime + COVEN_TURRET_SHORT_WAIT;
		}
		else
		{
			m_flLastSight = gpGlobals->curtime + COVEN_TURRET_MAX_WAIT;
		}

		return;
	}

	//Get our shot positions
	Vector vecMid = EyePosition();
	Vector vecMidEnemy = m_vecEnemyLKP;

	//Calculate dir and dist to enemy
	Vector	vecDirToEnemy = vecMidEnemy - vecMid;

	//We want to look at the enemy's eyes so we don't jitter
	Vector	vecDirToEnemyEyes = vecMidEnemy - vecMid;
	VectorNormalize(vecDirToEnemyEyes);

	QAngle vecAnglesToEnemy;
	VectorAngles(vecDirToEnemyEyes, vecAnglesToEnemy);

	if (m_flShotTime < gpGlobals->curtime && m_vecEnemyLKP != vec3_invalid)
	{
		Vector vecMuzzle, vecMuzzleDir;
		UpdateMuzzleMatrix();
		MatrixGetColumn(m_muzzleToWorld, 0, vecMuzzleDir);
		MatrixGetColumn(m_muzzleToWorld, 3, vecMuzzle);

		//Fire the gun
		if (DotProduct(vecDirToEnemy, vecMuzzleDir) >= DOT_10DEGREE) // 10 degree slop
		{
			if (HasSpawnFlags(SF_COVEN_TURRET_OUT_OF_AMMO))
			{
				DryFire();
			}
			else
			{
				ResetActivity();
				SetActivity((Activity)ACT_FLOOR_TURRET_FIRE);

				//Fire the weapon
				Shoot(vecMuzzle, vecMuzzleDir);
				ShootTimerUpdate();
			}
		}
	}
	else
	{
		SetActivity((Activity)ACT_FLOOR_TURRET_OPEN_IDLE);
	}

	//If we can see our enemy, face it
	m_vecGoalAngles.y = vecAnglesToEnemy.y;
	m_vecGoalAngles.x = vecAnglesToEnemy.x;

	//Turn to face
	UpdateFacing();

}

//-----------------------------------------------------------------------------
// Purpose: Allows the turret to fire on targets if they're visible
//-----------------------------------------------------------------------------
void CCoven_Turret::ActiveThink(void)
{
	//Allow descended classes a chance to do something before the think function
	if (PreThink(TURRET_ACTIVE))
		return;

	HackFindEnemy();

	//Update our think time
	SetNextThink(gpGlobals->curtime + 0.1f);

	CBasePlayer *pEnemy = ToHL2MPPlayer(GetEnemy());

	//If we've become inactive, go back to searching
	if (!pEnemy || m_bActive == false || GetEnemy() == NULL)
	{
		RemoveEnemy();
		m_flLastSight = gpGlobals->curtime + COVEN_TURRET_MAX_WAIT;
		SetThink(&CCoven_Turret::SearchThink);
		m_vecGoalAngles = GetAbsAngles();
		return;
	}

	//Get our shot positions
	Vector vecMid = EyePosition();
	Vector vecMidEnemy = pEnemy->GetPlayerMidPoint();

	// Store off our last seen location so we can suppress it later
	m_vecEnemyLKP = vecMidEnemy;

	//Look for our current enemy
	bool bEnemyInFOV = FInViewCone(GetEnemy());
	bool bEnemyVisible = FVisible(GetEnemy());
	bool isDead = !GetEnemy()->IsAlive() || pEnemy->KO;

	// Robin: This is a hack to get around the fact that the muzzle for the turret
	// is outside it's vcollide. This means that if it leans against a thin wall, 
	// the muzzle can be on the other side of the wall, where it's then able to see
	// and shoot at targets. This check ensures that nothing has come between the
	// center of the turret and the muzzle.
	if (bEnemyVisible)
	{
		trace_t tr;
		Vector vecCenter;
		CollisionProp()->CollisionToWorldSpace(Vector(0, 0, 52), &vecCenter);
		UTIL_TraceLine(vecCenter, vecMid, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr);
		if (tr.fraction != 1.0)
		{
			bEnemyVisible = false;
		}
	}

	//Calculate dir and dist to enemy
	Vector	vecDirToEnemy = vecMidEnemy - vecMid;
	float	flDistToEnemy = VectorNormalize(vecDirToEnemy);

	//See if they're past our FOV of attack
	if (bEnemyInFOV == false)
	{
		// Should we look for a new target?
		//ClearEnemyMemory();
		RemoveEnemy();

		if (HasSpawnFlags(SF_COVEN_TURRET_FASTRETIRE))
		{
			// Retire quickly in this case. (The case where we saw the player, but he hid again).
			m_flLastSight = gpGlobals->curtime + COVEN_TURRET_SHORT_WAIT;
		}
		else
		{
			m_flLastSight = gpGlobals->curtime + COVEN_TURRET_MAX_WAIT;
		}

		SetThink(&CCoven_Turret::SearchThink);
		m_vecGoalAngles = GetAbsAngles();

		SpinDown();

		return;
	}

	//BB: dont suppress a known dead player
	//Current enemy is not visible
	if ((bEnemyVisible == false) || isDead || (flDistToEnemy > COVEN_TURRET_RANGE))
	{
		//ClearEnemyMemory();
		RemoveEnemy();
		if (isDead)
		{
			m_flLastSight = gpGlobals->curtime + COVEN_TURRET_MAX_WAIT;
			SetThink(&CCoven_Turret::SearchThink);
		}
		else
		{
			m_flLastSight = gpGlobals->curtime + 1.0f;
			SetThink(&CCoven_Turret::SuppressThink);
		}
		return;
	}

	//BB: UPDATE DIRECTION HERE?

	if (!HasSpawnFlags(SF_COVEN_TURRET_OUT_OF_ENERGY) && flDistToEnemy <= COVEN_TURRET_LIGHT_RANGE)
		TurnOnLight();

	if (m_flShotTime < gpGlobals->curtime)
	{
		Vector vecMuzzle, vecMuzzleDir;

		UpdateMuzzleMatrix();
		MatrixGetColumn(m_muzzleToWorld, 0, vecMuzzleDir);
		MatrixGetColumn(m_muzzleToWorld, 3, vecMuzzle);

		Vector2D vecDirToEnemy2D = vecDirToEnemy.AsVector2D();
		Vector2D vecMuzzleDir2D = vecMuzzleDir.AsVector2D();

		bool bCanShoot = true;
		float minCos3d = DOT_10DEGREE; // 10 degrees slop

		if (flDistToEnemy < 30.0)
		{
			vecDirToEnemy2D.NormalizeInPlace();
			vecMuzzleDir2D.NormalizeInPlace();

			bCanShoot = (vecDirToEnemy2D.Dot(vecMuzzleDir2D) >= DOT_10DEGREE);
			minCos3d = DOT_45DEGREE; // 45 degrees
		}

		//Fire the gun
		if (bCanShoot) // 10 degree slop XY
		{
			float dot3d = DotProduct(vecDirToEnemy, vecMuzzleDir);

			if (HasSpawnFlags(SF_COVEN_TURRET_OUT_OF_AMMO))
			{
				DryFire();
			}
			else
			{
				if (dot3d >= minCos3d)
				{
					ResetActivity();
					SetActivity((Activity)ACT_FLOOR_TURRET_FIRE);

					//Fire the weapon
					//Msg("Shooting: %.2f %.2f %.2f - %d!\n", vecMuzzleDir.x, vecMuzzleDir.y, vecMuzzleDir.z, (dot3d < DOT_10DEGREE));
					Shoot(vecMuzzle, vecMuzzleDir, (dot3d < DOT_10DEGREE));

					ShootTimerUpdate();
				}
			}
		}
	}
	else
	{
		SetActivity((Activity)ACT_FLOOR_TURRET_OPEN_IDLE);
	}

	//If we can see our enemy, face it
	if (bEnemyVisible)
	{
		//We want to look at the enemy's eyes so we don't jitter
		Vector	vecDirToEnemyEyes = vecMidEnemy - vecMid;
		VectorNormalize(vecDirToEnemyEyes);

		QAngle vecAnglesToEnemy;
		VectorAngles(vecDirToEnemyEyes, vecAnglesToEnemy);

		m_vecGoalAngles.y = vecAnglesToEnemy.y;
		m_vecGoalAngles.x = vecAnglesToEnemy.x;
	}

	//Turn to face
	UpdateFacing();

}

//-----------------------------------------------------------------------------
// Purpose: Target doesn't exist or has eluded us, so search for one
//-----------------------------------------------------------------------------
void CCoven_Turret::SearchThink(void)
{
	//Allow descended classes a chance to do something before the think function
	if (PreThink(TURRET_SEARCHING))
		return;

	SetNextThink(gpGlobals->curtime + 0.05f);

	SetActivity((Activity)ACT_FLOOR_TURRET_OPEN_IDLE);

	//If our enemy has died, pick a new enemy
	if ((GetEnemy() != NULL) && (GetEnemy()->IsAlive() == false || ((CHL2MP_Player *)GetEnemy())->KO))
	{
		RemoveEnemy();
	}

	//Acquire the target
	if (GetEnemy() == NULL)
	{
		HackFindEnemy();
	}

	//If we've found a target, spin up the barrel and start to attack
	if (GetEnemy() != NULL)
	{
		//Give players a grace period
		if (GetEnemy()->IsPlayer())
		{
			m_flShotTime = gpGlobals->curtime + 0.5f;
		}
		else
		{
			m_flShotTime = gpGlobals->curtime + 0.1f;
		}

		m_flLastSight = 0;
		SetThink(&CCoven_Turret::ActiveThink);
		SetEyeState(TURRET_EYE_SEE_TARGET);

		SpinUp();

		if (gpGlobals->curtime > m_flNextActivateSoundTime)
		{
			EmitSound("NPC_FloorTurret.Activate");
			m_flNextActivateSoundTime = gpGlobals->curtime + 3.0;
		}
		return;
	}

	//Are we out of time and need to retract?
	if (gpGlobals->curtime > m_flLastSight)
	{
		//Before we retrace, make sure that we are spun down.
		m_flLastSight = 0;
		SetThink(&CCoven_Turret::Retire);
		return;
	}

	//Display that we're scanning
	m_vecGoalAngles.x = GetAbsAngles().x + (sin(gpGlobals->curtime * 1.0f) * 15.0f);
	m_vecGoalAngles.y = GetAbsAngles().y + (sin(gpGlobals->curtime * 2.0f) * 60.0f);

	//Turn and ping
	UpdateFacing();
	Ping();
}

//-----------------------------------------------------------------------------
// Purpose: Watch for a target to wander into our view
//-----------------------------------------------------------------------------
void CCoven_Turret::AutoSearchThink(void)
{
	//Allow descended classes a chance to do something before the think function
	if (PreThink(TURRET_AUTO_SEARCHING))
		return;

	//Spread out our thinking
	SetNextThink(gpGlobals->curtime + random->RandomFloat(0.2f, 0.4f));

	//If the enemy is dead, find a new one
	CHL2MP_Player *pEnemy = (CHL2MP_Player *)GetEnemy();
	if ((GetEnemy() != NULL) && pEnemy != NULL && (GetEnemy()->IsAlive() == false || pEnemy->KO))
	{
		RemoveEnemy();
	}

	//Acquire Target
	if (GetEnemy() == NULL)
	{
		HackFindEnemy();
	}

	//Deploy if we've got an active target
	if (GetEnemy() != NULL)
	{
		SetThink(&CCoven_Turret::Deploy);
		if (!m_bNoAlarmSounds)
		{
			EmitSound("NPC_FloorTurret.Alert");
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Fire!
//-----------------------------------------------------------------------------
void CCoven_Turret::Shoot(const Vector &vecSrc, const Vector &vecDirToEnemy, bool bStrict)
{
	FireBulletsInfo_t info;
	Vector vecDir = vecDirToEnemy;
	if (!bStrict && GetEnemy() != NULL && GetEnemy()->IsPlayer())
	{
		CBasePlayer *pPlayer = (CBasePlayer *)GetEnemy();
		//BB: this hangs
		//Vector vecDir = GetActualShootTrajectory( vecSrc );
		//BB: this is too accurate?
		int accuracy = pPlayer->m_floatCloakFactor * 32 + 32 - m_iLevel * 8;
		AngleVectors(GetLocalAngles(), &vecDir);
		//vecDir = GetEnemy()->GetLocalOrigin() + Vector(0,0,42) - vecSrc;
		vecDir = pPlayer->GetPlayerMidPoint() + Vector(random->RandomInt(-accuracy, accuracy), random->RandomInt(-accuracy, accuracy), random->RandomInt(-accuracy, accuracy)) - vecSrc;

		info.m_vecSrc = vecSrc;
		info.m_vecDirShooting = vecDir;
		info.m_iTracerFreq = 1;
		info.m_iShots = 1;
		info.m_iPlayerDamage = 8.0f;
		info.m_pAttacker = this;
		info.m_vecSpread = VECTOR_CONE_PRECALCULATED;
		info.m_flDistance = MAX_COORD_RANGE;
		info.m_iAmmoType = m_iAmmoType;
	}
	else
	{
		info.m_vecSrc = vecSrc;
		info.m_vecDirShooting = vecDirToEnemy;
		info.m_iTracerFreq = 1;
		info.m_iPlayerDamage = 8.0f;
		info.m_iShots = 1;
		info.m_pAttacker = this;
		info.m_vecSpread = GetAttackSpread(NULL, GetEnemy());
		info.m_flDistance = MAX_COORD_RANGE;
		info.m_iAmmoType = m_iAmmoType;
		vecDir = vecDirToEnemy;
	}

	FireBullets(info);
	EmitSound("NPC_FloorTurret.ShotSounds", m_ShotSounds);
	m_iAmmo--;
	DoMuzzleFlash();
}

//-----------------------------------------------------------------------------
// Purpose: The turret has been tipped over and will thrash for awhile
//-----------------------------------------------------------------------------
void CCoven_Turret::TippedThink(void)
{
	//Animate
	BaseClass::PreThink();

	SetNextThink(gpGlobals->curtime + 0.05f);
	RemoveEnemy();

	// If we're not on side anymore, stop thrashing
	if (!OnSide())
	{
		ReturnToLife();
		return;
	}

	//See if we should continue to thrash
	if (gpGlobals->curtime < m_flThrashTime)
	{
		if (m_flShotTime < gpGlobals->curtime)
		{
			if (HasSpawnFlags(SF_COVEN_TURRET_OUT_OF_AMMO) || m_iAmmo <= 0)
			{
				DryFire();
			}
			else if (IsCitizenTurret() == false)	// Citizen turrets don't wildly fire
			{
				Vector vecMuzzle, vecMuzzleDir;
				UpdateMuzzleMatrix();
				MatrixGetColumn(m_muzzleToWorld, 0, vecMuzzleDir);
				MatrixGetColumn(m_muzzleToWorld, 3, vecMuzzle);

				ResetActivity();
				SetActivity((Activity)ACT_FLOOR_TURRET_FIRE);
				Shoot(vecMuzzle, vecMuzzleDir);
			}

			m_flShotTime = gpGlobals->curtime + 0.05f;
		}

		m_vecGoalAngles.x = GetAbsAngles().x + random->RandomFloat(-60, 60);
		m_vecGoalAngles.y = GetAbsAngles().y + random->RandomFloat(-60, 60);

		UpdateFacing();
	}
	else
	{
		//Face forward
		m_vecGoalAngles = GetAbsAngles();

		//Set ourselves to close
		if (GetActivity() != ACT_FLOOR_TURRET_CLOSE)
		{
			SetActivity((Activity)ACT_FLOOR_TURRET_OPEN_IDLE);

			//If we're done moving to our desired facing, close up
			if (UpdateFacing() == false)
			{
				//Make any last death noises and anims
				EmitSound("NPC_FloorTurret.Die");
				SpinDown();

				SetActivity((Activity)ACT_FLOOR_TURRET_CLOSE);
				EmitSound("NPC_FloorTurret.Retract");
			}
		}
		else if (IsActivityFinished())
		{
			m_bActive = false;
			m_flLastSight = 0;

			SetActivity((Activity)ACT_FLOOR_TURRET_CLOSED_IDLE);

			//Try to look straight
			if (UpdateFacing() == false)
			{
				m_OnTipped.FireOutput(this, this);
				SetEyeState(TURRET_EYE_DEAD);

				// Start thinking slowly to see if we're ever set upright somehow
				SetThink(&CCoven_Turret::InactiveThink);
				SetNextThink(gpGlobals->curtime + 1.0f);
			}
		}
	}
}

const Vector CCoven_Turret::GetPlayerMidPoint(void) const
{
	//UpdateMuzzleMatrix();

	Vector vecOrigin;
	MatrixGetColumn(m_muzzleToWorld, 3, vecOrigin);

	Vector vecForward;
	MatrixGetColumn(m_muzzleToWorld, 0, vecForward);

	// Note: We back up into the model to avoid an edge case where the eyes clip out of the world and
	//		 cause problems with the PVS calculations -- jdw

	vecOrigin -= vecForward * 8.0f;

	return vecOrigin;
}

//-----------------------------------------------------------------------------
// Purpose: This turret is dead. See if it ever becomes upright again, and if 
//			so, become active again.
//-----------------------------------------------------------------------------
void CCoven_Turret::InactiveThink(void)
{
	// Wake up if we're not on our side
	if ( !OnSide() )
	{
		ReturnToLife();
		return;
	}

	// Blink if we have ammo or our current blink is "on" and we need to turn it off again
	// If we're on our side, ping and complain to the player
	if (m_bBlinkState == false)
	{
		// Ping when the light is going to come back on
		EmitSound("NPC_FloorTurret.Ping");
	}

	NotifyOwner();

	SetEyeState(TURRET_EYE_ALARM);
	SetNextThink(gpGlobals->curtime + 0.25f);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCoven_Turret::ReturnToLife(void)
{
	m_flThrashTime = 0;

	// Enable the tip controller
	EnableUprightController();

	// Return to life
	SetEnemy(NULL);
}

//-----------------------------------------------------------------------------
// Purpose: The turret is not doing anything at all
//-----------------------------------------------------------------------------
void CCoven_Turret::DisabledThink(void)
{
	SetNextThink(gpGlobals->curtime + 0.5);
	StopSound("NPC_FloorTurret.Alarm");

	if (OnSide())
	{
		m_OnTipped.FireOutput(this, this);
		SetEyeState(TURRET_EYE_DEAD);
		SetCollisionGroup(COLLISION_GROUP_WEAPON);
		//SetThink( NULL );
	}
}

//-----------------------------------------------------------------------------
// Purpose: The turret doesn't run base AI properly, which is a bad decision.
//			As a result, it has to manually find enemies.
//-----------------------------------------------------------------------------
void CCoven_Turret::HackFindEnemy(void)
{
	//BB: a Coven hack of a hack! I LOVE IT!
	if (m_iLastPlayerCheck < 0)
	{
		for (m_iLastPlayerCheck = 0; m_iLastPlayerCheck < gpGlobals->maxClients; m_iLastPlayerCheck++)
		{
			CHL2MP_Player *pPlayer = ToHL2MPPlayer(UTIL_PlayerByIndex(m_iLastPlayerCheck));
			if (pPlayer && pPlayer->GetTeamNumber() != GetTeamNumber() && pPlayer->IsAlive() && !pPlayer->KO)
			{
				//Get our shot positions
				Vector vecMid = EyePosition();
				Vector vecMidEnemy = pPlayer->GetPlayerMidPoint();
				//Calculate dir and dist to enemy
				Vector	vecDirToEnemy = vecMidEnemy - vecMid;
				float	flDistToEnemy = VectorNormalize(vecDirToEnemy);
				if (flDistToEnemy <= COVEN_TURRET_RANGE)
				{
					//Look for our current enemy
					bool bEnemyInFOV = FInViewCone(pPlayer);
					if (bEnemyInFOV)
					{
						bool bEnemyVisible = FVisible(pPlayer);

						if (bEnemyVisible)
						{
							trace_t tr;
							Vector vecCenter;
							CollisionProp()->CollisionToWorldSpace(Vector(0, 0, 52), &vecCenter);
							UTIL_TraceLine(vecCenter, vecMid, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr);
							if (tr.fraction != 1.0)
							{
								bEnemyVisible = false;
							}
						}
						if (bEnemyVisible)
						{
							SetEnemy(pPlayer);
							return;
						}
					}
				}
			}
		}
	}
	else //I already have a target
	{
		CHL2MP_Player *pPlayer = ToHL2MPPlayer(UTIL_PlayerByIndex(m_iLastPlayerCheck));
		if (pPlayer)
		{
			SetEnemy(pPlayer);
			return;
		}
	}
	RemoveEnemy();
}

//-----------------------------------------------------------------------------
// Purpose: Determines whether the turret is upright enough to function
// Output : Returns true if the turret is tipped over
//-----------------------------------------------------------------------------
inline bool CCoven_Turret::OnSide(void)
{
	Vector	up;
	GetVectors(NULL, NULL, &up);

	if (DotProduct(up, Vector(0, 0, 1)) < 0.5f)
	{
		bTipped = true;
	}
	else
		bTipped = false;
	return bTipped;
}

//-----------------------------------------------------------------------------
// Purpose: Allows a generic think function before the others are called
// Input  : state - which state the turret is currently in
//-----------------------------------------------------------------------------
bool CCoven_Turret::PreThink(covenTurretState_e state)
{
	//Msg("Turret %d: state:%d hp:%d ammo:%d xp:%d\n", entindex(), state, m_iHealth, m_iAmmo, m_iXP);

	//Animate
	BaseClass::PreThink();

	if (LightIsOn())
		m_iEnergy--;

	if (m_iAmmo <= 0)
		AddSpawnFlags(SF_COVEN_TURRET_OUT_OF_AMMO);
	if (m_iEnergy <= 0)
		AddSpawnFlags(SF_COVEN_TURRET_OUT_OF_ENERGY);

	// We're gonna blow up, so don't interrupt us
	if (state == TURRET_SELF_DESTRUCTING)
		return false;

	// Add the laser if it doesn't already exist
	if (state == TURRET_SEARCHING || state == TURRET_ACTIVE || state == TURRET_SUPPRESSING)
	{
		if (HasSpawnFlags(SF_COVEN_TURRET_OUT_OF_ENERGY))
		{
			TurnOffLight();
		}
		else
		{
			Vector vecMuzzle, vecMuzzleDir;
			UpdateMuzzleMatrix();
			MatrixGetColumn(m_muzzleToWorld, 0, vecMuzzleDir);
			MatrixGetColumn(m_muzzleToWorld, 3, vecMuzzle);
			Vector startPos = vecMuzzle - vecMuzzleDir * 3.6f + Vector(0, -0.5, 1.2);

			trace_t tr;
			UTIL_TraceLine(startPos, startPos + (vecMuzzleDir * 300), MASK_SHOT, this, COLLISION_GROUP_NONE, &tr);

			if (m_pFlashlightBeam != NULL)
			{
				m_pFlashlightBeam->FollowEntity(this);
				m_pFlashlightBeam->SetAbsStartPos(tr.startpos);
				m_pFlashlightBeam->SetAbsEndPos(tr.endpos);
				m_pFlashlightBeam->RelinkBeam();
				m_hLightGlow->SetAbsOrigin(tr.startpos);
			
				SlayerLightHandler(DOT_45DEGREE, 350.0f, 325.0f, 350.0f);
			}
		}
	}

	//See if we've tipped, but only do this if we're not being carried
	if (!IsBeingCarriedByPlayer())
	{
		if (!OnSide())
		{
			// If I still haven't fallen over after an NPC has tried to knock me down, let them know
		}
		else
		{
			OnTakeDamage(CTakeDamageInfo(GetContainingEntity(INDEXENT(0)), GetContainingEntity(INDEXENT(0)), random->RandomFloat(5.0f, 15.0f), DMG_CRUSH));
			if (!IsAlive()) //we died
				return false;

			// Take away the laser
			TurnOffLight();

			if (!HasSpawnFlags(SF_COVEN_TURRET_OUT_OF_AMMO))
			{
				//Thrash around for a bit
				m_flThrashTime = gpGlobals->curtime + random->RandomFloat(2.0f, 2.5f);
				SetNextThink(gpGlobals->curtime + 0.05f);

				SetThink(&CCoven_Turret::TippedThink);
				SetEyeState(TURRET_EYE_SEE_TARGET);

				SpinUp();
				if (!m_bNoAlarmSounds)
				{
					EmitSound("NPC_FloorTurret.Alarm");
				}
			}
			else
			{
				//Thrash around for a bit
				m_flThrashTime = 0;
				SetNextThink(gpGlobals->curtime + 0.05f);

				SetThink(&CCoven_Turret::TippedThink);

				// Become inactive
				/*SetThink( &CNPC_FloorTurret::InactiveThink );
				SetEyeState( TURRET_EYE_DEAD );*/
			}
			SetCollisionGroup(COLLISION_GROUP_WEAPON);

			//Disable the tip controller
			DisableUprightController();

			//Interrupt current think function
			return true;
		}
	}

	//Do not interrupt current think function
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Sets the state of the glowing eye attached to the turret
// Input  : state - state the eye should be in
//-----------------------------------------------------------------------------
void CCoven_Turret::SetEyeState(covenEyeState_t state)
{
	// Must have a valid eye to affect
	if (!m_hEyeGlow)
	{
		// Create our eye sprite
		m_hEyeGlow = CSprite::SpriteCreate(COVEN_TURRET_GLOW_SPRITE, GetLocalOrigin(), false);
		if (!m_hEyeGlow)
			return;

		m_hEyeGlow->SetTransparency(kRenderWorldGlow, 255, 0, 0, 128, kRenderFxNoDissipation);
		m_hEyeGlow->SetAttachment(this, m_iEyeAttachment);
	}

	m_iEyeState = state;

	//Set the state
	switch (state)
	{
	default:
	case TURRET_EYE_SEE_TARGET: //Fade in and scale up
		m_hEyeGlow->SetColor(255, 0, 0);
		m_hEyeGlow->SetBrightness(164, 0.1f);
		m_hEyeGlow->SetScale(0.4f, 0.1f);
		break;

	case TURRET_EYE_SEEKING_TARGET: //Ping-pongs

		//Toggle our state
		m_bBlinkState = !m_bBlinkState;
		m_hEyeGlow->SetColor(255, 128, 0);

		if (m_bBlinkState)
		{
			//Fade up and scale up
			m_hEyeGlow->SetScale(0.25f, 0.1f);
			m_hEyeGlow->SetBrightness(164, 0.1f);
		}
		else
		{
			//Fade down and scale down
			m_hEyeGlow->SetScale(0.2f, 0.1f);
			m_hEyeGlow->SetBrightness(64, 0.1f);
		}

		break;

	case TURRET_EYE_DORMANT: //Fade out and scale down
		m_hEyeGlow->SetColor(0, 255, 0);
		m_hEyeGlow->SetScale(0.1f, 0.5f);
		m_hEyeGlow->SetBrightness(64, 0.5f);
		break;

	case TURRET_EYE_DEAD: //Fade out slowly
		m_hEyeGlow->SetColor(255, 0, 0);
		m_hEyeGlow->SetScale(0.1f, 3.0f);
		m_hEyeGlow->SetBrightness(0, 3.0f);
		break;

	case TURRET_EYE_DISABLED:
		m_hEyeGlow->SetColor(0, 255, 0);
		m_hEyeGlow->SetScale(0.1f, 1.0f);
		m_hEyeGlow->SetBrightness(0, 1.0f);
		break;

	case TURRET_EYE_ALARM:
	{
		//Toggle our state
		m_bBlinkState = !m_bBlinkState;
		m_hEyeGlow->SetColor(255, 0, 0);

		if (m_bBlinkState)
		{
			//Fade up and scale up
			m_hEyeGlow->SetScale(0.75f, 0.05f);
			m_hEyeGlow->SetBrightness(192, 0.05f);
		}
		else
		{
			//Fade down and scale down
			m_hEyeGlow->SetScale(0.25f, 0.25f);
			m_hEyeGlow->SetBrightness(64, 0.25f);
		}
	}
	break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Make a pinging noise so the player knows where we are
//-----------------------------------------------------------------------------
void CCoven_Turret::Ping(void)
{
	//See if it's time to ping again
	if (m_flPingTime > gpGlobals->curtime)
		return;

	//Ping!
	EmitSound("NPC_FloorTurret.Ping");

	SetEyeState(TURRET_EYE_SEEKING_TARGET);

	m_flPingTime = gpGlobals->curtime + COVEN_TURRET_PING_TIME;
}

void CCoven_Turret::CreateEffects(void)
{
	if (m_pFlashlightBeam == NULL)
	{
		m_pFlashlightBeam = CBeam::BeamCreate(LASER_BEAM_SPRITE, 8.0f);
		if (m_pFlashlightBeam != NULL)
		{
			m_pFlashlightBeam->SetFadeLength(300.0f);
			m_pFlashlightBeam->SetType(TE_BEAMPOINTS);
			m_pFlashlightBeam->SetNoise(0);
			m_pFlashlightBeam->SetHaloTexture(m_nHaloSprite);
			m_pFlashlightBeam->SetHaloScale(3.0f);
			m_pFlashlightBeam->SetColor(255, 255, 255);
			m_pFlashlightBeam->SetWidth(55.0f);
			m_pFlashlightBeam->SetScrollRate(0);
			m_pFlashlightBeam->SetEndWidth(55.0f);
			m_pFlashlightBeam->SetBrightness(60.0f);
			m_pFlashlightBeam->SetBeamFlags(FBEAM_FOREVER | FBEAM_ONLYNOISEONCE | FBEAM_NOTILE | FBEAM_HALOBEAM);
			m_pFlashlightBeam->TurnOn(); //a dev might have brain damage
		}
	}
	// Must have a valid eye to affect
	if (m_hLightGlow == NULL)
	{
		// Create our eye sprite
		m_hLightGlow = CSprite::SpriteCreate(COVEN_TURRET_GLOW_SPRITE, GetLocalOrigin(), false);
		if (m_hLightGlow)
		{
			m_hLightGlow->SetTransparency(kRenderWorldGlow, 255, 255, 255, 128, kRenderFxNoDissipation);
			m_hLightGlow->SetBrightness(0);
		}
	}
}

void CCoven_Turret::OnBuildingComplete(void)
{
	SetActivity((Activity)ACT_FLOOR_TURRET_CLOSED_IDLE);
	SetThink(&CCoven_Turret::AutoSearchThink);
	SetEyeState(TURRET_EYE_DORMANT);
	//Stagger our starting times
	SetNextThink(gpGlobals->curtime + random->RandomFloat(0.1f, 0.3f));
	
	BaseClass::OnBuildingComplete();
}

//-----------------------------------------------------------------------------
// Purpose: Enable the turret and deploy
//-----------------------------------------------------------------------------
void CCoven_Turret::Enable(void)
{
	// Don't interrupt blowing up!
	if (m_bSelfDestructing)
		return;

	//This turret is on its side, it can't function
	//if (OnSide() || (IsAlive() == false))
	//	return;

	ReturnToLife();

	SetThink(&CCoven_Turret::AutoSearchThink);
	SetNextThink(gpGlobals->curtime + 0.05f);

	BaseClass::Enable();
}

void CCoven_Turret::RemoveEnemy()
{
	SetEnemy(NULL);
	m_iLastPlayerCheck = -1;
	TurnOffLight();
}

//-----------------------------------------------------------------------------
// Purpose: Retire the turret until enabled again
//-----------------------------------------------------------------------------
void CCoven_Turret::Disable(void)
{
	SetEyeState(TURRET_EYE_DORMANT);

	//This turret is on its side, it can't function
	if (m_bSelfDestructing)
		return;

	StopSound("NPC_FloorTurret.Alarm");

	if (m_bEnabled)
	{
		RemoveEnemy();
		SetThink(&CCoven_Turret::Retire);
		SetNextThink(gpGlobals->curtime + 0.1f);
	}
	else
		SetThink(&CCoven_Turret::DisabledThink);

	BaseClass::Disable();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCoven_Turret::InputEnable(inputdata_t &inputdata)
{
	Enable();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCoven_Turret::InputDisable(inputdata_t &inputdata)
{
	Disable();
}

//-----------------------------------------------------------------------------
// Purpose: Stops the turret from firing live rounds (still attempts to though)
//-----------------------------------------------------------------------------
void CCoven_Turret::InputDepleteAmmo(inputdata_t &inputdata)
{
	AddSpawnFlags(SF_COVEN_TURRET_OUT_OF_AMMO);
}

//-----------------------------------------------------------------------------
// Purpose: Allows the turret to fire live rounds again
//-----------------------------------------------------------------------------
void CCoven_Turret::InputRestoreAmmo(inputdata_t &inputdata)
{
	RemoveSpawnFlags(SF_COVEN_TURRET_OUT_OF_AMMO);
}

//-----------------------------------------------------------------------------
// Purpose: Reduce physics forces from the front
//-----------------------------------------------------------------------------
int CCoven_Turret::VPhysicsTakeDamage(const CTakeDamageInfo &info)
{
	/*bool bShouldIgnoreFromFront = false;

	// Ignore crossbow bolts hitting us from the front
	bShouldIgnoreFromFront = ( info.GetDamageType() & DMG_BULLET ) != 0;

	// Ignore bullets from the front
	if ( !bShouldIgnoreFromFront )
	{
	bShouldIgnoreFromFront = FClassnameIs( info.GetInflictor(), "crossbow_bolt" );
	}

	// Did it hit us on the front?
	if ( bShouldIgnoreFromFront )
	{
	Vector vecForward;
	GetVectors( &vecForward, NULL, NULL );
	Vector vecForce = info.GetDamageForce();
	VectorNormalize( vecForce );
	float flDot = DotProduct( vecForward, vecForce );
	if ( flDot < -0.85 )
	return 0;
	}*/

	return BaseClass::VPhysicsTakeDamage(info);
}

bool CCoven_Turret::CheckLevel(void)
{
	if (BaseClass::CheckLevel())
	{
		CovenBuildingInfo_t *bldgInfo = GetCovenBuildingData(m_BuildingType);
		m_iMaxAmmo = bldgInfo->iAmmo1[m_iLevel];
		m_iAmmo = m_iMaxAmmo;
		m_iMaxEnergy = bldgInfo->iAmmo2[m_iLevel];
		m_iEnergy = m_iMaxEnergy;
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
//-----------------------------------------------------------------------------
int CCoven_Turret::OnTakeDamage(const CTakeDamageInfo &info)
{
	if (mOwner.Get() == NULL || (info.GetAttacker() != NULL && info.GetAttacker()->GetTeamNumber() == GetTeamNumber() && !(info.GetDamageType() & DMG_SHOCK)))
		return 0;

	CTakeDamageInfo	newInfo = info;

	if (info.GetDamageType() & DMG_SHOCK)
	{
		if (info.GetAttacker() != NULL && info.GetAttacker()->IsPlayer() && info.GetAttacker()->GetTeamNumber() == GetTeamNumber() && !m_bSelfDestructing)
		{
			CHL2MP_Player *pAttacker = (CHL2MP_Player *)info.GetAttacker();
			if (pAttacker)
			{
				CovenBuildingInfo_t *bldgInfo = GetCovenBuildingData(m_BuildingType);
				int hp = 0;
				int ammo = 0;
				int energy = 0;
				int xp = 0;
				hp = min(sv_coven_building_max_energy_swing.GetInt(), ceil((m_iMaxHealth - m_iHealth) / sv_coven_building_hp_per_energy.GetFloat()));
				ammo = min(sv_coven_building_max_energy_swing.GetInt(), ceil((m_iMaxAmmo - m_iAmmo) / sv_coven_building_ammo_per_energy.GetFloat()));
				energy = min(sv_coven_building_max_energy_swing.GetInt(), ceil((m_iMaxEnergy - m_iEnergy) / sv_coven_building_energy_per_energy.GetFloat()));
				if (hp == 0 || (ammo + energy) == 0)
				{
					int maxXP = m_iMaxXP + sv_coven_building_max_energy_swing.GetInt();
					if (m_iLevel >= (bldgInfo->iMaxLevel - 2))
						maxXP = m_iMaxXP;
					xp = min(sv_coven_building_max_energy_swing.GetInt(), maxXP - m_iXP);
				}

				int total = min(ammo + hp + energy + xp, pAttacker->SuitPower_GetCurrentPercentage());
				int drain = min(total, hp);
				if (drain > 0)
				{
					m_iHealth = min(m_iHealth + sv_coven_building_hp_per_energy.GetInt() * drain, m_iMaxHealth);
					total -= drain;
					pAttacker->SuitPower_Drain(drain);
				}
				drain = min(total, ammo);
				if (drain > 0)
				{
					m_iAmmo = min(m_iAmmo + sv_coven_building_ammo_per_energy.GetInt() * drain, m_iMaxAmmo);
					RemoveSpawnFlags(SF_COVEN_TURRET_OUT_OF_AMMO);
					total -= drain;
					pAttacker->SuitPower_Drain(drain);
				}
				drain = min(total, energy);
				if (drain > 0)
				{
					m_iEnergy = min(m_iEnergy + sv_coven_building_energy_per_energy.GetInt() * drain, m_iMaxEnergy);
					RemoveSpawnFlags(SF_COVEN_TURRET_OUT_OF_ENERGY);
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
				//Msg("HP:%d Ammo:%d Energy:%d XP:%d\n", m_iHealth, m_iAmmo, m_iEnergy, m_iXP);
			
				return info.GetDamage();
			}
		}
	}

	WakeUp();

	if (info.GetDamageType() & (DMG_CLUB))
	{
		// Take extra force from melee hits
		newInfo.ScaleDamageForce(0.5f);

		// Disable our upright controller for some time
		//SuspendUprightController(0.5f);
	}
	else if (info.GetDamageType() & DMG_BLAST)
	{
		newInfo.ScaleDamageForce(1.25f);
	}
	/*else if ( (info.GetDamageType() & DMG_BULLET) && !(info.GetDamageType() & DMG_BUCKSHOT) )
	{
	// Bullets, but not buckshot, do extra push
	newInfo.ScaleDamageForce( 2.5f );
	}*/

	// Manually apply vphysics because CBaseCombatCharacter takedamage doesn't call back to CBaseEntity OnTakeDamage
	VPhysicsTakeDamage(newInfo);

	// Bump up our search time
	m_flLastSight = gpGlobals->curtime + COVEN_TURRET_MAX_WAIT;

	// Start looking around in anger if we were idle
	if (IsAlive() && m_bEnabled && GetActivity() == ACT_FLOOR_TURRET_CLOSED_IDLE && m_bSelfDestructing == false)
	{
		SetThink(&CCoven_Turret::Deploy);
	}

	newInfo.SetDamage((1.0f - sv_coven_building_strength.GetFloat() / 60.0f) * newInfo.GetDamage());

	return BaseClass::OnTakeDamage(newInfo);
}

void CCoven_Turret::ToggleLight(bool bToggle)
{
	if (bToggle)
	{
		Vector vecMuzzle, vecMuzzleDir;
		UpdateMuzzleMatrix();
		MatrixGetColumn(m_muzzleToWorld, 0, vecMuzzleDir);
		MatrixGetColumn(m_muzzleToWorld, 3, vecMuzzle);
		Vector startPos = vecMuzzle - vecMuzzleDir * 3.6f + Vector(0, -0.5, 1.2);

		m_hLightGlow->SetAbsOrigin(startPos);
		m_hLightGlow->SetColor(255, 255, 255);
		m_hLightGlow->SetBrightness(180, 0.1f);
		m_hLightGlow->SetScale(0.4f, 0.1f);
		m_pFlashlightBeam->TurnOff(); //a dev might have brain damage.
		EmitSound("HL2Player.FlashlightOn");
		AddEffects(EF_DIMLIGHT);
	}
	else
	{
		m_hLightGlow->SetColor(255, 255, 255);
		m_hLightGlow->SetScale(0.1f, 0.1f);
		m_hLightGlow->SetBrightness(0, 0.1f);
		m_pFlashlightBeam->TurnOn(); //a dev might have brain damage.
		EmitSound("HL2Player.FlashlightOff");
		RemoveEffects(EF_DIMLIGHT);
	}
}

void CCoven_Turret::TurnOnLight(void)
{
	if (!LightIsOn())
		ToggleLight(true);
}

void CCoven_Turret::TurnOffLight(void)
{
	if (LightIsOn())
		ToggleLight(false);
}

bool CCoven_Turret::LightIsOn(void)
{
	return m_pFlashlightBeam != NULL && m_hLightGlow != NULL && (m_pFlashlightBeam->GetEffects() & EF_NODRAW) == 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCoven_Turret::SpinUp(void)
{
	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCoven_Turret::SpinDown(void)
{
	
}

void CCoven_Turret::OnPhysGunDrop(CBasePlayer *pPhysGunUser, PhysGunDrop_t Reason)
{
	AdjustControllerAxis();
	BaseClass::OnPhysGunDrop(pPhysGunUser, Reason);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pVictim - 
// Output : float
//-----------------------------------------------------------------------------
float CCoven_Turret::GetAttackDamageScale(CBaseEntity *pVictim)
{
	return BaseClass::GetAttackDamageScale(pVictim);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
Vector CCoven_Turret::GetAttackSpread(CBaseCombatWeapon *pWeapon, CBaseEntity *pTarget)
{
	return VECTOR_CONE_10DEGREES;
}

//------------------------------------------------------------------------------
// Do we have a physics attacker?
//------------------------------------------------------------------------------
CBasePlayer *CCoven_Turret::HasPhysicsAttacker(float dt)
{
	// If the player is holding me now, or I've been recently thrown
	// then return a pointer to that player
	if (IsHeldByPhyscannon() || (gpGlobals->curtime - dt <= m_flLastPhysicsInfluenceTime))
	{
		return m_hPhysicsAttacker;
	}
	return NULL;
}
//-----------------------------------------------------------------------------
// Purpose: Draw any debug text overlays
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CCoven_Turret::DrawDebugTextOverlays(void)
{
	int text_offset = BaseClass::DrawDebugTextOverlays();

	if (m_debugOverlays & OVERLAY_TEXT_BIT)
	{
		if (VPhysicsGetObject())
		{
			char tempstr[512];
			Q_snprintf(tempstr, sizeof(tempstr), "Mass: %.2f kg / %.2f lb (%s)", VPhysicsGetObject()->GetMass(), kg2lbs(VPhysicsGetObject()->GetMass()), GetMassEquivalent(VPhysicsGetObject()->GetMass()));
			EntityText(text_offset, tempstr, 0);
			text_offset++;
		}
	}

	return text_offset;
}

bool CCoven_Turret::UpdateMuzzleMatrix()
{
	if (gpGlobals->tickcount != m_muzzleToWorldTick)
	{
		m_muzzleToWorldTick = gpGlobals->tickcount;
		GetAttachment(m_iMuzzleAttachment, m_muzzleToWorld);
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: We override this code because otherwise we start to move into the
//			tricky realm of player avoidance.  Since we don't go through the
//			normal NPC thinking but we ARE an NPC (...) we miss a bunch of 
//			book keeping.  This means we can become invisible and then never
//			reappear.
//-----------------------------------------------------------------------------
void CCoven_Turret::PlayerPenetratingVPhysics(void)
{
	// We don't care!
}

void CCoven_Turret::DoSelfDestructEffects(float percentage)
{
	// Figure out what our beep pitch will be
	float flBeepPitch = COVEN_SELF_DESTRUCT_BEEP_MIN_PITCH + ((COVEN_SELF_DESTRUCT_BEEP_MAX_PITCH - COVEN_SELF_DESTRUCT_BEEP_MIN_PITCH) * percentage);

	StopSound("NPC_FloorTurret.Alarm");

	// Play the beep
	CPASAttenuationFilter filter(this, "NPC_FloorTurret.Alarm");
	EmitSound_t params;
	params.m_pSoundName = "NPC_FloorTurret.Alarm";
	params.m_nPitch = floor(flBeepPitch);
	params.m_nFlags = SND_CHANGE_PITCH;
	EmitSound(filter, entindex(), params);

	// Flash our eye
	SetEyeState(TURRET_EYE_ALARM);

	// Randomly twitch
	m_vecGoalAngles.x = GetAbsAngles().x + random->RandomFloat(-60 * percentage, 60 * percentage);
	m_vecGoalAngles.y = GetAbsAngles().y + random->RandomFloat(-60 * percentage, 60 * percentage);
}

//-----------------------------------------------------------------------------
// Purpose: The countdown to destruction!
//-----------------------------------------------------------------------------
void CCoven_Turret::SelfDestructThink(void)
{
	BaseClass::SelfDestructThink();

	UpdateFacing();
}

//-----------------------------------------------------------------------------
// Purpose: Make us explode
//-----------------------------------------------------------------------------
void CCoven_Turret::SelfDestruct(void)
{
	// Ka-boom!
	TurnOffLight();

	if (OnSide())
		ComputeExtents(0.5f);

	BaseClass::SelfDestruct();
	// Create the dust effect in place
	//BB: this is a bug... this is from EP2... but looks terrible anyways
	/*m_hFizzleEffect = (CParticleSystem *)CreateEntityByName("info_particle_system");
	if (m_hFizzleEffect != NULL)
	{
		Vector vecUp;
		GetVectors(NULL, NULL, &vecUp);

		// Setup our basic parameters
		m_hFizzleEffect->KeyValue("start_active", "1");
		m_hFizzleEffect->KeyValue("effect_name", "explosion_turret_fizzle");
		m_hFizzleEffect->SetParent(this);
		m_hFizzleEffect->SetAbsOrigin(WorldSpaceCenter() + (vecUp * 12.0f));
		DispatchSpawn(m_hFizzleEffect);
		m_hFizzleEffect->Activate();
	}*/
}
