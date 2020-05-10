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
#include "hl2mp_bot_temp.h"


void ClientPutInServer( edict_t *pEdict, const char *playername );
void Bot_Think( CHL2MP_Player *pBot );

#if (defined(DEBUG_BOTS) || defined(DEBUG_BOTS_VISUAL))
//Debug visualization
ConVar	bot_debug("bot_debug", "3");
#endif

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

//#define BOT_SPEED_LIMIT	0.2f //78.46 degrees
#define BOT_SPEED_LIMIT_DEG	1620.0f //1620 degrees / second
#define BOT_SPEED_LIMIT_DEG_COMBAT	180.0f //900 degrees / second
#define BOT_REFLEX_DOT	0.85f //31.79 degrees
#define BOT_COMBAT_DOT	0.3f //72.5 degrees
#define BOT_ATTACK_DOT_LOOSE	0.85f //31.79 degrees
#define BOT_ATTACK_DOT_STRICT	0.95f //18.19 degrees
#define BOT_REACTION_TIME 0.3f //per difficulty
#define BOT_MAX_REACTION_TIME 0.9f //3 * REACTION_TIME

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
	float			stuckTimerThink;
	bool			stuckPCheck;
	float			goWild;
	bool			wildReverse;
	float			spawnTimer;
	float			guardTimer;
	bool			bGuarding;

	QAngle			forwardAngle;
	QAngle			objectiveAngle;
	QAngle			combatAngle;
	
	float			m_flJoinTeamTime;
	int				m_WantedTeam;
	int				m_WantedClass;

	int				m_lastPlayerCheck; //index
	CHandle<CBaseCombatCharacter> pCombatTarget;
	bool			bCombat;
	bool			bForceCombat;
	float			m_flLastCombat;
	float			m_flLastCombatDist;
	float			m_lastPlayerDot;
	float			m_flReactionTime;
	bool			alreadyReacted;
	float			lastThinkTime;

	QAngle			overrideAngle;
	float			m_flOverrideDur;
	float			m_flAbility1Timer;
	float			m_flAbility2Timer;
	float			m_flAbility3Timer;
	float			m_flAbility4Timer;

	int				m_lastNode; //index
	int				m_lastNodeProbe; //index (not id) of last probe into the botnet
	int				m_targetNode; //index
	bool			bPassedNodeLOS;
	bool			bLost;
	bool			bIgnoreZ;
	bool			bVisCheck;
	float			m_flBaseSpeed;

	int				m_objectiveType; //-1 none, 0 rogue, 1 cap point, 2 cts
	int				m_objective; //index
	int				m_lastCheckedObjective;

	int				m_role;

	int				strikes;
} botdata_t;

static botdata_t g_BotData[ MAX_PLAYERS ];

