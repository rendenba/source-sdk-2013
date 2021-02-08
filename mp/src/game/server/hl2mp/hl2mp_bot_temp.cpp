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
#include "covenbuilding.h"
#include "coven_turret.h"
#include "weapon_frag.h"

//Abilities 1-4
#define CLEAR_ABILITY_KEYS ~(IN_ABIL1 | IN_ABIL2 | IN_ABIL3 | IN_ABIL4) 

void ClientPutInServer( edict_t *pEdict, const char *playername );
void Bot_Think( CHL2MP_Player *pBot );

ConVar bot_debug_select("bot_debug_select", "2", FCVAR_CHEAT, "Selected entindex bot to debug.");
ConVar coven_debug_visual("coven_debug_visual", "0", FCVAR_CHEAT, "Debug coven visually.");

extern ConVar sv_coven_refuel_distance;
extern ConVar sv_coven_hp_per_ragdoll;

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

ConVar bot_difficulty("bot_difficulty", "1", FCVAR_GAMEDLL | FCVAR_NOTIFY, "Bot difficulty: 0 easy, 1 medium, 2 hard, 3 insane.");
ConVar bot_charge_combat("bot_charge_combat", "1600.0", FCVAR_GAMEDLL | FCVAR_NOTIFY, "Combat charge distance. (squared)");
ConVar bot_charge_noncombat("bot_charge_noncombat", "160000.0", FCVAR_GAMEDLL | FCVAR_NOTIFY, "Non-combat charge distance. (squared)");
ConVar bot_leap_combat("bot_leap_combat", "40000.0", FCVAR_GAMEDLL | FCVAR_NOTIFY, "Combat leap distance. (squared)");
ConVar bot_leap_noncombat("bot_leap_noncombat", "160000.0", FCVAR_GAMEDLL | FCVAR_NOTIFY, "Non-combat leap distance. (squared)");
ConVar bot_phase_combat("bot_phase_combat", "8100.0", FCVAR_GAMEDLL | FCVAR_NOTIFY, "Combat phase distance. (squared)");
ConVar bot_dodge_combat("bot_dodge_combat", "8100.0", FCVAR_GAMEDLL | FCVAR_NOTIFY, "Combat dodge distance. (squared)");

#ifdef NEXT_BOT
extern ConVar bot_mimic;
#else
ConVar bot_mimic( "bot_mimic", "0", 0, "Bot uses usercmd of player by index." );
#endif

static int BotNumber = 1;
static int g_iNextBotTeam = -1;
static int g_iNextBotClass = -1;

static char *botnames[2][14] =
{ { "Blade", "Gabriel Van Helsing", "Lucian", "Edgar Frog", "Allan Frog", "Anita Blake", "Simon Belmont", "Buffy Summers", "Abraham Van Helsing", "Mister", "Abraham Lincoln", "Quincey Morris", "Dr. Robert Neville", "Angel" },
{ "Edward Cullen", "Lestat de Lioncourt", "Louis de Pointe du Lac", "Liam", "Jeanette", "Therese", "Bill Compton", "Eric Northman", "Armand", "Eli", "David", "Dracula", "Count von Count", "Claudia" } };

typedef enum
{
	BOT_OBJECTIVE_UNDEFINED,
	BOT_OBJECTIVE_ROGUE,
	BOT_OBJECTIVE_CAPPOINT,
	BOT_OBJECTIVE_CTS,
	BOT_OBJECTIVE_GET_GAS,
	BOT_OBJECTIVE_RETURN_TO_APC,
	BOT_OBJECTIVE_FEED,
	BOT_OBJECTIVE_BUILD,
	BOT_OBJECTIVE_COUNT
} BotObjective_t;

typedef enum
{
	BOT_ROLE_UNDEFINED,
	BOT_ROLE_ATTACK,
	BOT_ROLE_DEFENSE,
	BOT_ROLE_COUNT
} BotRole_t;

//#define BOT_SPEED_LIMIT	0.2f //78.46 degrees
#define BOT_SPEED_LIMIT_DEG	1620.0f //1620 degrees / second
#define BOT_SPEED_LIMIT_DEG_COMBAT	90.0f //900 degrees / second
#define BOT_REFLEX_DOT	0.85f //31.79 degrees
#define BOT_COMBAT_DOT	0.3f //72.5 degrees
#define BOT_ATTACK_DOT_STRICT	0.95f //18.19 degrees
#define BOT_REACTION_TIME 0.3f //per difficulty
#define BOT_MAX_REACTION_TIME 0.9f //3 * REACTION_TIME

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

	float			m_flRespawnTime;
	bool			m_bIsPurchasingItems;
	bool			m_bHasNotPurchasedItems;

	int				m_lastPlayerCheck; //index
	BuildingType_t	m_lastBuildingChecked;
	CHandle<CBaseCombatCharacter> pCombatTarget;
	bool			bCombat;
	bool			bForceCombat;
	float			m_flLastCombat;
	Vector			m_vCombatLKP;
	const Vector	*m_vObjectiveLKP;
	float			m_flLastCombatDist;
	float			m_lastPlayerDot;
	float			m_flReactionTime;
	bool			alreadyReacted;
	float			lastThinkTime;
	float			flTimeout;

	const Vector	*m_pOverridePos;
	QAngle			overrideAngle;
	float			m_flOverrideDur;
	float			m_flAbilityTimer[COVEN_MAX_ABILITIES];

	int				m_lastNode; //index
	int				m_lastNodeProbe; //index (not id) of last probe into the botnet
	int				m_targetNode; //index
	bool			bPassedNodeLOS;
	bool			bLost;
	bool			bIgnoreZ;
	bool			bVisCheck;
	float			m_flBaseSpeed;
	bool			bNotIgnoringStrikes;

	BotObjective_t	m_objectiveType;
	int				m_objective; //index
	int				m_lastCheckedObjective;

	int				m_lastCheckedRagdoll;

	int				m_lastCheckedItem;

	float			m_flClassChange; //duration before checking to see if we want to change teams when dead

	BotRole_t		m_role;
	int				RN;

	int				strikes;
} botdata_t;

static botdata_t g_BotData[ MAX_PLAYERS ];

