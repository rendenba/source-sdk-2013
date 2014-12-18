//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#ifndef HL2MP_GAMERULES_H
#define HL2MP_GAMERULES_H
#pragma once

#include "gamerules.h"
#include "teamplay_gamerules.h"
#include "gamevars_shared.h"

#ifndef CLIENT_DLL
#include "hl2mp_player.h"
#include "physics_prop_ragdoll.h"
#include "filesystem.h"
#include "utlbuffer.h"
#endif

#define VEC_CROUCH_TRACE_MIN	HL2MPRules()->GetHL2MPViewVectors()->m_vCrouchTraceMin
#define VEC_CROUCH_TRACE_MAX	HL2MPRules()->GetHL2MPViewVectors()->m_vCrouchTraceMax

enum
{
	TEAM_COMBINE = 2,
	TEAM_REBELS,
};


#ifdef CLIENT_DLL
	#define CHL2MPRules C_HL2MPRules
	#define CHL2MPGameRulesProxy C_HL2MPGameRulesProxy
#endif

class CHL2MPGameRulesProxy : public CGameRulesProxy
{
public:
	DECLARE_CLASS( CHL2MPGameRulesProxy, CGameRulesProxy );
	DECLARE_NETWORKCLASS();
};

struct botnode
{
	int ID;
	CUtlVector<int> connectors;
	Vector location;
};

class HL2MPViewVectors : public CViewVectors
{
public:
	HL2MPViewVectors( 
		Vector vView,
		Vector vHullMin,
		Vector vHullMax,
		Vector vDuckHullMin,
		Vector vDuckHullMax,
		Vector vDuckView,
		Vector vObsHullMin,
		Vector vObsHullMax,
		Vector vDeadViewHeight,
		Vector vCrouchTraceMin,
		Vector vCrouchTraceMax ) :
			CViewVectors( 
				vView,
				vHullMin,
				vHullMax,
				vDuckHullMin,
				vDuckHullMax,
				vDuckView,
				vObsHullMin,
				vObsHullMax,
				vDeadViewHeight )
	{
		m_vCrouchTraceMin = vCrouchTraceMin;
		m_vCrouchTraceMax = vCrouchTraceMax;
	}

	Vector m_vCrouchTraceMin;
	Vector m_vCrouchTraceMax;	
};

class CHL2MPRules : public CTeamplayRules
{
public:
	DECLARE_CLASS( CHL2MPRules, CTeamplayRules );

#ifdef CLIENT_DLL

	DECLARE_CLIENTCLASS_NOBASE(); // This makes datatables able to access our private vars.

#else

	DECLARE_SERVERCLASS_NOBASE(); // This makes datatables able to access our private vars.
#endif
	
	CHL2MPRules();
	virtual ~CHL2MPRules();

	int TotalTeamXP(int team);
	void GiveItemXP(int team, float overridexp = 0.0f);
	void GiveItemXP_OLD(int team);
	void AddScore(int team, int score);
	bool LoadFromBuffer( char const *resourceName, CUtlBuffer &buf, IBaseFileSystem *pFileSystem, const char *pPathID );
	bool LoadFromBuffer( char const *resourceName, const char *pBuffer, IBaseFileSystem* pFileSystem, const char *pPathID = NULL );
	bool LoadCowFile( IBaseFileSystem *filesystem, const char *resourceName, const char *pathID );

	CUtlVector<CBaseEntity *> crates;
	botnode *botnet[COVEN_MAX_BOT_NODES];
	int bot_node_count;
	bool cowsloaded;
	bool cowsloadfail;

	virtual void Precache( void );
	virtual bool ShouldCollide( int collisionGroup0, int collisionGroup1 );
	virtual bool ClientCommand( CBaseEntity *pEdict, const CCommand &args );