void Bot_Combat_Check( CHL2MP_Player *pBot, CBaseEntity *pAtk )
{
	if (!pBot)
		return;

	if (g_BotData[pBot->entindex()-1].bCombat || pAtk->GetTeamNumber() == pBot->GetTeamNumber())
		return;
	
	//BB: easy bots don't fight back
	if (bot_difficulty.GetInt() < 1)
		return;

	CBaseCombatCharacter *pAttacker = ToBaseCombatCharacter(pAtk);

	if (pAttacker)
	{
		g_BotData[pBot->entindex() - 1].m_lastPlayerCheck = -1;
		if (pAttacker->IsPlayer())
			g_BotData[pBot->entindex() - 1].m_lastPlayerCheck = pAttacker->entindex();
		g_BotData[pBot->entindex() - 1].pCombatTarget = pAttacker;
	}
	else
		return;

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

CBaseCombatCharacter *BotGetEnemy(CHL2MP_Player *pBot)
{
	botdata_t *botdata = &g_BotData[ENTINDEX(pBot->edict()) - 1];
	if (botdata->pCombatTarget != NULL)
	{
		return botdata->pCombatTarget;
	}
	return 	ToBaseCombatCharacter(UTIL_PlayerByIndex(botdata->m_lastPlayerCheck));
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
	g_BotData[pPlayer->entindex() - 1].wildReverse = true;
	g_BotData[pPlayer->entindex() - 1].stuckTimer = 0.0f;
	g_BotData[pPlayer->entindex() - 1].stuckTimerThink = 0.0f;
	g_BotData[pPlayer->entindex()-1].spawnTimer = 0.0f;
	g_BotData[pPlayer->entindex()-1].guardTimer = 0.0f;
	g_BotData[pPlayer->entindex() - 1].bLost = true;
	g_BotData[pPlayer->entindex() - 1].bIgnoreZ = false;
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
	g_BotData[pPlayer->entindex() - 1].m_flLastCombatDist = MAX_TRACE_LENGTH;
	g_BotData[pPlayer->entindex() - 1].bGuarding = false;
	g_BotData[pPlayer->entindex() - 1].m_lastCheckedObjective = 0;
	g_BotData[pPlayer->entindex() - 1].m_lastPlayerDot = 1.0f;
	g_BotData[pPlayer->entindex() - 1].m_flReactionTime = -1.0f;
	g_BotData[pPlayer->entindex() - 1].alreadyReacted = false;
	g_BotData[pPlayer->entindex() - 1].stuckPCheck = true;
	g_BotData[pPlayer->entindex() - 1].bVisCheck = false;
	g_BotData[pPlayer->entindex() - 1].lastThinkTime = gpGlobals->curtime;
	g_BotData[pPlayer->entindex() - 1].pCombatTarget = NULL;
	g_BotData[pPlayer->entindex() - 1].bPassedNodeLOS = false;

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

void GetLost( CHL2MP_Player *pBot, bool iZ, bool visCheck )
{
#ifdef DEBUG_BOTS
	if (bot_debug.GetInt() == pBot->entindex())
	{
		Msg("GetLost: %s%s\n", iZ ? "ignoreZ " : "", visCheck ? "visCheck" : "");
	}
#endif
	botdata_t *botdata = &g_BotData[ ENTINDEX( pBot->edict() ) - 1 ];
	botdata->m_targetNode = 0;
	botdata->bPassedNodeLOS = false;
	botdata->m_lastNode = -1;
	botdata->bLost = true;
	botdata->bIgnoreZ = iZ;
	botdata->bVisCheck = visCheck;
	//Comendeering this function for resetall
	botdata->bCombat = botdata->bForceCombat = false;
	botdata->backwards = false;
	botdata->guardTimer = 0.0f;
	botdata->goWild = 0.0f;
	botdata->m_flReactionTime = -1.0f;
	botdata->alreadyReacted = false;

	botdata->m_lastNodeProbe = 0;
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
	
	//check our objectives validity
	if (doValidityCheck)
	{
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

unsigned int WeaponCheck(CHL2MP_Player *pBot)
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
							if (pActiveWeapon->NeedsReload1())
							{
								return IN_RELOAD;
							}
							else
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
							if (!pActiveWeapon->CanFire())
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
	
	CBaseCombatCharacter *pPlayer = BotGetEnemy(pBot);
	if (pPlayer && pPlayer->IsAlive() && pPlayer->GetTeamNumber() != pBot->GetTeamNumber())
	{
		Vector vecEnd;
		Vector vecSrc;
		bool isVampDoll = pPlayer->KO && pPlayer->myServerRagdoll != NULL;

		if (isVampDoll)
			vecSrc = pBot->EyePosition();// +Vector(0, 0, 36);
		else
			vecSrc = pBot->EyePosition();

		if (isVampDoll)
		{
			vecEnd = pPlayer->myServerRagdoll->GetAbsOrigin();
		}
		else
			vecEnd = pPlayer->GetPlayerMidPoint();

		Vector playerVec = vecEnd-vecSrc;
		float dist = playerVec.Length();

		if (botdata->bForceCombat || dist < 600) //400
		{
			float check = 0.1f;
			if (botdata->bForceCombat)
				check = 2.0f;
			if (isVampDoll || ((pPlayer->m_floatCloakFactor < check && !(pPlayer->GetFlags() & EF_NODRAW))))
			{
				Vector forward;
				AngleVectors(botdata->forwardAngle, &forward);
				VectorNormalize(playerVec);
				botdata->m_lastPlayerDot = DotProduct(playerVec,forward);
				float test = BOT_COMBAT_DOT;
				if (isVampDoll)
					test = 0.0f;
				if (botdata->bForceCombat || botdata->m_lastPlayerDot > test)
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
						botdata->m_flLastCombatDist = dist;
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
		GetLost(pBot, true, true);
	}

	botdata->bCombat = false;
	botdata->bForceCombat = false;
	botdata->m_flLastCombatDist = MAX_TRACE_LENGTH;
	botdata->pCombatTarget = NULL;
	botdata->m_lastPlayerCheck++;
	
	if (botdata->m_lastPlayerCheck > gpGlobals->maxClients)
		botdata->m_lastPlayerCheck = 0;
}

void FindNearestNode( CHL2MP_Player *pBot )
{
	CHL2MPRules *pRules;
	pRules = HL2MPRules();
	if (pRules->bot_node_count == 0 || !pBot->IsAlive())
		return;

	botdata_t *botdata = &g_BotData[ ENTINDEX( pBot->edict() ) - 1 ];

	botnode *temp, *cur;
	temp = pRules->botnet[botdata->m_lastNodeProbe];
	cur = pRules->botnet[botdata->m_targetNode];

	//skip "stop" nodes
	if (temp != NULL && temp->connectors.Count() > 1)
	{
		if (cur == NULL)
		{
#ifdef DEBUG_BOTS
			if (bot_debug.GetInt() == pBot->entindex())
			{
				Msg("FindNearestNode: First Node: %d\n", botdata->m_lastNodeProbe);
			}
#endif
			botdata->m_targetNode = botdata->m_lastNodeProbe;
		}
		//skip nodes without +/- 50 z
		else
		{
			//We found a closer node
			float z = abs(pBot->GetAbsOrigin().z - temp->location.z);
			if (botdata->bIgnoreZ || z < 50)
			{
				if ((botdata->bVisCheck && !botdata->bPassedNodeLOS) || ((pBot->GetAbsOrigin() - temp->location).Length() < (pBot->GetAbsOrigin() - cur->location).Length()))
				{
					if (botdata->bVisCheck)
					{
						trace_t trace;
						UTIL_TraceLine(pBot->EyePosition(), temp->location + Vector(0, 0, 16), MASK_SOLID, pBot, COLLISION_GROUP_PLAYER, &trace);
#ifdef DEBUG_BOTS_VISUAL
						if (bot_debug.GetInt() == pBot->entindex())
						{
							NDebugOverlay::Line(pBot->EyePosition(), temp->location + Vector(0, 0, 16), 255, 255, 0, false, 1.5f);
						}
#endif
						if (trace.fraction == 1.0f)
						{
#ifdef DEBUG_BOTS
							if (bot_debug.GetInt() == pBot->entindex())
							{
								Msg("FindNearestNode: LOS Found closer node: %d\n", botdata->m_lastNodeProbe);
							}
#endif
							botdata->m_targetNode = botdata->m_lastNodeProbe;
							botdata->bPassedNodeLOS = true;
						}
						else
						{
#ifdef DEBUG_BOTS
							if (bot_debug.GetInt() == pBot->entindex())
							{
								Msg("FindNearestNode: Failed LOS: %d\n", botdata->m_lastNodeProbe);
							}
#endif
						}
					}
					else
					{
#ifdef DEBUG_BOTS
						if (bot_debug.GetInt() == pBot->entindex())
						{
							Msg("FindNearestNode: NoLOS Found closer node: %d\n", botdata->m_lastNodeProbe);
						}
#endif
						botdata->m_targetNode = botdata->m_lastNodeProbe;
					}
				}
			}
			else
			{
#ifdef DEBUG_BOTS
				if (bot_debug.GetInt() == pBot->entindex())
				{
					Msg("FindNearestNode: ignoring Z: %d Z:%.2f\n", botdata->m_lastNodeProbe, z);
				}
#endif
			}
		}
	}

	botdata->m_lastNodeProbe++;
	if (botdata->m_lastNodeProbe > pRules->bot_node_count)
	{
#ifdef DEBUG_BOTS
		if (bot_debug.GetInt() == pBot->entindex())
		{
			Msg("FindNearestNode: Done! (%d/%d)\n", botdata->m_targetNode, pRules->bot_node_count);
		}
#endif
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

	if (!pBot->IsAlive() || pBot->KO)
		return 0;

	if (pBot->covenAbilities[COVEN_ABILITY_BATTLEYELL] > 0)
	{
		if (botdata->bCombat)
		{
			if (!(pBot->covenStatusEffects & COVEN_FLAG_BYELL))
			{
				int abilityNum = pBot->GetAbilityNumber(pBot->covenAbilities[COVEN_ABILITY_BATTLEYELL]);
				float cd = pBot->GetCooldown(abilityNum);
				if (gpGlobals->curtime >= cd || cd == 0.0f)
				{
					buttons |= pBot->covenAbilities[COVEN_ABILITY_BATTLEYELL];
				}
			}
		}
	}
	if (pBot->covenAbilities[COVEN_ABILITY_HASTE] > 0)
	{
		if (botdata->bCombat)
		{
			if (!(pBot->covenStatusEffects & COVEN_FLAG_SPRINT) && pBot->SuitPower_GetCurrentPercentage() > 5.0f)
			{
				int abilityNum = pBot->GetAbilityNumber(pBot->covenAbilities[COVEN_ABILITY_HASTE]);
				float cd = pBot->GetCooldown(abilityNum);
				if (gpGlobals->curtime >= cd || cd == 0.0f)
				{
					buttons |= pBot->covenAbilities[COVEN_ABILITY_HASTE];
					botdata->m_flAbility1Timer = 0.0f;
				}
			}
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
					buttons |= pBot->covenAbilities[COVEN_ABILITY_HASTE];
					botdata->m_flAbility1Timer = 0.0f;
				}
			}
		}
	}
	if (pBot->covenAbilities[COVEN_ABILITY_SHEERWILL] > 0)
	{
		if (botdata->bCombat)
		{
			if (!(pBot->covenStatusEffects & COVEN_FLAG_STATS))
			{
				int abilityNum = pBot->GetAbilityNumber(pBot->covenAbilities[COVEN_ABILITY_SHEERWILL]);
				float cd = pBot->GetCooldown(abilityNum);
				if (gpGlobals->curtime >= cd || cd == 0.0f)
				{
					buttons |= pBot->covenAbilities[COVEN_ABILITY_SHEERWILL];
				}
			}
		}
	}
	if (pBot->covenAbilities[COVEN_ABILITY_INTIMIDATINGSHOUT] > 0)
	{
		if (botdata->bCombat)
		{
			int abilityNum = pBot->GetAbilityNumber(pBot->covenAbilities[COVEN_ABILITY_INTIMIDATINGSHOUT]);
			float cd = pBot->GetCooldown(abilityNum);
			if (gpGlobals->curtime >= cd || cd == 0.0f && botdata->m_flLastCombatDist < COVEN_INTIMIDATINGSHOUT_DIST)
			{
				CBaseCombatCharacter *pEnemy = BotGetEnemy(pBot);
				if (pEnemy && pEnemy->IsAlive() && !pEnemy->KO)
					buttons |= pBot->covenAbilities[COVEN_ABILITY_INTIMIDATINGSHOUT];
			}
		}
	}
	//BB: TODO: random sneak walk
	//BB: TODO: phasing
	//BB: TODO: blood explode on cd and distance
	//BB: TODO: dodge
	if (pBot->covenAbilities[COVEN_ABILITY_LEAP] > 0)
	{
		if (botdata->bCombat)
		{
			int abilityNum = pBot->GetAbilityNumber(pBot->covenAbilities[COVEN_ABILITY_LEAP]);
			float cd = pBot->GetCooldown(abilityNum);
			if (gpGlobals->curtime >= cd || cd == 0.0f)
			{
				//CBaseCombatCharacter *pPlayer = BotGetEnemy(pBot);
				//if (pPlayer)
					//float distance = (pPlayer->GetLocalOrigin() - pBot->GetLocalOrigin()).Length();
				if (botdata->m_flLastCombatDist > 200.0f)
				{
					if (botdata->m_flAbility1Timer == 0.0f)
					{
						buttons |= IN_JUMP;
						botdata->m_flAbility1Timer = gpGlobals->curtime + 0.2f;
					}
					else
					{
						botdata->m_flAbility1Timer = 0.0f;
						buttons |= pBot->covenAbilities[COVEN_ABILITY_LEAP];
					}
				}
			}
		}
		else
		{
			if (!botdata->bLost)
			{
				CHL2MPRules *pRules;
				pRules = HL2MPRules();
				if (botdata->m_targetNode > -1 && pRules->botnet[botdata->m_targetNode] != NULL)
				{
					
					int abilityNum = pBot->GetAbilityNumber(pBot->covenAbilities[COVEN_ABILITY_LEAP]);
					float cd = pBot->GetCooldown(abilityNum);
					if (gpGlobals->curtime >= cd || cd == 0.0f)
					{
						if (botdata->goWild == 0.0f && botdata->stuckTimer == 0.0f && botdata->guardTimer == 0.0f)
						{
							float distance = (pRules->botnet[botdata->m_targetNode]->location - pBot->GetLocalOrigin()).Length();
							if (distance > 550.0f)
							{
								//BB: roof check
								trace_t trace;
								UTIL_TraceLine(pBot->GetLocalOrigin(), pBot->EyePosition() + Vector(0, 0, 128), MASK_PLAYERSOLID, pBot, COLLISION_GROUP_PLAYER, &trace);
								if (trace.fraction == 1.0f)
								{
									if (botdata->m_flAbility1Timer == 0.0f)
									{
										buttons |= IN_JUMP;
										botdata->m_flAbility1Timer = gpGlobals->curtime + 0.2f;
									}
									else
									{
										botdata->forwardAngle.x = -30;
										botdata->overrideAngle = botdata->forwardAngle;
										botdata->m_flOverrideDur = gpGlobals->curtime + 0.2f;
										botdata->m_flAbility1Timer = 0.0f;
										buttons |= pBot->covenAbilities[COVEN_ABILITY_LEAP];
									}
								}
							}
						}
					}
				}
			}
		}
	}
	if (pBot->covenAbilities[COVEN_ABILITY_BERSERK] > 0)
	{
		if (botdata->bCombat)
		{
			if (!(pBot->covenStatusEffects & COVEN_FLAG_BERSERK))
			{
				int abilityNum = pBot->GetAbilityNumber(pBot->covenAbilities[COVEN_ABILITY_BERSERK]);
				float cd = pBot->GetCooldown(abilityNum);
				if (gpGlobals->curtime >= cd || cd == 0.0f)
				{
					buttons |= pBot->covenAbilities[COVEN_ABILITY_BERSERK];
				}
			}
		}
	}
	if (pBot->covenAbilities[COVEN_ABILITY_DREADSCREAM] > 0)
	{
		if (botdata->bCombat)
		{
			int abilityNum = pBot->GetAbilityNumber(pBot->covenAbilities[COVEN_ABILITY_DREADSCREAM]);
			float cd = pBot->GetCooldown(abilityNum);
			if (gpGlobals->curtime >= cd || cd == 0.0f && botdata->m_flLastCombatDist < COVEN_DREADSCREAM_DIST)
			{
				buttons |= pBot->covenAbilities[COVEN_ABILITY_DREADSCREAM];
			}
		}
	}
	if (pBot->covenAbilities[COVEN_ABILITY_CHARGE] > 0)
	{
		if (botdata->bCombat)
		{
			int abilityNum = pBot->GetAbilityNumber(pBot->covenAbilities[COVEN_ABILITY_CHARGE]);
			float cd = pBot->GetCooldown(abilityNum);
			if (pBot->SuitPower_GetCurrentPercentage() > 10.0f && (gpGlobals->curtime >= cd || cd == 0.0f))
			{
				CBaseCombatCharacter *pPlayer = BotGetEnemy(pBot);
				if (pPlayer)
				{
					Vector vStart = pPlayer->GetLocalOrigin();
					Vector vEnd = pBot->GetLocalOrigin();
					float distance = (vStart - vEnd).Length2D(); //ignore z for distance check (flung players should not trigger a charge)
					if (distance > 100.0f)
					{
						botdata->m_flAbility2Timer = distance;
						//BB: I don't think I need to do this... angles should be "correct" from other functions
						//Vector forward = pPlayer->GetLocalOrigin() - pBot->GetLocalOrigin();
						//VectorAngles(forward, botdata->forwardAngle);
						buttons |= pBot->covenAbilities[COVEN_ABILITY_CHARGE];
					}
					else if (pBot->gorelock > GORELOCK_NONE && distance > 24.0f && distance < botdata->m_flAbility2Timer)
					{
						botdata->m_flAbility2Timer = distance;
						buttons |= pBot->covenAbilities[COVEN_ABILITY_CHARGE];
					}
					else if (pBot->gorelock > GORELOCK_NONE)
						botdata->m_flAbility2Timer = 0.0f;
				}
			}
		}
		else
		{
			if (!botdata->bLost)
			{
				CHL2MPRules *pRules;
				pRules = HL2MPRules();
				if (botdata->m_targetNode > -1 && pRules->botnet[botdata->m_targetNode] != NULL)
				{
					int abilityNum = pBot->GetAbilityNumber(pBot->covenAbilities[COVEN_ABILITY_CHARGE]);
					float cd = pBot->GetCooldown(abilityNum);
					if (gpGlobals->curtime >= cd || cd == 0.0f)
					{
						if (botdata->goWild == 0.0f && botdata->stuckTimer == 0.0f && botdata->guardTimer == 0.0f && pBot->SuitPower_GetCurrentPercentage() > 10.0f)
						{
							float distance = (pRules->botnet[botdata->m_targetNode]->location - pBot->GetLocalOrigin()).Length();
							float deltaZ = abs(pRules->botnet[botdata->m_targetNode]->location.z - pBot->GetLocalOrigin().z);
							if (distance > 550.0f && deltaZ < 24.0f)
							{
								botdata->m_flAbility2Timer = distance;
								//BB: I don't think I need to do this... angles should be "correct" from other functions
								//Vector forward = pRules->botnet[botdata->m_targetNode]->location - pBot->GetLocalOrigin();
								//VectorAngles(forward, botdata->forwardAngle);
								buttons |= pBot->covenAbilities[COVEN_ABILITY_CHARGE];
							}
							else if (pBot->gorelock > GORELOCK_NONE && distance > 30.0f && distance < botdata->m_flAbility2Timer)
							{
								botdata->m_flAbility2Timer = distance;
								buttons |= pBot->covenAbilities[COVEN_ABILITY_CHARGE];
							}
							else if (pBot->gorelock > GORELOCK_NONE)
								botdata->m_flAbility2Timer = 0.0f;
						}
					}
				}
			}
		}
	}
	if (pBot->covenAbilities[COVEN_ABILITY_BLOODLUST] > 0)
	{
		if (botdata->bCombat)
		{
			if (!(pBot->covenStatusEffects & COVEN_FLAG_BLUST) )
			{
				int abilityNum = pBot->GetAbilityNumber(pBot->covenAbilities[COVEN_ABILITY_BLOODLUST]);
				float cd = pBot->GetCooldown(abilityNum);
				if (gpGlobals->curtime >= cd || cd == 0.0f)
				{
					buttons |= pBot->covenAbilities[COVEN_ABILITY_BLOODLUST];
				}
			}
		}
	}
	if (pBot->covenAbilities[COVEN_ABILITY_BUILDTURRET] > 0)
	{
	}
	if (pBot->covenAbilities[COVEN_ABILITY_BUILDDISPENSER] > 0)
	{
	}

	/*else if (pBot->covenClassID == COVEN_CLASSID_AVENGER)
	{
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
	if (botdata->bCombat)
	{
	float cd = pBot->GetCooldown(0);
	CHL2MP_Player *pPlayer = ToHL2MPPlayer(UTIL_PlayerByIndex(botdata->m_lastPlayerCheck));
	bool isVampDoll = false;
	if (pPlayer)
	{
	isVampDoll = pPlayer->KO && pPlayer->myServerRagdoll != NULL;
	}
	if (!isVampDoll && pBot->GetLoadout(0) > 0 && (gpGlobals->curtime >= cd || cd == 0.0f) && botdata->m_lastPlayerDot > BOT_ATTACK_DOT_LOOSE) //don't waste water on ragdolls
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
	botdata->forwardAngle.x = 60;
	botdata->overrideAngle = botdata->forwardAngle;
	botdata->m_flOverrideDur = gpGlobals->curtime + 0.1f;
	buttons |= IN_ABIL1;
	}
	}
	}
	}*/
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
				botdata->stuckPCheck = true;
				botdata->nextjumptime = 0.0f;
				botdata->nextusetime = 0.0f;
				botdata->m_role = -1;
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

	//Weapon Swap Check
	buttons |= WeaponCheck(pBot);

	//Health Check
	HealthCheck(pBot);

	float forwardmove = 0.0f;
	float sidemove = botdata->sidemove;
	float upmove = 0.0f;
	trace_t trace; //throwaway trace

	byte  impulse = 0;
	float frametime = gpGlobals->frametime;

	//Pathing calcs
	if (pRules->botnet[botdata->m_targetNode] == NULL || botdata->bLost)
	{
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
				UTIL_TraceLine(pBot->EyePosition(), objloc, MASK_SOLID, pBot, COLLISION_GROUP_DEBRIS, &trace);
				if (trace.fraction != 1.0)
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
		if (!botdata->bLost && botdata->guardTimer == 0.0f && (pRules->botnet[botdata->m_targetNode]->location - pBot->GetLocalOrigin()).Length() < BOT_NODE_TOLERANCE) //10
		{
#ifdef DEBUG_BOTS
			if (bot_debug.GetInt() == pBot->entindex())
			{
				Msg("Reached Node: %d\n", pRules->botnet[botdata->m_targetNode]->ID);
			}
#endif
			if (pRules->botnet[botdata->m_targetNode]->connectors.Count() <= 1)
			{
				//dead end get lost for now
				//GetLost(pBot);
				//get bored after 10 seconds
				//X BOTPATCH just remove condition... do a getlost
				GetLost(pBot, false, true);
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
#ifdef DEBUG_BOTS
				if (bot_debug.GetInt() == pBot->entindex())
				{
					Msg("Headed to Node: %d\n", pRules->botnet[botdata->m_targetNode]->ID);
				}
#endif
				botdata->m_lastNode = pushedval;
			}
		}
		else
		{
#ifdef DEBUG_BOTS_VISUAL
			if (pRules->botnet[botdata->m_targetNode] != NULL && bot_debug.GetInt() == pBot->entindex())
			{
				NDebugOverlay::SweptBox(pRules->botnet[botdata->m_targetNode]->location, pRules->botnet[botdata->m_targetNode]->location + Vector(0, 0, 72), Vector(0, -16, -16), Vector(0, 16, 16), QAngle(90, 0, 0), 0, 255, 255, 50, 0.05f);
				NDebugOverlay::Sphere(pRules->botnet[botdata->m_targetNode]->location, QAngle(0, 0, 0), BOT_NODE_TOLERANCE, 255, 0, 0, 0, false, 0.05f);
			}
#endif
		}
	}

	//Actual movement calcs
	if (pBot->IsAlive() && (pBot->GetSolid() == SOLID_BBOX) && !pBot->IsEFlagSet(EFL_BOT_FROZEN))
	{
		Vector forward; //throwaway vector forward

		//BB: TODO: this mostly will work. Non-player NPCs do not properly compute MidPoints as they do not share a player hull. This needs to get fixed? Use center of mass from the collision model?
		//Combat Angle
		if (botdata->bForceCombat || botdata->bCombat)
		{
			CBaseCombatCharacter *pPlayer = BotGetEnemy(pBot);
			if (pPlayer)
			{
				bool isVampDoll = pPlayer->KO && pPlayer->myServerRagdoll != NULL;
				if (isVampDoll)
				{
					forward = (pPlayer->myServerRagdoll->GetAbsOrigin()) - (pBot->EyePosition());
				}
				else
				{
					if (pBot->GetTeamNumber() == COVEN_TEAMID_SLAYERS)
					{
						int accuracy = 24 - bot_difficulty.GetInt() * 8;
						forward = (pPlayer->GetPlayerMidPoint() + Vector(random->RandomInt(-accuracy, accuracy), random->RandomInt(-accuracy, accuracy), random->RandomInt(-accuracy, accuracy))) - pBot->EyePosition();
					}
					else //BB: vampires have perfect accuracy... doesn't really matter?
					{
						if (pPlayer->IsPlayer())
							forward = pPlayer->GetPlayerMidPoint() - pBot->EyePosition();
						else
						{
							forward = pPlayer->EyePosition() - pBot->EyePosition(); //BB: TODO: this will not work for supply depos?
							if (forward.Length() == 0)
								AngleVectors(pBot->GetLocalAngles(), &forward);
						}
					}
				}
				VectorAngles(forward, botdata->combatAngle);
				if (!isVampDoll)
				{
					UTIL_TraceLine(pBot->EyePosition(), pPlayer->GetPlayerMidPoint(), MASK_SHOT, pBot, COLLISION_GROUP_PLAYER, &trace);
					if ((trace.DidHitNonWorldEntity() || trace.fraction == 1.0f) && (botdata->m_lastPlayerDot > BOT_ATTACK_DOT_STRICT || !pPlayer->IsPlayer()))
					{
						CBaseCombatWeapon *pActiveWeapon = pBot->GetActiveWeapon();
						if (pActiveWeapon && pActiveWeapon->CanFire())
							buttons |= IN_ATTACK;
					}
				}
				if (pBot->GetTeamNumber() == COVEN_TEAMID_SLAYERS && !isVampDoll)
				{
					botdata->backwards = true;
					if (bot_difficulty.GetInt() > 0) //Slayers Attempt to juke
					{
						if (gpGlobals->curtime > botdata->nextstrafetime)
						{
							if (botdata->sidemove == 0.0f)
								botdata->sidemove = botdata->m_flBaseSpeed * (random->RandomInt(0, 2) - 1);// *0.5f;
							else
								botdata->sidemove = -botdata->sidemove;
							botdata->nextstrafetime = gpGlobals->curtime + random->RandomFloat(0.5f, 2.5f);
						}
						sidemove = botdata->sidemove;
					}
				}
				else if (pBot->GetTeamNumber() == COVEN_TEAMID_SLAYERS)
				{
					botdata->backwards = false;
					sidemove = botdata->sidemove = 0;
					if ((pPlayer->myServerRagdoll->GetAbsOrigin() - pBot->GetAbsOrigin()).Length() < 175 - 25 * bot_difficulty.GetInt())
					{
						CBaseCombatWeapon *pWeapon = pBot->Weapon_OwnsThisType("weapon_stake");
						CBaseCombatWeapon *pActiveWeapon = pBot->GetActiveWeapon();
						if (pWeapon)
						{
							// Switch to it if we don't have it out
							// Switch?
							if (pActiveWeapon != pWeapon)
							{
								pBot->Weapon_Switch(pWeapon);
							}
						}
						buttons |= IN_ATTACK;
						buttons |= IN_DUCK;
					}
				}
				//Vampires attempt to juke, except charging gores.
				else if (pBot->GetTeamNumber() == COVEN_TEAMID_VAMPIRES)
				{
					if (bot_difficulty.GetInt() > 0)
					{
						if (!(pBot->covenAbilities[COVEN_ABILITY_CHARGE] > 0 && pBot->gorelock > GORELOCK_NONE) && forward.Length() > 150.0f)
						{
							if (gpGlobals->curtime > botdata->nextstrafetime)
							{
								if (botdata->sidemove == 0.0f)
									botdata->sidemove = floor(botdata->m_flBaseSpeed * (random->RandomInt(0, 2) - 1));//*0.5f
								else
									botdata->sidemove = -botdata->sidemove;
								botdata->nextstrafetime = gpGlobals->curtime + random->RandomFloat(0.5f, 2.5f);
							}
							sidemove = botdata->sidemove;
						}
						else
						{
							botdata->nextstrafetime = gpGlobals->curtime + 1.0f;
							sidemove = botdata->sidemove = 0;
						}
					}
				}
			}
		}
		else
		{
			//Clear sidestep every so often
			if (gpGlobals->curtime >= botdata->nextstrafetime)
			{
				botdata->nextstrafetime = gpGlobals->curtime + 1.0f;
				sidemove = botdata->sidemove = 0;
			}
		}

		//Objective Angle
		if (pRules->botnet[botdata->m_targetNode] != NULL)
		{
			botdata->objectiveAngle = botdata->forwardAngle;
			//GoWild -> attempt to fix a stuck condition
			if (botdata->goWild > 0.0f)
			{
				if (gpGlobals->curtime > botdata->goWild)
				{
					botdata->goWild = 0.0f;
					botdata->wildReverse = true;
					botdata->turns = 0.0f;
					GetLost(pBot, true, true);
				}
				else if (botdata->wildReverse)
				{
					Vector vecEnd, vecSrc;
					float angledelta = 15.0f;
					int maxtries = (int)360.0 / angledelta;

					int dir = -1;
					if (botdata->left)
						dir = 1;

					botdata->objectiveAngle.x = 0;
					AngleVectors(botdata->objectiveAngle, &forward);
					//BB: center of mass is causing hell. Use a toecalc.
					vecSrc = pBot->GetLocalOrigin();// pBot->GetPlayerMidPoint() + 16.0f * forward; //player width is 32d

					while (--maxtries >= 0)
					{
						AngleVectors(botdata->objectiveAngle, &forward);
						vecEnd = vecSrc + forward * 32.0f; //10

						//UTIL_TraceLine(vecSrc, vecEnd, MASK_PLAYERSOLID, pBot, COLLISION_GROUP_PLAYER, &trace);
#ifdef DEBUG_BOTS_VISUAL
						if (bot_debug.GetInt() == pBot->entindex())
						{
							//NDebugOverlay::Line(vecSrc, vecEnd, 255, 0, 0, false, 1.0f);
							//NDebugOverlay::BoxDirection(vecSrc, VEC_HULL_MIN_SCALED(pBot), VEC_HULL_MAX_SCALED(pBot), forward, 255, 0, 0, 50, 1.0f);
							NDebugOverlay::SweptBox(vecSrc, vecEnd, VEC_HULL_MIN_SCALED(pBot), VEC_HULL_MAX_SCALED(pBot), botdata->objectiveAngle, 255, 0, 0, 150, 10.0f);
						}
#endif
						UTIL_TraceHull(vecSrc, vecEnd, VEC_HULL_MIN_SCALED(pBot), VEC_HULL_MAX_SCALED(pBot), MASK_PLAYERSOLID, pBot, COLLISION_GROUP_PLAYER, &trace);

						if (trace.fraction == 1.0)
						{
							maxtries = -1;
							//Msg("Trace not found!\n");
						}
						else
						{
							botdata->objectiveAngle.y += dir*angledelta;
							botdata->turns += angledelta;

							botdata->objectiveAngle.y = AngleNormalize(botdata->objectiveAngle.y);
						}
					}
					//Msg("%d %.02f\n", maxtries, botdata->objectiveAngle.y);
					if (maxtries == -1)
					{
						botdata->objectiveAngle.y -= 180.0f;
						botdata->objectiveAngle.y = AngleNormalize(botdata->objectiveAngle.y);
					}
					if (botdata->turns >= 360.0f)
					{
						botdata->left = !botdata->left;
						botdata->turns = 0.0f;
					}
					botdata->overrideAngle = botdata->objectiveAngle;
					botdata->m_flOverrideDur = gpGlobals->curtime + 2.0f;
					botdata->wildReverse = false;
					//botdata->objectiveAngle.y -= 180.0f;
					//botdata->objectiveAngle.y = AngleNormalize(botdata->objectiveAngle.y);
					//GetLost(pBot, true, true);
					//botdata->goWild = 0.0f;
				}
			}
			else if (botdata->guardTimer == 0.0f)
			{
				if (botdata->m_targetNode >= 0 && pRules->botnet[botdata->m_targetNode] != NULL)
				{
					forward = pRules->botnet[botdata->m_targetNode]->location - pBot->GetLocalOrigin();
					VectorAngles(forward, botdata->objectiveAngle);
				}
			}
			else if (gpGlobals->curtime > botdata->guardTimer)
			{
				botdata->guardTimer = 0.0f;
				botdata->bGuarding = false;
				GetLost(pBot);
			}
			else //guardtimer is set, slowly rotate and crouch
			{
				//Don't try to cap and already capped point
				if (CheckObjective(pBot, true))
				{
					if (botdata->bGuarding)
					{
						botdata->objectiveAngle.y += 1.0f;
					}
					else
					{
						Vector objloc = CurrentObjectiveLoc(pBot);
						objloc.z = pBot->GetAbsOrigin().z;
						forward = objloc - pBot->GetLocalOrigin();
						//Msg("%d %.02f\n", botdata->m_objective, forward.Length());
						if (botdata->m_objective > -1 && forward.Length() > 72 && !botdata->bCombat)
						{
							VectorAngles(forward, botdata->objectiveAngle);
						}
						else
						{
							botdata->bGuarding = true;
						}
					}
					buttons |= IN_DUCK;
				}
			}
		}

		//Forward momentum calc
		forwardmove = botdata->m_flBaseSpeed * (botdata->backwards ? -1 : 1);
		buttons |= botdata->backwards ? IN_BACK : IN_FORWARD; //freaking WAT. okay HL2 ladders...
		//Sidemove correction if we can't
		/*if (botdata->sidemove != 0.0f)
		{
			Vector right, up;
			AngleVectors(angle, &forward, &right, &up); //need to check both angles!
			if (sidemove < 0)
				right = -right;
			UTIL_TraceLine(pBot->GetAbsOrigin() + right * 16, pBot->EyePosition(), MASK_SHOT_HULL, pBot, COLLISION_GROUP_WEAPON, &trace);
			if (trace.fraction != 1.0)
				sidemove = botdata->sidemove = 0;
			*******************************************
			DO I NEED TO DO THIS??? WILL I EXCEED MAX VELOCITY?
			//forwardmove *= random->RandomFloat(0.5f, 1.0f); //juke
		}*/
	}

	botdata->objectiveAngle.y = AngleNormalize(botdata->objectiveAngle.y);
	botdata->combatAngle.y = AngleNormalize(botdata->combatAngle.y);
	if (botdata->m_flOverrideDur > 0.0f)
	{
		botdata->objectiveAngle = botdata->overrideAngle;
		if (gpGlobals->curtime > botdata->m_flOverrideDur)
			botdata->m_flOverrideDur = 0.0f;
	}
	else if (pBot->GetMoveType() == MOVETYPE_LADDER)
	{
		if (!botdata->bLost)
		{
			if (pRules->botnet[botdata->m_targetNode]->location.z > pBot->GetLocalOrigin().z)
				botdata->objectiveAngle.x = -90;
			else
				botdata->objectiveAngle.x = 90;
		}
		else
			buttons |= IN_JUMP; //get off this wasn't intended!
	}
	//BB: I just don't even have any words... I have to set angles every think?
	QAngle *desiredAngle = &botdata->objectiveAngle;
	if (pRules->botnet[botdata->m_targetNode] != NULL)
	{
		if (botdata->bCombat || botdata->bForceCombat)
		{
			Vector forward, desForward;
			AngleVectors(botdata->forwardAngle, &forward);
			AngleVectors(botdata->combatAngle, &desForward);
			float dotprod = DotProduct(forward, desForward);
			if (botdata->alreadyReacted)
			{
				desiredAngle = &botdata->combatAngle;
			}
			else
			{
				if (dotprod < BOT_REFLEX_DOT)
				{
					if (botdata->m_flReactionTime < 0.0f)
					{
						float reactionTime = BOT_MAX_REACTION_TIME - BOT_REACTION_TIME * bot_difficulty.GetInt();
						botdata->m_flReactionTime = gpGlobals->curtime + reactionTime;
						//Keep desiredangle = objectiveangle
					}
					else if (botdata->m_flReactionTime == 0.0f)
					{
						float reactionTime = BOT_MAX_REACTION_TIME - BOT_REACTION_TIME * bot_difficulty.GetInt();
						botdata->m_flReactionTime = gpGlobals->curtime + reactionTime;
						desiredAngle = NULL;
					}
				}
			}

			if (gpGlobals->curtime > botdata->m_flReactionTime)
			{
				botdata->m_flReactionTime = 0.0f;
				botdata->alreadyReacted = true;
			}


			if (dotprod >= BOT_REFLEX_DOT)
			{
				botdata->alreadyReacted = false;
				botdata->m_flReactionTime = 0.0f;
				desiredAngle = &botdata->combatAngle;
			}
		}
		
		//BB: speedlimit bots
		//BB: this is purposely not else... we can do this in the same think.
		if (desiredAngle)
		{
			if (botdata->m_flReactionTime <= 0.0f)
			{
				float thinkTime = gpGlobals->curtime - botdata->lastThinkTime;
				float speedLimit = BOT_SPEED_LIMIT_DEG * thinkTime;
				if (botdata->bCombat)
					speedLimit = (BOT_SPEED_LIMIT_DEG_COMBAT + 360 * bot_difficulty.GetInt()) * thinkTime;
				float yaw = botdata->forwardAngle.y - desiredAngle->y;
				yaw = AngleNormalize(yaw);
				//Msg("%.2f %.2f %.2f %.2f %.2f\n", botdata->forwardAngle.y, desiredAngle->y, yaw, thinkTime, speedLimit);
				if (abs(yaw) > speedLimit)
				{
					if (yaw > 0)
						desiredAngle->y = botdata->forwardAngle.y - speedLimit;
					else
						desiredAngle->y = botdata->forwardAngle.y + speedLimit;

					desiredAngle->y = AngleNormalize(desiredAngle->y);
					//Msg("%.2f %.2f\n\n", botdata->forwardAngle.y, desiredAngle->y);
				}

				//Pitch
				float pitch = botdata->forwardAngle.x - desiredAngle->x;
				pitch = AngleNormalize(pitch);
				if (abs(pitch) > speedLimit)
				{
					if (pitch > 0)
						desiredAngle->x = botdata->forwardAngle.x - speedLimit;
					else
						desiredAngle->x = botdata->forwardAngle.x + speedLimit;

					desiredAngle->x = AngleNormalize(desiredAngle->x);
				}
			}
			botdata->forwardAngle = *desiredAngle;
		}
	}
	pBot->SetLocalAngles(botdata->forwardAngle);

	//Stuck calcs
	if (!(pBot->GetFlags() & FL_FROZEN) && !(pBot->GetEFlags() & EFL_BOT_FROZEN))
	{
		float velocity = 0.0f;
		if (pBot->GetMoveType() == MOVETYPE_LADDER)
		{
			velocity = pBot->GetAbsVelocity().Length();
		}
		else
		{
			velocity = pBot->GetAbsVelocity().Length2D();
		}
		if (botdata->guardTimer == 0.0f && velocity < 30.0f) //STUCK? 10.0 4.0
		{
			if (botdata->stuckTimer > 0.0f)
			{
				//BB: combat shortcut to jump only?
				if (gpGlobals->curtime > botdata->stuckTimerThink)
				{
					if (pRules->botnet[botdata->m_targetNode] != NULL)
					{
						trace_t trace;
						Vector forward;
						AngleVectors(botdata->forwardAngle, &forward);
						UTIL_TraceLine(pBot->EyePosition(), pRules->botnet[botdata->m_targetNode]->location, MASK_SHOT, pBot, COLLISION_GROUP_PLAYER, &trace);
						if (trace.DidHitNonWorldEntity() && trace.m_pEnt)
						{
							//try a sidestep (collided with another player going the opposite direction)
							if (trace.m_pEnt->IsPlayer() && ((CHL2MP_Player *)trace.m_pEnt)->GetTeamNumber() == pBot->GetTeamNumber())
							{
								if (botdata->stuckPCheck)
								{
#ifdef DEBUG_BOTS
									if (bot_debug.GetInt() == pBot->entindex())
									{
										Msg("Stuck: Encountered Player: %d Trying Sidestep!\n", trace.m_pEnt->entindex());
									}
#endif
									botdata->nextstrafetime = gpGlobals->curtime + 1.0f;
									botdata->sidemove = botdata->m_flBaseSpeed;
									botdata->stuckPCheck = false;
								}
							}
							//Door
							else if (FClassnameIs(trace.m_pEnt, "prop_door_rotating") || FClassnameIs(trace.m_pEnt, "func_door") || FClassnameIs(trace.m_pEnt, "func_door_rotating"))
							{
								if (gpGlobals->curtime >= botdata->nextusetime)
								{
#ifdef DEBUG_BOTS
									if (bot_debug.GetInt() == pBot->entindex())
									{
										Msg("Stuck: Encountered Door: \"%s\"!\n", trace.m_pEnt->GetClassname());
									}
#endif
									buttons |= IN_USE;
									botdata->nextusetime = gpGlobals->curtime + 1.0f;
								}
							}
							else
							{
								//try a jump
								if (gpGlobals->curtime >= botdata->nextjumptime && pBot->GetMoveType() != MOVETYPE_LADDER)
								{
#ifdef DEBUG_BOTS
									if (bot_debug.GetInt() == pBot->entindex())
									{
										Msg("Stuck: Encountered Entity: \"%s\"Trying Jump!\n", trace.m_pEnt->GetClassname());
									}
#endif
									buttons |= IN_JUMP;
									botdata->nextjumptime = gpGlobals->curtime + 1.5f;
								}
							}
						}
						//Air
						else
						{
							//try a jump
							if (gpGlobals->curtime >= botdata->nextjumptime && pBot->GetMoveType() != MOVETYPE_LADDER)
							{
#ifdef DEBUG_BOTS
								if (bot_debug.GetInt() == pBot->entindex())
								{
									Msg("Stuck: No Obstacle: Trying Jump!\n");
								}
#endif
								buttons |= IN_JUMP;
								botdata->nextjumptime = gpGlobals->curtime + 1.5f;
							}
						}
						if (gpGlobals->curtime - botdata->stuckTimer >= 1.0f && botdata->goWild == 0.0f) //STUCK!
						{
							botdata->goWild = gpGlobals->curtime + 2.0f;
							botdata->wildReverse = true;
							botdata->turns = 0.0f;

							botdata->stuckTimer = 0.0f;
							botdata->stuckPCheck = true;
							botdata->strikes++;
#ifdef DEBUG_BOTS
							if (bot_debug.GetInt() == pBot->entindex())
							{
								Msg("Stuck! Going Wild! Strikes: %d\n", botdata->strikes);
							}
#endif
						}
					}
					botdata->stuckTimerThink = gpGlobals->curtime + 0.1f;
				}
			}
			else
			{
				botdata->stuckTimer = gpGlobals->curtime;
			}
		}
		else if (botdata->stuckTimer > 0.0f)
		{
#ifdef DEBUG_BOTS
			if (bot_debug.GetInt() == pBot->entindex())
			{
				Msg("Stuck condition alleviated!\n");
			}
#endif
			botdata->stuckTimer = 0.0f;
			botdata->strikes = 0;
		}
	}

	botdata->lastThinkTime = gpGlobals->curtime;

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

	//Msg("%d %.02f %.02f %.02f %.02f\n", pBot->covenClassID, botdata->m_flBaseSpeed, forwardmove, sidemove, botdata->nextstrafetime);
	RunPlayerMove(pBot, botdata->forwardAngle, forwardmove, sidemove, upmove, buttons, impulse, frametime);
}




//#endif

