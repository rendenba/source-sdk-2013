//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#ifndef HL2MP_PLAYER_H
#define HL2MP_PLAYER_H
#pragma once

class CHL2MP_Player;

#include "basemultiplayerplayer.h"
#include "hl2_playerlocaldata.h"
#include "hl2_player.h"
#include "simtimer.h"
#include "soundenvelope.h"
#include "hl2mp_player_shared.h"
#include "hl2mp_gamerules.h"
#include "utldict.h"
#include "rope.h"

//=============================================================================
// >> HL2MP_Player
//=============================================================================
class CHL2MPPlayerStateInfo
{
public:
	HL2MPPlayerState m_iPlayerState;
	const char *m_pStateName;

	void (CHL2MP_Player::*pfnEnterState)();	// Init and deinit the state.
	void (CHL2MP_Player::*pfnLeaveState)();

	void (CHL2MP_Player::*pfnPreThink)();	// Do a PreThink() in this state.
};

//BB: fuuuuuuuuuu....
enum GoreChargeState
{
	GORELOCK_NONE = 0,
	GORELOCK_CHARGING,
	GORELOCK_CHARGING_IN_UVLIGHT
};

class CHL2MP_Player : public CHL2_Player
{
public:
	DECLARE_CLASS( CHL2MP_Player, CHL2_Player );

	CHL2MP_Player();
	~CHL2MP_Player( void );
	
	static CHL2MP_Player *CreatePlayer( const char *className, edict_t *ed )
	{
		CHL2MP_Player::s_PlayerEdict = ed;
		return (CHL2MP_Player*)CreateEntityByName( className );
	}

	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	virtual bool LevelUp(int lvls, bool bBoostStats = false, bool bSound = false, bool bAutoLevel = false, bool bResetHP = false, bool bEffect = false);
	int XPForKill(CHL2MP_Player *pAttacker);

	virtual bool	UseOverride();
	virtual void	DestroyAllBuildings(void);
	virtual void	CleanUpGrapplingHook(void);
	virtual void	Precache( void );
	virtual void	Spawn( void );
	virtual void	PostThink( void );
	virtual void	PreThink( void );
	virtual void	PlayerDeathThink( void );
	virtual void	SetAnimation( PLAYER_ANIM playerAnim );
	virtual bool	HandleCommand_JoinTeam( int team );
	virtual bool	HandleCommand_SelectClass( int select );
	virtual bool	ClientCommand( const CCommand &args );
	virtual void	CreateViewModel( int viewmodelindex = 0 );
	virtual bool	BecomeRagdollOnClient( const Vector &force );
	virtual void	Event_Killed( const CTakeDamageInfo &info );
	bool			DropItem(void);
	bool			CovenStatusDamageHandle(CTakeDamageInfo &info, int iDmgType, CovenStatus_t iStatus); //BB: handle packaged damage effects like SLOW and WEAKNESS
	virtual int		OnTakeDamage( const CTakeDamageInfo &inputInfo );
	virtual int		OnTakeDamage_Alive( const CTakeDamageInfo &inputInfo );
	virtual bool	WantsLagCompensationOnEntity( const CBasePlayer *pPlayer, const CUserCmd *pCmd, const CBitVec<MAX_EDICTS> *pEntityTransmitBits ) const;
	virtual void	FireBullets ( const FireBulletsInfo_t &info );
	virtual bool	Weapon_Switch( CBaseCombatWeapon *pWeapon, int viewmodelindex = 0);
	virtual bool	BumpWeapon( CBaseCombatWeapon *pWeapon );
	virtual void	ChangeTeam( int iTeam );
	virtual void	PickupObject ( CBaseEntity *pObject, bool bLimitMassAndSize );
	virtual void	PlayStepSound( Vector &vecOrigin, surfacedata_t *psurface, float fvol, bool force );
	virtual void	Weapon_Drop( CBaseCombatWeapon *pWeapon, const Vector *pvecTarget = NULL, const Vector *pVelocity = NULL );
	virtual void	UpdateOnRemove( void );
	virtual void	DeathSound( const CTakeDamageInfo &info );
	virtual float	Feed(int iIndex);
	bool			CreateDeathBox(void);
	bool			HasDroppableItems(void);
	virtual CBaseEntity* EntSelectSpawnPoint(void);
		
