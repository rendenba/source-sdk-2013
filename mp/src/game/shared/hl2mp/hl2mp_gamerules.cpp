//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "hl2mp_gamerules.h"
#include "viewport_panel_names.h"
#include "gameeventdefs.h"
#include <KeyValues.h>
#include "ammodef.h"

#ifdef CLIENT_DLL
	#include "c_hl2mp_player.h"
#else

	#include "eventqueue.h"
	#include "player.h"
	#include "gamerules.h"
	#include "game.h"
	#include "items.h"
	#include "entitylist.h"
	#include "mapentities.h"
	#include "in_buttons.h"
	#include <ctype.h>
	#include "voice_gamemgr.h"
	#include "iscorer.h"
	#include "hl2mp_player.h"
	#include "weapon_hl2mpbasehlmpcombatweapon.h"
	#include "team.h"
	#include "voice_gamemgr.h"
	#include "hl2mp_gameinterface.h"
	#include "hl2mp_cvars.h"

//BB: BOTS!
//#ifdef DEBUG	
	#include "hl2mp_bot_temp.h"
//#endif

extern void respawn(CBaseEntity *pEdict, bool fCopyCorpse);

extern bool FindInList( const char **pStrings, const char *pToFind );

ConVar sv_hl2mp_weapon_respawn_time( "sv_hl2mp_weapon_respawn_time", "20", FCVAR_GAMEDLL | FCVAR_NOTIFY );
ConVar sv_hl2mp_item_respawn_time( "sv_hl2mp_item_respawn_time", "30", FCVAR_GAMEDLL | FCVAR_NOTIFY );
ConVar sv_report_client_settings("sv_report_client_settings", "0", FCVAR_GAMEDLL | FCVAR_NOTIFY );

//BB: Coven ConVars
#if defined(COVEN_DEVELOPER_MODE)
ConVar sv_coven_minplayers("sv_coven_minplayers", "0", FCVAR_GAMEDLL | FCVAR_NOTIFY );//3
ConVar sv_coven_freezetime("sv_coven_freezetime", "0", FCVAR_GAMEDLL | FCVAR_NOTIFY );//5
ConVar sv_coven_usexpitems("sv_coven_usexpitems", "1", FCVAR_GAMEDLL | FCVAR_NOTIFY );
ConVar sv_coven_usects("sv_coven_usects", "1", FCVAR_GAMEDLL | FCVAR_NOTIFY );
ConVar sv_coven_warmuptime("sv_coven_warmuptime", "0", FCVAR_GAMEDLL | FCVAR_NOTIFY );//10
#else
ConVar sv_coven_minplayers("sv_coven_minplayers", "4", FCVAR_GAMEDLL | FCVAR_NOTIFY );//3
ConVar sv_coven_freezetime("sv_coven_freezetime", "5", FCVAR_GAMEDLL | FCVAR_NOTIFY );//5
ConVar sv_coven_usexpitems("sv_coven_usexpitems", "1", FCVAR_GAMEDLL | FCVAR_NOTIFY );
ConVar sv_coven_usects("sv_coven_usects", "1", FCVAR_GAMEDLL | FCVAR_NOTIFY );
ConVar sv_coven_warmuptime("sv_coven_warmuptime", "20", FCVAR_GAMEDLL | FCVAR_NOTIFY );//10
#endif

ConVar sv_coven_xp_basekill("sv_coven_xp_basekill", "40", FCVAR_GAMEDLL | FCVAR_NOTIFY );
ConVar sv_coven_xp_inckill("sv_coven_xp_inckill", "4", FCVAR_GAMEDLL | FCVAR_NOTIFY );
ConVar sv_coven_xp_diffkill("sv_coven_xp_diffkill", "12", FCVAR_GAMEDLL | FCVAR_NOTIFY );
ConVar sv_coven_xp_cappersec("sv_coven_xp_cappersec", "1.0", FCVAR_GAMEDLL | FCVAR_NOTIFY );
ConVar sv_coven_pts_cappersec("sv_coven_pts_cappersec", "0.75", FCVAR_GAMEDLL | FCVAR_NOTIFY );
ConVar sv_coven_pts_cts("sv_coven_pts_cts", "125", FCVAR_GAMEDLL | FCVAR_NOTIFY );
ConVar sv_coven_pts_item("sv_coven_pts_item", "5", FCVAR_GAMEDLL | FCVAR_NOTIFY );
ConVar sv_coven_cts_returntime("sv_coven_cts_returntime", "16", FCVAR_GAMEDLL | FCVAR_NOTIFY);

extern ConVar mp_chattime;

extern CBaseEntity	 *g_pLastCombineSpawn;
extern CBaseEntity	 *g_pLastRebelSpawn;

#define WEAPON_MAX_DISTANCE_FROM_SPAWN 64

static char temparray[256];

#endif


REGISTER_GAMERULES_CLASS( CHL2MPRules );

BEGIN_NETWORK_TABLE_NOBASE( CHL2MPRules, DT_HL2MPRules )

	#ifdef CLIENT_DLL
		RecvPropInt( RECVINFO( num_cap_points ) ),
		RecvPropBool( RECVINFO( m_bTeamPlayEnabled ) ),
		RecvPropArray3( RECVINFO_ARRAY(cap_point_status), RecvPropInt( RECVINFO(cap_point_status[0]))),
		RecvPropArray3( RECVINFO_ARRAY(cap_point_coords), RecvPropFloat( RECVINFO(cap_point_coords[0]))),
		RecvPropArray3( RECVINFO_ARRAY(cap_point_state), RecvPropInt( RECVINFO(cap_point_state[0]))),
		RecvPropEHandle( RECVINFO(cts_net) ),
		RecvPropFloat( RECVINFO(SpawnCTS) ),
	#else
		SendPropInt( SENDINFO( num_cap_points ) ),
		SendPropBool( SENDINFO( m_bTeamPlayEnabled ) ),
		SendPropArray3( SENDINFO_ARRAY3(cap_point_status), SendPropInt( SENDINFO_ARRAY(cap_point_status), 0, SPROP_VARINT | SPROP_UNSIGNED ) ),
		SendPropArray3( SENDINFO_ARRAY3(cap_point_coords), SendPropFloat( SENDINFO_ARRAY(cap_point_coords), 0, SPROP_NOSCALE ) ),
		SendPropArray3( SENDINFO_ARRAY3(cap_point_state), SendPropInt( SENDINFO_ARRAY(cap_point_state), 0, SPROP_NOSCALE ) ),
		SendPropEHandle( SENDINFO(cts_net) ),
		SendPropFloat( SENDINFO(SpawnCTS) ),
	#endif

END_NETWORK_TABLE()


LINK_ENTITY_TO_CLASS( hl2mp_gamerules, CHL2MPGameRulesProxy );
IMPLEMENT_NETWORKCLASS_ALIASED( HL2MPGameRulesProxy, DT_HL2MPGameRulesProxy )

static HL2MPViewVectors g_HL2MPViewVectors(
	Vector( 0, 0, 64 ),       //VEC_VIEW (m_vView) 
							  
	Vector(-16, -16, 0 ),	  //VEC_HULL_MIN (m_vHullMin)
	Vector( 16,  16,  72 ),	  //VEC_HULL_MAX (m_vHullMax)
							  					
	Vector(-16, -16, 0 ),	  //VEC_DUCK_HULL_MIN (m_vDuckHullMin)
	Vector( 16,  16,  36 ),	  //VEC_DUCK_HULL_MAX	(m_vDuckHullMax)
	Vector( 0, 0, 28 ),		  //VEC_DUCK_VIEW		(m_vDuckView)
							  					
	Vector(-10, -10, -10 ),	  //VEC_OBS_HULL_MIN	(m_vObsHullMin)
	Vector( 10,  10,  10 ),	  //VEC_OBS_HULL_MAX	(m_vObsHullMax)
							  					
	Vector( 0, 0, 14 ),		  //VEC_DEAD_VIEWHEIGHT (m_vDeadViewHeight)

	Vector(-16, -16, 0 ),	  //VEC_CROUCH_TRACE_MIN (m_vCrouchTraceMin)
	Vector( 16,  16,  60 )	  //VEC_CROUCH_TRACE_MAX (m_vCrouchTraceMax)
);

static const char *s_PreserveEnts[] =
{
	"ai_network",
	"ai_hint",
	"hl2mp_gamerules",
	"team_manager",
	"player_manager",
	"env_soundscape",
	"env_soundscape_proxy",
	"env_soundscape_triggerable",
	"env_sun",
	"env_wind",
	"env_fog_controller",
	"func_brush",
	"func_wall",
	"func_buyzone",
	"func_illusionary",
	"infodecal",
	"info_projecteddecal",
	"info_node",
	"info_target",
	"info_node_hint",
	"info_player_deathmatch",
	"info_player_combine",
	"info_player_rebel",
	"info_map_parameters",
	"keyframe_rope",
	"move_rope",
	"info_ladder",
	"player",
	"point_viewcontrol",
	"scene_manager",
	"shadow_control",
	"sky_camera",
	"soundent",
	"trigger_soundscape",
	"viewmodel",
	"predicted_viewmodel",
	"worldspawn",
	"point_devshot_camera",
	"item_xp_slayers",
	"item_xp_vampires",
	"item_ammo_crate",
	"item_cts",
	"", // END Marker
};
//BB: TODO: item_ammo_crate might need to come off this list once they are actually baked into maps...


