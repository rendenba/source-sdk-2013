#ifndef COVEN_TURRET_H
#define COVEN_TURRET_H
#ifdef _WIN32
#pragma once
#endif

#include "covenbuilding.h"
#include "particle_system.h"
#include "hl2mp_player.h"

//Turret states
enum covenTurretState_e
{
	TURRET_SEARCHING,
	TURRET_AUTO_SEARCHING,
	TURRET_ACTIVE,
	TURRET_SUPPRESSING,
	TURRET_DEPLOYING,
	TURRET_RETIRING,
	TURRET_TIPPED,
	TURRET_SELF_DESTRUCTING,

	TURRET_STATE_TOTAL
};

//Eye states
enum covenEyeState_t
{
	TURRET_EYE_SEE_TARGET,			//Sees the target, bright and big
	TURRET_EYE_SEEKING_TARGET,		//Looking for a target, blinking (bright)
	TURRET_EYE_DORMANT,				//Not active
	TURRET_EYE_DEAD,				//Completely invisible
	TURRET_EYE_DISABLED,			//Turned off, must be reactivated before it'll deploy again (completely invisible)
	TURRET_EYE_ALARM,				// On side, but warning player to pick it back up
};

//Spawnflags
// BUG: These all stomp Base NPC spawnflags. Any Base NPC code called by this
//		this class may have undesired side effects due to these being set.
#define SF_COVEN_TURRET_AUTOACTIVATE			0x00000020
#define SF_COVEN_TURRET_STARTINACTIVE			0x00000040
#define SF_COVEN_TURRET_FASTRETIRE				0x00000080
#define SF_COVEN_TURRET_OUT_OF_AMMO				0x00000100
#define SF_COVEN_TURRET_CITIZEN					0x00000200	// Citizen modified turret
#define SF_COVEN_TURRET_OUT_OF_ENERGY			0x00000400

#define	COVEN_TURRET_MIN_SPIN_DOWN				1.0f

#define	LASER_BEAM_SPRITE						"sprites/light_glow01.vmt"//"sprites/glow01.vmt"

#define	COVEN_TURRET_MODEL						"models/combine_turrets/floor_turret.mdl"
#define	COVEN_TURRET_MODEL_CITIZEN				"models/combine_turrets/citizen_turret.mdl"
#define COVEN_TURRET_GLOW_SPRITE				"sprites/glow1.vmt"
#define	COVEN_TURRET_RANGE						1200
#define	COVEN_TURRET_LIGHT_RANGE				350
#define	COVEN_TURRET_MAX_WAIT					5
#define COVEN_TURRET_SHORT_WAIT					2.0		// Used for FAST_RETIRE spawnflag
#define	COVEN_TURRET_PING_TIME					1.0f	//LPB!!

//Aiming variables
#define	COVEN_TURRET_MAX_NOHARM_PERIOD		0.0f
#define	COVEN_TURRET_MAX_GRACE_PERIOD		3.0f

class CBeam;
class CSprite;

//-----------------------------------------------------------------------------
// Purpose: Floor turret
//-----------------------------------------------------------------------------
class CCoven_Turret : public CCovenBuilding
{
	DECLARE_CLASS(CCoven_Turret, CCovenBuilding);
public:

	CCoven_Turret(void);

	//BB: Coven things!
	int				m_iLastPlayerCheck;

	//BB: this will need to get networked soon
	int				m_iAmmo;
	int				m_iMaxAmmo;
	int				m_iEnergy;
	int				m_iMaxEnergy;
	bool			bTipped;

	void			ShootTimerUpdate();

	virtual float	BuildTime(void) { return 2.2f; };


	void					RemoveEnemy();
	void					SetEnemy(CBaseCombatCharacter *pEnemy);
	virtual CBaseEntity		*GetEnemy(void) { return m_hEnemy; };
	virtual void			Precache(void);
	virtual void			Spawn(void);
	virtual void			Activate(void);
	virtual void			UpdateOnRemove(void);
	virtual int				OnTakeDamage(const CTakeDamageInfo &info);
	virtual void			PlayerPenetratingVPhysics(void);
	virtual int				VPhysicsTakeDamage(const CTakeDamageInfo &info);
	virtual void			OnBuildingComplete(void);
	virtual bool			CheckLevel(void);
	virtual BuildingType const	MyType() { return BUILDING_TURRET; };
	virtual int				GetAmmo(int index);
	virtual int				GetMaxAmmo(int index);

	const char *GetTracerType(void) { return "AR2Tracer"; }

	bool	ShouldSavePhysics() { return true; }

	bool	HandleInteraction(int interactionType, void *data, CBaseCombatCharacter *sourceEnt);

