//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Basic BOT handling.
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//


#include "cbase.h"
#include "player.h"
#include "hl2mp_player.h"
#include "in_buttons.h"
#include "movehelper_server.h"

void ClientPutInServer( edict_t *pEdict, const char *playername );
void Bot_Think( CHL2MP_Player *pBot );

//BB: BOTS!
//#ifdef DEBUG

ConVar bot_forcefireweapon( "bot_forcefireweapon", "", 0, "Force bots with the specified weapon to fire." );
ConVar bot_forceattack2( "bot_forceattack2", "0", 0, "When firing, use attack2." );
ConVar bot_forceattackon( "bot_forceattackon", "0", 0, "When firing, don't tap fire, hold it down." );
ConVar bot_flipout( "bot_flipout", "0", 0, "When on, all bots fire their guns." );
ConVar bot_defend( "bot_defend", "0", 0, "Set to a team number, and that team will all keep their combat shields raised." );
ConVar bot_changeclass( "bot_changeclass", "0", 0, "Force all bots to change to the specified class." );
ConVar bot_zombie( "bot_zombie", "0", 0, "Brraaaaaiiiins." );
static ConVar bot_mimic_yaw_offset( "bot_mimic_yaw_offset", "0", 0, "Offsets the bot yaw." );
ConVar bot_attack( "bot_attack", "0", 0, "Shoot!" );

ConVar bot_sendcmd( "bot_sendcmd", "", 0, "Forces bots to send the specified command." );

ConVar bot_crouch( "bot_crouch", "0", 0, "Bot crouches" );

ConVar bot_difficulty("bot_difficulty", "1", FCVAR_ARCHIVE, "Bot difficulty: 0 easy, 1 medium, 2 hard."); 

#ifdef NEXT_BOT
extern ConVar bot_mimic;
#else
ConVar bot_mimic( "bot_mimic", "0", 0, "Bot uses usercmd of player by index." );
#endif

static int BotNumber = 1;
static int g_iNextBotTeam = -1;
static int g_iNextBotClass = -1;

static char *botnames[2][10] = 
{{"Blade","Gabriel Van Helsing","Lucian","Edgar Frog", "Allan Frog","Anita Blake","Simon Belmont","Buffy Summers","Abraham Van Helsing","Mister"},
{"Edward Cullen","Lestat de Lioncourt","Louis de Pointe du Lac","Liam","Jeanette","Therese","Bill Compton","Eric Northman","Armand","Eli"}};

#define OBJECTIVE_TYPE_NONE -1
#define OBJECTIVE_TYPE_ROGUE 0
#define OBJECTIVE_TYPE_CAPPOINT 1
#define OBJECTIVE_TYPE_CTS 2

#define ROLE_ATK 1
#define ROLE_DEF 2

typedef struct
{
	bool			backwards;
	bool			left;
	float			turns;

	float			nextstrafetime;
	float			nextjumptime;
	float			nextusetime;
	float			sidemove;

	float			stuckTimer;
	float			goWild;
	float			wildReverse;
	float			spawnTimer;
	float			guardTimer;
	bool			bGuarding;

	QAngle			forwardAngle;
	QAngle			lastAngles;
	Vector			lastPos;
	
	float			m_flJoinTeamTime;
	int				m_WantedTeam;
	int				m_WantedClass;

	int				m_lastPlayerCheck; //index
	bool			bCombat;
	bool			bForceCombat;
	float			m_flLastCombat;

	QAngle			overrideAngle;
	float			m_flOverrideDur;
	float			m_flAbility1Timer;
	float			m_flAbility2Timer;
	float			m_flAbility3Timer;
	float			m_flAbility4Timer;

	int				m_lastNode; //index
	int				m_lastNodeProbe; //index (not id) of last probe into the botnet
	int				m_targetNode; //index
	bool			bLost;
	float			m_flBaseSpeed;

	int				m_objectiveType; //-1 none, 0 rogue, 1 cap point, 2 cts
	int				m_objective; //index
	int				m_lastCheckedObjective;

	int				m_role;

	int				strikes;
} botdata_t;

unsigned int Bot_Ability_Think(CHL2MP_Player *pBot);

static botdata_t g_BotData[ MAX_PLAYERS ];

void Bot_Combat_Check( CHL2MP_Player *pBot, CBaseEntity *pAtk )
{
	if (!pBot)
		return;

	if (g_BotData[pBot->entindex()-1].bCombat || !pAtk->IsPlayer() || pAtk->GetTeamNumber() == pBot->GetTeamNumber())
		return;
	
	if (bot_difficulty.GetInt() < 1)
		return;

	g_BotData[pBot->entindex()-1].m_lastPlayerCheck = ((CHL2MP_Player *)pAtk)->entindex();
	g_BotData[pBot->entindex()-1].bCombat = true;
	g_BotData[pBot->entindex()-1].bForceCombat = true;
}

void BotRemove( CHL2MP_Player *pBot )
{
	if (!pBot)
		return;

	g_BotData[pBot->entindex()-1].m_WantedTeam = 0;
	g_BotData[pBot->entindex()-1].m_flJoinTeamTime = 0;
	//BotNumber--;
}

bool Bot_Right_Team( CHL2MP_Player *pBot )
{
	if (!pBot)
		return true;
	
	return g_BotData[pBot->entindex()-1].m_WantedTeam == pBot->GetTeamNumber();
}

void Set_Bot_Base_Velocity(CHL2MP_Player *pBot)
{
	botdata_t *botdata = &g_BotData[ENTINDEX(pBot->edict()) - 1];
	if (pBot->GetTeamNumber() == COVEN_TEAMID_SLAYERS)
	{
		if (pBot->covenClassID == COVEN_CLASSID_AVENGER)
			botdata->m_flBaseSpeed = (float)COVEN_BASESPEED_AVENGER;
		else if (pBot->covenClassID == COVEN_CLASSID_HELLION)
			botdata->m_flBaseSpeed = (float)COVEN_BASESPEED_HELLION;
		else if (pBot->covenClassID == COVEN_CLASSID_REAVER)
			botdata->m_flBaseSpeed = (float)COVEN_BASESPEED_REAVER;
		else if (pBot->covenClassID == COVEN_CLASSID_PRIEST)
			botdata->m_flBaseSpeed = (float)COVEN_BASESPEED_PRIEST;
		else if (pBot->covenClassID == COVEN_CLASSID_DEADEYE)
			botdata->m_flBaseSpeed = (float)COVEN_BASESPEED_DEADEYE;
	}
	else if (pBot->GetTeamNumber() == COVEN_TEAMID_VAMPIRES)
	{
		if (pBot->covenClassID == COVEN_CLASSID_FIEND)
			botdata->m_flBaseSpeed = (float)COVEN_BASESPEED_FIEND;
		else if (pBot->covenClassID == COVEN_CLASSID_GORE)
			botdata->m_flBaseSpeed = (float)COVEN_BASESPEED_GORE;
		else if (pBot->covenClassID == COVEN_CLASSID_DEGEN)
			botdata->m_flBaseSpeed = (float)COVEN_BASESPEED_DEGEN;
		else if (pBot->covenClassID == COVEN_CLASSID_SOULEAT)
			botdata->m_flBaseSpeed = (float)COVEN_BASESPEED_SOULEAT;
		//else if (pBot->covenClassID == COVEN_CLASSID_BLOOD)
		//	botdata->m_flBaseSpeed = (float)COVEN_BASESPEED_BLOOD;
	}
	else
		botdata->m_flBaseSpeed = 600.0f;
}