#ifdef CLIENT_DLL
	void RecvProxy_HL2MPRules( const RecvProp *pProp, void **pOut, void *pData, int objectID )
	{
		CHL2MPRules *pRules = HL2MPRules();
		Assert( pRules );
		*pOut = pRules;
	}

	BEGIN_RECV_TABLE( CHL2MPGameRulesProxy, DT_HL2MPGameRulesProxy )
		RecvPropDataTable( "hl2mp_gamerules_data", 0, 0, &REFERENCE_RECV_TABLE( DT_HL2MPRules ), RecvProxy_HL2MPRules )
	END_RECV_TABLE()
#else
	void* SendProxy_HL2MPRules( const SendProp *pProp, const void *pStructBase, const void *pData, CSendProxyRecipients *pRecipients, int objectID )
	{
		CHL2MPRules *pRules = HL2MPRules();
		Assert( pRules );
		return pRules;
	}

	BEGIN_SEND_TABLE( CHL2MPGameRulesProxy, DT_HL2MPGameRulesProxy )
		SendPropDataTable( "hl2mp_gamerules_data", 0, &REFERENCE_SEND_TABLE( DT_HL2MPRules ), SendProxy_HL2MPRules )
	END_SEND_TABLE()
#endif

#ifndef CLIENT_DLL

	class CVoiceGameMgrHelper : public IVoiceGameMgrHelper
	{
	public:
		virtual bool		CanPlayerHearPlayer( CBasePlayer *pListener, CBasePlayer *pTalker, bool &bProximity )
		{
			return ( pListener->GetTeamNumber() == pTalker->GetTeamNumber() );
		}
	};
	CVoiceGameMgrHelper g_VoiceGameMgrHelper;
	IVoiceGameMgrHelper *g_pVoiceGameMgrHelper = &g_VoiceGameMgrHelper;

#endif

// NOTE: the indices here must match TEAM_TERRORIST, TEAM_CT, TEAM_SPECTATOR, etc.
char *sTeamNames[] =
{
	"Unassigned",
	"Spectator",
	"Slayers",
	"Vampires",
};

CHL2MPRules::CHL2MPRules()
{
#ifndef CLIENT_DLL
	// Create the team managers
	for ( int i = 0; i < ARRAYSIZE( sTeamNames ); i++ )
	{
		CTeam *pTeam = static_cast<CTeam*>(CreateEntityByName( "team_manager" ));
		pTeam->Init( sTeamNames[i], i );

		g_Teams.AddToTail( pTeam );
	}

	Q_memset(botnet, 0, sizeof(botnet));
	bot_node_count = 0;

	thects = NULL;
	cts_inplay = false;
	cts_return_timer = 0.0f;
	SpawnCTS = 0.0f;

	m_bTeamPlayEnabled = teamplay.GetBool();
	m_flIntermissionEndTime = 0.0f;
	m_flGameStartTime = 0;

	m_hRespawnableItemsAndWeapons.RemoveAll();
	m_tmNextPeriodicThink = 0;
	m_flRestartGameTime = 0;
	m_bCompleteReset = false;
	m_bHeardAllPlayersReady = false;
	m_bAwaitingReadyRestart = false;
	m_bChangelevelDone = false;

	cowsloaded = false;
	cowsloadfail = false;

	num_cap_points = 0;
	last_verified_cap_point = 0;
	scoreTimer = 0.0f;

	covenGameState = -1;
	covenGameStateTimer = 0.0f;
	covenFlashTimer = 0.0f;

	covenSlayerRespawnTime = 0.0f;

#endif
}

float CHL2MPRules::AverageLevel(int team, int &n)
{
	float ret = 0.0f;
#ifndef CLIENT_DLL
	n = 0;
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CHL2MP_Player *pPlayer = (CHL2MP_Player *)UTIL_PlayerByIndex( i );
		if (pPlayer && pPlayer->GetTeamNumber() == team)
		{
			ret += pPlayer->covenLevelCounter;
			n++;
		}
	}
	if (n > 0)
		ret = ret / n;
#endif
	return ret;
}

float CHL2MPRules::GetSlayerRespawnTime()
{
	float ret = 0.0f;
#ifndef CLIENT_DLL
	int n = 0;
	if (gpGlobals->curtime > covenSlayerRespawnTime)
		covenSlayerRespawnTime = 0.0f;

	if (covenSlayerRespawnTime > 0.0f)
		return covenSlayerRespawnTime;
	else
	{
		ret = gpGlobals->curtime + COVEN_RESPAWNTIME_BASE + COVEN_RESPAWNTIME_SLAYERS_MULT*AverageLevel(COVEN_TEAMID_SLAYERS, n);
		covenSlayerRespawnTime = ret;
	}
#endif
	return ret;
}

const CViewVectors* CHL2MPRules::GetViewVectors()const
{
	return &g_HL2MPViewVectors;
}

const HL2MPViewVectors* CHL2MPRules::GetHL2MPViewVectors()const
{
	return &g_HL2MPViewVectors;
}
	
CHL2MPRules::~CHL2MPRules( void )
{
#ifndef CLIENT_DLL
	// Note, don't delete each team since they are in the gEntList and will 
	// automatically be deleted from there, instead.
	g_Teams.Purge();
	for (int i = 0; i < bot_node_count; i++)
	{
		if (botnet[i])
		{
			delete botnet[i];
			botnet[i] = NULL;
		}
	}
#endif
}

bool CHL2MPRules::LoadFromBuffer( char const *resourceName, CUtlBuffer &buf, IBaseFileSystem *pFileSystem, const char *pPathID )
{
#ifndef CLIENT_DLL
	while (buf.IsValid())
	{
		buf.GetDelimitedString( GetNoEscCharConversion(), temparray, 256 );
		const char *s = temparray;
		if (!s || *s == 0 )
		{
			return false;
		}

#ifdef COVEN_DEVELOPER_MODE
		Msg("%s\n", s);
#endif

		if (Q_strcmp(s,"botnode") == 0)
		{
			int num,id;
			buf.GetDelimitedString( GetNoEscCharConversion(), temparray, 256 );
			const char *t = temparray;
			UTIL_StringToIntArray(&id, 1, t);
			float locs[3];
			buf.GetDelimitedString( GetNoEscCharConversion(), temparray, 256 );
			const char *u = temparray;
			UTIL_StringToVector(locs, u);
			botnode *node;
			node = new botnode;
			node->ID = id;
			node->location = Vector(locs[0], locs[1], locs[2]);
			buf.GetDelimitedString( GetNoEscCharConversion(), temparray, 256 );
			const char *v = temparray;
			UTIL_StringToIntArray(&num, 1, v);
			for (int i = 0; i < num; i++)
			{
				buf.GetDelimitedString( GetNoEscCharConversion(), temparray, 256 );
				const char *w = temparray;
				int n;
				UTIL_StringToIntArray(&n, 1, w);
				node->connectors.AddToTail(n);
			}
			if (node->ID < COVEN_MAX_BOT_NODES)
			{
				botnet[node->ID] = node;
				if (bot_node_count < node->ID+1)
				{
					bot_node_count = node->ID+1;
				}
			}
		}
		else if (Q_strcmp(s,"ammocrate") == 0)
		{
			buf.GetDelimitedString( GetNoEscCharConversion(), temparray, 256 );
			const char *t = temparray;
			float locs[3];
			UTIL_StringToVector(locs, t);
			CBaseEntity *ent = CreateEntityByName( "item_ammo_crate" );
			ent->SetLocalOrigin(Vector(locs[0], locs[1], locs[2]+15.0f));
			buf.GetDelimitedString( GetNoEscCharConversion(), temparray, 256 );
			const char *u = temparray;
			UTIL_StringToVector(locs, u);
			ent->SetLocalAngles(QAngle(locs[0], locs[1], locs[2]));
			crates.AddToTail(ent);
			ent->Spawn();
		}
		else if (Q_strcmp(s,"cappoint") == 0)
		{
			if (num_cap_points < COVEN_MAX_CAP_POINTS)
			{
				buf.GetDelimitedString( GetNoEscCharConversion(), temparray, 256 );
				const char *t = temparray;
				float locs[3];
				UTIL_StringToVector(locs, t);
				int index = num_cap_points*3;
				cap_point_coords.Set(index, locs[0]);
				cap_point_coords.Set(index+1, locs[1]);
				cap_point_coords.Set(index+2, locs[2]);
				cap_point_status.Set(num_cap_points, 60);
				cap_point_timers[num_cap_points] = 0.0f;
				cap_point_state.Set(num_cap_points, 0);
				buf.GetDelimitedString( GetNoEscCharConversion(), temparray, 256 );
				const char *u = temparray;
				int n;
				UTIL_StringToIntArray(&n, 1, u);
				cap_point_distance[num_cap_points] = n;
				buf.GetDelimitedString( GetNoEscCharConversion(), temparray, 256 );
				const char *v = temparray;
				Q_snprintf(cap_point_names[num_cap_points], sizeof(cap_point_names[num_cap_points]), "%s", v);
				buf.GetDelimitedString( GetNoEscCharConversion(), temparray, 256 );
				const char *w = temparray;
				int m;
				UTIL_StringToIntArray(&m, 1, w);
				cap_point_sightcheck[num_cap_points] = m;
				num_cap_points++;
			}
		}
		else if (Q_strcmp(s,"vamp_xp") == 0)
		{
			buf.GetDelimitedString( GetNoEscCharConversion(), temparray, 256 );
			const char *t = temparray;
			float locs[3];
			UTIL_StringToVector(locs, t);
			if (sv_coven_usexpitems.GetInt() > 0)
			{
				CBaseEntity *ent = CreateEntityByName( "item_xp_vampires" );
				ent->SetLocalOrigin(Vector(locs[0], locs[1], locs[2]+10.0f));
				ent->SetLocalAngles(QAngle(random->RandomInt(0,180), random->RandomInt(0,180), random->RandomInt(0,180)));
				ent->Spawn();
			}
		}
		else if (Q_strcmp(s,"slay_xp") == 0)
		{
			buf.GetDelimitedString( GetNoEscCharConversion(), temparray, 256 );
			const char *t = temparray;
			float locs[3];
			UTIL_StringToVector(locs, t);
			if (sv_coven_usexpitems.GetInt() > 0)
			{
				CBaseEntity *ent = CreateEntityByName( "item_xp_slayers" );
				ent->SetLocalOrigin(Vector(locs[0], locs[1], locs[2]+10.0f));
				ent->SetLocalAngles(QAngle(random->RandomInt(0,180), random->RandomInt(0,90), random->RandomInt(0,180)));
				ent->Spawn();
			}
		}
		else if (Q_strcmp(s,"cts") == 0)
		{
			buf.GetDelimitedString( GetNoEscCharConversion(), temparray, 256 );
			const char *t = temparray;
			float locs[3];
			UTIL_StringToVector(locs, t);
			if (sv_coven_usects.GetInt() > 0)
			{
				CBaseEntity *ent = CreateEntityByName( "item_cts" );
				cts_position = Vector(locs[0], locs[1], locs[2]+10.0f);
				ent->SetLocalOrigin(cts_position);
				ent->SetLocalAngles(QAngle(random->RandomInt(0,180), random->RandomInt(0,90), random->RandomInt(0,180)));
				ent->Spawn();
				ent->AddSpawnFlags(SF_NORESPAWN);
				thects = ent;
				cts_inplay = true;
			}
		}
		else if (Q_strcmp(s,"ctszone") == 0)
		{
			buf.GetDelimitedString( GetNoEscCharConversion(), temparray, 256 );
			const char *t = temparray;
			float locs[3];
			UTIL_StringToVector(locs, t);
			if (sv_coven_usects.GetInt() > 0)
			{
				cts_zone = Vector(locs[0], locs[1], locs[2]);
			}
			buf.GetDelimitedString( GetNoEscCharConversion(), temparray, 256 );
			const char *u = temparray;
			int n;
			UTIL_StringToIntArray(&n, 1, u);
			if (sv_coven_usects.GetInt() > 0)
			{
				cts_zone_radius = n;
			}
		}
	}
#endif
	return true;
}