	int FlashlightIsOn( void );
	void FlashlightTurnOn( void );
	void FlashlightTurnOff( void );
	void	PrecacheFootStepSounds( void );
	bool	ValidatePlayerModel( const char *pModel );

	QAngle GetAnimEyeAngles( void ) { return m_angEyeAngles.Get(); }

	Vector GetAttackSpread( CBaseCombatWeapon *pWeapon, CBaseEntity *pTarget = NULL );

	void CheatImpulseCommands( int iImpulse );
	void CreateRagdollEntity( int damagetype );
	void GiveAllItems( void );
	void GiveDefaultItems( void );
	int CovenGiveWeaponAmmo(float flAmount); //Amount is 0.00-1.00 percentage of max ammo
	int CovenGiveAmmo(float flAmount, int iMin = 1, float fCrateLevel = 1.0f); //Amount is 0.00-1.00 percentage of max ammo

	void NoteWeaponFired( void );

	void ResetAnimation( void );
	void SetPlayerModel( void );
	void SetPlayerTeamModel( void );
	Activity TranslateTeamActivity( Activity ActToTranslate );
	
	float GetNextModelChangeTime( void ) { return m_flNextModelChangeTime; }
	float GetNextTeamChangeTime( void ) { return m_flNextTeamChangeTime; }
	void  PickDefaultSpawnTeam( void );
	void  SetupPlayerSoundsByModel( const char *pModelName );
	const char *GetPlayerModelSoundPrefix( void );
	int	  GetPlayerModelType( void ) { return m_iPlayerSoundType;	}
	
	void  DetonateTripmines( void );

	void Reset();

	bool IsReady();
	void SetReady( bool bReady );

	void CHL2MP_Player::Touch(CBaseEntity *pOther);

	void CheckChatText( char *p, int bufsize );

	void State_Transition( HL2MPPlayerState newState );
	void State_Enter( HL2MPPlayerState newState );
	void State_Leave();
	void State_PreThink();
	CHL2MPPlayerStateInfo *State_LookupInfo( HL2MPPlayerState state );

	void State_Enter_ACTIVE();
	void State_PreThink_ACTIVE();
	void State_Enter_OBSERVER_MODE();
	void State_PreThink_OBSERVER_MODE();

	void GiveTeamXPCentered(int team, int xp, CBasePlayer *ignore);
	void GiveBuffInRadius(int iTeam, CovenStatus_t iStatus, int iMagnitude, float flDuration, float flDistance, int iClassID = 0);
	void ResetAbilities();
	int GetAbilityNumber(int keyNum);

	void Extinguish();

	//BB: thinking functions consolidated for convienience
	void DoStatusThink();
	bool DoAbilityThink();
	void DoVampirePreThink();
	void DoVampirePostThink();
	void DoSlayerPreThink();
	void DoSlayerPostThink();

	void DoRadiusAbility(CovenAbility_t iAbility, int iAbilityNum, int iLevel, CovenStatus_t iBuff, bool bSameTeam);
	void BoostStats(CovenAbility_t iReason, int iAbilityNum);
	void StartEffect(int iEffect);
	void StopEffect(int iEffect);
	void RemoveStatBoost();

	//BB: vampire helper functions
	void DoLeap(int iAbilityNum);
	void CheckGore();
	bool DoGorePhase(int iAbilityNum);
	bool DoGoreCharge();
	void BloodExplode(int iAbilityNum);
	void DoBerserk(int iAbilityNum);
	void VampireCheckRegen(float maxpercent);
	void VampireCheckResurrect();
	void VampireManageRagdoll();
	void VampireReSolidify();
	void StealthCalc();
	void DodgeHandler();
	void VampireEnergyHandler();
	bool ToggleDodge(int iAbilityNumber);
	void UnDodge();