//-----------------------------------------------------------------------------
// Purpose: Create a new Bot and put it in the game.
// Output : Pointer to the new Bot, or NULL if there's no free clients.
//-----------------------------------------------------------------------------
CBasePlayer *BotPutInServer( bool bFrozen, int iTeam )
{
	if (iTeam < 2)
		return NULL;

	g_iNextBotTeam = iTeam;

	//BotNumber
	char botname[ 64 ];
	Q_snprintf( botname, sizeof( botname ), "%s", botnames[iTeam-2][random->RandomInt(0,9)] );

	// This is an evil hack, but we use it to prevent sv_autojointeam from kicking in.

	edict_t *pEdict = engine->CreateFakeClient( botname );

	if (!pEdict)
	{
		Msg( "Failed to create Bot.\n");
		return NULL;
	}

	// Allocate a CBasePlayer for the bot, and call spawn
	//ClientPutInServer( pEdict, botname );
	CHL2MP_Player *pPlayer = ((CHL2MP_Player *)CBaseEntity::Instance( pEdict ));
	pPlayer->ClearFlags();
	pPlayer->AddFlag( FL_CLIENT | FL_FAKECLIENT );

	pPlayer->ChangeTeam(iTeam);

	if ( bFrozen )
		pPlayer->AddEFlags( EFL_BOT_FROZEN );

	pPlayer->Spawn();

	BotNumber++;

	g_BotData[pPlayer->entindex()-1].m_WantedTeam = iTeam;
	g_BotData[pPlayer->entindex()-1].m_flJoinTeamTime = gpGlobals->curtime + 0.3;
	g_BotData[pPlayer->entindex()-1].m_lastNode = -1;
	g_BotData[pPlayer->entindex()-1].m_targetNode = 0;
	g_BotData[pPlayer->entindex()-1].m_lastNodeProbe = 0;
	g_BotData[pPlayer->entindex()-1].m_lastPlayerCheck = 0;
	g_BotData[pPlayer->entindex()-1].goWild = 0.0f;
	g_BotData[pPlayer->entindex()-1].stuckTimer = 0.0f;
	g_BotData[pPlayer->entindex()-1].spawnTimer = 0.0f;
	g_BotData[pPlayer->entindex()-1].guardTimer = 0.0f;
	g_BotData[pPlayer->entindex()-1].bLost = true;
	g_BotData[pPlayer->entindex()-1].bCombat = false;
	g_BotData[pPlayer->entindex()-1].bForceCombat = false;
	g_BotData[pPlayer->entindex()-1].left = false;
	g_BotData[pPlayer->entindex()-1].turns = 0.0f;
	g_BotData[pPlayer->entindex()-1].strikes = 0;
	g_BotData[pPlayer->entindex()-1].m_objective = -1;
	g_BotData[pPlayer->entindex()-1].m_objectiveType = -1;
	g_BotData[pPlayer->entindex()-1].m_role = -1;
	g_BotData[pPlayer->entindex() - 1].m_flBaseSpeed = 600.0f;
	g_BotData[pPlayer->entindex() - 1].m_flOverrideDur = 0.0f;
	g_BotData[pPlayer->entindex() - 1].m_flAbility1Timer = 0.0f;
	g_BotData[pPlayer->entindex() - 1].m_flAbility2Timer = 0.0f;
	g_BotData[pPlayer->entindex() - 1].m_flAbility3Timer = 0.0f;
	g_BotData[pPlayer->entindex() - 1].m_flAbility4Timer = 0.0f;
	g_BotData[pPlayer->entindex() - 1].m_flLastCombat = 0.0f;
	g_BotData[pPlayer->entindex() - 1].wildReverse = 0.0f;
	g_BotData[pPlayer->entindex() - 1].bGuarding = false;
	g_BotData[pPlayer->entindex() - 1].m_lastCheckedObjective = 0;

	return pPlayer;
}

Vector CurrentObjectiveLoc( CHL2MP_Player *pBot )
{
	Vector ret(0,0,0);
	botdata_t *botdata = &g_BotData[ ENTINDEX( pBot->edict() ) - 1 ];
	if (botdata->m_objectiveType == OBJECTIVE_TYPE_CAPPOINT)
	{
		if (botdata->m_objective > -1)
		{
			int i = 3*botdata->m_objective;
			ret.x = HL2MPRules()->cap_point_coords.Get(i);
			ret.y = HL2MPRules()->cap_point_coords.Get(i+1);
			ret.z = HL2MPRules()->cap_point_coords.Get(i+2);
		}
	}
	return ret;
}

Vector GetObjectiveLoc(int locationIndex)
{
	Vector ret(0, 0, 0);
	if (locationIndex)
	{
		int i = 3 * locationIndex;
		ret.x = HL2MPRules()->cap_point_coords.Get(i);
		ret.y = HL2MPRules()->cap_point_coords.Get(i + 1);
		ret.z = HL2MPRules()->cap_point_coords.Get(i + 2);
	}
	return ret;
}

void GetLost( CHL2MP_Player *pBot )
{
	botdata_t *botdata = &g_BotData[ ENTINDEX( pBot->edict() ) - 1 ];
	botdata->m_targetNode = 0;
	botdata->m_lastNode = -1;
	botdata->bLost = true;
	//Comendeering this function for resetall
	botdata->bCombat = botdata->bForceCombat = false;
	botdata->backwards = false;
	botdata->guardTimer = 0.0f;
	botdata->goWild = 0.0f;
}