void CHL2MPRules::AddScore(int team, int score)
{
#ifndef CLIENT_DLL
	CTeam *pt = GetGlobalTeam( team );
	pt->AddScore(score);
#endif
}

void CHL2MPRules::GiveItemXP(int team, float overridexp)
{
#ifndef CLIENT_DLL
	int n = 0;
	float avg = AverageLevel(team, n);
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CHL2MP_Player *pPlayer = (CHL2MP_Player *)UTIL_PlayerByIndex( i );
		if (pPlayer && pPlayer->GetTeamNumber() == team)
		{
			//float basexp = ((avg-1.0f)*COVEN_XP_INCREASE_PER_LEVEL+COVEN_MAX_XP_PER_LEVEL) / ((float)n) / COVEN_XP_ITEM_SCALE;
			float basexp = ((avg-1.0f)*COVEN_XP_INCREASE_PER_LEVEL+COVEN_MAX_XP_PER_LEVEL)/20.0f;
			//float calcxp = basexp*(1.0f+avg-pPlayer->covenLevelCounter);
			float calcxp = basexp*(1.0f+avg-pPlayer->covenLevelCounter);
			float xp = max(calcxp,basexp);
			if (overridexp > 0.0f)
				xp = overridexp;
			pPlayer->GiveXP(xp);
			//Msg("Player: %s Level: %d XP: %.02f\n", pPlayer->GetPlayerName(), pPlayer->covenLevelCounter, xp);
		}
	}
#endif
}

void CHL2MPRules::GiveItemXP_OLD(int team)
{
#ifndef CLIENT_DLL
	/*float txp = 0.0f;
	float n = 0.0f;
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CHL2MP_Player *pPlayer = (CHL2MP_Player *)UTIL_PlayerByIndex( i );
		if (pPlayer && pPlayer->GetTeamNumber() == team)
		{
			txp += pPlayer->GetTotalXP();
			n += 1.0f;
		}
	}
	
	if (n > 0.0f)
	{
		float mini = n*120.0f;
		txp = max(txp, mini);
		int avgxp = txp/n;
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CHL2MP_Player *pPlayer = (CHL2MP_Player *)UTIL_PlayerByIndex( i );
			if (pPlayer && pPlayer->GetTeamNumber() == team)
			{
				float xp = max(((1.0f/n)-(pPlayer->GetTotalXP()-avgxp)/txp)*(avgxp/COVEN_XP_ITEM_SCALE),1.0f);
				//Msg("Player: %d, %fxp\n",pPlayer->GetTotalXP(), xp);
				pPlayer->GiveXP(xp);
			}
		}
	}*/
#endif
}

int CHL2MPRules::TotalTeamXP(int team)
{
	int sum = 0;
#ifndef CLIENT_DLL
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CHL2MP_Player *pPlayer = (CHL2MP_Player *)UTIL_PlayerByIndex( i );
		if (pPlayer && pPlayer->GetTeamNumber() == team)
		{
			sum += pPlayer->GetTotalXP();
		}
	}
#endif
	return sum;
}

bool CHL2MPRules::LoadFromBuffer( char const *resourceName, const char *pBuffer, IBaseFileSystem* pFileSystem, const char *pPathID )
{
#ifndef CLIENT_DLL
	if ( !pBuffer )
		return true;

	int nLen = Q_strlen( pBuffer );
	CUtlBuffer buf( pBuffer, nLen, CUtlBuffer::READ_ONLY | CUtlBuffer::TEXT_BUFFER );
	return LoadFromBuffer( resourceName, buf, pFileSystem, pPathID );
#endif
	return false;
}

bool CHL2MPRules::LoadCowFile( IBaseFileSystem *filesystem, const char *resourceName, const char *pathID )
{
#ifndef CLIENT_DLL

	FileHandle_t f = filesystem->Open(resourceName, "rb", pathID);
	if (!f)
		return false;

	// load file into a null-terminated buffer
	int fileSize = filesystem->Size(f);
	unsigned bufSize = ((IFileSystem *)filesystem)->GetOptimalReadSize( f, fileSize + 1 );

	char *buffer = (char*)((IFileSystem *)filesystem)->AllocOptimalReadBuffer( f, bufSize );
	
	Assert(buffer);
	
	((IFileSystem *)filesystem)->ReadEx(buffer, bufSize, fileSize, f); // read into local buffer

	buffer[fileSize] = 0; // null terminate file as EOF

	filesystem->Close( f );	// close file after reading

	bool retOK = LoadFromBuffer( resourceName, buffer, filesystem );

	((IFileSystem *)filesystem)->FreeOptimalReadBuffer( buffer );

	return retOK;
#endif
	return false;
}

void CHL2MPRules::CreateStandardEntities( void )
{

#ifndef CLIENT_DLL
	// Create the entity that will send our data to the client.

	BaseClass::CreateStandardEntities();

	g_pLastCombineSpawn = NULL;
	g_pLastRebelSpawn = NULL;

#ifdef DBGFLAG_ASSERT
	CBaseEntity *pEnt = 
#endif
	CBaseEntity::Create( "hl2mp_gamerules", vec3_origin, vec3_angle );
	Assert( pEnt );
#endif
}

//=========================================================
// FlWeaponRespawnTime - what is the time in the future
// at which this weapon may spawn?
//=========================================================
float CHL2MPRules::FlWeaponRespawnTime( CBaseCombatWeapon *pWeapon )
{
#ifndef CLIENT_DLL
	if ( weaponstay.GetInt() > 0 )
	{
		// make sure it's only certain weapons
		if ( !(pWeapon->GetWeaponFlags() & ITEM_FLAG_LIMITINWORLD) )
		{
			return 0;		// weapon respawns almost instantly
		}
	}

	return sv_hl2mp_weapon_respawn_time.GetFloat();
#endif

	return 0;		// weapon respawns almost instantly
}


bool CHL2MPRules::IsIntermission( void )
{
#ifndef CLIENT_DLL
	return m_flIntermissionEndTime > gpGlobals->curtime;
#endif

	return false;
}

void CHL2MPRules::PlayerKilled( CBasePlayer *pVictim, const CTakeDamageInfo &info )
{
#ifndef CLIENT_DLL
	if ( IsIntermission() )
		return;
	BaseClass::PlayerKilled( pVictim, info );
#endif
}

