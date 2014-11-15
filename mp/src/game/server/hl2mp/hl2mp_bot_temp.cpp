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

typedef struct
{
	bool			backwards;
	bool			left;
	int				turns;

	float			nextstrafetime;
	float			nextjumptime;
	float			nextusetime;
	float			sidemove;

	float			stuckTimer;
	float			goWild;
	float			spawnTimer;
	float			guardTimer;

	QAngle			forwardAngle;
	QAngle			lastAngles;
	Vector			lastPos;
	
	float			m_flJoinTeamTime;
	int				m_WantedTeam;
	int				m_WantedClass;

	int				m_lastPlayerCheck; //index
	bool			bCombat;

	int				m_lastNode; //index
	int				m_lastNodeProbe; //index (not id) of last probe into the botnet
	int				m_targetNode; //index
	bool			bLost;
} botdata_t;

static botdata_t g_BotData[ MAX_PLAYERS ];

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
	g_BotData[pPlayer->entindex()-1].m_lastNode = 0;
	g_BotData[pPlayer->entindex()-1].m_targetNode = 0;
	g_BotData[pPlayer->entindex()-1].m_lastNodeProbe = 0;
	g_BotData[pPlayer->entindex()-1].m_lastPlayerCheck = 0;
	g_BotData[pPlayer->entindex()-1].goWild = 0.0f;
	g_BotData[pPlayer->entindex()-1].stuckTimer = 0.0f;
	g_BotData[pPlayer->entindex()-1].spawnTimer = 0.0f;
	g_BotData[pPlayer->entindex()-1].guardTimer = 0.0f;
	g_BotData[pPlayer->entindex()-1].bLost = true;
	g_BotData[pPlayer->entindex()-1].bCombat = false;
	g_BotData[pPlayer->entindex()-1].left = false;
	g_BotData[pPlayer->entindex()-1].turns = 0;

	return pPlayer;
}


void GetLost( CHL2MP_Player *pBot )
{
	botdata_t *botdata = &g_BotData[ ENTINDEX( pBot->edict() ) - 1 ];
	botdata->m_targetNode = 0;
	botdata->bLost = true;
	CBaseCombatWeapon *pWeapon = pBot->Weapon_OwnsThisType( "weapon_stake" );
	if (pWeapon)
		pBot->SwitchToNextBestWeapon(pWeapon);
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
			vecSrc = pBot->GetLocalOrigin() + Vector( 0, 0, 36 );

		if (isVampDoll)
		{
			vecEnd = pPlayer->myServerRagdoll->GetAbsOrigin();
		}
		else
			vecEnd = pPlayer->GetLocalOrigin() + Vector( 0, 0, 36 );

		Vector playerVec = vecEnd-vecSrc;

		if ((playerVec).Length() < 400)
		{
			if (isVampDoll || ((pPlayer->m_floatCloakFactor < 0.1f && !(pPlayer->GetFlags() & EF_NODRAW))))
			{
				QAngle angle = pBot->GetLocalAngles();
				Vector forward;
				AngleVectors(angle, &forward);
				VectorNormalize(playerVec);
				float botDot = DotProduct(playerVec,forward);
				float test = 0.3f;
				if (isVampDoll)
					test = 0.0f;
				if (botDot > test)
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
						return;
					}
				}
			}
		}
	}

	if (botdata->bCombat)
	{
		botdata->backwards = false;
		GetLost(pBot);
	}

	botdata->bCombat = false;
	botdata->m_lastPlayerCheck++;
	
	if (botdata->m_lastPlayerCheck > gpGlobals->maxClients)
		botdata->m_lastPlayerCheck = 0;
}