//BB: TODO: defenders!
bool CheckObjective( CHL2MP_Player *pBot, bool doValidityCheck ) //returns true if the node was valid. false if node was invalid.
{
	botdata_t *botdata = &g_BotData[ ENTINDEX( pBot->edict() ) - 1 ];
	if (botdata->m_role < 0)
	{
		if (pBot->GetTeamNumber() == COVEN_TEAMID_VAMPIRES)
		{
			if (pBot->covenClassID == COVEN_CLASSID_FIEND)
			{
				botdata->m_role = ROLE_ATK;
			}
			else
			{
				botdata->m_role = ROLE_DEF;
			}
		}
		else if (pBot->GetTeamNumber() == COVEN_TEAMID_SLAYERS)
		{
			if (pBot->covenClassID == COVEN_CLASSID_HELLION)
			{
				botdata->m_role = ROLE_ATK;
			}
			else
			{
				botdata->m_role = ROLE_DEF;
			}
		}
	}

	if (botdata->m_objectiveType < 0)
	{
		botdata->m_objectiveType = OBJECTIVE_TYPE_CAPPOINT;
	}

	if (botdata->m_objective < 0)
	{
		if (botdata->m_role == ROLE_ATK)
		{
			botdata->m_objective = random->RandomInt(0, HL2MPRules()->num_cap_points-1);
			/*float shortestdist = -1.0f;
			botdata->m_objective = 0;
			int count = HL2MPRules()->num_cap_points;
			for (int i = 0; i < count; i++)
			{
				int index = 3*i;
				float dist = (pBot->GetLocalOrigin()-Vector(HL2MPRules()->cap_point_coords.Get(index),HL2MPRules()->cap_point_coords.Get(index+1),HL2MPRules()->cap_point_coords.Get(index+2))).Length();
				if ((shortestdist < 0.0f || dist < shortestdist) && HL2MPRules()->cap_point_state.Get(i) != pBot->GetTeamNumber())
				{
					shortestdist = dist;
					botdata->m_objective = i;
				}
			}*/
		}
		else if (botdata->m_role == ROLE_DEF)
		{
			botdata->m_objective = random->RandomInt(0, HL2MPRules()->num_cap_points-1);//BB: TODO: random for now, later do closest or more intelligent
		}
	}
	
	if (doValidityCheck)
	{
		//check our objectives validity
		if (botdata->m_role == ROLE_ATK)
		{
			//X BOTPATCH if there are no valid options, go ahead with a random and skip this check
			if ((pBot->GetTeamNumber() == COVEN_TEAMID_SLAYERS && HL2MPRules()->s_caps < HL2MPRules()->num_cap_points) || (pBot->GetTeamNumber() == COVEN_TEAMID_VAMPIRES && HL2MPRules()->v_caps < HL2MPRules()->num_cap_points))
			{
				if (HL2MPRules()->cap_point_state.Get(botdata->m_objective) == pBot->GetTeamNumber())
				{
					botdata->m_objective = -1;
					botdata->m_objectiveType = -1;
					//X BOTPATCH getlost
					GetLost(pBot);
					//X BOTPATCH clear guardtimer
					botdata->guardTimer = 0.0f;
					botdata->bGuarding = false;
					return false;
				}
			}
		}
		else if (botdata->m_role == ROLE_DEF)
		{
			if ((pBot->GetTeamNumber() == COVEN_TEAMID_SLAYERS && HL2MPRules()->s_caps < HL2MPRules()->num_cap_points) || (pBot->GetTeamNumber() == COVEN_TEAMID_VAMPIRES && HL2MPRules()->v_caps < HL2MPRules()->num_cap_points))
			{
				if (HL2MPRules()->cap_point_state.Get(botdata->m_objective) == pBot->GetTeamNumber())
				{
					botdata->m_objective = -1;
					botdata->m_objectiveType = -1;
					GetLost(pBot);
					botdata->guardTimer = 0.0f;
					botdata->bGuarding = false;
					return false;
				}
			}
		}
	}
	else
	{
		//BB: this is kludgy, but this only needs to happen PRIOR to guard set
		//BB: this type always checks to see if it is crossing into a zone then attempts to cap it
		if (botdata->m_role == ROLE_DEF)
		{
			if (botdata->guardTimer == 0.0f)
			{
				botdata->m_lastCheckedObjective++;
				if (botdata->m_lastCheckedObjective >= HL2MPRules()->num_cap_points)
					botdata->m_lastCheckedObjective = 0;
				Vector objloc = GetObjectiveLoc(botdata->m_lastCheckedObjective);
				Vector goalloc = GetObjectiveLoc(botdata->m_objective);
				if (HL2MPRules()->cap_point_state.Get(botdata->m_lastCheckedObjective) != pBot->GetTeamNumber() && (objloc - pBot->GetLocalOrigin()).Length() < (goalloc - pBot->GetLocalOrigin()).Length())
				{
					botdata->m_objectiveType = OBJECTIVE_TYPE_CAPPOINT;
					botdata->m_objective = botdata->m_lastCheckedObjective;
				}
			}
		}
		else
		{
			if (botdata->guardTimer == 0.0f)
			{
				botdata->m_lastCheckedObjective++;
				if (botdata->m_lastCheckedObjective >= HL2MPRules()->num_cap_points)
					botdata->m_lastCheckedObjective = 0;
				Vector objloc = GetObjectiveLoc(botdata->m_lastCheckedObjective);
				if (HL2MPRules()->cap_point_state.Get(botdata->m_lastCheckedObjective) != pBot->GetTeamNumber() && (objloc - pBot->GetLocalOrigin()).Length() < HL2MPRules()->cap_point_distance[botdata->m_lastCheckedObjective]) //BB: TODO: do I need LOS on this?
				{
					botdata->m_objectiveType = OBJECTIVE_TYPE_CAPPOINT;
					botdata->m_objective = botdata->m_lastCheckedObjective;
				}
			}
		}
	}
	
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Run through all the Bots in the game and let them think.
//-----------------------------------------------------------------------------
void Bot_RunAll( void )
{
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CHL2MP_Player *pPlayer = ToHL2MPPlayer( UTIL_PlayerByIndex( i ) );

		if ( pPlayer && (pPlayer->GetFlags() & FL_FAKECLIENT) )
		{
			Bot_Think( pPlayer );
		}
	}
}

bool RunMimicCommand( CUserCmd& cmd )
{
	if ( bot_mimic.GetInt() <= 0 )
		return false;

	if ( bot_mimic.GetInt() > gpGlobals->maxClients )
		return false;

	
	CBasePlayer *pPlayer = UTIL_PlayerByIndex( bot_mimic.GetInt()  );
	if ( !pPlayer )
		return false;

	if ( !pPlayer->GetLastUserCommand() )
		return false;

	cmd = *pPlayer->GetLastUserCommand();
	cmd.viewangles[YAW] += bot_mimic_yaw_offset.GetFloat();

	return true;
}

unsigned int AmmoCheck(CHL2MP_Player *pBot)
{
	if (pBot->GetTeamNumber() == COVEN_TEAMID_SLAYERS)
	{
		botdata_t *botdata = &g_BotData[ENTINDEX(pBot->edict()) - 1];
		if (botdata->bCombat)
		{
			CHL2MP_Player *pPlayer = ToHL2MPPlayer(UTIL_PlayerByIndex(botdata->m_lastPlayerCheck));
			if (pPlayer)
			{
				bool isVampDoll = pPlayer->KO && pPlayer->myServerRagdoll != NULL;
				CBaseCombatWeapon *pWeapon = pBot->Weapon_OwnsThisType("weapon_stake");
				CBaseCombatWeapon *pActiveWeapon = pBot->GetActiveWeapon();
				if (pWeapon)
				{
					if (isVampDoll)
					{
						// Switch to it if we don't have it out
						// Switch?
						if (pActiveWeapon != pWeapon)
						{
							pBot->Weapon_Switch(pWeapon);
						}
					}
					else
					{
						if (pActiveWeapon == pWeapon)
						{
							CBaseCombatWeapon *nextBest = g_pGameRules->GetNextBestWeapon(pBot, pActiveWeapon);
							if (nextBest && nextBest->HasAmmo())
								pBot->SwitchToNextBestWeapon(pWeapon);
						}
						else
						{
							if (pActiveWeapon->NeedsReload1())
							{
								return IN_RELOAD;
							}
						}
					}
				}
			}
		}
		else
		{
			CBaseCombatWeapon *pWeapon = pBot->Weapon_OwnsThisType("weapon_stake");
			if (pWeapon)
			{
				CBaseCombatWeapon *pActiveWeapon = pBot->GetActiveWeapon();
				if (pActiveWeapon == pWeapon)
				{
					CBaseCombatWeapon *nextBest = g_pGameRules->GetNextBestWeapon(pBot, pActiveWeapon);
					if (nextBest && nextBest->HasAmmo())
						pBot->SwitchToNextBestWeapon(pWeapon);
				}
				else if (pActiveWeapon != pWeapon && pActiveWeapon->HasAmmo())
				{
					if (pActiveWeapon->NeedsReload1())
					{
						return IN_RELOAD;
					}
				}
				else
					pBot->Weapon_Switch(pWeapon);
			}
		}
		
	}
	return 0;
}

void HealthCheck(CHL2MP_Player *pBot)
{
	if (pBot->GetTeamNumber() == COVEN_TEAMID_VAMPIRES)
	{
		
	}
	else //Slayers
	{

	}
}