void CHL2MPRules::FreezeAll(bool unfreeze)
{
#ifndef CLIENT_DLL
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );

		if (pPlayer && pPlayer->GetTeamNumber() > TEAM_SPECTATOR)
		{
			if (unfreeze)
			{
				pPlayer->RemoveFlag( FL_FROZEN );
				pPlayer->RemoveEFlags( EFL_BOT_FROZEN );
			}
			else
			{
				pPlayer->AddFlag( FL_FROZEN );
				pPlayer->AddEFlags( EFL_BOT_FROZEN );
			}
		}
	}
#endif
}

void CHL2MPRules::RestartRound()
{
#ifndef CLIENT_DLL
	doll_collector.RemoveAll();
	for (int i = 0; i < num_cap_points; i++)
	{
		cap_point_status.Set(i, 60);
		cap_point_timers[i] = 0.0f;
		cap_point_state.Set(i, 0);
	}
	GetGlobalTeam( COVEN_TEAMID_SLAYERS )->SetScore(0);
	GetGlobalTeam( COVEN_TEAMID_VAMPIRES )->SetScore(0);
	CleanUpMap();

	for (int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CHL2MP_Player *pPlayer = (CHL2MP_Player*) UTIL_PlayerByIndex( i );

		if ( !pPlayer )
			continue;

		pPlayer->RemoveAllItems(true);
		pPlayer->covenLevelCounter = 0;
		pPlayer->SetXP(0.0f);
		Q_memset(pPlayer->covenLoadouts,0,sizeof(pPlayer->covenLoadouts));
		Q_memset(pPlayer->covenLevelsSpent,0,sizeof(pPlayer->covenLevelsSpent));
		pPlayer->m_lifeState = LIFE_RESPAWNABLE;
		pPlayer->Spawn();
	}
#endif
}

void CHL2MPRules::Think( void )
{

#ifndef CLIENT_DLL
	
	CGameRules::Think();

	DollCollectorThink();

	int numVampires = 0;
	int numSlayers = 0;

	if (!cowsloaded)
	{
		char tempfile[MAX_PATH];
		Q_snprintf( tempfile, sizeof( tempfile ), "maps/%s_cows.txt", STRING(gpGlobals->mapname) );
		Msg("Loading cow file: %s\n", tempfile);
		if (!LoadCowFile( filesystem, tempfile, "GAME" ))
		{
			//either a failure to load, or simply not implemented for this map
			//setup alternate game mode for this map...
			cowsloadfail = true;
			Msg("Failed to load cow file, defaulting.\n");
		}
		else
		{
			//Successful load
			Msg("Cow file loaded!\n");
		}
		cowsloaded = true;
	}

	//BB: coven game state things!
	if (covenGameState < COVEN_GAME_STATE_WARMUP)
	{
		covenGameState++;
		covenGameStateTimer = gpGlobals->curtime + sv_coven_warmuptime.GetInt();
	}
	else if (covenGameState == COVEN_GAME_STATE_WARMUP)
	{
		if (gpGlobals->curtime > covenFlashTimer && sv_coven_warmuptime.GetInt() > 0)
		{
			covenFlashTimer = gpGlobals->curtime + 5.0f;
			UTIL_ClientPrintAll( HUD_PRINTCENTER, "Warmup..." );
		}
		if (gpGlobals->curtime > covenGameStateTimer)
		{
			covenGameState++;
			covenFlashTimer = 0.0f;
			covenGameStateTimer = gpGlobals->curtime + sv_coven_freezetime.GetInt();
			if (sv_coven_warmuptime.GetInt() > 0)
			{
				RestartRound();
				if (cts_inplay)
				{
					if (thects)
					{
						UTIL_Remove(thects);
						thects = NULL;
					}
					if (sv_coven_freezetime.GetInt() > 0)
						SpawnCTS = gpGlobals->curtime + sv_coven_freezetime.GetInt();
					else
						SpawnCTS = gpGlobals->curtime + 5.0f;
					cts_return_timer = 0.0f;
				}
			}
			if (sv_coven_freezetime.GetInt() > 0)
				FreezeAll();

		}
	}
	else if (covenGameState == COVEN_GAME_STATE_FREEZE)
	{
		if (gpGlobals->curtime > covenFlashTimer && sv_coven_freezetime.GetInt() > 0)
		{
			covenFlashTimer = gpGlobals->curtime + 25.0f;
			UTIL_ClientPrintAll( HUD_PRINTCENTER, "READY?" );
		}
		if (gpGlobals->curtime > covenGameStateTimer)
		{
			covenGameState++;
			covenFlashTimer = 0.0f;
			if (sv_coven_freezetime.GetInt() > 0 || sv_coven_warmuptime.GetInt() > 0)
				UTIL_ClientPrintAll( HUD_PRINTCENTER, "FIGHT!" );
			covenGameStateTimer = 0.0f;
			FreezeAll(true);
		}

	}

	s_caps = 0;
	v_caps = 0;
	//BB: coven cap point things!
	for (int i = 0; i < COVEN_MAX_CAP_POINTS; i++)
	{
		if (cap_point_state[i] == COVEN_TEAMID_SLAYERS)
			s_caps++;
		else if (cap_point_state[i] == COVEN_TEAMID_VAMPIRES)
			v_caps++;

		if (gpGlobals->curtime > cap_point_timers[i])
		{
			cap_point_timers[i] = gpGlobals->curtime + 0.2f;
			int n = cap_point_status.Get(i);
			if (cap_point_state[i] == COVEN_TEAMID_VAMPIRES)
			{
				if (n > 0)
					n--;
			}
			else if (cap_point_state[i] == COVEN_TEAMID_SLAYERS)
			{
				if (n < 120)
					n++;
			}
			else
			{
				if (n > 60 && n < 120)
					n--;
				else if (n < 60 && n > 0)
					n++;
			}
			cap_point_status.Set(i, n);
		}
	}

	//BB: Coven CTS Things!
	if (cts_inplay && thects == NULL && SpawnCTS > 0.0f && gpGlobals->curtime > SpawnCTS)
	{
		CBaseEntity *ent = CreateEntityByName( "item_cts" );
		ent->SetLocalOrigin(cts_position);
		ent->SetLocalAngles(QAngle(random->RandomInt(0,180), random->RandomInt(0,90), random->RandomInt(0,180)));
		ent->Spawn();
		ent->AddSpawnFlags(SF_NORESPAWN);
		thects = ent;
		thects->EmitSound( "AlyxEmp.Charge" );
		SpawnCTS = 0.0f;
	}
	if (cts_inplay && cts_return_timer > 0.0f && gpGlobals->curtime > cts_return_timer)
	{
		cts_return_timer = 0.0f;
		GetGlobalTeam( COVEN_TEAMID_VAMPIRES )->AddScore(sv_coven_pts_cts.GetFloat()/2.0f);
		GiveItemXP(COVEN_TEAMID_VAMPIRES, sv_coven_xp_basekill.GetInt());
		if (thects)
		{
			thects->EmitSound( "AlyxEmp.Charge" );
			UTIL_Remove(thects);
			thects = NULL;
			SpawnCTS = gpGlobals->curtime + 1.0f;
		}
		const char *killer_weapon_name = "cap_cts_vamp";
		IGameEvent *event = gameeventmanager->CreateEvent( "player_death" );
		if( event )
		{
			event->SetInt("userid", 0);
			event->SetInt("attacker",  0);
			event->SetString("weapon", killer_weapon_name );
			event->SetString("point", "Supplies");
			event->SetInt( "priority", 7 );
			gameeventmanager->FireEvent( event );
		}
	}
	if (cts_inplay)
	{
		cts_net.Set(thects);
	}

	//BB: Coven Player Things!
	if (sv_coven_minplayers.GetInt()*2 > gpGlobals->maxClients)
		sv_coven_minplayers.SetValue((float)floor(gpGlobals->maxClients/2.0f));

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );

		if (pPlayer)
		{
			float xp_tick = 0.0f;

			if (pPlayer->covenRespawnTimer > 0.0f && gpGlobals->curtime > pPlayer->covenRespawnTimer)
				pPlayer->Spawn();

			if (pPlayer->GetTeamNumber() == COVEN_TEAMID_SLAYERS)
			{
				xp_tick = sv_coven_xp_cappersec.GetFloat() * s_caps * 2.0f/num_cap_points;
				numSlayers++;
			}
			else if (pPlayer->GetTeamNumber() == COVEN_TEAMID_VAMPIRES)
			{
				xp_tick = sv_coven_xp_cappersec.GetFloat() * v_caps * 2.0f/num_cap_points;
				numVampires++;
			}

			if (gpGlobals->curtime > scoreTimer)
				((CHL2MP_Player *)pPlayer)->GiveXP(xp_tick);

			//BB: seriously? this is really dumb.
			if ( pPlayer->IsBot() )
			{
				/*if (!Bot_Right_Team((CHL2MP_Player *)pPlayer))
				{
					if (pPlayer->GetTeamNumber() == COVEN_TEAMID_VAMPIRES)
					{
						numSlayers++;
						numVampires--;
					}
					if (pPlayer->GetTeamNumber() == COVEN_TEAMID_SLAYERS)
					{
						numVampires++;
						numSlayers--;
					}
				}*/
				Bot_Think((CHL2MP_Player*)pPlayer);
			}
		}
	}

	//BB: add team scores and reset
	if (gpGlobals->curtime > scoreTimer)
	{
		//BB: make sure to check above if you change this formula if we want flat xp gains for scoreTimer intervals.
		scoreTimer = gpGlobals->curtime + 1.0f;
		
		int mult = 1;
		//JAM: MERCY CLAUSE
		if (s_caps == num_cap_points || v_caps == num_cap_points)
			mult = 3;//6
		if (!IsIntermission())
		{
			GetGlobalTeam( COVEN_TEAMID_SLAYERS )->AddScore(2.0f*sv_coven_pts_cappersec.GetFloat()*s_caps*mult/num_cap_points);
			GetGlobalTeam( COVEN_TEAMID_VAMPIRES )->AddScore(2.0f*sv_coven_pts_cappersec.GetFloat()*v_caps*mult/num_cap_points);
		}
	}

	//BB: add bots to make playercounts
	if (numSlayers < sv_coven_minplayers.GetInt() && !cowsloadfail)
	{
		CHL2MP_Player *pBot = (CHL2MP_Player *)BotPutInServer(false, TEAM_COMBINE);
		if (pBot)
		{
			pBot->covenClassID = random->RandomInt(1,COVEN_CLASSCOUNT_SLAYERS);
			Set_Bot_Base_Velocity(pBot);
			Bot_Think(pBot);
		}
	}
	else if (numSlayers > sv_coven_minplayers.GetInt())
	{
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
			if (pPlayer && pPlayer->IsBot() && pPlayer->GetTeamNumber() == COVEN_TEAMID_SLAYERS)
			{
				BotRemove((CHL2MP_Player *)pPlayer);
				ClientDisconnected(pPlayer->edict());
				UTIL_Remove(pPlayer);
				engine->ServerCommand( UTIL_VarArgs( "kickid %d %s\n", pPlayer->GetUserID(), "Bot Kicked" ) );
				break;
			}
		}
	}

	if (numVampires < sv_coven_minplayers.GetInt() && !cowsloadfail)
	{
		CHL2MP_Player *pBot = (CHL2MP_Player *)BotPutInServer(false, TEAM_REBELS);
		if (pBot)
		{
			pBot->covenClassID = random->RandomInt(1,COVEN_CLASSCOUNT_VAMPIRES);
			Set_Bot_Base_Velocity(pBot);
			Bot_Think(pBot);
		}
	}
	else if (numVampires > sv_coven_minplayers.GetInt())
	{
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
			if (pPlayer && pPlayer->IsBot() && pPlayer->GetTeamNumber() == COVEN_TEAMID_VAMPIRES)
			{
				BotRemove((CHL2MP_Player *)pPlayer);
				ClientDisconnected(pPlayer->edict());
				UTIL_Remove(pPlayer);
				engine->ServerCommand( UTIL_VarArgs( "kickid %d %s\n", pPlayer->GetUserID(), "Bot Kicked" ) );
				break;
			}
		}
	}

	//BB: game over stuff... TODO: combine with player stuff above for efficiencies?
	if ( g_fGameOver )   // someone else quit the game already
	{
		// check to see if we should change levels now
		if ( m_flIntermissionEndTime < gpGlobals->curtime )
		{
			if ( !m_bChangelevelDone )
			{
				ChangeLevel(); // intermission is over
				m_bChangelevelDone = true;
			}
		}

		return;
	}