void Bot_Combat_Check( CHL2MP_Player *pBot, CBaseEntity *pAtk )
{
	if (!pBot || !pAtk)
		return;

	if (g_BotData[pBot->entindex()-1].bCombat || pAtk->GetTeamNumber() == pBot->GetTeamNumber())
		return;
	
	//BB: easy bots don't fight back
	if (bot_difficulty.GetInt() < 1)
		return;

	CBaseCombatCharacter *pAttacker = ToBaseCombatCharacter(pAtk);

	if (pAttacker)
	{
		g_BotData[pBot->entindex() - 1].m_lastPlayerCheck = 0;
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
	HL2MPRules()->botnameUsed[pBot->GetTeamNumber() - 2][g_BotData[pBot->entindex() - 1].RN] = false;
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
	CBaseCombatCharacter *pPlayer = ToBaseCombatCharacter(UTIL_PlayerByIndex(botdata->m_lastPlayerCheck));
	if (pPlayer && pPlayer->IsBuilderClass())
	{
		switch (botdata->m_lastBuildingChecked)
		{
			case BUILDING_DEFAULT:
			{
				botdata->m_lastBuildingChecked = BUILDING_AMMOCRATE;
				CHL2_Player *hl2Player = ToHL2Player(pPlayer);
				return ToBaseCombatCharacter(hl2Player->m_hDispenser);
			}
			case BUILDING_AMMOCRATE:
			{
				botdata->m_lastBuildingChecked = BUILDING_TURRET;
				CHL2_Player *hl2Player = ToHL2Player(pPlayer);
				return ToBaseCombatCharacter(hl2Player->m_hTurret);
			}
			default: case BUILDING_TURRET:
			{
				botdata->m_lastBuildingChecked = BUILDING_DEFAULT;
				return pPlayer;
			}
		}
	}
	else
		botdata->m_lastBuildingChecked = BUILDING_DEFAULT;

	return pPlayer;
}

float Bot_Velocity(CHL2MP_Player *pBot)
{
	botdata_t *botdata = &g_BotData[ENTINDEX(pBot->edict()) - 1];
	float vel = pBot->MaxSpeed();
	return vel * (botdata->backwards ? -1 : 1);
}

void Set_Bot_Base_Velocity(CHL2MP_Player *pBot)
{
	botdata_t *botdata = &g_BotData[ENTINDEX(pBot->edict()) - 1];
	CovenClassInfo_t *info = GetCovenClassData(pBot->covenClassID);
	botdata->m_flBaseSpeed = info->flBaseSpeed;
}

inline bool ValidTurret(CCoven_Turret *pTurret)
{
	CovenBuildingInfo_t *info = GetCovenBuildingData(BUILDING_TURRET);
	return !pTurret->bTipped && (pTurret->m_iHealth < pTurret->m_iMaxHealth || pTurret->m_iLevel < (info->iMaxLevel - 1) || pTurret->GetAmmo(3) < info->iAmmo1[pTurret->m_iLevel] + info->iAmmo2[pTurret->m_iLevel]);
}

//-----------------------------------------------------------------------------
// Purpose: Create a new Bot and put it in the game.
// Output : Pointer to the new Bot, or NULL if there's no free clients.
//-----------------------------------------------------------------------------
CBasePlayer *BotPutInServer(bool bFrozen, CovenTeamID_t iTeam)
{
	if (iTeam < 2)
		return NULL;

	g_iNextBotTeam = iTeam;

	//BotNumber
	char botname[ 64 ];
	int rn = random->RandomInt(0, 13);
	if (!HL2MPRules()->botnameUsed[iTeam - 2][rn])
	{
		Q_snprintf(botname, sizeof(botname), "%s", botnames[iTeam - 2][rn]);


		// This is an evil hack, but we use it to prevent sv_autojointeam from kicking in.

		edict_t *pEdict = engine->CreateFakeClient(botname);

		if (!pEdict)
		{
			Msg("Failed to create Bot.\n");
			return NULL;
		}

		HL2MPRules()->botnameUsed[iTeam - 2][rn] = true;

		// Allocate a CBasePlayer for the bot, and call spawn
		//ClientPutInServer( pEdict, botname );
		CHL2MP_Player *pPlayer = ((CHL2MP_Player *)CBaseEntity::Instance(pEdict));
		pPlayer->ClearFlags();
		//pPlayer->SetSolid(SOLID_VPHYSICS);
		pPlayer->AddFlag(FL_CLIENT | FL_FAKECLIENT);

		pPlayer->ChangeTeam(iTeam);

		if (bFrozen)
			pPlayer->AddEFlags(EFL_BOT_FROZEN);

		pPlayer->HandleCommand_SelectClass(0);

		BotNumber++;

		g_BotData[pPlayer->entindex() - 1].m_WantedTeam = iTeam;
		g_BotData[pPlayer->entindex() - 1].m_flJoinTeamTime = gpGlobals->curtime + 0.3;
		g_BotData[pPlayer->entindex() - 1].m_lastNode = -1;
		g_BotData[pPlayer->entindex() - 1].m_targetNode = 0;
		g_BotData[pPlayer->entindex() - 1].m_lastNodeProbe = 0;
		g_BotData[pPlayer->entindex() - 1].m_lastPlayerCheck = 0;
		g_BotData[pPlayer->entindex() - 1].goWild = 0.0f;
		g_BotData[pPlayer->entindex() - 1].wildReverse = true;
		g_BotData[pPlayer->entindex() - 1].stuckTimer = 0.0f;
		g_BotData[pPlayer->entindex() - 1].stuckTimerThink = 0.0f;
		g_BotData[pPlayer->entindex() - 1].spawnTimer = 0.0f;
		g_BotData[pPlayer->entindex() - 1].guardTimer = 0.0f;
		g_BotData[pPlayer->entindex() - 1].bLost = true;
		g_BotData[pPlayer->entindex() - 1].bIgnoreZ = false;
		g_BotData[pPlayer->entindex() - 1].bCombat = false;
		g_BotData[pPlayer->entindex() - 1].bForceCombat = false;
		g_BotData[pPlayer->entindex() - 1].left = false;
		g_BotData[pPlayer->entindex() - 1].turns = 0.0f;
		g_BotData[pPlayer->entindex() - 1].strikes = 0;
		g_BotData[pPlayer->entindex() - 1].m_objective = -1;
		g_BotData[pPlayer->entindex() - 1].m_objectiveType = BOT_OBJECTIVE_UNDEFINED;
		g_BotData[pPlayer->entindex() - 1].m_role = BOT_ROLE_UNDEFINED;
		g_BotData[pPlayer->entindex() - 1].m_flBaseSpeed = 600.0f;
		g_BotData[pPlayer->entindex() - 1].m_flOverrideDur = 0.0f;
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
		g_BotData[pPlayer->entindex() - 1].m_lastBuildingChecked = BUILDING_DEFAULT;
		g_BotData[pPlayer->entindex() - 1].RN = rn;
		g_BotData[pPlayer->entindex() - 1].flTimeout = 0.0f;
		g_BotData[pPlayer->entindex() - 1].m_vCombatLKP.Init();
		g_BotData[pPlayer->entindex() - 1].m_vObjectiveLKP = NULL;
		g_BotData[pPlayer->entindex() - 1].m_pOverridePos = NULL;
		g_BotData[pPlayer->entindex() - 1].m_flRespawnTime = 0.0f;
		g_BotData[pPlayer->entindex() - 1].m_bIsPurchasingItems = false;
		g_BotData[pPlayer->entindex() - 1].m_bHasNotPurchasedItems = true;
		g_BotData[pPlayer->entindex() - 1].m_lastCheckedItem = 0;
		g_BotData[pPlayer->entindex() - 1].m_lastCheckedRagdoll = 0;
		g_BotData[pPlayer->entindex() - 1].bNotIgnoringStrikes = true;
		g_BotData[pPlayer->entindex() - 1].m_flClassChange = gpGlobals->curtime + random->RandomInt(60, 120); //check class change every 60 - 120 seconds

		Set_Bot_Base_Velocity(pPlayer);

		return pPlayer;
	}
	return NULL;
}

const Vector *CurrentObjectiveLoc( CHL2MP_Player *pBot )
{
	const Vector *ret = NULL;
	botdata_t *botdata = &g_BotData[ ENTINDEX( pBot->edict() ) - 1 ];
	CHL2MPRules *pRules = HL2MPRules();
	if (botdata->m_objectiveType == BOT_OBJECTIVE_CAPPOINT)
	{
		if (botdata->m_objective > -1)
			ret = &pRules->cap_points.Get(botdata->m_objective)->GetAbsOrigin();
	}
	else if (botdata->m_objectiveType == BOT_OBJECTIVE_GET_GAS)
	{
		if (botdata->m_objective > -1)
			if (pRules->hGasCans[botdata->m_objective] != NULL)
				ret = &pRules->hGasCans[botdata->m_objective]->GetAbsOrigin();
	}
	else if (botdata->m_objectiveType == BOT_OBJECTIVE_RETURN_TO_APC)
	{
		return &pRules->pAPC->GetAbsOrigin();
	}
	else if (botdata->m_objectiveType == BOT_OBJECTIVE_FEED)
	{
		if (pRules->doll_collector[botdata->m_lastCheckedRagdoll] != NULL)
			return &pRules->doll_collector[botdata->m_lastCheckedRagdoll]->GetAbsOrigin();
	}
	else if (botdata->m_objectiveType == BOT_OBJECTIVE_BUILD)
	{
		return botdata->m_vObjectiveLKP;
	}
	else //BOT_OBJECTIVE_ROGUE
	{
		if (botdata->m_objective > 0)
			ret = &pRules->pBotNet[botdata->m_objective]->location;
	}
	return ret;
}

//BB: TODO: this is effectively currently copying a copy of a vector... improve!
Vector GetObjectiveLoc(int locationIndex)
{
	Vector ret(0, 0, 0);
	if (locationIndex)
	{
		ret = HL2MPRules()->cap_points.Get(locationIndex)->GetAbsOrigin();
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
	botdata->m_pOverridePos = NULL;
	botdata->m_vObjectiveLKP = NULL;
	botdata->m_role = BOT_ROLE_UNDEFINED;
	botdata->m_objectiveType = BOT_OBJECTIVE_UNDEFINED;
	botdata->m_objective = -1;
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
	botdata->bNotIgnoringStrikes = true;
	botdata->nextstrafetime = 0.0f;

	botdata->m_lastNodeProbe = 0;
}

void CheckItem(CHL2MP_Player *pBot)
{
	botdata_t *botdata = &g_BotData[ENTINDEX(pBot->edict()) - 1];
	CHL2MPRules *pRules = HL2MPRules();
	CUtlVector<CBaseEntity *> *hItems = NULL;

	if (!pRules || !pRules->cowsloaded)
		return;

	if (pBot->GetTeamNumber() == COVEN_TEAMID_SLAYERS)
		hItems = &pRules->hSlayerXP;
	else
		hItems = &pRules->hVampireXP;

	if (botdata->m_pOverridePos == NULL)
	{
		botdata->m_lastCheckedItem++;
		if (botdata->m_lastCheckedItem >= hItems->Count())
			botdata->m_lastCheckedItem = 0;

		if ((*hItems)[botdata->m_lastCheckedItem] != NULL && !(*hItems)[botdata->m_lastCheckedItem]->IsEffectActive(EF_NODRAW))
		{
			float distance = FLT_MAX;
			const Vector *itemOrigin = &(*hItems)[botdata->m_lastCheckedItem]->GetAbsOrigin();
			distance = ((*itemOrigin) - pBot->GetLocalOrigin()).LengthSqr();
			if (distance <= 90000.0f)
			{
				trace_t tr;
				UTIL_TraceLine(pBot->EyePosition(), (*itemOrigin), MASK_SHOT, pBot, COLLISION_GROUP_NONE, &tr);
				if (tr.DidHitNonWorldEntity() && FClassnameIs(tr.m_pEnt, "item_xp*"))
				{
					botdata->m_pOverridePos = itemOrigin;
				}
			}
		}
	}
	if ((*hItems)[botdata->m_lastCheckedItem] == NULL || (*hItems)[botdata->m_lastCheckedItem]->IsEffectActive(EF_NODRAW))
	{
		if (botdata->m_pOverridePos == &(*hItems)[botdata->m_lastCheckedItem]->GetAbsOrigin())
			botdata->m_pOverridePos = NULL;
	}
}

//BB: TODO: defenders!
bool CheckObjective( CHL2MP_Player *pBot, bool doValidityCheck, unsigned int *buttons = NULL ) //returns true if the node was valid. false if node was invalid.
{
	botdata_t *botdata = &g_BotData[ ENTINDEX( pBot->edict() ) - 1 ];
	CHL2MPRules *pRules = HL2MPRules();
	if (botdata->m_role == BOT_ROLE_UNDEFINED)
	{
		if (pBot->covenClassID == COVEN_CLASSID_FIEND || pBot->covenClassID == COVEN_CLASSID_HELLION)
		{
			botdata->m_role = BOT_ROLE_ATTACK;
		}
		else
		{
			botdata->m_role = BOT_ROLE_DEFENSE;
		}
	}

	if (botdata->m_objectiveType == BOT_OBJECTIVE_UNDEFINED)
	{
		if (pRules->covenActiveGameMode == COVEN_GAMEMODE_CAPPOINT)
		{
			int r = random->RandomInt(0, 10);
			if (r < 1)
				botdata->m_objectiveType = BOT_OBJECTIVE_ROGUE;
			else if ((r < 3 && pRules->covenCTSStatus > COVEN_CTS_STATUS_UNDEFINED) || pRules->num_cap_points == 0)
				botdata->m_objectiveType = BOT_OBJECTIVE_CTS;
			else
			{
				botdata->m_objectiveType = BOT_OBJECTIVE_CAPPOINT;

				if (pBot->IsBuilderClass() && pBot->SuitPower_GetCurrentPercentage() >= 60 && pBot->m_hTurret != NULL)
				{
					CCoven_Turret *pTurret = static_cast<CCoven_Turret *>(pBot->m_hTurret.Get());
					if (ValidTurret(pTurret) && random->RandomInt(0, 1) > 0)
					{
						botdata->m_objectiveType = BOT_OBJECTIVE_BUILD;
						botdata->m_vObjectiveLKP = &pTurret->EyePosition();
					}
				}
			}
		}
		else if (pRules->covenActiveGameMode == COVEN_GAMEMODE_COVEN)
		{
			if (pRules->pAPC != NULL)
			{
				if (pBot->GetTeamNumber() == COVEN_TEAMID_SLAYERS)
				{
					if (pBot->HasStatus(COVEN_STATUS_HAS_GAS))
					{
						botdata->m_objectiveType = BOT_OBJECTIVE_RETURN_TO_APC;
					}
					else
					{
						int r = random->RandomInt(0, 10);
						if (r < 1)
							botdata->m_objectiveType = BOT_OBJECTIVE_ROGUE;
						else
						{
							botdata->m_objectiveType = BOT_OBJECTIVE_GET_GAS;

							if (pBot->IsBuilderClass() && pBot->SuitPower_GetCurrentPercentage() >= 60 && pBot->m_hTurret != NULL)
							{
								CCoven_Turret *pTurret = static_cast<CCoven_Turret *>(pBot->m_hTurret.Get());
								if (ValidTurret(pTurret) && random->RandomInt(0, 1) > 0)
								{
									botdata->m_objectiveType = BOT_OBJECTIVE_BUILD;
									botdata->m_vObjectiveLKP = &pTurret->EyePosition();
								}
							}
						}
					}
				}
				else
				{
					botdata->m_objectiveType = BOT_OBJECTIVE_ROGUE;
				}
			}
		}
		else
		{
			botdata->m_objectiveType = BOT_OBJECTIVE_ROGUE;
		}
	}

	if (botdata->m_objective < 0)
	{
		if (botdata->m_objectiveType == BOT_OBJECTIVE_CAPPOINT)
		{
			if (botdata->m_role == BOT_ROLE_ATTACK)
			{
				botdata->m_objective = random->RandomInt(0, pRules->num_cap_points - 1);
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
			else if (botdata->m_role == BOT_ROLE_DEFENSE)
			{
				botdata->m_objective = random->RandomInt(0, pRules->num_cap_points - 1);//BB: TODO: random for now, later do closest or more intelligent
			}
		}
		else if (botdata->m_objectiveType == BOT_OBJECTIVE_GET_GAS)
		{
			botdata->m_objective = pRules->iValidGasCans[random->RandomInt(0, pRules->iValidGasCans.Count() - 1)];
		}
		else //BOT_OBJECTIVE_ROGUE
		{
			botdata->m_objective = random->RandomInt(1, pRules->bot_node_count - 1);
		}
	}

	if (botdata->m_objectiveType == BOT_OBJECTIVE_FEED)
	{
		if (pRules->doll_collector[botdata->m_lastCheckedRagdoll] == NULL || pBot->GetFedHP(botdata->m_lastCheckedRagdoll) >= sv_coven_hp_per_ragdoll.GetInt())
		{
			GetLost(pBot);
		}
	}
	else if (botdata->m_objectiveType == BOT_OBJECTIVE_BUILD)
	{
		if (pBot->m_hTurret == NULL || pBot->SuitPower_GetCurrentPercentage() < 5.0f)
		{
			GetLost(pBot);
		}
		else
		{
			CCoven_Turret *pTurret = static_cast<CCoven_Turret *>(pBot->m_hTurret.Get());
			if (ValidTurret(pTurret))
			{
				if (botdata->m_pOverridePos == NULL)
				{
					if (((*botdata->m_vObjectiveLKP) - pBot->EyePosition()).LengthSqr() <= 62500.0f)
					{
						trace_t tr;
						UTIL_TraceLine(pBot->EyePosition(), (*botdata->m_vObjectiveLKP), MASK_SHOT, pBot, COLLISION_GROUP_NONE, &tr);
						if (tr.DidHitNonWorldEntity() && tr.m_pEnt == pBot->m_hTurret)
						{
							botdata->m_pOverridePos = botdata->m_vObjectiveLKP;
						}
					}
				}
			}
			else
			{
				GetLost(pBot);
			}
		}
	}
	
	if (pRules->covenActiveGameMode == COVEN_GAMEMODE_CAPPOINT)
	{
		//check our objectives validity
		if (doValidityCheck)
		{
			if (botdata->m_role == BOT_ROLE_ATTACK)
			{
				//X BOTPATCH if there are no valid options, go ahead with a random and skip this check
				if ((pBot->GetTeamNumber() == COVEN_TEAMID_SLAYERS && pRules->s_caps < pRules->num_cap_points) || (pBot->GetTeamNumber() == COVEN_TEAMID_VAMPIRES && pRules->v_caps < pRules->num_cap_points))
				{
					if (pRules->cap_point_state.Get(botdata->m_objective) == pBot->GetTeamNumber())
					{
						//X BOTPATCH getlost
						GetLost(pBot);
						//X BOTPATCH clear guardtimer
						botdata->bGuarding = false;
						return false;
					}
				}
			}
			else if (botdata->m_role == BOT_ROLE_DEFENSE)
			{
				if ((pBot->GetTeamNumber() == COVEN_TEAMID_SLAYERS && pRules->s_caps < pRules->num_cap_points) || (pBot->GetTeamNumber() == COVEN_TEAMID_VAMPIRES && pRules->v_caps < pRules->num_cap_points))
				{
					if (pRules->cap_point_state.Get(botdata->m_objective) == pBot->GetTeamNumber())
					{
						GetLost(pBot);
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
			if (botdata->m_objectiveType == BOT_OBJECTIVE_CAPPOINT)
			{
				if (botdata->m_role == BOT_ROLE_DEFENSE)
				{
					if (botdata->guardTimer == 0.0f)
					{
						botdata->m_lastCheckedObjective++;
						if (botdata->m_lastCheckedObjective >= pRules->num_cap_points)
							botdata->m_lastCheckedObjective = 0;
						Vector objloc = GetObjectiveLoc(botdata->m_lastCheckedObjective);
						Vector goalloc = GetObjectiveLoc(botdata->m_objective);
						if (pRules->cap_point_state.Get(botdata->m_lastCheckedObjective) != pBot->GetTeamNumber() && (objloc - pBot->GetLocalOrigin()).LengthSqr() < (goalloc - pBot->GetLocalOrigin()).LengthSqr())
						{
							botdata->m_objectiveType = BOT_OBJECTIVE_CAPPOINT;
							botdata->m_objective = botdata->m_lastCheckedObjective;
						}
					}
				}
				else
				{
					if (botdata->guardTimer == 0.0f)
					{
						botdata->m_lastCheckedObjective++;
						if (botdata->m_lastCheckedObjective >= pRules->num_cap_points)
							botdata->m_lastCheckedObjective = 0;
						Vector objloc = GetObjectiveLoc(botdata->m_lastCheckedObjective);
						if (pRules->cap_point_state.Get(botdata->m_lastCheckedObjective) != pBot->GetTeamNumber() && (objloc - pBot->GetLocalOrigin()).LengthSqr() < pRules->cap_point_distance[botdata->m_lastCheckedObjective]) //BB: TODO: do I need LOS on this?
						{
							botdata->m_objectiveType = BOT_OBJECTIVE_CAPPOINT;
							botdata->m_objective = botdata->m_lastCheckedObjective;
						}
					}
				}
			}
		}
	}
	else if (pRules->covenActiveGameMode == COVEN_GAMEMODE_COVEN)
	{
		if (botdata->m_objectiveType == BOT_OBJECTIVE_GET_GAS)
		{
			if (pBot->HasStatus(COVEN_STATUS_HAS_GAS))
			{
				botdata->m_pOverridePos = NULL;
				botdata->m_vObjectiveLKP = NULL;
				botdata->m_objectiveType = BOT_OBJECTIVE_RETURN_TO_APC;
				if (botdata->m_targetNode > 0)
				{
					botdata->flTimeout = gpGlobals->curtime + (pRules->pBotNet[botdata->m_targetNode]->location - pBot->GetLocalOrigin()).Length() / botdata->m_flBaseSpeed * 2.0f + 1.0f;
					botdata->m_lastNode = -1;
				}
			}
			else
			{
				if (botdata->m_pOverridePos == NULL)
				{
					botdata->m_lastCheckedObjective++;
					if (botdata->m_lastCheckedObjective >= pRules->hGasCans.Count())
						botdata->m_lastCheckedObjective = 0;
					if (pRules->hGasCans[botdata->m_lastCheckedObjective] != NULL && !pRules->hGasCans[botdata->m_lastCheckedObjective]->IsEffectActive(EF_NODRAW))
					{
						const Vector *checkOrigin = &pRules->hGasCans[botdata->m_lastCheckedObjective]->GetAbsOrigin();
						if (((*checkOrigin) - pBot->GetLocalOrigin()).LengthSqr() <= 62500.0f)
						{
							trace_t tr;
							UTIL_TraceLine(pBot->EyePosition(), (*checkOrigin), MASK_SHOT, pBot, COLLISION_GROUP_NONE, &tr);
							if (tr.DidHitNonWorldEntity() && FClassnameIs(tr.m_pEnt, "item_gas"))
							{
								botdata->m_pOverridePos = checkOrigin;
							}
						}
					}
				}
				if (pRules->hGasCans[botdata->m_objective] == NULL || pRules->hGasCans[botdata->m_objective]->IsEffectActive(EF_NODRAW))
				{
					botdata->m_objective = pRules->iValidGasCans[random->RandomInt(0, pRules->iValidGasCans.Count())];
				}
			}
		}
		else if (botdata->m_objectiveType == BOT_OBJECTIVE_RETURN_TO_APC)
		{
			botdata->m_pOverridePos = NULL;
			trace_t tr;
			botdata->m_vObjectiveLKP = &pRules->pAPC->GetAbsOrigin();
			UTIL_TraceLine(pBot->EyePosition(), (*botdata->m_vObjectiveLKP), MASK_SHOT, pBot, COLLISION_GROUP_NONE, &tr);
			if (tr.DidHitNonWorldEntity() && FClassnameIs(tr.m_pEnt, "coven_prop_physics") && tr.fraction < 0.8f)
			{
				botdata->m_pOverridePos = botdata->m_vObjectiveLKP;
#ifdef DEBUG_BOTS
				if (bot_debug.GetInt() == pBot->entindex())
				{
					Msg("Encountered: APC! (%f)\n", tr.fraction);
				}
#endif
				if (tr.fraction < 0.3f)
				{
					if (buttons && gpGlobals->curtime > botdata->nextusetime && !pBot->IsPerformingDeferredAction())
					{
						(*buttons) |= IN_USE;
						botdata->nextusetime = gpGlobals->curtime + 0.4f;
						GetLost(pBot, false, true);
					}
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
			CHL2MP_Player *pEnemy = ToHL2MPPlayer(UTIL_PlayerByIndex(botdata->m_lastPlayerCheck));
			if (pEnemy)
			{
				bool isVampDoll = pEnemy->KO && pEnemy->myServerRagdoll != NULL;
				CBaseCombatWeapon *pStake = pBot->Weapon_OwnsThisType("weapon_stake");
				CBaseCombatWeapon *pHolyWater = pBot->Weapon_OwnsThisType("weapon_holywater");
				CBaseCombatWeapon *pFrag = pBot->Weapon_OwnsThisType("weapon_frag");
				CBaseCombatWeapon *pStunFrag = pBot->Weapon_OwnsThisType("weapon_stunfrag");
				CBaseCombatWeapon *pActiveWeapon = pBot->GetActiveWeapon();
				if (pStake)
				{
					if (isVampDoll)
					{
						// Switch to it if we don't have it out
						// Switch?
						if (pActiveWeapon != pStake)
						{
							if (pActiveWeapon->NeedsReload1())
							{
								return IN_RELOAD;
							}
							else
								pBot->Weapon_Switch(pStake);
						}
					}
					else
					{
						if (pActiveWeapon == pStake)
						{
							pBot->SwitchToNextBestWeapon(pStake);
						}
						else if (pActiveWeapon == pHolyWater)
						{
							if (botdata->m_flLastCombatDist < 62500.0f)
							{
								pBot->SwitchToNextBestWeapon(pActiveWeapon);
							}
						}
						else if (pActiveWeapon == pFrag || pActiveWeapon == pStunFrag)
						{
							if (botdata->m_flLastCombatDist < 160000.0f)
							{
								pBot->SwitchToNextBestWeapon(pActiveWeapon);
							}
						}
						else
						{
							if (pHolyWater && pHolyWater->HasAmmo() && botdata->m_flLastCombatDist >= 62500.0f && botdata->m_flLastCombatDist < 202500.0f)
							{
								pBot->Weapon_Switch(pHolyWater);
							}
							else if (pFrag && pFrag->HasAmmo() && botdata->m_flLastCombatDist >= 160000.0f)
							{
								pBot->Weapon_Switch(pFrag);
							}
							else if (pStunFrag && pStunFrag->HasAmmo() && botdata->m_flLastCombatDist >= 160000.0f)
							{
								pBot->Weapon_Switch(pStunFrag);
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
		}
		else
		{
			if (!pBot->IsPerformingDeferredAction())
			{
				CBaseCombatWeapon *pStake = pBot->Weapon_OwnsThisType("weapon_stake");
				CBaseCombatWeapon *pStunStick = pBot->Weapon_OwnsThisType("weapon_stunstick");
				CBaseCombatWeapon *pHarpoon = pBot->Weapon_OwnsThisType("weapon_harpoon");
				CBaseCombatWeapon *pActiveWeapon = pBot->GetActiveWeapon();
				if (pStunStick && pActiveWeapon == pStunStick && botdata->m_objectiveType != BOT_OBJECTIVE_BUILD)
				{
					CBaseCombatWeapon *pNewWeapon = g_pGameRules->GetNextBestWeapon(pBot, pStunStick);
					if (pNewWeapon && pNewWeapon != pStake)
						pBot->SwitchToNextBestWeapon(pStunStick);
				}
				else
				{
					if (pActiveWeapon == pStake)
					{
						pBot->SwitchToNextBestWeapon(pStake);
					}
					else if (pHarpoon && pBot->coven_hook_state > COVEN_HOOK_FIRED)
					{
						return IN_ATTACK2;
					}
					else if (pActiveWeapon != pStake && pActiveWeapon->HasAmmo())
					{
						if (pActiveWeapon->NeedsReload1())
						{
							return IN_RELOAD;
						}
					}
					else if (pActiveWeapon != pStunStick)
						pBot->Weapon_Switch(pStake);
				}
			}
		}
		
	}
	return 0;
}

void BotRespawn(CHL2MP_Player *pBot)
{
	botdata_t *botdata = &g_BotData[ENTINDEX(pBot->edict()) - 1];

	botdata->m_flRespawnTime = gpGlobals->curtime;
	if (pBot->GetTeamNumber() == COVEN_TEAMID_SLAYERS)
	{
		botdata->m_bHasNotPurchasedItems = true;
		botdata->m_bIsPurchasingItems = random->RandomInt(0, 9) < 9; //90% chance to purchase items
	}
}

void BotDeath(CHL2MP_Player *pBot)
{
	botdata_t *botdata = &g_BotData[ENTINDEX(pBot->edict()) - 1];

	if (gpGlobals->curtime > botdata->m_flClassChange)
	{
		botdata->m_flClassChange = gpGlobals->curtime + random->RandomInt(60, 120); //check class change every 60 - 120 seconds
		if (random->RandomInt(0, 9) < 4) //50% chance to change class
			pBot->HandleCommand_SelectClass(0);
	}
}

void HealthCheck(CHL2MP_Player *pBot)
{
	botdata_t *botdata = &g_BotData[ENTINDEX(pBot->edict()) - 1];

	if (pBot->GetTeamNumber() == COVEN_TEAMID_VAMPIRES)
	{
		if (!botdata->bCombat)
		{
			CHL2MPRules *pRules = HL2MPRules();
			if (pRules)
			{
				if (botdata->m_objectiveType != BOT_OBJECTIVE_FEED && botdata->m_pOverridePos == NULL)
				{
					botdata->m_lastCheckedRagdoll++;
					if (botdata->m_lastCheckedRagdoll >= COVEN_MAX_RAGDOLLS)
						botdata->m_lastCheckedRagdoll = 0;

					if (pRules->doll_collector[botdata->m_lastCheckedRagdoll] != NULL && pBot->GetFedHP(botdata->m_lastCheckedRagdoll) < sv_coven_hp_per_ragdoll.GetInt())
					{
						const Vector *bodyOrigin = &pRules->doll_collector[botdata->m_lastCheckedRagdoll]->GetAbsOrigin();
						if (((*bodyOrigin) - pBot->GetLocalOrigin()).LengthSqr() <= 250000.0f)
						{
							trace_t tr;
							UTIL_TraceLine(pBot->EyePosition(), (*bodyOrigin), MASK_SHOT, pBot, COLLISION_GROUP_WEAPON, &tr);
							if (tr.DidHitNonWorldEntity() && tr.m_pEnt == pRules->doll_collector[botdata->m_lastCheckedRagdoll])
							{
								botdata->m_pOverridePos = bodyOrigin;
								botdata->m_objectiveType = BOT_OBJECTIVE_FEED;
							}
						}
					}
				}
			}
		}
	}
	else //Slayers
	{
		if (!botdata->bCombat)
		{
			if (pBot->HasMedkit() && !pBot->IsPerformingDeferredAction() && !pBot->IsReloading())
			{
				CovenItemInfo_t *info = GetCovenItemData(COVEN_ITEM_MEDKIT);
				int iMissingHP = pBot->GetMaxHealth() - pBot->GetHealth();
				if (iMissingHP >= info->iMagnitude)
					pBot->UseCovenItem(COVEN_ITEM_MEDKIT);
			}
			if (pBot->HasPills() && !pBot->IsPerformingDeferredAction() && !pBot->IsReloading())
			{
				CovenItemInfo_t *info = GetCovenItemData(COVEN_ITEM_PILLS);
				if (!pBot->HasStatus(COVEN_STATUS_HASTE) || pBot->GetStatusMagnitude(COVEN_STATUS_HASTE) < info->flMaximum || pBot->GetStatusTime(COVEN_STATUS_HASTE) < info->flUseTime)
					pBot->UseCovenItem(COVEN_ITEM_PILLS);
			}
		}
		else if (!pBot->IsPerformingDeferredAction())
		{
			CovenItemInfo_t *info = GetCovenItemData(COVEN_ITEM_STIMPACK);
			int iMissingHP = pBot->GetMaxHealth() - pBot->GetHealth();
			if (pBot->HasStimpack() && iMissingHP >= info->iMagnitude)
				pBot->UseCovenItem(COVEN_ITEM_STIMPACK);
		}
	}
}

void PurchaseCheck(CHL2MP_Player *pBot)
{
	if (pBot->GetTeamNumber() == COVEN_TEAMID_SLAYERS)
	{
		botdata_t *botdata = &g_BotData[ENTINDEX(pBot->edict()) - 1];
		if (botdata->m_bIsPurchasingItems && botdata->m_bHasNotPurchasedItems)
		{
			CovenItemID_t item = (CovenItemID_t)random->RandomInt(COVEN_ITEM_STIMPACK, COVEN_ITEM_SLAM);
			if (pBot->PurchaseCovenItem(item))
				botdata->m_bHasNotPurchasedItems = random->RandomInt(0, 9) < 9; //90% chance to keep purchasing
			else
				botdata->m_bHasNotPurchasedItems = false;
		}
	}
}

void PlayerCheck( CHL2MP_Player *pBot )
{
	botdata_t *botdata = &g_BotData[ ENTINDEX( pBot->edict() ) - 1 ];
	
	CBaseCombatCharacter *pPlayer = BotGetEnemy(pBot);
	int num_players = HL2MPRules()->PlayerCount();
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
		float dist = playerVec.LengthSqr();

		if (botdata->bForceCombat || dist < 640000.0f) //600
		{
			float check = 1.0f;
			//clear force combat if perfectly stealthed and/or fails check?
			//is this right?
			if (!botdata->bForceCombat)
			{
				float cloak = pPlayer->m_floatCloakFactor.Get();
				if (!botdata->bCombat && cloak < 1.0f)
				{
					float alpha = 1.0f - cloak;
					if (alpha <= COVEN_MIN_CLOAK_CROUCH * 1.2f) //some wiggle room for float error
						check = 0.02f * (bot_difficulty.GetInt() + 1);
					else if (alpha <= COVEN_MIN_CLOAK_WALK * 1.2f)
						check = 0.08f + 0.04f * bot_difficulty.GetInt();
					check *= 0.5f * num_players; //attempt to approximate the same probability per think
				}
				else if (cloak >= 1.0f)
					check = 0.0f;
			}
			if (isVampDoll || ((random->RandomFloat() <= check && !(pPlayer->GetFlags() & EF_NODRAW))))
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
						UTIL_TraceLine(vecSrc, vecEnd, MASK_SHOT, pBot, COLLISION_GROUP_WEAPON, &trace);
					//BB: BLEH.
					else if (pPlayer->IsABuilding())
					{
						CCovenBuilding *bldg = ToCovenBuilding(pPlayer);
						if (bldg->MyType() == BUILDING_TURRET && ((CCoven_Turret *)bldg)->bTipped)
							UTIL_TraceLine(vecSrc, vecEnd, MASK_SHOT, pBot, COLLISION_GROUP_WEAPON, &trace);
						else
							UTIL_TraceLine(vecSrc, vecEnd, MASK_SHOT, pBot, COLLISION_GROUP_PLAYER, &trace);
					}
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
						botdata->m_vCombatLKP = vecEnd;
						botdata->pCombatTarget = pPlayer;
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
		GetLost(pBot, true, true);
	}

	botdata->bCombat = false;
	botdata->bForceCombat = false;
	botdata->m_flLastCombatDist = MAX_TRACE_LENGTH;
	botdata->pCombatTarget = NULL;
	if (botdata->m_lastBuildingChecked == BUILDING_DEFAULT)
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

	BotNode_t *temp, *cur;
	temp = pRules->pBotNet[botdata->m_lastNodeProbe];
	cur = pRules->pBotNet[botdata->m_targetNode];

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
				if ((botdata->bVisCheck && !botdata->bPassedNodeLOS) || ((pBot->GetAbsOrigin() - temp->location).LengthSqr() < (pBot->GetAbsOrigin() - cur->location).LengthSqr()))
				{
					if (botdata->bVisCheck)
					{
						trace_t trace;
						UTIL_TraceLine(pBot->EyePosition(), temp->location + Vector(0, 0, 16), MASK_SOLID, pBot, COLLISION_GROUP_PLAYER, &trace);
						/*BOTS_DEBUG_VISUAL*****************************************************************************/
						if (coven_debug_visual.GetBool())
						{
							if (bot_debug_select.GetInt() == pBot->entindex())
							{
								NDebugOverlay::Line(pBot->EyePosition(), temp->location + Vector(0, 0, 16), 255, 255, 0, false, 1.5f);
							}
						}
						/***********************************************************************************************/
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
		botdata->flTimeout = gpGlobals->curtime + (pRules->pBotNet[botdata->m_targetNode]->location - pBot->GetLocalOrigin()).Length() / botdata->m_flBaseSpeed * 2.0f + 1.0f;
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

unsigned int Bot_Ability_Think(CHL2MP_Player *pBot, unsigned int &buttons)
{
	botdata_t *botdata = &g_BotData[ENTINDEX(pBot->edict()) - 1];
	unsigned int key = 0;

	if (!pBot->IsAlive() || pBot->KO)
		return 0;

	if (pBot->IsPerformingDeferredAbility())
	{
		buttons &= ~(IN_ATTACK | IN_ATTACK2 | IN_RELOAD);
		return 0;
	}

	if (pBot->HasAbility(COVEN_ABILITY_BATTLEYELL))
	{
		if (botdata->bCombat)
		{
			if (!pBot->HasStatus(COVEN_STATUS_BATTLEYELL))
			{
				int abilityNum = pBot->AbilityKey(COVEN_ABILITY_BATTLEYELL, &key);
				if (!pBot->IsInCooldown(abilityNum))
				{
					buttons |= key;
				}
			}
		}
	}
	if (pBot->HasAbility(COVEN_ABILITY_HASTE))
	{
		if (botdata->bCombat)
		{
			if (!pBot->HasStatus(COVEN_STATUS_HASTE) && pBot->SuitPower_GetCurrentPercentage() > 5.0f)
			{
				int abilityNum = pBot->AbilityKey(COVEN_ABILITY_HASTE, &key);
				if (!pBot->IsInCooldown(abilityNum))
				{
					buttons |= key;
					botdata->m_flAbilityTimer[abilityNum] = 0.0f;
				}
			}
		}
		else
		{
			if (pBot->HasStatus(COVEN_STATUS_HASTE))
			{
				int abilityNum = pBot->AbilityKey(COVEN_ABILITY_HASTE, &key);
				if (botdata->m_flAbilityTimer[abilityNum] == 0.0f)
				{
					botdata->m_flAbilityTimer[abilityNum] = gpGlobals->curtime + 5.0f;
				}
				else if (gpGlobals->curtime > botdata->m_flAbilityTimer[abilityNum])
				{
					pBot->AbilityKey(COVEN_ABILITY_HASTE, &key);
					buttons |= key;
					botdata->m_flAbilityTimer[abilityNum] = 0.0f;
				}
			}
		}
	}
	if (pBot->HasAbility(COVEN_ABILITY_SHEERWILL))
	{
		if (pBot->GetStatusTime(COVEN_STATUS_STATBOOST) - gpGlobals->curtime < 5.0f)
		{
			int abilityNum = pBot->AbilityKey(COVEN_ABILITY_SHEERWILL, &key);
			if (!pBot->IsInCooldown(abilityNum))
			{
				buttons |= key;
			}
		}
	}
	if (pBot->HasAbility(COVEN_ABILITY_DASH))
	{
		if (botdata->bCombat)
		{
			int abilityNum = pBot->AbilityKey(COVEN_ABILITY_DASH, &key);
			CovenAbilityInfo_t *info = GetCovenAbilityData(COVEN_ABILITY_DASH);
			if (!pBot->IsInCooldown(abilityNum) && botdata->m_flLastCombatDist < info->flRange && botdata->m_lastPlayerDot > DOT_25DEGREE)
			{
				CBaseCombatCharacter *pEnemy = BotGetEnemy(pBot);
				if (pEnemy && pEnemy->IsPlayer() && pEnemy->IsAlive() && !pEnemy->KO)
					buttons |= key;
			}
		}
	}
	if (pBot->HasAbility(COVEN_ABILITY_LIGHTWAVE))
	{
		if (botdata->bCombat)
		{
			int abilityNum = pBot->AbilityKey(COVEN_ABILITY_LIGHTWAVE, &key);
			CovenAbilityInfo_t *info = GetCovenAbilityData(COVEN_ABILITY_LIGHTWAVE);
			if (!pBot->IsInCooldown(abilityNum) && botdata->m_flLastCombatDist < info->flRange * 0.9f)
			{
				CBaseCombatCharacter *pEnemy = BotGetEnemy(pBot);
				if (pEnemy && pEnemy->IsPlayer() && pEnemy->IsAlive() && !pEnemy->KO)
					buttons |= key;
			}
		}
		else
		{
			int abilityNum = pBot->AbilityKey(COVEN_ABILITY_LIGHTWAVE, &key);
			if (!pBot->IsInCooldown(abilityNum) && pBot->GetStatusMagnitude(COVEN_STATUS_INNERLIGHT) < 2 && gpGlobals->curtime > botdata->m_flAbilityTimer[abilityNum])
			{
				if (random->RandomInt(0, 2) <= 1)
					buttons |= key;
				botdata->m_flAbilityTimer[abilityNum] = gpGlobals->curtime + 3.0f;
			}
		}
	}
	if (pBot->HasAbility(COVEN_ABILITY_DARKWILL))
	{
		if (pBot->GetStatusTime(COVEN_STATUS_STATBOOST) - gpGlobals->curtime < 5.0f)
		{
			int abilityNum = pBot->AbilityKey(COVEN_ABILITY_DARKWILL, &key);
			if (!pBot->IsInCooldown(abilityNum))
			{
				buttons |= key;
			}
		}
	}
	if (pBot->HasAbility(COVEN_ABILITY_INTIMIDATINGSHOUT))
	{
		if (botdata->bCombat)
		{
			int abilityNum = pBot->AbilityKey(COVEN_ABILITY_INTIMIDATINGSHOUT, &key);
			CovenAbilityInfo_t *info = GetCovenAbilityData(COVEN_ABILITY_INTIMIDATINGSHOUT);
			if (!pBot->IsInCooldown(abilityNum) && botdata->m_flLastCombatDist < info->flRange)
			{
				CBaseCombatCharacter *pEnemy = BotGetEnemy(pBot);
				if (pEnemy && pEnemy->IsPlayer() && pEnemy->IsAlive() && !pEnemy->KO)
					buttons |= key;
			}
		}
	}
	//BB: TODO: random sneak walk
	if (pBot->HasAbility(COVEN_ABILITY_DODGE))
	{
		if (botdata->bCombat)
		{
			if (!pBot->HasStatus(COVEN_STATUS_DODGE))
			{
				int abilityNum = pBot->AbilityKey(COVEN_ABILITY_DODGE, &key);
				if (!pBot->IsInCooldown(abilityNum) && botdata->m_flLastCombatDist >= bot_dodge_combat.GetFloat())
				{
					CBaseCombatCharacter *pEnemy = BotGetEnemy(pBot);
					if (pEnemy && pEnemy->IsAlive() && !pEnemy->KO)
					{
						buttons |= key;
						buttons &= ~IN_ATTACK;
						botdata->m_flAbilityTimer[abilityNum] = 0.25f + gpGlobals->curtime;
					}
				}
			}
		}
		else
		{
			if (pBot->HasStatus(COVEN_STATUS_DODGE))
			{
				pBot->AbilityKey(COVEN_ABILITY_DODGE, &key);
				buttons |= key;
			}
		}
	}
	if (pBot->HasAbility(COVEN_ABILITY_PHASE))
	{
		if (botdata->bCombat)
		{
			if (!pBot->HasStatus(COVEN_STATUS_PHASE))
			{
				int abilityNum = pBot->AbilityKey(COVEN_ABILITY_PHASE, &key);
				if (!pBot->IsInCooldown(abilityNum) && botdata->m_flLastCombatDist >= bot_phase_combat.GetFloat())
				{
					CBaseCombatCharacter *pEnemy = BotGetEnemy(pBot);
					if (pEnemy && pEnemy->IsAlive() && !pEnemy->KO)
					{
						buttons |= key;
						buttons &= ~IN_ATTACK;
						botdata->m_flAbilityTimer[abilityNum] = 0.25f + gpGlobals->curtime;
					}
				}
			}
		}
		else
		{
			if (pBot->HasStatus(COVEN_STATUS_PHASE))
			{
				pBot->AbilityKey(COVEN_ABILITY_PHASE, &key);
				buttons |= key;
			}
		}
	}
	if (pBot->HasAbility(COVEN_ABILITY_DETONATEBLOOD))
	{
		if (botdata->bCombat)
		{
			int abilityNum = pBot->AbilityKey(COVEN_ABILITY_DETONATEBLOOD, &key);
			CovenAbilityInfo_t *info = GetCovenAbilityData(COVEN_ABILITY_DETONATEBLOOD);
			if (!pBot->IsInCooldown(abilityNum) && botdata->m_flLastCombatDist < info->flRange)
			{
				CBaseCombatCharacter *pEnemy = BotGetEnemy(pBot);
				if (pEnemy && pEnemy->IsAlive() && !pEnemy->KO)
				{
					buttons |= key;
				}
			}
		}
		/*else
		{
			if (!botdata->bLost && botdata->m_pOverridePos == NULL)
			{
				int abilityNum = pBot->AbilityKey(COVEN_ABILITY_DETONATEBLOOD, &key);
				if (!pBot->IsInCooldown(abilityNum) && (float)pBot->GetHealth() / pBot->GetMaxHealth() > 0.5f && (!pBot->HasStatus(COVEN_STATUS_MASOCHIST) || pBot->GetStatusTime(COVEN_STATUS_MASOCHIST) - gpGlobals->curtime < 1.0f))
				{
					buttons |= key;
				}
			}
		}*/
	}
	if (pBot->HasAbility(COVEN_ABILITY_LEAP))
	{
		if (botdata->bCombat)
		{
			int abilityNum = pBot->AbilityKey(COVEN_ABILITY_LEAP, &key);
			if (!pBot->IsInCooldown(abilityNum))
			{
				//CBaseCombatCharacter *pPlayer = BotGetEnemy(pBot);
				//if (pPlayer)
					//float distance = (pPlayer->GetLocalOrigin() - pBot->GetLocalOrigin()).Length();
				if (botdata->m_flLastCombatDist >= bot_leap_combat.GetFloat())
				{
					if (botdata->m_flAbilityTimer[abilityNum] == 0.0f)
					{
						buttons |= IN_JUMP;
						botdata->m_flAbilityTimer[abilityNum] = gpGlobals->curtime + 0.2f;
					}
					else
					{
						botdata->m_flAbilityTimer[abilityNum] = 0.0f;
						buttons |= key;
					}
				}
			}
		}
		else
		{
			if (!botdata->bLost && botdata->m_pOverridePos == NULL)
			{
				CHL2MPRules *pRules;
				pRules = HL2MPRules();
				if (botdata->m_targetNode > -1 && pRules->pBotNet[botdata->m_targetNode] != NULL)
				{
					int abilityNum = pBot->AbilityKey(COVEN_ABILITY_LEAP, &key);
					if (!pBot->IsInCooldown(abilityNum))
					{
						if (botdata->goWild == 0.0f && botdata->stuckTimer == 0.0f && botdata->guardTimer == 0.0f)
						{
							float distance = (pRules->pBotNet[botdata->m_targetNode]->location - pBot->GetLocalOrigin()).LengthSqr();
							if (distance >= bot_leap_noncombat.GetFloat())
							{
								//BB: roof check
								trace_t trace;
								UTIL_TraceLine(pBot->GetLocalOrigin(), pBot->EyePosition() + Vector(0, 0, 128), MASK_PLAYERSOLID, pBot, COLLISION_GROUP_PLAYER, &trace);
								if (trace.fraction == 1.0f)
								{
									if (botdata->m_flAbilityTimer[abilityNum] == 0.0f)
									{
										buttons |= IN_JUMP;
										botdata->m_flAbilityTimer[abilityNum] = gpGlobals->curtime + 0.2f;
									}
									else
									{
										botdata->forwardAngle.x = -30;
										botdata->overrideAngle = botdata->forwardAngle;
										botdata->m_flOverrideDur = gpGlobals->curtime + 0.2f;
										botdata->m_flAbilityTimer[abilityNum] = 0.0f;
										buttons |= key;
									}
								}
							}
						}
					}
				}
			}
		}
	}
	if (pBot->HasAbility(COVEN_ABILITY_BERSERK))
	{
		if (botdata->bCombat)
		{
			if (!pBot->HasStatus(COVEN_STATUS_BERSERK))
			{
				int abilityNum = pBot->AbilityKey(COVEN_ABILITY_BERSERK, &key);
				if (!pBot->IsInCooldown(abilityNum))
				{
					buttons |= key;
				}
			}
		}
	}
	if (pBot->HasAbility(COVEN_ABILITY_CHARGE))
	{
		if (botdata->bCombat)
		{
			int abilityNum = pBot->AbilityKey(COVEN_ABILITY_CHARGE, &key);
			CBaseCombatCharacter *pPlayer = BotGetEnemy(pBot);
			if (pPlayer)
			{
				float distance = (pPlayer->GetAbsOrigin() - pBot->GetAbsOrigin()).Length2DSqr(); //ignore z for distance check (flung players should not trigger a charge)
				if (pBot->gorelock > GORELOCK_NONE)
				{
					if (distance > 576.0f && distance < botdata->m_flAbilityTimer[abilityNum])
					{
						botdata->m_flAbilityTimer[abilityNum] = distance;
						buttons |= key;
					}
					else
						botdata->m_flAbilityTimer[abilityNum] = 0.0f;
				}
				else if (pBot->SuitPower_GetCurrentPercentage() >= 6.0f && !pBot->IsInCooldown(abilityNum))
				{
					if (distance >= bot_charge_combat.GetFloat() && botdata->m_lastPlayerDot > BOT_ATTACK_DOT_STRICT)
					{
						botdata->m_flAbilityTimer[abilityNum] = distance;
						//BB: I don't think I need to do this... angles should be "correct" from other functions
						//Vector forward = pPlayer->GetLocalOrigin() - pBot->GetLocalOrigin();
						//VectorAngles(forward, botdata->forwardAngle);
						buttons |= key;
					}
				}
				else
					botdata->m_flAbilityTimer[abilityNum] = 0.0f;
			}
			else
				botdata->m_flAbilityTimer[abilityNum] = 0.0f;
		}
		else
		{
			CHL2MPRules *pRules = HL2MPRules();
			int abilityNum = pBot->AbilityKey(COVEN_ABILITY_CHARGE, &key);
			if ((!botdata->bLost && botdata->m_pOverridePos == NULL) && (botdata->m_targetNode > -1 && pRules->pBotNet[botdata->m_targetNode] != NULL))
			{
				float distance = (pRules->pBotNet[botdata->m_targetNode]->location - pBot->GetLocalOrigin()).LengthSqr();
				if (pBot->gorelock > GORELOCK_NONE)
				{
					if (distance > 900.0f && distance < botdata->m_flAbilityTimer[abilityNum])
					{
						botdata->m_flAbilityTimer[abilityNum] = distance;
						buttons |= key;
					}
					else
						botdata->m_flAbilityTimer[abilityNum] = 0.0f;
				}
				else if (!pBot->IsInCooldown(abilityNum) && botdata->goWild == 0.0f && botdata->stuckTimer == 0.0f && botdata->guardTimer == 0.0f && pBot->SuitPower_GetCurrentPercentage() > 6.0f)
				{
					float deltaZ = abs(pRules->pBotNet[botdata->m_targetNode]->location.z - pBot->GetLocalOrigin().z);
					if (distance >= bot_charge_noncombat.GetFloat() && deltaZ < 32.0f)
					{
						botdata->m_flAbilityTimer[abilityNum] = distance;
						//BB: I don't think I need to do this... angles should be "correct" from other functions
						//Vector forward = pRules->pBotNet[botdata->m_targetNode]->location - pBot->GetLocalOrigin();
						//VectorAngles(forward, botdata->forwardAngle);
						buttons |= key;
					}
				}
			}
			else
				botdata->m_flAbilityTimer[abilityNum] = 0.0f;
		}
	}
	if (pBot->HasAbility(COVEN_ABILITY_DREADSCREAM))
	{
		if (botdata->bCombat)
		{
			int abilityNum = pBot->AbilityKey(COVEN_ABILITY_DREADSCREAM, &key);
			CovenAbilityInfo_t *info = GetCovenAbilityData(COVEN_ABILITY_DREADSCREAM);
			if (!pBot->IsInCooldown(abilityNum) && botdata->m_flLastCombatDist < info->flRange)
			{
				buttons |= key;
			}
		}
	}
	if (pBot->HasAbility(COVEN_ABILITY_BLOODLUST))
	{
		if (botdata->bCombat)
		{
			if (!pBot->HasStatus(COVEN_STATUS_BLOODLUST))
			{
				int abilityNum = pBot->AbilityKey(COVEN_ABILITY_BLOODLUST, &key);
				if (!pBot->IsInCooldown(abilityNum))
				{
					buttons |= key;
				}
			}
		}
	}
	if (pBot->HasAbility(COVEN_ABILITY_BUILDTURRET))
	{
		if (botdata->bCombat)
		{
			int abilityNum = pBot->AbilityKey(COVEN_ABILITY_BUILDTURRET, &key);
			CovenAbilityInfo_t *info = GetCovenAbilityData(COVEN_ABILITY_BUILDTURRET);
			if (!pBot->IsInCooldown(abilityNum) && pBot->SuitPower_GetCurrentPercentage() >= info->flCost)
			{
				CCovenBuilding *bldg = ToCovenBuilding(pBot->m_hTurret);
				if (bldg == NULL || ((CCoven_Turret *)bldg)->bTipped || ((CCoven_Turret *)bldg)->GetAmmo(3) == 0 || gpGlobals->curtime - botdata->m_flAbilityTimer[abilityNum] > 120.0f) //old ass turret. clean it up.
				{
					buttons |= key;
					botdata->m_flAbilityTimer[abilityNum] = gpGlobals->curtime;
				}
			}
		}
	}
	if (pBot->HasAbility(COVEN_ABILITY_BUILDDISPENSER))
	{
	}
	if (pBot->HasAbility(COVEN_ABILITY_INNERLIGHT))
	{
		if (botdata->bCombat)
		{
			int abilityNum = pBot->AbilityKey(COVEN_ABILITY_INNERLIGHT, &key);
			CovenAbilityInfo_t *info = GetCovenAbilityData(COVEN_ABILITY_INNERLIGHT);
			if (!pBot->IsInCooldown(abilityNum) && botdata->m_flLastCombatDist < info->flRange * 0.9f)
			{
				CBaseCombatCharacter *pEnemy = BotGetEnemy(pBot);
				if (pEnemy && pEnemy->IsPlayer() && pEnemy->IsAlive() && !pEnemy->KO)
				{
					buttons &= CLEAR_ABILITY_KEYS;
					buttons &= ~IN_ATTACK;
					buttons |= key;
				}
			}
		}
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

void Bot_Reached_Node(CHL2MP_Player *pBot, const Vector *objLoc)
{
	CHL2MPRules *pRules;
	pRules = HL2MPRules();

	botdata_t *botdata = &g_BotData[ENTINDEX(pBot->edict()) - 1];
#ifdef DEBUG_BOTS
	if (bot_debug.GetInt() == pBot->entindex())
	{
		Msg("Bot objective type: %d\n", botdata->m_objectiveType);
		Msg("Reached Node: %d\n", pRules->pBotNet[botdata->m_targetNode]->ID);
	}
#endif
	if (pRules->pBotNet[botdata->m_targetNode]->connectors.Count() <= 1)
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
			redflag = pRules->pBotNet[botdata->m_lastNode]->ID;
		}
		int n = pRules->pBotNet[botdata->m_targetNode]->connectors.Count();
		int sel = -1;

		if (n == 2 && redflag > -1) //early cull... always go forward
		{
			sel = pRules->pBotNet[botdata->m_targetNode]->connectors[0];
			if (sel == redflag)
				sel = pRules->pBotNet[botdata->m_targetNode]->connectors[1];
		}
		else
		{
			float shortestdist = -1.0f;
			for (int i = 0; i < n; i++)
			{
				if (pRules->pBotNet[botdata->m_targetNode]->connectors[i] == redflag || pRules->pBotNet[pRules->pBotNet[botdata->m_targetNode]->connectors[i]]->connectors.Size() == 1)
					continue;
				float dist = (pRules->pBotNet[pRules->pBotNet[botdata->m_targetNode]->connectors[i]]->location - *objLoc).LengthSqr();
#ifdef DEBUG_BOTS
				if (bot_debug.GetInt() == pBot->entindex())
				{
					Msg("Objective: %f %f %f\n", (*objLoc).x, (*objLoc).y, (*objLoc).z);
				}
#endif
				if (shortestdist < 0.0f || dist < shortestdist)
				{
					shortestdist = dist;
					sel = pRules->pBotNet[botdata->m_targetNode]->connectors[i];
				}
			}
		}
		botdata->m_targetNode = sel;
#ifdef DEBUG_BOTS
		if (bot_debug.GetInt() == pBot->entindex())
		{
			Msg("Headed to Node: %d\n", pRules->pBotNet[botdata->m_targetNode]->ID);
		}
#endif
		botdata->m_lastNode = pushedval;
		if (botdata->m_targetNode > 0 && botdata->m_lastNode > 0)
		{
			botdata->flTimeout = gpGlobals->curtime + (pRules->pBotNet[botdata->m_targetNode]->location - pRules->pBotNet[botdata->m_lastNode]->location).Length() / botdata->m_flBaseSpeed * 2.0f + 1.0f;
#ifdef DEBUG_BOTS
			if (bot_debug.GetInt() == pBot->entindex())
			{
				Msg("Timeout set to: %f\n", botdata->flTimeout - gpGlobals->curtime);
			}
#endif
		}
		else
			botdata->flTimeout = gpGlobals->curtime + 5.0f;
		/*BOTS_DEBUG_VISUAL*****************************************************************************/
		if (coven_debug_visual.GetBool())
		{
			if (bot_debug_select.GetInt() == pBot->entindex())
			{
				NDebugOverlay::Line(pBot->EyePosition(), *objLoc, 255, 255, 0, false, botdata->flTimeout - gpGlobals->curtime);
			}
		}
		/***********************************************************************************************/
	}
}

//-----------------------------------------------------------------------------
// Purpose: Run this Bot's AI for one frame.
//-----------------------------------------------------------------------------
void Bot_Think( CHL2MP_Player *pBot )
{
	float frametime = gpGlobals->frametime;

	CHL2MPRules *pRules = HL2MPRules();
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
			}
		}
		else
		{
			botdata->spawnTimer = gpGlobals->curtime + 1.0f;
		}
	}

	unsigned int buttons = 0;

	//Purchase Check
	PurchaseCheck(pBot);

	//Combat Check
	PlayerCheck(pBot);

	//Action Check
	if (pBot->IsPerformingDeferredAction())
	{
		if (botdata->bCombat && !pBot->IsPerformingDeferredAbility())
			pBot->CancelDeferredAction();
		else
		{
			if (pBot->MovementCancelActionCheck())
			{
				pBot->SetLocalAngles(botdata->forwardAngle);
				RunPlayerMove(pBot, botdata->forwardAngle, 0.0f, 0.0f, 0.0f, buttons, 0, frametime);
				return;
			}
			else if (pBot->CurrentDeferredAction() == COVEN_ACTION_REFUEL)
			{
				botdata->backwards = true;
				float velocity = Bot_Velocity(pBot);
				float distance = (pBot->GetAbsOrigin() - (pRules->pAPC->GetAbsOrigin())).LengthSqr();
				if (distance > 0.9f * sv_coven_refuel_distance.GetFloat())
				{
					botdata->backwards = false;
					velocity = -velocity;
				}
				else if (distance > 0.8f * sv_coven_refuel_distance.GetFloat())
				{
					velocity = 0.0f;
					botdata->backwards = false;
				}
				pBot->SetLocalAngles(botdata->forwardAngle);
				RunPlayerMove(pBot, botdata->forwardAngle, velocity, 0.0f, 0.0f, buttons | /*IN_DUCK |*/ IN_BACK, 0, frametime);
				return;
			}
		}
	}

	//Objective Check
	CheckObjective(pBot, false, &buttons);

	//Item Check
	CheckItem(pBot);

	//Health Check
	HealthCheck(pBot);

	//Weapon Swap Check
	buttons |= WeaponCheck(pBot);

	float forwardmove = 0.0f;
	float sidemove = botdata->sidemove;
	float upmove = 0.0f;
	trace_t trace; //throwaway trace

	byte  impulse = 0;

	//Pathing calcs
	if (pRules->pBotNet[botdata->m_targetNode] == NULL || botdata->bLost)
	{
		FindNearestNode(pBot);
	}
	else if (botdata->goWild == 0.0f)
	{
		const Vector *objLoc = CurrentObjectiveLoc(pBot);
		float flMinDistance;
		if (botdata->m_objectiveType == BOT_OBJECTIVE_CAPPOINT)
			flMinDistance = pRules->cap_point_distance[botdata->m_objective];
		else if (botdata->m_objectiveType == BOT_OBJECTIVE_FEED)
			flMinDistance = 9025.0f;
		else if (botdata->m_objectiveType == BOT_OBJECTIVE_BUILD)
			flMinDistance = 9025.0f;
		else //BOT_OBJECTIVE_ROGUE
			flMinDistance = BOT_NODE_TOLERANCE;
		//reached objective
		if (objLoc && (*objLoc - pBot->GetLocalOrigin()).LengthSqr() < flMinDistance && !botdata->bCombat)
		{
			if (botdata->m_objectiveType == BOT_OBJECTIVE_CAPPOINT)
			{
				//X BOTPATCH add vis check if vis check is appropriate
				bool lineOfSight = true;
				if (pRules->cap_point_sightcheck[botdata->m_objective])
				{
					UTIL_TraceLine(pBot->EyePosition(), *objLoc, MASK_SOLID, pBot, COLLISION_GROUP_DEBRIS, &trace);
					if (trace.fraction != 1.0)
						lineOfSight = false;
				}
				if (lineOfSight)
				{
					//X BOTPATCH if no valid objectives, dont set guardtimer, but clear the objective to -1 and getlost
					if ((pBot->GetTeamNumber() == COVEN_TEAMID_SLAYERS && pRules->s_caps == pRules->num_cap_points) || (pBot->GetTeamNumber() == COVEN_TEAMID_VAMPIRES && pRules->v_caps == pRules->num_cap_points))
					{
						GetLost(pBot);
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
						GetLost(pBot);
						botdata->bGuarding = false;
					}
				}
			}
			else if (botdata->m_objectiveType == BOT_OBJECTIVE_FEED)
			{
				if (pRules->doll_collector[botdata->m_lastCheckedRagdoll] != NULL)
				{
					botdata->bNotIgnoringStrikes = false;
					buttons |= (IN_USE | IN_DUCK);
				}
			}
			else if (botdata->m_objectiveType == BOT_OBJECTIVE_BUILD)
			{
				if (pBot->m_hTurret != NULL)
				{
					CCoven_Turret *pTurret = static_cast<CCoven_Turret *>(pBot->m_hTurret.Get());
					if (ValidTurret(pTurret) && !pBot->IsPerformingDeferredAction())
					{
						CBaseCombatWeapon *pStunStick = pBot->Weapon_OwnsThisType("weapon_stunstick");
						if (pStunStick && pBot->GetActiveWeapon() != pStunStick)
							pBot->Weapon_Switch(pStunStick);
						botdata->bNotIgnoringStrikes = false;
						buttons |= IN_ATTACK;
					}
					else if (pBot->IsPerformingDeferredAction())
					{
						botdata->bNotIgnoringStrikes = false;
					}
					else
					{
						GetLost(pBot);
					}
				}
			}
			else //BOT_OBJECTIVE_ROGUE
			{
				GetLost(pBot);
				botdata->bGuarding = false;
			}
		}
		else
		{
			if (botdata->m_objectiveType == BOT_OBJECTIVE_FEED || botdata->m_objectiveType == BOT_OBJECTIVE_BUILD)
			{
				botdata->bNotIgnoringStrikes = true;
			}
		}
		//reached node
		if (!botdata->bLost && botdata->guardTimer == 0.0f && (pRules->pBotNet[botdata->m_targetNode]->location - pBot->GetLocalOrigin()).LengthSqr() < BOT_NODE_TOLERANCE) //10
		{
			if (objLoc)
				Bot_Reached_Node(pBot, objLoc);
		}
		else if (!botdata->bLost && botdata->guardTimer == 0.0f)
		{
			if (gpGlobals->curtime > botdata->flTimeout && botdata->bNotIgnoringStrikes)
			{
#ifdef DEBUG_BOTS
				if (bot_debug.GetInt() == pBot->entindex())
				{
					Msg("Timed out! (%f)\n", botdata->flTimeout - gpGlobals->curtime);
				}
#endif
				Vector forward;
				Vector vecSrc = pBot->GetLocalOrigin();
				botdata->objectiveAngle.x = 0;
				AngleVectors(botdata->objectiveAngle, &forward);
				Vector vecEnd = vecSrc + forward * 32.0f;
				UTIL_TraceHull(vecSrc, vecEnd, VEC_HULL_MIN_SCALED(pBot), VEC_HULL_MAX_SCALED(pBot), MASK_PLAYERSOLID, pBot, COLLISION_GROUP_PLAYER, &trace);
				if (trace.DidHitNonWorldEntity() && objLoc)
					Bot_Reached_Node(pBot, objLoc);
				else
					GetLost(pBot, false, true);
			}
			/*BOTS_DEBUG_VISUAL*****************************************************************************/
			if (coven_debug_visual.GetBool())
			{
				if (pRules->pBotNet[botdata->m_targetNode] != NULL && bot_debug_select.GetInt() == pBot->entindex())
				{
					NDebugOverlay::SweptBox(pRules->pBotNet[botdata->m_targetNode]->location, pRules->pBotNet[botdata->m_targetNode]->location + Vector(0, 0, 72), Vector(0, -16, -16), Vector(0, 16, 16), QAngle(90, 0, 0), 0, 255, 255, 50, 0.05f);
					NDebugOverlay::Sphere(pRules->pBotNet[botdata->m_targetNode]->location, QAngle(0, 0, 0), BOT_NODE_TOLERANCE, 255, 0, 0, 0, false, 0.05f);
				}
			}
			/***********************************************************************************************/
		}
	}

	if (botdata->bNotIgnoringStrikes)
	{
		//Forward momentum calc
		forwardmove = Bot_Velocity(pBot);
		buttons |= botdata->backwards ? IN_BACK : IN_FORWARD; //freaking WAT. okay HL2 ladders...
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
						int accuracy = pPlayer->m_floatCloakFactor.Get() * 32 + 24 - bot_difficulty.GetInt() * 8;
						if (botdata->m_flLastCombat > 0.0f)
							forward = (botdata->m_vCombatLKP + Vector(random->RandomInt(-accuracy, accuracy), random->RandomInt(-accuracy, accuracy), random->RandomInt(-accuracy, accuracy))) - pBot->EyePosition();
						else
						forward = (pPlayer->GetPlayerMidPoint() + Vector(random->RandomInt(-accuracy, accuracy), random->RandomInt(-accuracy, accuracy), random->RandomInt(-accuracy, accuracy))) - pBot->EyePosition();
					}
					else //BB: vampires have perfect accuracy... doesn't really matter?
					{
						if (botdata->m_flLastCombat > 0.0f)
						{
							forward = botdata->m_vCombatLKP - pBot->EyePosition();
						}
						else
						{
							if (pPlayer->IsPlayer())
								forward = pPlayer->GetPlayerMidPoint() - pBot->EyePosition();
							else if (pPlayer->IsABuilding())
							{
								forward = pPlayer->EyePosition() - pBot->EyePosition(); //BB: TODO: this will not work for supply depos?
								float temp_length = forward.Length();
								//if (temp_length == 0)
								//	AngleVectors(pBot->GetLocalAngles(), &forward);
								if (temp_length < botdata->m_flBaseSpeed)
								{
									CCovenBuilding *building = ToCovenBuilding(pPlayer);
									//forwardmove = max(0.75f * botdata->m_flBaseSpeed, temp_length);
									if ((building->MyType() == BUILDING_TURRET && ((CCoven_Turret *)building)->bTipped) || temp_length < 100.0f)
									{
										buttons |= IN_DUCK;
									}
								}
							}
						}
					}
				}
				VectorAngles(forward, botdata->combatAngle);
				if (!isVampDoll)
				{
					CBaseCombatWeapon *pActiveWeapon = pBot->GetActiveWeapon();
					UTIL_TraceLine(pBot->EyePosition(), pPlayer->GetPlayerMidPoint(), MASK_SHOT, pBot, COLLISION_GROUP_PLAYER, &trace);
					float attackDot = pBot->GetTeamNumber() == COVEN_TEAMID_VAMPIRES ? DOT_45DEGREE : (pActiveWeapon && pActiveWeapon->IsGrenade()) ? DOT_25DEGREE : BOT_ATTACK_DOT_STRICT;
					if ((trace.DidHitNonWorldEntity() || trace.fraction == 1.0f) && (botdata->m_lastPlayerDot > attackDot || !pPlayer->IsPlayer()))
					{
						bool bSwing = true;
						if (pBot->HasAbility(COVEN_ABILITY_PHASE))
						{
							if (pBot->HasStatus(COVEN_STATUS_PHASE) && (gpGlobals->curtime < botdata->m_flAbilityTimer[pBot->AbilityKey(COVEN_ABILITY_PHASE)] || botdata->m_flLastCombatDist > 4096.0f))
								bSwing = false;
						}
						if (pBot->HasAbility(COVEN_ABILITY_DODGE))
						{
							if (pBot->HasStatus(COVEN_STATUS_DODGE) && (gpGlobals->curtime < botdata->m_flAbilityTimer[pBot->AbilityKey(COVEN_ABILITY_DODGE)] || botdata->m_flLastCombatDist > 4096.0f))
								bSwing = false;
						}
						if (bSwing && pActiveWeapon)
						{
							
							if (pActiveWeapon->IsGrenade())
							{
								CWeaponFrag *pGrenade = static_cast<CWeaponFrag *>(pActiveWeapon);
								if (pGrenade)
								{
									if (pGrenade->CanFire())
									{
										switch (pGrenade->GrenadeType())
										{
										case GRENADE_TYPE_FRAG:
										case GRENADE_TYPE_STUN:
											if (botdata->m_flLastCombatDist < 225625.0f)
												buttons |= IN_ATTACK2;
											else
												buttons |= IN_ATTACK;
											break;
										case GRENADE_TYPE_HOLYWATER:
											if (botdata->m_flLastCombatDist < 90000.0f)
												buttons |= IN_ATTACK2;
											else
												buttons |= IN_ATTACK;
											break;
										default:
											break;
										}

									}
									else
										buttons &= ~(IN_ATTACK | IN_ATTACK2);
								}
							}
							else if (pActiveWeapon->CanFire())
								buttons |= IN_ATTACK;
						}
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
					if ((pPlayer->myServerRagdoll->GetAbsOrigin() - pBot->GetAbsOrigin()).LengthSqr() < 30625.0f - 625.0f * bot_difficulty.GetInt())
					{
						CBaseCombatWeapon *pStake = pBot->Weapon_OwnsThisType("weapon_stake");
						CBaseCombatWeapon *pActiveWeapon = pBot->GetActiveWeapon();
						if (pStake)
						{
							// Switch to it if we don't have it out
							// Switch?
							if (pActiveWeapon != pStake)
							{
								pBot->Weapon_Switch(pStake);
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
						if (!(pBot->HasAbility(COVEN_ABILITY_CHARGE) && pBot->gorelock > GORELOCK_NONE) && forward.LengthSqr() > 22500.0f)
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
		if (pRules->pBotNet[botdata->m_targetNode] != NULL)
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
					int maxtries = 360 / angledelta;

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

						/*BOTS_DEBUG_VISUAL*****************************************************************************/
						if (coven_debug_visual.GetBool())
						{
							if (bot_debug_select.GetInt() == pBot->entindex())
							{
								//NDebugOverlay::Line(vecSrc, vecEnd, 255, 0, 0, false, 1.0f);
								//NDebugOverlay::BoxDirection(vecSrc, VEC_HULL_MIN_SCALED(pBot), VEC_HULL_MAX_SCALED(pBot), forward, 255, 0, 0, 50, 1.0f);
								NDebugOverlay::SweptBox(vecSrc, vecEnd, VEC_HULL_MIN_SCALED(pBot), VEC_HULL_MAX_SCALED(pBot), botdata->objectiveAngle, 255, 0, 0, 150, 10.0f);
							}
						}
						/***********************************************************************************************/

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
				if (botdata->m_pOverridePos != NULL)
				{
					forward = *botdata->m_pOverridePos - pBot->EyePosition();
					VectorAngles(forward, botdata->objectiveAngle);
				}
				else if (botdata->m_targetNode >= 0 && pRules->pBotNet[botdata->m_targetNode] != NULL)
				{
					forward = pRules->pBotNet[botdata->m_targetNode]->location - pBot->GetLocalOrigin();
					VectorAngles(forward, botdata->objectiveAngle);
				}
			}
			else if (gpGlobals->curtime > botdata->guardTimer)
			{
				botdata->bGuarding = false;
				GetLost(pBot);
			}
			else //guardtimer is set, slowly rotate and crouch
			{
				//Don't try to cap an already capped point
				if (CheckObjective(pBot, true))
				{
					if (botdata->bGuarding)
					{
						botdata->objectiveAngle.y += 1.0f;
					}
					else
					{
						Vector objLoc = *CurrentObjectiveLoc(pBot);
						objLoc.z = pBot->GetAbsOrigin().z;
						forward = objLoc - pBot->GetLocalOrigin();
						//Msg("%d %.02f\n", botdata->m_objective, forward.Length());
						if (botdata->m_objective > -1 && forward.LengthSqr() > 5184.0f && !botdata->bCombat)
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
			if (pRules->pBotNet[botdata->m_targetNode]->location.z > pBot->GetLocalOrigin().z)
				botdata->objectiveAngle.x = -90;
			else
				botdata->objectiveAngle.x = 90;
		}
		else
			buttons |= IN_JUMP; //get off this wasn't intended!
	}
	//BB: I just don't even have any words... I have to set angles every think?
	QAngle *desiredAngle = &botdata->objectiveAngle;
	if (pRules->pBotNet[botdata->m_targetNode] != NULL)
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
					speedLimit = (BOT_SPEED_LIMIT_DEG_COMBAT + 90 * bot_difficulty.GetInt()) * thinkTime;
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
	if (!(pBot->GetFlags() & FL_FROZEN) && !(pBot->GetEFlags() & EFL_BOT_FROZEN) && botdata->bNotIgnoringStrikes)
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
				//BB: TODO: combat shortcut to jump only?
				if (gpGlobals->curtime > botdata->stuckTimerThink)
				{
					if (pRules->pBotNet[botdata->m_targetNode] != NULL)
					{
						trace_t trace;
						Vector forward;
						AngleVectors(botdata->forwardAngle, &forward);
						UTIL_TraceLine(pBot->EyePosition(), pRules->pBotNet[botdata->m_targetNode]->location, MASK_SHOT, pBot, COLLISION_GROUP_PLAYER, &trace);
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
							else if (FClassnameIs(trace.m_pEnt, "coven_prop_physics"))
							{
#ifdef DEBUG_BOTS
								if (bot_debug.GetInt() == pBot->entindex())
								{
									Msg("Stuck: Encountered: \"%s\"! Trying Sidestep!\n", trace.m_pEnt->GetClassname());
								}
#endif
								botdata->nextstrafetime = gpGlobals->curtime + 1.0f;
								botdata->sidemove = botdata->m_flBaseSpeed;
							}
							else if (FClassnameIs(trace.m_pEnt, "prop_physics"))
							{
#ifdef DEBUG_BOTS
								if (bot_debug.GetInt() == pBot->entindex())
								{
									Msg("Stuck: Encountered: \"%s\"! Trying Attack!\n", trace.m_pEnt->GetClassname());
								}
#endif
								buttons |= IN_ATTACK;
							}
							else
							{
								//try a jump
								if (gpGlobals->curtime >= botdata->nextjumptime && pBot->GetMoveType() != MOVETYPE_LADDER)
								{
#ifdef DEBUG_BOTS
									if (bot_debug.GetInt() == pBot->entindex())
									{
										Msg("Stuck: Encountered Entity: \"%s\"! Trying Jump!\n", trace.m_pEnt->GetClassname());
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

	//This needs to be the last call... because we are going to be modifying view angles for abilities
	Bot_Ability_Think(pBot, buttons);

	//Msg("%d %.02f %.02f %.02f %.02f\n", pBot->covenClassID, botdata->m_flBaseSpeed, forwardmove, sidemove, botdata->nextstrafetime);
	RunPlayerMove(pBot, botdata->forwardAngle, forwardmove, sidemove, upmove, buttons, impulse, frametime);
}




//#endif

