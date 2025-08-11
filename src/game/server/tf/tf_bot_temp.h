//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef TF_BOT_TEMP_H
#define TF_BOT_TEMP_H
#ifdef _WIN32
#pragma once
#endif

#define BOT_NODE_TOLERANCE 400.0f

// If iTeam or iClass is -1, then a team or class is randomly chosen.
<<<<<<< HEAD:mp/src/game/server/hl2mp/hl2mp_bot_temp.h
CBasePlayer *BotPutInServer( bool bFrozen, CovenTeamID_t iTeam );
void BotRemove( CHL2MP_Player *pBot );
void Bot_Think( CHL2MP_Player *pBot );
void Bot_Reached_Node( CHL2MP_Player *pBot, const Vector *objLoc );
void Set_Bot_Base_Velocity( CHL2MP_Player *pBot );
bool Bot_Right_Team( CHL2MP_Player *pBot );
float Bot_Velocity(CHL2MP_Player *pBot);
void FindNearestNode( CHL2MP_Player *pBot );
void PlayerCheck( CHL2MP_Player *pBot );
void GetLost( CHL2MP_Player *pBot, bool iZ = false, bool visCheck = false );
void BotRespawn( CHL2MP_Player *pBot );
void BotDeath( CHL2MP_Player *pBot );
void Bot_Combat_Check( CHL2MP_Player *pBot, CBaseEntity *pAtk );
=======
CBasePlayer *BotPutInServer( bool bTargetDummy, bool bFrozen, int iTeam, int iClass, const char *pszCustomName );

void Bot_RunAll();

>>>>>>> upstream/master:src/game/server/tf/tf_bot_temp.h

#endif // TF_BOT_TEMP_H
