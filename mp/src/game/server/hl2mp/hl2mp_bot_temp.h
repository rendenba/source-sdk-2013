//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#ifndef BOT_BASE_H
#define BOT_BASE_H
#ifdef _WIN32
#pragma once
#endif

#define BOT_NODE_TOLERANCE 20.0f

// If iTeam or iClass is -1, then a team or class is randomly chosen.
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

void Bot_Combat_Check( CHL2MP_Player *pBot, CBaseEntity *pAtk );

#endif // BOT_BASE_H

