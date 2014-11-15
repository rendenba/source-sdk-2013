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

	virtual bool LevelUp( int lvls );
	int XPForKill(CHL2MP_Player *pAttacker);
	void ResetVitals( void );

	virtual void Precache( void );
	virtual void Spawn( void );
	virtual void PostThink( void );
	virtual void PreThink( void );
	virtual void PlayerDeathThink( void );
	virtual void SetAnimation( PLAYER_ANIM playerAnim );
	virtual bool HandleCommand_JoinTeam( int team );
	virtual bool HandleCommand_SelectClass( int select );
	virtual bool ClientCommand( const CCommand &args );
	virtual void CreateViewModel( int viewmodelindex = 0 );
	virtual bool BecomeRagdollOnClient( const Vector &force );
	virtual void Event_Killed( const CTakeDamageInfo &info );
	virtual int OnTakeDamage( const CTakeDamageInfo &inputInfo );
	virtual int OnTakeDamage_Alive( const CTakeDamageInfo &inputInfo );
	virtual bool WantsLagCompensationOnEntity( const CBasePlayer *pPlayer, const CUserCmd *pCmd, const CBitVec<MAX_EDICTS> *pEntityTransmitBits ) const;
	virtual void FireBullets ( const FireBulletsInfo_t &info );
	virtual bool Weapon_Switch( CBaseCombatWeapon *pWeapon, int viewmodelindex = 0);
	virtual bool BumpWeapon( CBaseCombatWeapon *pWeapon );
	virtual void ChangeTeam( int iTeam );
	virtual void PickupObject ( CBaseEntity *pObject, bool bLimitMassAndSize );
	virtual void PlayStepSound( Vector &vecOrigin, surfacedata_t *psurface, float fvol, bool force );
	virtual void Weapon_Drop( CBaseCombatWeapon *pWeapon, const Vector *pvecTarget = NULL, const Vector *pVelocity = NULL );
	virtual void UpdateOnRemove( void );
	virtual void DeathSound( const CTakeDamageInfo &info );
	float Feed();
	virtual CBaseEntity* EntSelectSpawnPoint( void );
		
	int FlashlightIsOn( void );
	void FlashlightTurnOn( void );
	void FlashlightTurnOff( void );
	void	PrecacheFootStepSounds( void );
	bool	ValidatePlayerModel( const char *pModel );

	QAngle GetAnimEyeAngles( void ) { return m_angEyeAngles.Get(); }

	Vector GetAttackSpread( CBaseCombatWeapon *pWeapon, CBaseEntity *pTarget = NULL );

	void CheatImpulseCommands( int iImpulse );
	void CreateRagdollEntity( void );
	void GiveAllItems( void );
	void GiveDefaultItems( void );

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
	void GiveBuffInRadius(int team, int buff, int mag, float duration, float distance, int classid);
	void ResetStats();

	void Extinguish();

	//BB: thinking functions consolidated for convienience
	void DoStatusThink();
	void DoVampirePreThink();
	void DoVampirePostThink();
	void DoSlayerPreThink();
	void DoSlayerPostThink();

	//BB: vampire helper functions
	void DoLeap();
	void DoGorePhase();
	void DoGoreCharge();
	void RecalcGoreDrain();
	void DoBloodLust(int lev);
	void DoDreadScream(int lev);
	void DoBerserk(int lev);
	void DoVampireAbilityThink();
	void VampireCheckGore();
	void VampireCheckRegen();
	void VampireCheckResurrect();
	void VampireManageRagdoll();
	void VampireReSolidify();
	void VampireStealthCalc();

	//BB: slayer helper functions
	void DoBattleYell(int lev);
	void DoSheerWill(int lev);
	void DoIntimidatingShout(int lev);
	void RevengeCheck();
	void GenerateBandage();
	void ThrowHolywaterGrenade();
	bool AttachTripmine();
	void CheckThrowPosition(const Vector &vecEye, Vector &vecSrc);
	void SlayerHolywaterThink();
	void SlayerGutcheckThink();
	void DoSlayerAbilityThink();
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
	bool gorelock;

	bool rezsound;
	float solidcooldown;

	int num_trip_mines;

	Vector lock_ts;

	//BB: coven timers
	float coven_timer_damage;
	float coven_timer_regen;
	float coven_timer_feed;
	float coven_timer_leapdetectcooldown;
	float coven_timer_vstealth;
	float coven_timer_gcheck;
	float coven_timer_holywater;

	int coven_debug_nodeloc;
	int coven_debug_prevnode;

	//BB: coven loadout/abil stuff
	int GetLoadout(int n);
	int GetLevelsSpent();
	void SpendPoint(int on);
	int PointsToSpend();
	void RefreshLoadout();
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
	CNetworkVar( int, m_iLevel);
	CNetworkVar( int, m_iClass);

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