	//BB: slayer helper functions
	void Pushback(const Vector *direction, float flMagnitude, float flPopVelocity = 320.0f, bool bDoFreeze = true);
	void PushbackThink();
	void UnleashSoul();
	void DashHandler();
	void Dash(int iAbilityNum);
	void DoInnerLight(int iAbilityNum);
	//void SlayerLightHandler(); This is handled higher up now.
	void GrappingHookHandler();
	void EnergyHandler();
	bool ToggleHaste(int iAbilityNum);
	void RevengeCheck();
	void GenerateBandage(int lev);
	bool BuildTurret(int iAbilityNum);
	bool BuildDispenser(int iAbilityNum);
	void ThrowHolywaterGrenade(int lev);
	bool AttachTripmine(int iAbilityNum);
	void CheckThrowPosition(const Vector &vecEye, Vector &vecSrc);
	void SlayerHolywaterThink();
	void SlayerBuyZoneThink();
	void GutcheckThink();
	void SlayerSoulThink();
	void SlayerVampLeapDetect();
	void Taunt();

	virtual bool StartObserverMode( int mode );
	virtual void StopObserverMode( void );

	float DamageForce( const Vector &size, float damage );


	Vector m_vecTotalBulletForce;	//Accumulator for bullet force in a single frame

	// Tracks our ragdoll entity.
	CNetworkHandle( CBaseEntity, m_hRagdoll );	// networked entity handle 

	virtual bool	CanHearAndReadChatFrom( CBasePlayer *pPlayer );

	int lastCheckedCapPoint;
	float lastCapPointTime;

	int s_nExplosionTexture;

	Vector store_loc;
	GoreChargeState gorelock;

	bool rezsound;
	float solidcooldown;

	//BB: grapping hook containers
	CHandle<CRopeKeyframe> pCovenRope;
	CovenHookState_t coven_hook_state;
	float coven_rope_constant; //extra length
	Vector coven_hook_anchor;

	CUtlVector<CItem *> medkits;

	Vector lock_ts;
	Vector lock_dash;
	float lock_vel;
	float lock_vel_rate;

	//BB: coven energies
	//int coven_detonate_power;
	int coven_soul_power;

	//BB: coven timers
	float coven_timer_damage;
	float coven_timer_regen;
	float coven_timer_feed;
	float coven_timer_leapdetectcooldown;
	float coven_timer_vstealth;
	float coven_timer_light;
	float coven_timer_detonate;
	//float coven_timer_gcheck;
	float coven_timer_soul;
	float coven_timer_holywater;
	float coven_timer_innerlight;
	float coven_timer_dash;
	float coven_timer_pushback;

	//BB: coven loadout/abil stuff
	int GetLoadout(int n);
	int SetLoadout(int n, int val);
	int GetLevelsSpent();
	bool SpendPoint(int on);
	bool SpendPointBOT(int on);
	int PointsToSpend();
	int covenLoadouts[2][COVEN_MAX_CLASSCOUNT][4];
	int covenLevelsSpent[2][COVEN_MAX_CLASSCOUNT];
		
private:

	CNetworkQAngle( m_angEyeAngles );
	CPlayerAnimState   m_PlayerAnimState;

	int m_iLastWeaponFireUsercmd;
	int m_iModelType;
	CNetworkVar( int, m_iSpawnInterpCounter );
	CNetworkVar( int, m_iPlayerSoundType );

	//BB: Variables!!!
	CNetworkVar(int, m_iLevel);

	float m_flNextModelChangeTime;
	float m_flNextTeamChangeTime;

	float m_flSlamProtectTime;	

	HL2MPPlayerState m_iPlayerState;
	CHL2MPPlayerStateInfo *m_pCurStateInfo;

	bool ShouldRunRateLimitedCommand( const CCommand &args );

	// This lets us rate limit the commands the players can execute so they don't overflow things like reliable buffers.
	CUtlDict<float,int>	m_RateLimitLastCommandTimes;

    bool m_bEnterObserver;
	bool m_bReady;

	bool coven_display_autolevel;
};

inline CHL2MP_Player *ToHL2MPPlayer( CBaseEntity *pEntity )
{
	if ( !pEntity || !pEntity->IsPlayer() )
		return NULL;

	return dynamic_cast<CHL2MP_Player*>( pEntity );
}

#endif //HL2MP_PLAYER_H