//	float flTimeLimit = mp_timelimit.GetFloat() * 60;
	float flFragLimit = fraglimit.GetFloat();
	
	if ( GetMapRemainingTime() < 0 )
	{
		GoToIntermission();
		return;
	}

	if ( flFragLimit )
	{
		if( IsTeamplay() == true )
		{
			CTeam *pCombine = g_Teams[TEAM_COMBINE];
			CTeam *pRebels = g_Teams[TEAM_REBELS];

			if ( pCombine->GetScore() >= flFragLimit || pRebels->GetScore() >= flFragLimit )
			{
				GoToIntermission();
				return;
			}
		}
		else
		{
			// check if any player is over the frag limit
			for ( int i = 1; i <= gpGlobals->maxClients; i++ )
			{
				CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );

				if ( pPlayer && pPlayer->FragCount() >= flFragLimit )
				{
					GoToIntermission();
					return;
				}
			}
		}
	}

	if ( gpGlobals->curtime > m_tmNextPeriodicThink )
	{		
		CheckAllPlayersReady();
		CheckRestartGame();
		m_tmNextPeriodicThink = gpGlobals->curtime + 1.0;
	}

	if ( m_flRestartGameTime > 0.0f && m_flRestartGameTime <= gpGlobals->curtime )
	{
		RestartGame();
	}

	if( m_bAwaitingReadyRestart && m_bHeardAllPlayersReady )
	{
		UTIL_ClientPrintAll( HUD_PRINTCENTER, "All players ready. Game will restart in 5 seconds" );
		UTIL_ClientPrintAll( HUD_PRINTCONSOLE, "All players ready. Game will restart in 5 seconds" );

		m_flRestartGameTime = gpGlobals->curtime + 5;
		m_bAwaitingReadyRestart = false;
	}

	ManageObjectRelocation();

#endif
}

void CHL2MPRules::GoToIntermission( void )
{
#ifndef CLIENT_DLL
	if ( g_fGameOver )
		return;

	g_fGameOver = true;

	m_flIntermissionEndTime = gpGlobals->curtime + mp_chattime.GetInt();

	for ( int i = 0; i < MAX_PLAYERS; i++ )
	{
		CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );

		if ( !pPlayer )
			continue;

		pPlayer->ShowViewPortPanel( PANEL_SCOREBOARD );
		pPlayer->AddFlag( FL_FROZEN );
	}
#endif
	
}

bool CHL2MPRules::CheckGameOver()
{
#ifndef CLIENT_DLL
	if ( g_fGameOver )   // someone else quit the game already
	{
		// check to see if we should change levels now
		if ( m_flIntermissionEndTime < gpGlobals->curtime )
		{
			ChangeLevel(); // intermission is over			
		}

		return true;
	}
#endif

	return false;
}

// when we are within this close to running out of entities,  items 
// marked with the ITEM_FLAG_LIMITINWORLD will delay their respawn
#define ENTITY_INTOLERANCE	100

//=========================================================
// FlWeaponRespawnTime - Returns 0 if the weapon can respawn 
// now,  otherwise it returns the time at which it can try
// to spawn again.
//=========================================================
float CHL2MPRules::FlWeaponTryRespawn( CBaseCombatWeapon *pWeapon )
{
#ifndef CLIENT_DLL
	if ( pWeapon && (pWeapon->GetWeaponFlags() & ITEM_FLAG_LIMITINWORLD) )
	{
		if ( gEntList.NumberOfEntities() < (gpGlobals->maxEntities - ENTITY_INTOLERANCE) )
			return 0;

		// we're past the entity tolerance level,  so delay the respawn
		return FlWeaponRespawnTime( pWeapon );
	}
#endif
	return 0;
}

//=========================================================
// VecWeaponRespawnSpot - where should this weapon spawn?
// Some game variations may choose to randomize spawn locations
//=========================================================
Vector CHL2MPRules::VecWeaponRespawnSpot( CBaseCombatWeapon *pWeapon )
{
#ifndef CLIENT_DLL
	CWeaponHL2MPBase *pHL2Weapon = dynamic_cast< CWeaponHL2MPBase*>( pWeapon );

	if ( pHL2Weapon )
	{
		return pHL2Weapon->GetOriginalSpawnOrigin();
	}
#endif
	
	return pWeapon->GetAbsOrigin();
}

#ifndef CLIENT_DLL

CItem* IsManagedObjectAnItem( CBaseEntity *pObject )
{
	return dynamic_cast< CItem*>( pObject );
}

CWeaponHL2MPBase* IsManagedObjectAWeapon( CBaseEntity *pObject )
{
	return dynamic_cast< CWeaponHL2MPBase*>( pObject );
}

bool GetObjectsOriginalParameters( CBaseEntity *pObject, Vector &vOriginalOrigin, QAngle &vOriginalAngles )
{
	if ( CItem *pItem = IsManagedObjectAnItem( pObject ) )
	{
		if ( pItem->m_flNextResetCheckTime > gpGlobals->curtime )
			 return false;
		
		vOriginalOrigin = pItem->GetOriginalSpawnOrigin();
		vOriginalAngles = pItem->GetOriginalSpawnAngles();

		pItem->m_flNextResetCheckTime = gpGlobals->curtime + sv_hl2mp_item_respawn_time.GetFloat();
		return true;
	}
	else if ( CWeaponHL2MPBase *pWeapon = IsManagedObjectAWeapon( pObject )) 
	{
		if ( pWeapon->m_flNextResetCheckTime > gpGlobals->curtime )
			 return false;

		vOriginalOrigin = pWeapon->GetOriginalSpawnOrigin();
		vOriginalAngles = pWeapon->GetOriginalSpawnAngles();

		pWeapon->m_flNextResetCheckTime = gpGlobals->curtime + sv_hl2mp_weapon_respawn_time.GetFloat();
		return true;
	}

	return false;
}