	// Think functions
	virtual void	Retire(void);
	virtual void	Deploy(void);
	virtual void	ActiveThink(void);
	virtual void	SearchThink(void);
	virtual void	AutoSearchThink(void);
	virtual void	TippedThink(void);
	virtual void	InactiveThink(void);
	virtual void	SuppressThink(void);
	virtual void	DisabledThink(void);
	virtual void	SelfDestructThink(void);
	virtual void	HackFindEnemy(void);
	virtual void	SelfDestruct(void);

	virtual float	GetAttackDamageScale(CBaseEntity *pVictim);
	virtual Vector	GetAttackSpread(CBaseCombatWeapon *pWeapon, CBaseEntity *pTarget);

	// Do we have a physics attacker?
	CBasePlayer *HasPhysicsAttacker(float dt);
	bool IsHeldByPhyscannon()	{ return VPhysicsGetObject() && (VPhysicsGetObject()->GetGameFlags() & FVPHYSICS_PLAYER_HELD); }

	// Inputs
	void	InputToggle(inputdata_t &inputdata);
	void	InputEnable(inputdata_t &inputdata);
	void	InputDisable(inputdata_t &inputdata);
	void	InputDepleteAmmo(inputdata_t &inputdata);
	void	InputRestoreAmmo(inputdata_t &inputdata);

	float	MaxYawSpeed(void);

	Vector EyePosition(void)
	{
		UpdateMuzzleMatrix();

		Vector vecOrigin;
		MatrixGetColumn(m_muzzleToWorld, 3, vecOrigin);

		Vector vecForward;
		MatrixGetColumn(m_muzzleToWorld, 0, vecForward);

		// Note: We back up into the model to avoid an edge case where the eyes clip out of the world and
		//		 cause problems with the PVS calculations -- jdw

		vecOrigin -= vecForward * 8.0f;

		return vecOrigin;
	}

	virtual const Vector GetPlayerMidPoint(void) const;

	Vector	EyeOffset(Activity nActivity) { return Vector(0, 0, 58); }

	// Restore the turret to working operation after falling over
	void	ReturnToLife(void);

	int		DrawDebugTextOverlays(void);

	static float	fMaxTipControllerVelocity;
	static float	fMaxTipControllerAngularVelocity;

private:
		CHandle<CBaseCombatCharacter>		m_hEnemy;
		int									m_nHaloSprite;
		void								CreateEffects(void);

protected:
	virtual bool	PreThink(covenTurretState_e state);
	virtual void	Shoot(const Vector &vecSrc, const Vector &vecDirToEnemy, bool bStrict = false);
	virtual void	SetEyeState(covenEyeState_t state);
	void			Ping(void);
	virtual void	Enable(void);
	virtual void	Disable(void);
	void			SpinUp(void);
	void			SpinDown(void);
	void			ToggleLight(bool bToggle);
	void			TurnOnLight(void);
	void			TurnOffLight(void);
	bool			LightIsOn(void);

	virtual bool	OnSide(void);
	virtual void	DoSelfDestructEffects(float percentage);

	bool	IsCitizenTurret(void) { return HasSpawnFlags(SF_COVEN_TURRET_CITIZEN); };
	bool	UpdateFacing(void);
	void	DryFire(void);
	void	UpdateMuzzleMatrix();

	matrix3x4_t m_muzzleToWorld;
	int		m_muzzleToWorldTick;
	int		m_iAmmoType;

	bool	m_bActive;		//Denotes the turret is deployed and looking for targets
	bool	m_bBlinkState;
	bool	m_bNoAlarmSounds;

	float	m_flShotTime;
	float	m_flLastSight;
	float	m_flThrashTime;
	float	m_flNextActivateSoundTime;
	bool	m_bCarriedByPlayer;
	bool	m_bUseCarryAngles;
	float	m_flPlayerDropTime;
	int		m_iKeySkin;

	QAngle	m_vecGoalAngles;

	int						m_iEyeAttachment;
	int						m_iMuzzleAttachment;
	covenEyeState_t			m_iEyeState;
	CHandle<CSprite>		m_hEyeGlow;
	CHandle<CSprite>		m_hLightGlow;
	CHandle<CBeam>			m_pFlashlightBeam;

	Vector	m_vecEnemyLKP;

	// physics influence
	CHandle<CBasePlayer>	m_hPhysicsAttacker;
	float					m_flLastPhysicsInfluenceTime;

	static const char		*m_pShotSounds[];

	COutputEvent m_OnDeploy;
	COutputEvent m_OnRetire;
	COutputEvent m_OnTipped;

	HSOUNDSCRIPTHANDLE			m_ShotSounds;

	DECLARE_DATADESC();
};

#endif