void PlayerCheck( CHL2MP_Player *pBot )
{
	botdata_t *botdata = &g_BotData[ ENTINDEX( pBot->edict() ) - 1 ];

	CHL2MP_Player *pPlayer = ToHL2MPPlayer( UTIL_PlayerByIndex( botdata->m_lastPlayerCheck ) );

	if (pPlayer && pPlayer->IsAlive() && pPlayer->GetTeamNumber() != pBot->GetTeamNumber())
	{
		Vector vecEnd;
		Vector vecSrc;
		bool isVampDoll = pPlayer->KO && pPlayer->myServerRagdoll != NULL;

		if (isVampDoll)
			vecSrc = pBot->GetAbsOrigin() + Vector( 0, 0, 36 );
		else
			vecSrc = pBot->EyePosition();//pBot->GetLocalOrigin() + Vector( 0, 0, 36 );

		if (isVampDoll)
		{
			vecEnd = pPlayer->myServerRagdoll->GetAbsOrigin();
		}
		else
			vecEnd = pPlayer->GetLocalOrigin() + Vector( 0, 0, 36 );

		Vector playerVec = vecEnd-vecSrc;

		if (botdata->bForceCombat || (playerVec).Length() < 600) //400
		{
			float check = 0.1f;
			if (botdata->bForceCombat)
				check = 2.0f;
			if (isVampDoll || ((pPlayer->m_floatCloakFactor < check && !(pPlayer->GetFlags() & EF_NODRAW))))
			{
				Vector forward;
				AngleVectors(botdata->lastAngles, &forward);
				VectorNormalize(playerVec);
				float botDot = DotProduct(playerVec,forward);
				float test = 0.3f;
				if (isVampDoll)
					test = 0.0f;
				if (botdata->bForceCombat || botDot > test)
				{
					trace_t trace;

					if (isVampDoll)
						UTIL_TraceLine(vecSrc, vecEnd, MASK_SHOT, pBot, COLLISION_GROUP_WEAPON, &trace );
					else
						UTIL_TraceLine(vecSrc, vecEnd, MASK_SHOT, pBot, COLLISION_GROUP_PLAYER, &trace );

					CBaseEntity	*pHitEnt = trace.m_pEnt;
					if (pHitEnt == pPlayer || (isVampDoll && pHitEnt == pPlayer->myServerRagdoll))
					{
						//if (pHitEnt && isVampDoll)
						//	Msg("%f %f %f\n", pHitEnt->GetAbsOrigin().x, pHitEnt->GetAbsOrigin().y, pHitEnt->GetAbsOrigin().z);
						botdata->bCombat = true;
						botdata->guardTimer = 0.0f;
						botdata->bGuarding = false;
						return;
					}
				}
			}
		}

		if (botdata->bCombat)
		{
			if (botdata->m_flLastCombat == 0.0f)
			{
				botdata->m_flLastCombat = gpGlobals->curtime + 5.0f;
				return;
			}
			else if (gpGlobals->curtime > botdata->m_flLastCombat)
			{
				botdata->m_flLastCombat = 0.0f;
			}
			else
				return;
		}
	}

	if (botdata->bCombat)
	{
		botdata->backwards = false;
		GetLost(pBot);
	}

	botdata->bCombat = false;
	botdata->bForceCombat = false;
	botdata->m_lastPlayerCheck++;
	
	if (botdata->m_lastPlayerCheck > gpGlobals->maxClients)
		botdata->m_lastPlayerCheck = 0;
}