void CHL2MPRules::ManageObjectRelocation( void )
{
	int iTotal = m_hRespawnableItemsAndWeapons.Count();

	if ( iTotal > 0 )
	{
		for ( int i = 0; i < iTotal; i++ )
		{
			CBaseEntity *pObject = m_hRespawnableItemsAndWeapons[i].Get();
			
			if ( pObject )
			{
				Vector vSpawOrigin;
				QAngle vSpawnAngles;

				if ( GetObjectsOriginalParameters( pObject, vSpawOrigin, vSpawnAngles ) == true )
				{
					float flDistanceFromSpawn = (pObject->GetAbsOrigin() - vSpawOrigin ).Length();

					if ( flDistanceFromSpawn > WEAPON_MAX_DISTANCE_FROM_SPAWN )
					{
						bool shouldReset = false;
						IPhysicsObject *pPhysics = pObject->VPhysicsGetObject();

						if ( pPhysics )
						{
							shouldReset = pPhysics->IsAsleep();
						}
						else
						{
							shouldReset = (pObject->GetFlags() & FL_ONGROUND) ? true : false;
						}

						if ( shouldReset )
						{
							pObject->Teleport( &vSpawOrigin, &vSpawnAngles, NULL );
							pObject->EmitSound( "AlyxEmp.Charge" );

							IPhysicsObject *pPhys = pObject->VPhysicsGetObject();

							if ( pPhys )
							{
								pPhys->Wake();
							}
						}
					}
				}
			}
		}
	}
}

//=========================================================
//AddLevelDesignerPlacedWeapon
//=========================================================
void CHL2MPRules::AddLevelDesignerPlacedObject( CBaseEntity *pEntity )
{
	if ( m_hRespawnableItemsAndWeapons.Find( pEntity ) == -1 )
	{
		m_hRespawnableItemsAndWeapons.AddToTail( pEntity );
	}
}

//=========================================================
//RemoveLevelDesignerPlacedWeapon
//=========================================================
void CHL2MPRules::RemoveLevelDesignerPlacedObject( CBaseEntity *pEntity )
{
	if ( m_hRespawnableItemsAndWeapons.Find( pEntity ) != -1 )
	{
		m_hRespawnableItemsAndWeapons.FindAndRemove( pEntity );
	}
}

//=========================================================
// Where should this item respawn?
// Some game variations may choose to randomize spawn locations
//=========================================================
Vector CHL2MPRules::VecItemRespawnSpot( CItem *pItem )
{
	return pItem->GetOriginalSpawnOrigin();
}

//=========================================================
// What angles should this item use to respawn?
//=========================================================
QAngle CHL2MPRules::VecItemRespawnAngles( CItem *pItem )
{
	return pItem->GetOriginalSpawnAngles();
}

//=========================================================
// At what time in the future may this Item respawn?
//=========================================================
float CHL2MPRules::FlItemRespawnTime( CItem *pItem )
{
	return sv_hl2mp_item_respawn_time.GetFloat();
}


//=========================================================
// CanHaveWeapon - returns false if the player is not allowed
// to pick up this weapon
//=========================================================
bool CHL2MPRules::CanHavePlayerItem( CBasePlayer *pPlayer, CBaseCombatWeapon *pItem )
{
	if ( weaponstay.GetInt() > 0 )
	{
		if ( pPlayer->Weapon_OwnsThisType( pItem->GetClassname(), pItem->GetSubType() ) )
			 return false;
	}

	return BaseClass::CanHavePlayerItem( pPlayer, pItem );
}

#endif

//=========================================================
// WeaponShouldRespawn - any conditions inhibiting the
// respawning of this weapon?
//=========================================================
int CHL2MPRules::WeaponShouldRespawn( CBaseCombatWeapon *pWeapon )
{
#ifndef CLIENT_DLL
	if ( pWeapon->HasSpawnFlags( SF_NORESPAWN ) )
	{
		return GR_WEAPON_RESPAWN_NO;
	}
#endif

	return GR_WEAPON_RESPAWN_YES;
}

//-----------------------------------------------------------------------------
// Purpose: Player has just left the game
//-----------------------------------------------------------------------------
void CHL2MPRules::ClientDisconnected( edict_t *pClient )
{
#ifndef CLIENT_DLL
	// Msg( "CLIENT DISCONNECTED, REMOVING FROM TEAM.\n" );

	CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance( pClient );
	if ( pPlayer )
	{
		CHL2MP_Player *play = (CHL2MP_Player *)pPlayer;
		if (play->GetTeamNumber() == COVEN_TEAMID_VAMPIRES && play->myServerRagdoll != NULL)
		{
			if (play->myServerRagdoll == play->m_hRagdoll)
				play->m_hRagdoll = NULL;
			UTIL_Remove(play->myServerRagdoll);
			play->myServerRagdoll = NULL;
		}
		// Remove the player from his team
		if ( pPlayer->GetTeam() )
		{
			pPlayer->GetTeam()->RemovePlayer( pPlayer );
		}

		
	}

	BaseClass::ClientDisconnected( pClient );

#endif
}


//=========================================================
// Deathnotice. 
//=========================================================
void CHL2MPRules::DeathNotice( CBasePlayer *pVictim, const CTakeDamageInfo &info )
{
#ifndef CLIENT_DLL
	// Work out what killed the player, and send a message to all clients about it
	const char *killer_weapon_name = "world";		// by default, the player is killed by the world
	int killer_ID = 0;

	// Find the killer & the scorer
	CBaseEntity *pInflictor = info.GetInflictor();
	CBaseEntity *pKiller = info.GetAttacker();
	CBasePlayer *pScorer = GetDeathScorer( pKiller, pInflictor );

	// Custom kill type?
	if ( info.GetDamageCustom() )
	{
		killer_weapon_name = GetDamageCustomString( info );
		if ( pScorer )
		{
			killer_ID = pScorer->GetUserID();
		}
	}
	else
	{
		// Is the killer a client?
		if ( pScorer )
		{
			killer_ID = pScorer->GetUserID();
			
			if ( pInflictor )
			{
				if ( pInflictor == pScorer )
				{
					// If the inflictor is the killer,  then it must be their current weapon doing the damage
					if ( pScorer->GetActiveWeapon() )
					{
						killer_weapon_name = pScorer->GetActiveWeapon()->GetClassname();
					}
				}
				else
				{
					killer_weapon_name = pInflictor->GetClassname();  // it's just that easy
				}
			}
		}
		else
		{
			killer_weapon_name = pInflictor->GetClassname();
		}

		// strip the NPC_* or weapon_* from the inflictor's classname
		if ( strncmp( killer_weapon_name, "weapon_", 7 ) == 0 )
		{
			killer_weapon_name += 7;
		}
		else if ( strncmp( killer_weapon_name, "npc_", 4 ) == 0 )
		{
			killer_weapon_name += 4;
		}
		else if ( strncmp( killer_weapon_name, "func_", 5 ) == 0 )
		{
			killer_weapon_name += 5;
		}
		else if ( strstr( killer_weapon_name, "physics" ) )
		{
			killer_weapon_name = "physics";
		}

		if ( strcmp( killer_weapon_name, "prop_combine_ball" ) == 0 )
		{
			killer_weapon_name = "combine_ball";
		}
		else if ( strcmp( killer_weapon_name, "grenade_ar2" ) == 0 )
		{
			killer_weapon_name = "smg1_grenade";
		}
		else if ( strcmp( killer_weapon_name, "satchel" ) == 0 || strcmp( killer_weapon_name, "tripmine" ) == 0)
		{
			killer_weapon_name = "slam";
		}
		else if ( strcmp( killer_weapon_name, "crowbar" ) == 0 )
		{
			killer_weapon_name = "claw";
		}


	}

	IGameEvent *event = gameeventmanager->CreateEvent( "player_death" );
	if( event )
	{
		event->SetInt("userid", pVictim->GetUserID() );
		event->SetInt("attacker", killer_ID );
		event->SetString("weapon", killer_weapon_name );
		event->SetInt( "priority", 7 );
		gameeventmanager->FireEvent( event );
	}
#endif

}

void CHL2MPRules::ClientSettingsChanged( CBasePlayer *pPlayer )
{
#ifndef CLIENT_DLL
	
	CHL2MP_Player *pHL2Player = ToHL2MPPlayer( pPlayer );

	if ( pHL2Player == NULL )
		return;

	const char *pCurrentModel = modelinfo->GetModelName( pPlayer->GetModel() );
	const char *szModelName = engine->GetClientConVarValue( engine->IndexOfEdict( pPlayer->edict() ), "cl_playermodel" );

	//If we're different.
	if ( stricmp( szModelName, pCurrentModel ) )
	{
		//Too soon, set the cvar back to what it was.
		//Note: this will make this function be called again
		//but since our models will match it'll just skip this whole dealio.
		if ( pHL2Player->GetNextModelChangeTime() >= gpGlobals->curtime )
		{
			char szReturnString[512];

			Q_snprintf( szReturnString, sizeof (szReturnString ), "cl_playermodel %s\n", pCurrentModel );
			engine->ClientCommand ( pHL2Player->edict(), szReturnString );

			Q_snprintf( szReturnString, sizeof( szReturnString ), "Please wait %d more seconds before trying to switch.\n", (int)(pHL2Player->GetNextModelChangeTime() - gpGlobals->curtime) );
			ClientPrint( pHL2Player, HUD_PRINTTALK, szReturnString );
			return;
		}

		if ( HL2MPRules()->IsTeamplay() == false )
		{
			pHL2Player->SetPlayerModel();

			const char *pszCurrentModelName = modelinfo->GetModelName( pHL2Player->GetModel() );

			char szReturnString[128];
			Q_snprintf( szReturnString, sizeof( szReturnString ), "Your player model is: %s\n", pszCurrentModelName );

			ClientPrint( pHL2Player, HUD_PRINTTALK, szReturnString );
		}
		else
		{
			//BB: WAT
			/*if ( Q_stristr( szModelName, "models/human") )
			{
				pHL2Player->ChangeTeam( TEAM_REBELS );
			}
			else
			{
				pHL2Player->ChangeTeam( TEAM_COMBINE );
			}*/
		}
	}
	if ( sv_report_client_settings.GetInt() == 1 )
	{
		UTIL_LogPrintf( "\"%s\" cl_cmdrate = \"%s\"\n", pHL2Player->GetPlayerName(), engine->GetClientConVarValue( pHL2Player->entindex(), "cl_cmdrate" ));
	}


	BaseClass::ClientSettingsChanged( pPlayer );
#endif
	
}

