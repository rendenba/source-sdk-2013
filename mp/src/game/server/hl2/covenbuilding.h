#ifndef COVENBUILDING_H
#define COVENBUILDING_H
#ifdef _WIN32
#pragma once
#endif

#include "player_pickup.h"
#include "particle_system.h"
#include "hl2_player.h"
#include "ai_basenpc.h"

#define SF_COVENBUILDING_INERT				(1 << 0) //doesn't take input "damage" or damage

#define COVEN_SELF_DESTRUCT_BEEP_MIN_DELAY		0.1f
#define COVEN_SELF_DESTRUCT_BEEP_MAX_DELAY		0.75f
#define COVEN_SELF_DESTRUCT_BEEP_MIN_PITCH		100.0f
#define COVEN_SELF_DESTRUCT_BEEP_MAX_PITCH		225.0f

class CCovenBuildingTipController;

enum BuildingType
{
	BUILDING_DEFAULT,
	BUILDING_AMMOCRATE,
	BUILDING_TURRET
};

class CCovenBuilding : public CBaseCombatCharacter, public CDefaultPlayerPickupVPhysics
{
public:
	DECLARE_CLASS(CCovenBuilding, CBaseCombatCharacter);
	
	CCovenBuilding();

	virtual void	Spawn(void);
	virtual void	Precache(void);
	bool			CreateVPhysics(void);

	virtual void	InputKill(inputdata_t &data);
	virtual int		ObjectCaps(void) { return (BaseClass::ObjectCaps() | FCAP_IMPULSE_USE); };
	virtual void	Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	virtual void	UpdateOnRemove(void);
	Activity		GetActivity(void) { return m_Activity; };
	void			SetActivity(Activity NewActivity);
	void			ResetActivity(void) { m_Activity = ACT_RESET; };
	virtual bool	CanBecomeServerRagdoll(void) { return false; };
	virtual bool	CanBecomeRagdoll(void) { return false; };
	//virtual bool	BecomeRagdollOnClient(const Vector &force) { return false; };
	virtual void	Event_Killed(const CTakeDamageInfo &info);

	virtual bool	PreThink(void);
	virtual void	BuildingThink(void);
	virtual void	SelfDestructThink(void);

	virtual float	BuildTime(void) { return 1.0f; };
	virtual float	SelfDestructTime(void) { return 4.0f; };
	bool			IsDoneBuilding(void) { return m_flBuildTime == 0.0f || gpGlobals->curtime >= m_flBuildTime; };
	virtual void	OnBuildingComplete(void);
	virtual void	Enable(void) { m_bEnabled = true; };
	virtual void	Disable(void) { m_bEnabled = false; };
	virtual void	Detonate(void);
	virtual void	SelfDestruct(void);
	virtual void	NotifyOwner(void);
	virtual int		GetAmmo(int index) { return -1; };
	virtual int		GetMaxAmmo(int index) { return -1; };

	// Player pickup
	virtual void	OnPhysGunPickup(CBasePlayer *pPhysGunUser, PhysGunPickup_t reason);
	virtual void	OnPhysGunDrop(CBasePlayer *pPhysGunUser, PhysGunDrop_t Reason);
	virtual bool	HasPreferredCarryAnglesForPlayer(CBasePlayer *pPlayer);
	virtual QAngle	PreferredCarryAngles(void);
	virtual bool	OnAttemptPhysGunPickup(CBasePlayer *pPhysGunUser, PhysGunPickup_t reason);
	bool			IsBeingCarriedByPlayer(void) { return m_bCarriedByPlayer; };
	bool			WasJustDroppedByPlayer(void);

	virtual void	Activate(void);
	virtual bool	IsABuilding(void) const { return true; };

	int				BloodColor(void) { return DONT_BLEED; };

	void			EnableUprightController(void);
	void			DisableUprightController(void);
	void			SuspendUprightController(float duration);

	virtual int		OnTakeDamage(const CTakeDamageInfo &info);

	virtual bool	CheckLevel(void);

	CHandle<CHL2_Player>	mOwner;
	int m_iLevel;
	int m_iXP;
	const int m_iMaxXP = 200;

	virtual float	MaxTipControllerVelocity() { return 300.0f * 300.0f; };
	virtual float	MaxTipControllerAngularVelocity() { return 90.0f * 90.0f; };

	virtual BuildingType MyType() { return BUILDING_DEFAULT; };

private:
	CHandle<CCovenBuildingTipController>	m_pMotionController;
	Activity								m_Activity;				// Current animation state
	void									ResolveActivityToSequence(Activity NewActivity, int &iSequence);
	int										m_nSequence;
	void									SetActivityAndSequence(Activity NewActivity, int iSequence);
	void									Build(void);
	void									Spark(float height, int mag, int length);
	float									m_flBottom; //to get around buildings with legs
	float									m_flTop;

protected:

	float	m_flBuildTime;
	float	m_flDestructTime;
	bool	m_bSelfDestructing;	// Going to blow up
	float	m_flPingTime;
	int		s_nExplosionTexture;

	bool	m_bCarriedByPlayer;
	float	m_flPlayerDropTime;
	bool	m_bUseCarryAngles;

	void			ComputeExtents(float scaleFactor = 1.0f);
	virtual void	DoSelfDestructEffects(float percentage);

	// physics influence
	CHandle<CBasePlayer>	m_hPhysicsAttacker;
	float					m_flLastPhysicsInfluenceTime;
	COutputEvent m_OnPhysGunPickup;
	COutputEvent m_OnPhysGunDrop;

	COutputEvent	m_OnUsed;

	int					m_poseAim_Pitch;
	int					m_poseAim_Yaw;
	int					m_poseMove_Yaw;
	bool				m_bEnabled;		//Denotes whether the turret is able to deploy or not

	DECLARE_DATADESC();
};

//
// Tip controller
//

class CCovenBuildingTipController : public CPointEntity, public IMotionEvent
{
	DECLARE_CLASS(CCovenBuildingTipController, CPointEntity);
	DECLARE_DATADESC();

public:

	~CCovenBuildingTipController(void);
	void Spawn(void);
	void Activate(void);
	void Enable(bool state = true);
	void Suspend(float time);
	float SuspendedTill(void);

	bool Enabled(void);

	static CCovenBuildingTipController	*CreateTipController(CCovenBuilding *pOwner)
	{
		if (pOwner == NULL)
			return NULL;

		CCovenBuildingTipController *pController = (CCovenBuildingTipController *)Create("covenbuilding_tipcontroller", pOwner->GetAbsOrigin(), pOwner->GetAbsAngles());

		if (pController != NULL)
		{
			pController->m_pParentBuilding = pOwner;
		}

		return pController;
	}

	// IMotionEvent
	virtual simresult_e	Simulate(IPhysicsMotionController *pController, IPhysicsObject *pObject, float deltaTime, Vector &linear, AngularImpulse &angular);

private:
	bool						m_bEnabled;
	float						m_flSuspendTime;
	Vector						m_worldGoalAxis;
	Vector						m_localTestAxis;
	IPhysicsMotionController	*m_pController;
	float						m_angularLimit;
	CCovenBuilding				*m_pParentBuilding;
};

inline CCovenBuilding *ToCovenBuilding(CBaseEntity *pEntity)
{
	if (!pEntity)
		return NULL;

	return dynamic_cast<CCovenBuilding*>(pEntity);
}

#endif