void FindNearestNode( CHL2MP_Player *pBot )
{
	CHL2MPRules *pRules;
	pRules = HL2MPRules();
	if (pRules->bot_node_count == 0 )
		return;

	botdata_t *botdata = &g_BotData[ ENTINDEX( pBot->edict() ) - 1 ];

	botnode *temp, *cur;
	temp = pRules->botnet[botdata->m_lastNodeProbe];
	cur = pRules->botnet[botdata->m_targetNode];

	//skip "stop" nodes
	if (temp != NULL && temp->connectors.Count() > 1)
	{
		//skip nodes without +/- 50 z
		if (abs(pBot->GetAbsOrigin().z - temp->location.z) < 50)
		{
			//We found a closer node
			if (cur == NULL || (pBot->GetAbsOrigin() - temp->location).Length() < (pBot->GetAbsOrigin() - cur->location).Length())
			{
				botdata->m_targetNode = botdata->m_lastNodeProbe;
			}
		}
	}

	botdata->m_lastNodeProbe++;
	if (botdata->m_lastNodeProbe > pRules->bot_node_count)
	{
		botdata->m_lastNodeProbe = 0;
		botdata->bLost = false;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Simulates a single frame of movement for a player
// Input  : *fakeclient - 
//			*viewangles - 
//			forwardmove - 
//			sidemove - 
//			upmove - 
//			buttons - 
//			impulse - 
//			msec - 
// Output : 	virtual void
//-----------------------------------------------------------------------------
static void RunPlayerMove( CHL2MP_Player *fakeclient, const QAngle& viewangles, float forwardmove, float sidemove, float upmove, unsigned int buttons, byte impulse, float frametime )
{
	if ( !fakeclient )
		return;

	CUserCmd cmd;

	// Store off the globals.. they're gonna get whacked
	float flOldFrametime = gpGlobals->frametime;
	float flOldCurtime = gpGlobals->curtime;

	float flTimeBase = gpGlobals->curtime + gpGlobals->frametime - frametime;
	fakeclient->SetTimeBase( flTimeBase );

	Q_memset( &cmd, 0, sizeof( cmd ) );

	if ( !RunMimicCommand( cmd ) && !bot_zombie.GetBool() )
	{
		VectorCopy( viewangles, cmd.viewangles );
		cmd.forwardmove = forwardmove;
		cmd.sidemove = sidemove;
		cmd.upmove = upmove;
		cmd.buttons = buttons;
		cmd.impulse = impulse;
		cmd.random_seed = random->RandomInt( 0, 0x7fffffff );
	}

	if( bot_crouch.GetInt() )
		cmd.buttons |= IN_DUCK;

	if ( bot_attack.GetBool() )
		cmd.buttons |= IN_ATTACK;

	MoveHelperServer()->SetHost( fakeclient );
	fakeclient->PlayerRunCommand( &cmd, MoveHelperServer() );

	// save off the last good usercmd
	fakeclient->SetLastUserCommand( cmd );

	// Clear out any fixangle that has been set
	fakeclient->pl.fixangle = FIXANGLE_NONE;

	// Restore the globals..
	gpGlobals->frametime = flOldFrametime;
	gpGlobals->curtime = flOldCurtime;
}

unsigned int Bot_Ability_Think(CHL2MP_Player *pBot)
{
	botdata_t *botdata = &g_BotData[ENTINDEX(pBot->edict()) - 1];
	unsigned int buttons = 0;
	if (pBot->GetTeamNumber() == COVEN_TEAMID_SLAYERS)
	{
		if (pBot->covenClassID == COVEN_CLASSID_REAVER)
		{
			if (pBot->PointsToSpend() > 0)
			{
				if (pBot->SpendPointBOT(3))
				{
					int rn = random->RandomInt(0, 2);
					pBot->SpendPointBOT(rn);
				}
			}
			if (botdata->bCombat)
			{
				float cd = pBot->GetCooldown(0);
				if (pBot->GetLoadout(0) > 0 && !(pBot->covenStatusEffects & COVEN_FLAG_SPRINT) && pBot->SuitPower_GetCurrentPercentage() > 5.0f && (gpGlobals->curtime >= cd || cd == 0.0f))
				{
					buttons |= IN_ABIL1;
					botdata->m_flAbility1Timer = 0.0f;
				}
				cd = pBot->GetCooldown(1);
				if (pBot->GetLoadout(1) > 0 && !(pBot->covenStatusEffects & COVEN_FLAG_STATS) && (gpGlobals->curtime >= cd || cd == 0.0f))
				{
					buttons |= IN_ABIL2;
				}
				//BB: TODO: INT SHOUT
			}
			else
			{
				if (pBot->covenStatusEffects & COVEN_FLAG_SPRINT)
				{
					if (botdata->m_flAbility1Timer == 0.0f)
					{
						botdata->m_flAbility1Timer = gpGlobals->curtime + 5.0f;
					}
					else if (gpGlobals->curtime > botdata->m_flAbility1Timer)
					{
						buttons |= IN_ABIL1;
						botdata->m_flAbility1Timer = 0.0f;
					}
				}
			}
		}
		else if (pBot->covenClassID == COVEN_CLASSID_AVENGER)
		{
			if (pBot->PointsToSpend() > 0)
			{
				if (pBot->SpendPointBOT(3))
				{
					int rn = random->RandomInt(0, 2);
					pBot->SpendPointBOT(rn);
				}
			}
			if (botdata->bCombat)
			{
				float cd = pBot->GetCooldown(0);
				if (pBot->GetLoadout(0) > 0 && !(pBot->covenStatusEffects & COVEN_FLAG_BYELL) && (gpGlobals->curtime >= cd || cd == 0.0f))
				{
					buttons |= IN_ABIL1;
				}
			}
			else
			{
				float cd = pBot->GetCooldown(1);
				int ld = pBot->GetLoadout(1);
				if (ld > 0 && pBot->GetHealth() < 100.0f && (gpGlobals->curtime >= cd || cd == 0.0f) && pBot->medkits.Size() < ld)
				{
					buttons |= IN_ABIL2;
				}
			}
		}
		else if (pBot->covenClassID == COVEN_CLASSID_HELLION)
		{
			if (pBot->PointsToSpend() > 0)
			{
				if (pBot->SpendPointBOT(0))
					if (pBot->SpendPointBOT(3))
					{
						int rn = random->RandomInt(1, 2);
						pBot->SpendPointBOT(rn);
					}
			}
			if (botdata->bCombat)
			{
				float cd = pBot->GetCooldown(0);
				CHL2MP_Player *pPlayer = ToHL2MPPlayer(UTIL_PlayerByIndex(botdata->m_lastPlayerCheck));
				bool isVampDoll = false;
				if (pPlayer)
				{
					isVampDoll = pPlayer->KO && pPlayer->myServerRagdoll != NULL;
				}
				if (!isVampDoll && pBot->GetLoadout(0) > 0 && (gpGlobals->curtime >= cd || cd == 0.0f)) //don't waste water on ragdolls
				{
					buttons |= IN_ABIL1;
				}
			}
			else
			{
				//BB: TODO: Randomize this timing a little?
				float cd = pBot->GetCooldown(0);
				if (pBot->GetLoadout(0) > 0 && pBot->GetHealth() < 100.0f && (gpGlobals->curtime >= cd || cd == 0.0f))
				{
					botdata->lastAngles.x = 60;
					botdata->overrideAngle = botdata->lastAngles;
					botdata->m_flOverrideDur = gpGlobals->curtime + 0.1f;
					buttons |= IN_ABIL1;
				}
			}
		}
	}
	else //Vampires
	{
		if (pBot->covenClassID == COVEN_CLASSID_FIEND)
		{
			if (pBot->PointsToSpend() > 0)
			{
				if (pBot->SpendPointBOT(0))
					if (pBot->SpendPointBOT(3))
					{
						int rn = random->RandomInt(1, 2);
						pBot->SpendPointBOT(rn);
					}
			}
			if (botdata->bCombat)
			{
				float cd = pBot->GetCooldown(0);
				float distance = 0.0f;
				CHL2MP_Player *pPlayer = ToHL2MPPlayer(UTIL_PlayerByIndex(botdata->m_lastPlayerCheck));
				if (pPlayer)
				{
					distance = (pPlayer->GetLocalOrigin() - pBot->GetLocalOrigin()).Length();
				}
				if (pBot->GetLoadout(0) > 0 && (gpGlobals->curtime >= cd || cd == 0.0f) && distance > 200.0f)
				{
					if (botdata->m_flAbility1Timer == 0.0f)
					{
						buttons |= IN_JUMP;
						botdata->m_flAbility1Timer = gpGlobals->curtime + 0.2f;
					}
					else
					{
						botdata->m_flAbility1Timer = 0.0f;
						buttons |= IN_ABIL1;
					}
				}
				cd = pBot->GetCooldown(1);
				if (pBot->GetLoadout(1) > 0 && !(pBot->covenStatusEffects & COVEN_FLAG_BERSERK) && (gpGlobals->curtime >= cd || cd == 0.0f))
				{
					buttons |= IN_ABIL2;
				}
				//BB: TODO: dodge
			}
			else
			{
				CHL2MPRules *pRules;
				pRules = HL2MPRules();
				float cd = pBot->GetCooldown(0);
				float distance = 0.0f;
				if (botdata->m_targetNode > -1 && pRules->botnet[botdata->m_targetNode] != NULL)
				{
					distance = (pRules->botnet[botdata->m_targetNode]->location - pBot->GetLocalOrigin()).Length();
				}
				if (pBot->GetLoadout(0) > 0 && botdata->goWild == 0.0f && !botdata->bLost && botdata->stuckTimer == 0.0f && botdata->guardTimer == 0.0f && (gpGlobals->curtime >= cd || cd == 0.0f) && distance > 425.0f) //BB: TODO: && roofcheck
				{
					if (botdata->m_flAbility1Timer == 0.0f)
					{
						buttons |= IN_JUMP;
						botdata->m_flAbility1Timer = gpGlobals->curtime + 0.2f;
					}
					else
					{
						botdata->lastAngles.x = -30;
						botdata->overrideAngle = botdata->lastAngles;
						botdata->m_flOverrideDur = gpGlobals->curtime + 0.2f;
						botdata->m_flAbility1Timer = 0.0f;
						buttons |= IN_ABIL1;
					}
				}
				//BB: TODO: random sneak walk
			}
		}
		else if (pBot->covenClassID == COVEN_CLASSID_GORE)
		{
			if (pBot->PointsToSpend() > 0)
			{
				if (pBot->SpendPointBOT(1))
					if (pBot->SpendPointBOT(3))
					{
						int rn = random->RandomInt(0, 2);
						pBot->SpendPointBOT(rn);
					}
			}
			if (botdata->bCombat)
			{
				float cd = pBot->GetCooldown(0);
				if (pBot->GetLoadout(0) > 0 && (gpGlobals->curtime >= cd || cd == 0.0f))
				{
					buttons |= IN_ABIL1;
				}
				cd = pBot->GetCooldown(1);
				float distance = 0.0f;
				CHL2MP_Player *pPlayer = ToHL2MPPlayer(UTIL_PlayerByIndex(botdata->m_lastPlayerCheck));
				if (pPlayer)
				{
					Vector vStart = pPlayer->GetLocalOrigin();
					vStart.z = 0.0f;
					Vector vEnd = pBot->GetLocalOrigin();
					vEnd.z = 0.0f;
					distance = (vStart - vEnd).Length(); //ignore z for distance check (flung players should not trigger a charge)

					if (pBot->GetLoadout(1) > 0 && pBot->SuitPower_GetCurrentPercentage() > 10.0f && (gpGlobals->curtime >= cd || cd == 0.0f))
					{
						if (distance > 100.0f)
						{
							botdata->m_flAbility2Timer = distance;
							//BB: I don't think I need to do this... angles should be "correct" from other functions
							//Vector forward = pPlayer->GetLocalOrigin() - pBot->GetLocalOrigin();
							//VectorAngles(forward, botdata->lastAngles);
							buttons |= IN_ABIL2;
						}
						else if (pBot->gorelock && distance > 24.0f && distance < botdata->m_flAbility2Timer)
						{
							botdata->m_flAbility2Timer = distance;
							buttons |= IN_ABIL2;
						}
						else if (pBot->gorelock)
							botdata->m_flAbility2Timer = 0.0f;
					}
				}
				//BB: TODO: phasing
			}
			else
			{
				CHL2MPRules *pRules;
				pRules = HL2MPRules();
				float cd = pBot->GetCooldown(1);
				float distance = 0.0f;
				float deltaZ = 0.0f;
				if (botdata->m_targetNode > -1 && pRules->botnet[botdata->m_targetNode] != NULL)
				{
					distance = (pRules->botnet[botdata->m_targetNode]->location - pBot->GetLocalOrigin()).Length();
					deltaZ = abs(pRules->botnet[botdata->m_targetNode]->location.z - pBot->GetLocalOrigin().z);

					if (pBot->GetLoadout(1) > 0 && botdata->goWild == 0.0f && !botdata->bLost && botdata->stuckTimer == 0.0f && botdata->guardTimer == 0.0f && pBot->SuitPower_GetCurrentPercentage() > 10.0f && (gpGlobals->curtime >= cd || cd == 0.0f))
					{
						if (distance > 450.0f && deltaZ < 24.0f)
						{
							botdata->m_flAbility2Timer = distance;
							//BB: I don't think I need to do this... angles should be "correct" from other functions
							//Vector forward = pRules->botnet[botdata->m_targetNode]->location - pBot->GetLocalOrigin();
							//VectorAngles(forward, botdata->lastAngles);
							buttons |= IN_ABIL2;
						}
						else if (pBot->gorelock && distance > 30.0f && distance < botdata->m_flAbility2Timer)
						{
							botdata->m_flAbility2Timer = distance;
							buttons |= IN_ABIL2;
						}
						else if (pBot->gorelock)
							botdata->m_flAbility2Timer = 0.0f;
					}
				}
				//BB: TODO: phasing
			}
		}
		else if (pBot->covenClassID == COVEN_CLASSID_DEGEN)
		{
			if (pBot->PointsToSpend() > 0)
			{
				if (pBot->SpendPointBOT(2))
					if (pBot->SpendPointBOT(3))
					{
						int rn = random->RandomInt(0, 1);
						pBot->SpendPointBOT(rn);
					}
			}
			if (botdata->bCombat)
			{
				float cd = pBot->GetCooldown(0);
				if (pBot->GetLoadout(0) > 0 && !(pBot->covenStatusEffects & COVEN_FLAG_BLUST) && (gpGlobals->curtime >= cd || cd == 0.0f))
				{
					buttons |= IN_ABIL1;
				}
				cd = pBot->GetCooldown(1);
				if (pBot->GetLoadout(1) > 0 && !(pBot->covenStatusEffects & COVEN_FLAG_STATS) && (gpGlobals->curtime >= cd || cd == 0.0f))
				{
					buttons |= IN_ABIL2;
				}
				//BB: TODO: blood explode on cd and distance
			}
			else
			{
				//BB: TODO: blood explode?
			}
		}
	}
	return buttons;
}

//-----------------------------------------------------------------------------
// Purpose: Run this Bot's AI for one frame.
//-----------------------------------------------------------------------------
void Bot_Think( CHL2MP_Player *pBot )
{
	CHL2MPRules *pRules;
	pRules = HL2MPRules();
	// Make sure we stay being a bot
	pBot->AddFlag( FL_FAKECLIENT );

	botdata_t *botdata = &g_BotData[ ENTINDEX( pBot->edict() ) - 1 ];

	QAngle angle = pBot->GetLocalAngles();//botdata->lastAngles;


	//BB: not sure why the fuck I have to do it this way...
	/*if (pBot->GetTeamNumber() != botdata->m_WantedTeam)
	{
		pBot->ChangeTeam(botdata->m_WantedTeam);
		return;
	}*/

	//BB: try to spawn
	if (!pBot->IsAlive())
	{
		// Respawn the bot
		//buttons |= IN_JUMP;
		//BB: again, W T F
		if (botdata->spawnTimer > 0.0f)
		{
			if (gpGlobals->curtime > botdata->spawnTimer)
			{
				botdata->spawnTimer = 0.0f;
				pBot->Spawn();
				botdata->stuckTimer = 0.0f;
				botdata->nextjumptime = 0.0f;
				botdata->nextusetime = 0.0f;
				botdata->m_role = -1;
				GetLost(pBot);
			}
		}
		else
		{
			botdata->spawnTimer = gpGlobals->curtime + 1.0f;
		}
	}

	unsigned int buttons = 0;

	//Combat Check
	PlayerCheck(pBot);

	//Objective Check
	CheckObjective(pBot, false);

	//Ammo Check
	buttons |= AmmoCheck(pBot);

	//Health Check
	HealthCheck(pBot);

	float forwardmove = 0.0f;
	float sidemove = botdata->sidemove;
	float upmove = 0.0f;

	byte  impulse = 0;
	float frametime = gpGlobals->frametime;

	//Pathing calcs
	if (pRules->botnet[botdata->m_targetNode] == NULL || botdata->bLost)
	{
		//Msg("LOST!\n");
		FindNearestNode(pBot);
	}
	else if (botdata->goWild == 0.0f)
	{
		//reached objective
		Vector objloc = CurrentObjectiveLoc(pBot);
		if (botdata->m_objective > -1 && (objloc - pBot->GetLocalOrigin()).Length() < HL2MPRules()->cap_point_distance[botdata->m_objective] && !botdata->bCombat)
		{
			//X BOTPATCH add vis check if vis check is appropriate
			bool lineOfSight = true;
			if (pRules->cap_point_sightcheck[botdata->m_objective])
			{
				trace_t tr;
				UTIL_TraceLine(pBot->EyePosition(), objloc, MASK_SOLID, pBot, COLLISION_GROUP_DEBRIS, &tr);
				if (tr.fraction != 1.0)
					lineOfSight = false;
			}
			if (lineOfSight)
			{
				//X BOTPATCH if no valid objectives, dont set guardtimer, but clear the objective to -1 and getlost
				if ((pBot->GetTeamNumber() == COVEN_TEAMID_SLAYERS && HL2MPRules()->s_caps == HL2MPRules()->num_cap_points) || (pBot->GetTeamNumber() == COVEN_TEAMID_VAMPIRES && HL2MPRules()->v_caps == HL2MPRules()->num_cap_points))
				{
					botdata->m_objective = -1;
					botdata->m_objectiveType = -1;
					GetLost(pBot);
					botdata->guardTimer = 0.0f;
					botdata->bGuarding = false;
				}
				//X BOTPATCH just going to keep resetting guardtimer... is this a problem??? YES! check for cap of objective and then move on, or give up
				else if (botdata->guardTimer == 0.0f)
				{
					//One last check. Keeps me from infinity loop.
					if (CheckObjective(pBot, true))
						botdata->guardTimer = gpGlobals->curtime + 10.0f;
				}
				else if (gpGlobals->curtime > botdata->guardTimer)
				{
					botdata->m_objective = -1;
					botdata->m_objectiveType = -1;
					GetLost(pBot);
					botdata->guardTimer = 0.0f;
					botdata->bGuarding = false;
				}
			}
		}
		//reached node
		if (!botdata->bLost && botdata->guardTimer == 0.0f && (pRules->botnet[botdata->m_targetNode]->location - pBot->GetLocalOrigin()).Length() < 20) //10
		{
			//Msg("Reached: %d\n", pRules->botnet[botdata->m_targetNode]->ID);
			if (pRules->botnet[botdata->m_targetNode]->connectors.Count() <= 1)
			{
				//dead end get lost for now
				//GetLost(pBot);
				//get bored after 10 seconds
				//X BOTPATCH just remove condition... do a getlost
				GetLost(pBot);
				//botdata->guardTimer = gpGlobals->curtime + 10.0f;
			}
			else
			{
				int pushedval = botdata->m_targetNode;
				int redflag = -1;
				if (botdata->m_lastNode > -1)
				{
					redflag = pRules->botnet[botdata->m_lastNode]->ID;
				}
				int n = pRules->botnet[botdata->m_targetNode]->connectors.Count();
				int sel = -1;

				if (n == 2 && redflag > -1) //early cull... always go forward
				{
					sel = pRules->botnet[botdata->m_targetNode]->connectors[0];
					if (sel == redflag)
						sel = pRules->botnet[botdata->m_targetNode]->connectors[1];
				}
				else
				{
					if (botdata->m_objectiveType > OBJECTIVE_TYPE_ROGUE)
					{
						float shortestdist = -1.0f;
						for (int i = 0; i < n; i++)
						{
							if (pRules->botnet[botdata->m_targetNode]->connectors[i] == redflag || pRules->botnet[pRules->botnet[botdata->m_targetNode]->connectors[i]]->connectors.Size() == 1)
								continue;
							float dist = (pRules->botnet[pRules->botnet[botdata->m_targetNode]->connectors[i]]->location-objloc).Length();
							if (shortestdist < 0.0f || dist < shortestdist)
							{
								shortestdist = dist;
								sel = pRules->botnet[botdata->m_targetNode]->connectors[i];
							}
						}
					}
					else //rogue or invalid
					{
						n--;
						if (redflag > -1)
							n--;
						sel = random->RandomInt(0,n);
						int t = sel;
						for (int i = 0; i < t; i++)
						{
							if (pRules->botnet[botdata->m_targetNode]->connectors[i] == redflag)
								sel++;
						}
						sel = pRules->botnet[botdata->m_targetNode]->connectors[sel];
					}
				}
				botdata->m_targetNode = sel;
				//Msg("Headed to: %d\n", pRules->botnet[botdata->m_targetNode]->ID);
				botdata->m_lastNode = pushedval;
			}
		}
	}

	//Actual movement calcs
	if ( pBot->IsAlive() && (pBot->GetSolid() == SOLID_BBOX) )
	{
		trace_t trace;

		if ( !pBot->IsEFlagSet(EFL_BOT_FROZEN) && botdata->guardTimer == 0.0f)
		{
			//Forward move calc
			forwardmove = botdata->m_flBaseSpeed * (botdata->backwards ? -1 : 1);
			if ( botdata->sidemove != 0.0f)
			{
				forwardmove *= random->RandomFloat( 0.1f, 1.0f );
			}
		}

		if ( !pBot->IsEFlagSet(EFL_BOT_FROZEN))
		{
			Vector forward;
			if (pBot->GetMoveType() == MOVETYPE_LADDER)
			{
				//forwardmove = 0;
				buttons |= IN_JUMP;
			}
			
			//GoWild -> attempt to fix a stuck condition
			if (botdata->goWild > 0.0f && !botdata->bCombat)
			{
				if (gpGlobals->curtime > botdata->goWild)
				{
					botdata->goWild = 0.0f;
					botdata->turns = 0.0f;
					botdata->wildReverse = 0.0f;
					GetLost(pBot);
				}
				else if (gpGlobals->curtime > botdata->wildReverse) //always greater the first time
				{
					Vector vecEnd;

					float angledelta = 15.0;

					int maxtries = (int)360.0 / angledelta;

					Vector vecSrc;

					int dir = -1;
					if (botdata->left)
						dir = 1;

					while (--maxtries >= 0)
					{
						AngleVectors(angle, &forward);
						angle.x = 0;

						vecSrc = pBot->GetLocalOrigin();// +Vector(0, 0, 36);

						vecEnd = vecSrc + forward * 10; //10

						//UTIL_TraceLine(vecSrc, vecEnd, MASK_PLAYERSOLID, pBot, COLLISION_GROUP_PLAYER, &trace);
						UTIL_TraceHull(vecSrc, vecEnd, VEC_HULL_MIN_SCALED(pBot), VEC_HULL_MAX_SCALED(pBot), MASK_PLAYERSOLID, pBot, COLLISION_GROUP_PLAYER, &trace);

						if (trace.fraction == 1.0)
						{
							maxtries = -1;
							//Msg("Trace not found!\n");
						}
						else
						{
							angle.y += dir*angledelta;
							botdata->turns += angledelta;


							if (angle.y > 180)
								angle.y -= 360;
							else if (angle.y < -180)
								angle.y += 360;

							botdata->forwardAngle = angle;
							botdata->lastAngles = angle;
						}
					}
					//Msg("%d %.02f\n", maxtries, angle.y);
					if (maxtries == -1)
					{
						angle.y -= 180.0f;
						if (angle.y > 180)
							angle.y -= 360;
						else if (angle.y < -180)
							angle.y += 360;
					}
					if (botdata->turns >= 360.0f)
					{
						botdata->left = !botdata->left;
						botdata->turns = 0.0f;
					}
					botdata->lastAngles = angle;
					botdata->wildReverse = gpGlobals->curtime + 1.0f;
				}
			}
			else if (botdata->guardTimer == 0.0f)
			{
				if (botdata->bCombat)
				{
					CHL2MP_Player *pPlayer = ToHL2MPPlayer( UTIL_PlayerByIndex( botdata->m_lastPlayerCheck ) );
					if (pPlayer)
					{
						bool isVampDoll = pPlayer->KO && pPlayer->myServerRagdoll != NULL;
						if (isVampDoll)
						{
							forward = (pPlayer->myServerRagdoll->GetAbsOrigin()) - (pBot->GetAbsOrigin()+Vector(0,0,72));
						}
						else
						{
							int accuracy = 24 - bot_difficulty.GetInt() * 8;

							if (pBot->GetTeamNumber() == COVEN_TEAMID_SLAYERS)
								forward = (pPlayer->GetLocalOrigin() + Vector(random->RandomInt(-accuracy, accuracy), random->RandomInt(-accuracy, accuracy), random->RandomInt(-accuracy, accuracy))) - pBot->GetLocalOrigin();
							else //vampires have perfect accuracy... doesn't really matter?
								forward = pPlayer->GetLocalOrigin() - pBot->GetLocalOrigin();
						}
						VectorAngles(forward, angle);
						botdata->forwardAngle = angle;
						botdata->lastAngles = angle;
						if (isVampDoll)
							buttons |= IN_ATTACK;
						else
						{
							UTIL_TraceLine(pBot->GetAbsOrigin() + Vector(0, 0, 72), pPlayer->GetAbsOrigin() + Vector(0, 0, 72), MASK_SHOT, pBot, COLLISION_GROUP_PLAYER, &trace);
							if (trace.DidHitNonWorldEntity() || trace.fraction == 1.0f)
							{
								CBaseCombatWeapon *pActiveWeapon = pBot->GetActiveWeapon();
								if (pActiveWeapon && pActiveWeapon->CanFire())
									buttons |= IN_ATTACK;
							}
						}
						if (pBot->GetTeamNumber() == COVEN_TEAMID_SLAYERS && !isVampDoll)
						{
							botdata->backwards = true;
						}
						else if (pBot->GetTeamNumber() == COVEN_TEAMID_SLAYERS)
						{
							forwardmove = botdata->m_flBaseSpeed;
							if (bot_difficulty.GetInt() < 2)
								buttons |= IN_DUCK;
							else if ((pPlayer->myServerRagdoll->GetAbsOrigin() - pBot->GetAbsOrigin()).Length() < 150)
								buttons |= IN_DUCK;
						}
						//Vampires attempt to juke on hard mode, except charging gores.
						else if (pBot->GetTeamNumber() == COVEN_TEAMID_VAMPIRES && !(pBot->covenClassID == COVEN_CLASSID_GORE && pBot->gorelock) && forward.Length() > 150.0f)
						{
							forwardmove = botdata->m_flBaseSpeed;
							if (bot_difficulty.GetInt() > 1)
							{
								if (gpGlobals->curtime > botdata->nextstrafetime)
								{
									if (botdata->sidemove == 0.0f)
										botdata->sidemove = botdata->m_flBaseSpeed * (random->RandomInt(0, 2) - 1);
									else
										botdata->sidemove = -botdata->sidemove;
									botdata->nextstrafetime = gpGlobals->curtime + random->RandomFloat(0.5f, 2.5f);
								}
								sidemove = botdata->sidemove;
							}
						}
					}
				}
				else if (botdata->m_targetNode >= 0 && pRules->botnet[botdata->m_targetNode] != NULL)
				{
					if (!botdata->bLost)
						forward = pRules->botnet[botdata->m_targetNode]->location - pBot->GetLocalOrigin();
					//if (pBot->GetTeamNumber() == 2)
					//	Msg("%f %f %f\n", forward.x, forward.y, forward.z);
					VectorAngles(forward, angle);
					botdata->forwardAngle = angle;
					botdata->lastAngles = angle;
				}
			}
			else if (gpGlobals->curtime > botdata->guardTimer)
			{
				botdata->guardTimer = 0.0f;
				botdata->bGuarding = false;
				GetLost(pBot);
			}
			else
			{
				//Don't try to cap and already capped point
				if (CheckObjective(pBot, true))
				{
					//guardtimer is set, slowly rotate and crouch
					if (botdata->bGuarding)
					{
						angle.y += 4.0f;
						if (angle.y > 180.0f)
							angle.y -= 360.0f;
						else if (angle.y < -180.0f)
							angle.y += 360.0f;
					}
					else
					{
						Vector objloc = CurrentObjectiveLoc(pBot);
						objloc.z = pBot->GetAbsOrigin().z;
						forward = objloc - pBot->GetLocalOrigin();
						//Msg("%d %.02f\n", botdata->m_objective, forward.Length());
						if (botdata->m_objective > -1 && forward.Length() > 72 && !botdata->bCombat)
						{
							VectorAngles(forward, angle);
						}
						else
						{
							botdata->bGuarding = true;
						}
					}
					buttons |= IN_DUCK;
					botdata->forwardAngle = angle;
					botdata->lastAngles = angle;
					forwardmove = botdata->m_flBaseSpeed;
				}
			}

            //X BOTPATCH dont do sidemove/strafetime if guardtimer... no it's fine for now
			if (gpGlobals->curtime >= botdata->nextstrafetime)
			{
				botdata->nextstrafetime = gpGlobals->curtime + 1.0f;

				if (botdata->bCombat && random->RandomInt(0, 5) == 0)
				{
					botdata->sidemove = botdata->m_flBaseSpeed * (random->RandomInt(0, 2) - 1);
				}
				else
				{
					botdata->sidemove = 0;
				}
				sidemove = botdata->sidemove;

				/*if ( random->RandomInt( 0, 20 ) == 0 )
				{
				botdata->backwards = true;
				}
				else
				{
				botdata->backwards = false;
				}*/
			}
			if (botdata->sidemove != 0.0f)
			{
				trace_t	tr;
				Vector forward, right, up;
				AngleVectors(angle, &forward, &right, &up);
				if (sidemove < 0)
					right = -right;
				UTIL_TraceLine(pBot->GetAbsOrigin() + right * 16, pBot->GetAbsOrigin() - up * 25, MASK_SHOT_HULL, pBot, COLLISION_GROUP_WEAPON, &tr);
				if (tr.fraction == 1.0)
					sidemove = botdata->sidemove = 0;
			}
		}
		else
			GetLost(pBot);
	}

	//BB: I just don't even have any words...
	if (!botdata->bLost)
		pBot->SetLocalAngles(botdata->lastAngles);

	//Stuck calcs
	if (!(pBot->GetFlags() & FL_FROZEN) && !(pBot->GetEFlags() & EFL_BOT_FROZEN))
	{
		if (botdata->guardTimer == 0.0f && (pBot->GetLocalOrigin() - botdata->lastPos).Length() < 10.0f) //STUCK? 4.0
		{
			if (botdata->stuckTimer > 0.0f)
			{
				if (gpGlobals->curtime - botdata->stuckTimer >= 0.2f && gpGlobals->curtime >= botdata->nextusetime)
				{
					//door? ladder?
					if (pBot->GetMoveType() != MOVETYPE_LADDER)
					{
						buttons |= IN_USE;
						botdata->nextusetime = gpGlobals->curtime + 3.0f;
					}
				}
				if (gpGlobals->curtime - botdata->stuckTimer >= 0.5f && gpGlobals->curtime >= botdata->nextjumptime && pBot->GetMoveType() != MOVETYPE_LADDER)
				{
					//try a jump
					buttons |= IN_JUMP;
					botdata->nextjumptime = gpGlobals->curtime + 3.5f;
				}
				if (gpGlobals->curtime - botdata->stuckTimer >= 1.0f)
				{
					//try a sidestep
					botdata->nextstrafetime = gpGlobals->curtime + 1.0f;
					botdata->sidemove = botdata->m_flBaseSpeed;
				}
				if (gpGlobals->curtime - botdata->stuckTimer >= 1.3f && botdata->goWild == 0.0f) //STUCK!
				{
					botdata->goWild = gpGlobals->curtime + 2.0f;
					botdata->turns = 0.0f;

					botdata->stuckTimer = 0.0f;
					botdata->strikes++;
				}
			}
			else
			{
				botdata->stuckTimer = gpGlobals->curtime;
			
			}
		}
		else
		{
			botdata->lastPos = pBot->GetLocalOrigin();
			botdata->stuckTimer = 0.0f;
			botdata->strikes = 0;
		}
	}

	//BB: strikes clause
	if (botdata->strikes == 3 && !(pBot->GetFlags() & FL_FROZEN) && !(pBot->GetEFlags() & EFL_BOT_FROZEN))
	{
		botdata->strikes = 0;
		pBot->CommitSuicide();
		return;
	}

	//X BOTPATCH remove this code into BotAbilityThink() and add more intelligent leveling for passives
	//This needs to be the last call... because we are going to be modifying view angles for abilities
	buttons |= Bot_Ability_Think(pBot);

	if (botdata->m_flOverrideDur > 0.0f)
	{
		pBot->SetLocalAngles(botdata->overrideAngle);
		if (gpGlobals->curtime > botdata->m_flOverrideDur)
			botdata->m_flOverrideDur = 0.0f;
	}

	//Msg("%d %.02f %.02f\n", pBot->covenClassID, botdata->m_flBaseSpeed, forwardmove);
	RunPlayerMove( pBot, pBot->GetLocalAngles(), forwardmove, sidemove, upmove, buttons, impulse, frametime );
}




//#endif