int CHL2MPRules::PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget )
{
#ifndef CLIENT_DLL
	// half life multiplay has a simple concept of Player Relationships.
	// you are either on another player's team, or you are not.
	if ( !pPlayer || !pTarget || !pTarget->IsPlayer() || IsTeamplay() == false )
		return GR_NOTTEAMMATE;

	if ( (*GetTeamID(pPlayer) != '\0') && (*GetTeamID(pTarget) != '\0') && !stricmp( GetTeamID(pPlayer), GetTeamID(pTarget) ) )
	{
		return GR_TEAMMATE;
	}
#endif

	return GR_NOTTEAMMATE;
}

const char *CHL2MPRules::GetGameDescription( void )
{ 
	//BB: always teamplay... always Coven
	if ( IsTeamplay() )
		return "Coven"; 

	return "Deathmatch"; 
} 

bool CHL2MPRules::IsConnectedUserInfoChangeAllowed( CBasePlayer *pPlayer )
{
	return true;
}
 
float CHL2MPRules::GetMapRemainingTime()
{
	// if timelimit is disabled, return 0
	if ( mp_timelimit.GetInt() <= 0 )
		return 0;

	// timelimit is in minutes

	float timeleft = (m_flGameStartTime + mp_timelimit.GetInt() * 60.0f ) - gpGlobals->curtime;

	return timeleft;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHL2MPRules::Precache( void )
{
	CBaseEntity::PrecacheScriptSound( "AlyxEmp.Charge" );
}

bool CHL2MPRules::ShouldCollide( int collisionGroup0, int collisionGroup1 )
{
	if ( collisionGroup0 > collisionGroup1 )
	{
		// swap so that lowest is always first
		V_swap(collisionGroup0,collisionGroup1);
	}

	if ( (collisionGroup0 == COLLISION_GROUP_PLAYER || collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT) &&
		collisionGroup1 == COLLISION_GROUP_WEAPON )
	{
		return false;
	}

	return BaseClass::ShouldCollide( collisionGroup0, collisionGroup1 ); 

}

bool CHL2MPRules::ClientCommand( CBaseEntity *pEdict, const CCommand &args )
{
#ifndef CLIENT_DLL
	if( BaseClass::ClientCommand( pEdict, args ) )
		return true;


	CHL2MP_Player *pPlayer = (CHL2MP_Player *) pEdict;

	if ( pPlayer->ClientCommand( args ) )
		return true;
#endif

	return false;
}

// shared ammo definition
// JAY: Trying to make a more physical bullet response
#define BULLET_MASS_GRAINS_TO_LB(grains)	(0.002285*(grains)/16.0f)
#define BULLET_MASS_GRAINS_TO_KG(grains)	lbs2kg(BULLET_MASS_GRAINS_TO_LB(grains))

// exaggerate all of the forces, but use real numbers to keep them consistent
#define BULLET_IMPULSE_EXAGGERATION			3.5
// convert a velocity in ft/sec and a mass in grains to an impulse in kg in/s
#define BULLET_IMPULSE(grains, ftpersec)	((ftpersec)*12*BULLET_MASS_GRAINS_TO_KG(grains)*BULLET_IMPULSE_EXAGGERATION)


CAmmoDef *GetAmmoDef()
{
	static CAmmoDef def;
	static bool bInitted = false;
	
	if ( !bInitted )
	{
		bInitted = true;

		def.AddAmmoType("AR2",				DMG_BULLET,					TRACER_LINE_AND_WHIZ,	0,			0,			60,			BULLET_IMPULSE(200, 1225),	0 );
		def.AddAmmoType("AR2AltFire",		DMG_DISSOLVE,				TRACER_NONE,			0,			0,			3,			0,							0 );
		def.AddAmmoType("Pistol",			DMG_BULLET,					TRACER_LINE_AND_WHIZ,	0,			0,			60,		BULLET_IMPULSE(200, 1225),	0 );
		def.AddAmmoType("SMG1",				DMG_BULLET,					TRACER_LINE_AND_WHIZ,	0,			0,			90,		BULLET_IMPULSE(200, 1225),	0 );
		def.AddAmmoType("357",				DMG_BULLET,					TRACER_LINE_AND_WHIZ,	0,			0,			18,			BULLET_IMPULSE(800, 5000),	0 );
		def.AddAmmoType("XBowBolt",			DMG_BULLET,					TRACER_LINE,			0,			0,			12,			BULLET_IMPULSE(800, 8000),	0 );
		def.AddAmmoType("Buckshot",			DMG_BULLET | DMG_BUCKSHOT,	TRACER_LINE,			0,			0,			16,			BULLET_IMPULSE(400, 1200),	0 );
		def.AddAmmoType("RPG_Round",		DMG_BURN,					TRACER_NONE,			0,			0,			3,			0,							0 );
		def.AddAmmoType("SMG1_Grenade",		DMG_BURN,					TRACER_NONE,			0,			0,			3,			0,							0 );
		def.AddAmmoType("Grenade",			DMG_BURN,					TRACER_NONE,			0,			0,			5,			0,							0 );
		def.AddAmmoType("slam",				DMG_BURN,					TRACER_NONE,			0,			0,			5,			0,							0 );
	}

	return &def;
}

#ifdef CLIENT_DLL

	ConVar cl_autowepswitch(
		"cl_autowepswitch",
		"1",
		FCVAR_ARCHIVE | FCVAR_USERINFO,
		"Automatically switch to picked up weapons (if more powerful)" );

#else

//BB: BOTS!
#ifdef DEBUG

	// Handler for the "bot" command.
	void Bot_f()
	{		
		// Look at -count.
		int count = 1;
		count = clamp( count, 1, 16 );

		int iTeam = TEAM_COMBINE;
				
		// Look at -frozen.
		bool bFrozen = false;
			
		// Ok, spawn all the bots.
		while ( --count >= 0 )
		{
			BotPutInServer( bFrozen, iTeam );
		}
	}


	ConCommand cc_Bot( "bot", Bot_f, "Add a bot.", FCVAR_NOTIFY/*FCVAR_CHEAT*/ );

#endif

#if defined(COVEN_DEVELOPER_MODE)
	//BB: xp scaling for testing or just tom foolery... set to 1 for normal
	ConVar coven_xp_scale( "coven_xp_scale", "4", FCVAR_NOTIFY | FCVAR_CHEAT );

	//BB: >0 = ignore respawn timers for testing or just tom foolery... set to 0 for normal
	ConVar coven_ignore_respawns( "coven_ignore_respawns", "0", FCVAR_NOTIFY | FCVAR_CHEAT );
#else
	//BB: xp scaling for testing or just tom foolery... set to 1 for normal
	ConVar coven_xp_scale( "coven_xp_scale", "1", FCVAR_NOTIFY | FCVAR_CHEAT );

	//BB: >0 = ignore respawn timers for testing or just tom foolery... set to 0 for normal
	ConVar coven_ignore_respawns( "coven_ignore_respawns", "0", FCVAR_NOTIFY | FCVAR_CHEAT );
#endif

	bool CHL2MPRules::FShouldSwitchWeapon( CBasePlayer *pPlayer, CBaseCombatWeapon *pWeapon )
	{		
		if ( pPlayer->GetActiveWeapon() && pPlayer->IsNetClient() )
		{
			// Player has an active item, so let's check cl_autowepswitch.
			const char *cl_autowepswitch = engine->GetClientConVarValue( engine->IndexOfEdict( pPlayer->edict() ), "cl_autowepswitch" );
			if ( cl_autowepswitch && atoi( cl_autowepswitch ) <= 0 )
			{
				return false;
			}
		}

		return BaseClass::FShouldSwitchWeapon( pPlayer, pWeapon );
	}

#endif

#ifndef CLIENT_DLL

//BB: lets collect a doll
void CHL2MPRules::AddDoll(CBaseEntity *doll)
{
	if (doll_collector.Count() > 20)
	{
		doll_collector.Remove(0);
	}
	doll_collector.AddToTail(doll);
}

//BB: deal with dolls (think)
void CHL2MPRules::DollCollectorThink()
{
	for (int i=0; i < doll_collector.Count(); i++)
	{
		CBaseEntity *temp;
		temp = doll_collector[i];
		if (temp != NULL && ((CRagdollProp *)temp)->flClearTime > 0.0f)
		{
			if (gpGlobals->curtime > ((CRagdollProp *)temp)->flClearTime)
			{
				doll_collector.Remove(i);
				UTIL_Remove(temp);
				temp = NULL;
			}
			else
			{
				if (temp->GetRenderMode() != kRenderTransTexture)
					temp->SetRenderMode( kRenderTransTexture );
				if (((CRagdollProp *)temp)->flClearTime - gpGlobals->curtime < 4.5f)
				{
					temp->SetRenderColorA(255.0f*(((CRagdollProp *)temp)->flClearTime - gpGlobals->curtime)/4.5f);
				}
			}
		}
	}
}


void CHL2MPRules::RestartGame()
{
	// bounds check
	if ( mp_timelimit.GetInt() < 0 )
	{
		mp_timelimit.SetValue( 0 );
	}
	m_flGameStartTime = gpGlobals->curtime;
	if ( !IsFinite( m_flGameStartTime.Get() ) )
	{
		Warning( "Trying to set a NaN game start time\n" );
		m_flGameStartTime.GetForModify() = 0.0f;
	}

	CleanUpMap();
	
	// now respawn all players
	for (int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CHL2MP_Player *pPlayer = (CHL2MP_Player*) UTIL_PlayerByIndex( i );

		if ( !pPlayer )
			continue;

		if ( pPlayer->GetActiveWeapon() )
		{
			pPlayer->GetActiveWeapon()->Holster();
		}
		pPlayer->RemoveAllItems( true );
		respawn( pPlayer, false );
		pPlayer->Reset();
	}

	// Respawn entities (glass, doors, etc..)

	CTeam *pRebels = GetGlobalTeam( TEAM_REBELS );
	CTeam *pCombine = GetGlobalTeam( TEAM_COMBINE );

	if ( pRebels )
	{
		pRebels->SetScore( 0 );
	}

	if ( pCombine )
	{
		pCombine->SetScore( 0 );
	}

	m_flIntermissionEndTime = 0;
	m_flRestartGameTime = 0.0;		
	m_bCompleteReset = false;

	IGameEvent * event = gameeventmanager->CreateEvent( "round_start" );
	if ( event )
	{
		event->SetInt("fraglimit", 0 );
		event->SetInt( "priority", 6 ); // HLTV event priority, not transmitted

		event->SetString("objective","DEATHMATCH");

		gameeventmanager->FireEvent( event );
	}
}

void CHL2MPRules::CleanUpMap()
{
	// Recreate all the map entities from the map data (preserving their indices),
	// then remove everything else except the players.

	// Get rid of all entities except players.
	CBaseEntity *pCur = gEntList.FirstEnt();
	while ( pCur )
	{
		CBaseHL2MPCombatWeapon *pWeapon = dynamic_cast< CBaseHL2MPCombatWeapon* >( pCur );
		// Weapons with owners don't want to be removed..
		if ( pWeapon )
		{
			if ( !pWeapon->GetPlayerOwner() )
			{
				UTIL_Remove( pCur );
			}
		}
		// remove entities that has to be restored on roundrestart (breakables etc)
		else if ( !FindInList( s_PreserveEnts, pCur->GetClassname() ) )
		{
			UTIL_Remove( pCur );
		}

		pCur = gEntList.NextEnt( pCur );
	}

	// Really remove the entities so we can have access to their slots below.
	gEntList.CleanupDeleteList();

	// Cancel all queued events, in case a func_bomb_target fired some delayed outputs that
	// could kill respawning CTs
	g_EventQueue.Clear();

	// Now reload the map entities.
	class CHL2MPMapEntityFilter : public IMapEntityFilter
	{
	public:
		virtual bool ShouldCreateEntity( const char *pClassname )
		{
			// Don't recreate the preserved entities.
			if ( !FindInList( s_PreserveEnts, pClassname ) )
			{
				return true;
			}
			else
			{
				// Increment our iterator since it's not going to call CreateNextEntity for this ent.
				if ( m_iIterator != g_MapEntityRefs.InvalidIndex() )
					m_iIterator = g_MapEntityRefs.Next( m_iIterator );

				return false;
			}
		}


		virtual CBaseEntity* CreateNextEntity( const char *pClassname )
		{
			if ( m_iIterator == g_MapEntityRefs.InvalidIndex() )
			{
				// This shouldn't be possible. When we loaded the map, it should have used 
				// CCSMapLoadEntityFilter, which should have built the g_MapEntityRefs list
				// with the same list of entities we're referring to here.
				Assert( false );
				return NULL;
			}
			else
			{
				CMapEntityRef &ref = g_MapEntityRefs[m_iIterator];
				m_iIterator = g_MapEntityRefs.Next( m_iIterator );	// Seek to the next entity.

				if ( ref.m_iEdict == -1 || engine->PEntityOfEntIndex( ref.m_iEdict ) )
				{
					// Doh! The entity was delete and its slot was reused.
					// Just use any old edict slot. This case sucks because we lose the baseline.
					return CreateEntityByName( pClassname );
				}
				else
				{
					// Cool, the slot where this entity was is free again (most likely, the entity was 
					// freed above). Now create an entity with this specific index.
					return CreateEntityByName( pClassname, ref.m_iEdict );
				}
			}
		}

	public:
		int m_iIterator; // Iterator into g_MapEntityRefs.
	};
	CHL2MPMapEntityFilter filter;
	filter.m_iIterator = g_MapEntityRefs.Head();

	// DO NOT CALL SPAWN ON info_node ENTITIES!

	MapEntity_ParseAllEntities( engine->GetMapEntitiesString(), &filter, true );
}

void CHL2MPRules::CheckChatForReadySignal( CHL2MP_Player *pPlayer, const char *chatmsg )
{
	if( m_bAwaitingReadyRestart && FStrEq( chatmsg, mp_ready_signal.GetString() ) )
	{
		if( !pPlayer->IsReady() )
		{
			pPlayer->SetReady( true );
		}		
	}
}

void CHL2MPRules::CheckRestartGame( void )
{
	// Restart the game if specified by the server
	int iRestartDelay = mp_restartgame.GetInt();

	if ( iRestartDelay > 0 )
	{
		if ( iRestartDelay > 60 )
			iRestartDelay = 60;


		// let the players know
		char strRestartDelay[64];
		Q_snprintf( strRestartDelay, sizeof( strRestartDelay ), "%d", iRestartDelay );
		UTIL_ClientPrintAll( HUD_PRINTCENTER, "Game will restart in %s1 %s2", strRestartDelay, iRestartDelay == 1 ? "SECOND" : "SECONDS" );
		UTIL_ClientPrintAll( HUD_PRINTCONSOLE, "Game will restart in %s1 %s2", strRestartDelay, iRestartDelay == 1 ? "SECOND" : "SECONDS" );

		m_flRestartGameTime = gpGlobals->curtime + iRestartDelay;
		m_bCompleteReset = true;
		mp_restartgame.SetValue( 0 );
	}

	if( mp_readyrestart.GetBool() )
	{
		m_bAwaitingReadyRestart = true;
		m_bHeardAllPlayersReady = false;
		

		const char *pszReadyString = mp_ready_signal.GetString();


		// Don't let them put anything malicious in there
		if( pszReadyString == NULL || Q_strlen(pszReadyString) > 16 )
		{
			pszReadyString = "ready";
		}

		IGameEvent *event = gameeventmanager->CreateEvent( "hl2mp_ready_restart" );
		if ( event )
			gameeventmanager->FireEvent( event );

		mp_readyrestart.SetValue( 0 );

		// cancel any restart round in progress
		m_flRestartGameTime = -1;
	}
}

void CHL2MPRules::CheckAllPlayersReady( void )
{
	for (int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CHL2MP_Player *pPlayer = (CHL2MP_Player*) UTIL_PlayerByIndex( i );

		if ( !pPlayer )
			continue;
		if ( !pPlayer->IsReady() )
			return;
	}
	m_bHeardAllPlayersReady = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CHL2MPRules::GetChatFormat( bool bTeamOnly, CBasePlayer *pPlayer )
{
	if ( !pPlayer )  // dedicated server output
	{
		return NULL;
	}

	const char *pszFormat = NULL;

	// team only
	if ( bTeamOnly == TRUE )
	{
		if ( pPlayer->GetTeamNumber() == TEAM_SPECTATOR )
		{
			pszFormat = "HL2MP_Chat_Spec";
		}
		else
		{
			const char *chatLocation = GetChatLocation( bTeamOnly, pPlayer );
			if ( chatLocation && *chatLocation )
			{
				pszFormat = "HL2MP_Chat_Team_Loc";
			}
			//BB: far be it to me to understand why this isn't implemented...
			else
			{
				if (pPlayer->IsAlive())
				{
					pszFormat = "HL2MP_Chat_Team";
				}
				else
				{
					pszFormat = "HL2MP_Chat_Team_Dead";
				}
			}
		}
	}
	// everyone
	else
	{
		if ( pPlayer->GetTeamNumber() != TEAM_SPECTATOR )
		{
			//BB: again, why not implement this correctly?
			if (pPlayer->IsAlive())
			{
				pszFormat = "HL2MP_Chat_All";
			}
			else
			{
				pszFormat = "HL2MP_Chat_AllDead";
			}
		}
		else
		{
			pszFormat = "HL2MP_Chat_AllSpec";
		}
	}

	return pszFormat;
}

#endif
