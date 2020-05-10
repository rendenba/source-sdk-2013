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

//#define DEBUG_BOTS
//#define DEBUG_BOTS_VISUAL

#define BOT_NODE_TOLERANCE 20.0f

// If iTeam or iClass is -1, then a team or class is randomly chosen.
CBasePlayer *BotPutInServer( bool bFrozen, int iTeam );
void BotRemove( CHL2MP_Player *pBot );
void Bot_Think( CHL2MP_Player *pBot );
void Set_Bot_Base_Velocity( CHL2MP_Player *pBot );
bool Bot_Right_Team( CHL2MP_Player *pBot );
void FindNearestNode( CHL2MP_Player *pBot );
void PlayerCheck( CHL2MP_Player *pBot );
void GetLost( CHL2MP_Player *pBot, bool iZ = false, bool visCheck = false );

void Bot_Combat_Check( CHL2MP_Player *pBot, CBaseEntity *pAtk );

#endif // BOT_BASE_H