	virtual float FlWeaponRespawnTime( CBaseCombatWeapon *pWeapon );
	virtual float FlWeaponTryRespawn( CBaseCombatWeapon *pWeapon );
	virtual Vector VecWeaponRespawnSpot( CBaseCombatWeapon *pWeapon );
	virtual int WeaponShouldRespawn( CBaseCombatWeapon *pWeapon );
	virtual void Think( void );
	virtual void CreateStandardEntities( void );
	virtual void ClientSettingsChanged( CBasePlayer *pPlayer );
	virtual int PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget );
	virtual void GoToIntermission( void );
	virtual void DeathNotice( CBasePlayer *pVictim, const CTakeDamageInfo &info );
	virtual const char *GetGameDescription( void );
	// derive this function if you mod uses encrypted weapon info files
	virtual const unsigned char *GetEncryptionKey( void ) { return (unsigned char *)"x9Ke0BY7"; }
	virtual const CViewVectors* GetViewVectors() const;
	const HL2MPViewVectors* GetHL2MPViewVectors() const;

	float GetMapRemainingTime();
	void CleanUpMap();
	void CheckRestartGame();
	void RestartGame();
	void RestartRound();
	void FreezeAll(bool unfreeze = false);
	float GetSlayerRespawnTime();
	float AverageLevel(int team, int &n);

	float covenSlayerRespawnTime;
	int covenGameState;
	float covenGameStateTimer;
	float covenFlashTimer;

	CBaseEntity *thects;
	
#ifndef CLIENT_DLL
	virtual Vector VecItemRespawnSpot( CItem *pItem );
	virtual QAngle VecItemRespawnAngles( CItem *pItem );
	virtual float	FlItemRespawnTime( CItem *pItem );
	virtual bool	CanHavePlayerItem( CBasePlayer *pPlayer, CBaseCombatWeapon *pItem );
	virtual bool FShouldSwitchWeapon( CBasePlayer *pPlayer, CBaseCombatWeapon *pWeapon );

	void	AddLevelDesignerPlacedObject( CBaseEntity *pEntity );
	void	RemoveLevelDesignerPlacedObject( CBaseEntity *pEntity );
	void	ManageObjectRelocation( void );
	void    CheckChatForReadySignal( CHL2MP_Player *pPlayer, const char *chatmsg );
	const char *GetChatFormat( bool bTeamOnly, CBasePlayer *pPlayer );

	//BB: yeah, probably should have made this into a class... hindsight.
	char cap_point_names[COVEN_MAX_CAP_POINTS][MAX_PLAYER_NAME_LENGTH];
	float cap_point_timers[COVEN_MAX_CAP_POINTS];
	int cap_point_distance[COVEN_MAX_CAP_POINTS];
	int cap_point_sightcheck[COVEN_MAX_CAP_POINTS];
	float scoreTimer;
	int last_verified_cap_point;

	void AddDoll( CBaseEntity *doll );
	void DollCollectorThink( void );
	//BB: my little doll collector
	CUtlVector<CBaseEntity *> doll_collector;

#endif
	virtual void ClientDisconnected( edict_t *pClient );

	bool CheckGameOver( void );
	bool IsIntermission( void );

	void PlayerKilled( CBasePlayer *pVictim, const CTakeDamageInfo &info );

	
	//BB: we always want to force teamplay
	bool	IsTeamplay( void ) { return true;	}
	void	CheckAllPlayersReady( void );

	virtual bool IsConnectedUserInfoChangeAllowed( CBasePlayer *pPlayer );

	CNetworkVar( int, num_cap_points );
	CNetworkArray( int, cap_point_status, COVEN_MAX_CAP_POINTS );
	CNetworkArray( float, cap_point_coords, COVEN_MAX_CAP_POINTS*3 );
	CNetworkArray( int, cap_point_state, COVEN_MAX_CAP_POINTS );

	bool cts_inplay;
	Vector cts_zone;
	Vector cts_position;
	int cts_zone_radius;
	float cts_return_timer;
	CNetworkVar(float,SpawnCTS);

	CNetworkVar (EHANDLE, cts_net);
	
private:
	
	CNetworkVar( bool, m_bTeamPlayEnabled );
	CNetworkVar( float, m_flGameStartTime );
	CUtlVector<EHANDLE> m_hRespawnableItemsAndWeapons;
	float m_tmNextPeriodicThink;
	float m_flRestartGameTime;
	bool m_bCompleteReset;
	bool m_bAwaitingReadyRestart;
	bool m_bHeardAllPlayersReady;

#ifndef CLIENT_DLL
	bool m_bChangelevelDone;
#endif
};

inline CHL2MPRules* HL2MPRules()
{
	return static_cast<CHL2MPRules*>(g_pGameRules);
}

#endif //HL2MP_GAMERULES_H