void FindNearestNode( CHL2MP_Player *pBot )
{
	CHL2MPRules *pRules;
	pRules = HL2MPRules();
	if (pRules->botnet.Count() == 0 )
		return;

	botdata_t *botdata = &g_BotData[ ENTINDEX( pBot->edict() ) - 1 ];

	botnode *temp, *cur;
	temp = pRules->botnet[botdata->m_lastNodeProbe];
	cur = pRules->botnet[botdata->m_targetNode];

	//skip "stop" nodes
	if (temp->connectors.Count() > 1)
	{
		//skip nodes without +/- 50 z
		if (abs(pBot->GetAbsOrigin().z - temp->location.z) < 50)
		{
			//We found a closer node
			if ((pBot->GetAbsOrigin() - temp->location).Length() < (pBot->GetAbsOrigin() - cur->location).Length())
			{
				botdata->m_targetNode = botdata->m_lastNodeProbe;
			}
		}
	}

	botdata->m_lastNodeProbe++;
	if (botdata->m_lastNodeProbe >= pRules->botnet.Count())
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
static void RunPlayerMove( CHL2MP_Player *fakeclient, const QAngle& viewangles, float forwardmove, float sidemove, float upmove, unsigned short buttons, byte impulse, float frametime )
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

//-----------------------------------------------------------------------------
// Purpose: Run this Bot's AI for one frame.
//-----------------------------------------------------------------------------
void Bot_Think( CHL2MP_Player *pBot )
{
	unsigned short buttons = 0;
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
				GetLost(pBot);
			}
		}
		else
		{
			botdata->spawnTimer = gpGlobals->curtime + 1.0f;
		}
	}

	//Combat Check
	PlayerCheck(pBot);

	QAngle vecViewAngles;
	float forwardmove = 0.0;
	float sidemove = botdata->sidemove;
	float upmove = 0.0;

	byte  impulse = 0;
	float frametime = gpGlobals->frametime;

	vecViewAngles = pBot->GetLocalAngles();

	if (botdata->bLost && botdata->goWild == 0.0f)
	{
		//Msg("LOST!\n");
		FindNearestNode(pBot);
	}
	else
	{
		//reached node
		if ((pRules->botnet[botdata->m_targetNode]->location - pBot->GetLocalOrigin()).Length() < 10)
		{
			//Msg("Reached: %d\n", pRules->botnet[botdata->m_targetNode]->ID);
			if (pRules->botnet[botdata->m_targetNode]->connectors.Count() <= 1)
			{
				//dead end get lost for now
				//GetLost(pBot);
				//get bored after 10 seconds
				botdata->guardTimer = gpGlobals->curtime + 10.0f;
			}
			else
			{
				int pushedval = botdata->m_targetNode;
				int redflag = pRules->botnet[botdata->m_lastNode]->ID;
				int n = pRules->botnet[botdata->m_targetNode]->connectors.Count()-2;
				int sel = random->RandomInt(0,n);
				int m = pRules->botnet.Count();
				int i = 0;
				do
				{
					if (pRules->botnet[botdata->m_targetNode]->connectors[i] == redflag)
					{
						sel++;
					}
					i++;
				} while (i <= sel);
				int val = pRules->botnet[botdata->m_targetNode]->connectors[sel];
				int update = 0;
				for (int j = 0; j < m; j++)
				{
					if (pRules->botnet[j]->ID == val)
					{
						update = j;
						j = m;
					}
				}
				botdata->m_targetNode = update;
				//Msg("Headed to: %d\n", pRules->botnet[botdata->m_targetNode]->ID);
				botdata->m_lastNode = pushedval;
			}
		}
	}

	if ( pBot->IsAlive() && (pBot->GetSolid() == SOLID_BBOX) )
	{
		trace_t trace;

		if ( !pBot->IsEFlagSet(EFL_BOT_FROZEN) && botdata->guardTimer == 0.0f)
		{
			forwardmove = 600 * ( botdata->backwards ? -1 : 1 );
			if ( botdata->sidemove != 0.0f )
			{
				forwardmove *= random->RandomFloat( 0.1, 1.0f );
			}
		}

		if ( !pBot->IsEFlagSet(EFL_BOT_FROZEN))
		{
			Vector forward;
			QAngle angle = pBot->GetLocalAngles();//botdata->lastAngles;
			if (botdata->goWild > 0.0f)
			{
				if (gpGlobals->curtime > botdata->goWild)
				{
					botdata->goWild = 0.0f;
					GetLost(pBot);
				}
				Vector vecEnd;

				float angledelta = 15.0;

				int maxtries = (int)360.0/angledelta;

				angle = pBot->GetLocalAngles();

				Vector vecSrc;

				int dir = -1;
				botdata->turns++;
				if (botdata->left)
					dir = 1;

				if (botdata->left && botdata->turns > 1)
				{
					botdata->left = false;
					botdata->turns = 0;
				}
				else if (!botdata->left && botdata->turns > 1)
				{
					botdata->left = true;
					botdata->turns = 0;
				}

				while ( --maxtries >= 0 )
				{
					AngleVectors( angle, &forward );

					vecSrc = pBot->GetLocalOrigin();// + Vector( 0, 0, 36 );

					vecEnd = vecSrc + forward * 10;

					//UTIL_TraceLine(vecSrc, vecEnd, MASK_PLAYERSOLID, pBot, COLLISION_GROUP_NONE, &trace);
					UTIL_TraceHull( vecSrc, vecEnd, VEC_HULL_MIN_SCALED( pBot ), VEC_HULL_MAX_SCALED( pBot ), 
						MASK_PLAYERSOLID, pBot, COLLISION_GROUP_PLAYER, &trace );

					if ( trace.fraction == 1.0 )
					{
						maxtries = -1;
					}
					else
					{
						angle.y += dir*angledelta;
						//Msg("%d %.02f\n", maxtries, angle.y);


						if ( angle.y > 180 )
							angle.y -= 360;
						else if ( angle.y < -180 )
							angle.y += 360;
						//if (angle.y > 360)
						//	angle.y = 0;

						botdata->forwardAngle = angle;
						botdata->lastAngles = angle;
					}
				}
				pBot->SetLocalAngles( angle );
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
							int accuracy = 19;
							if (pPlayer->IsBot())
								accuracy = 25;
							forward = (pPlayer->GetLocalOrigin() + Vector(random->RandomInt(-accuracy,accuracy), random->RandomInt(-accuracy,accuracy), random->RandomInt(-accuracy,accuracy))) - pBot->GetLocalOrigin();
						}
						VectorAngles(forward, angle);
						botdata->forwardAngle = angle;
						botdata->lastAngles = angle;
						buttons |= IN_ATTACK;
						if (pBot->GetTeamNumber() == COVEN_TEAMID_SLAYERS && !isVampDoll)
						{
							botdata->backwards = true;
						}
						else if (pBot->GetTeamNumber() == COVEN_TEAMID_SLAYERS)
						{
							forwardmove = 600;
							buttons |= IN_DUCK;
							CBaseCombatWeapon *pWeapon = pBot->Weapon_OwnsThisType( "weapon_stake" );
							if ( pWeapon )
							{
								// Switch to it if we don't have it out
								CBaseCombatWeapon *pActiveWeapon = pBot->GetActiveWeapon();

								// Switch?
								if ( pActiveWeapon != pWeapon )
								{
									pBot->Weapon_Switch( pWeapon );
								}
							}
						}
						else if (pBot->GetTeamNumber() == COVEN_TEAMID_VAMPIRES)
						{
							forwardmove = 600;
						}
					}
				}
				else if (botdata->m_targetNode >= 0)
				{
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
				GetLost(pBot);
			}
			else
			{
				//guardtimer is set, slowly rotate and crouch
				angle.y += 2.0f;
				if ( angle.y > 180 )
					angle.y -= 360;
				else if ( angle.y < -180 )
					angle.y += 360;
				buttons |= IN_DUCK;
				botdata->forwardAngle = angle;
				botdata->lastAngles = angle;
			}

			if ( gpGlobals->curtime >= botdata->nextstrafetime )
			{
				botdata->nextstrafetime = gpGlobals->curtime + 1.0f;

				if ( random->RandomInt( 0, 5 ) == 0)
				{
					botdata->sidemove = -600.0f + 1200.0f * random->RandomFloat( 0, 2 );
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

			pBot->SetLocalAngles( angle );
			vecViewAngles = angle;
		}

		/*// Is my team being forced to defend?
		if ( bot_defend.GetInt() == pBot->GetTeamNumber() )
		{
			buttons |= IN_ATTACK2;
		}
		// If bots are being forced to fire a weapon, see if I have it
		else if ( bot_forcefireweapon.GetString() )
		{
			CBaseCombatWeapon *pWeapon = pBot->Weapon_OwnsThisType( bot_forcefireweapon.GetString() );
			if ( pWeapon )
			{
				// Switch to it if we don't have it out
				CBaseCombatWeapon *pActiveWeapon = pBot->GetActiveWeapon();

				// Switch?
				if ( pActiveWeapon != pWeapon )
				{
					pBot->Weapon_Switch( pWeapon );
				}
				else
				{
					// Start firing
					// Some weapons require releases, so randomise firing
					if ( bot_forceattackon.GetBool() || (RandomFloat(0.0,1.0) > 0.5) )
					{
						buttons |= bot_forceattack2.GetBool() ? IN_ATTACK2 : IN_ATTACK;
					}
				}
			}
		}*/

		/*if ( bot_flipout.GetInt() )
		{
			if ( bot_forceattackon.GetBool() || (RandomFloat(0.0,1.0) > 0.5) )
			{
				buttons |= bot_forceattack2.GetBool() ? IN_ATTACK2 : IN_ATTACK;
			}
		}
		
		if ( strlen( bot_sendcmd.GetString() ) > 0 )
		{
			//send the cmd from this bot
			CCommand args;
			args.Tokenize( bot_sendcmd.GetString() );
			pBot->ClientCommand( args );

			bot_sendcmd.SetValue("");
		}*/
	}

	/*if ( bot_flipout.GetInt() >= 2 )
	{

		QAngle angOffset = RandomAngle( -1, 1 );

		botdata->lastAngles += angOffset;

		for ( int i = 0 ; i < 2; i++ )
		{
			if ( fabs( botdata->lastAngles[ i ] - botdata->forwardAngle[ i ] ) > 15.0f )
			{
				if ( botdata->lastAngles[ i ] > botdata->forwardAngle[ i ] )
				{
					botdata->lastAngles[ i ] = botdata->forwardAngle[ i ] + 15;
				}
				else
				{
					botdata->lastAngles[ i ] = botdata->forwardAngle[ i ] - 15;
				}
			}
		}

		botdata->lastAngles[ 2 ] = 0;

		pBot->SetLocalAngles( botdata->lastAngles );
	}*/

	if (botdata->guardTimer == 0.0f && (pBot->GetLocalOrigin() - botdata->lastPos).Length() < 4.0f) //STUCK?
	{
		if (botdata->stuckTimer > 0.0f)
		{
			if (gpGlobals->curtime - botdata->stuckTimer >= 0.2f && gpGlobals->curtime >= botdata->nextusetime)
			{
				//door?
				buttons |= IN_USE;
				botdata->nextusetime = gpGlobals->curtime + 3.0f;
			}
			if (gpGlobals->curtime - botdata->stuckTimer >= 0.5f && gpGlobals->curtime >= botdata->nextjumptime)
			{
				//try a jump
				buttons |= IN_JUMP;
				botdata->nextjumptime = gpGlobals->curtime + 3.0f;
			}
			if (gpGlobals->curtime - botdata->stuckTimer >= 2.0f) //STUCK!
			{
				botdata->goWild = gpGlobals->curtime + 8.0f;
				botdata->turns = 0;
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
	}

	RunPlayerMove( pBot, pBot->GetLocalAngles(), forwardmove, sidemove, upmove, buttons, impulse, frametime );
}




//#endif

