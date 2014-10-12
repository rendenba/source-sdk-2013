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


// If iTeam or iClass is -1, then a team or class is randomly chosen.
CBasePlayer *BotPutInServer( bool bFrozen, int iTeam );
void BotRemove( CHL2MP_Player *pBot );
void Bot_Think( CHL2MP_Player *pBot );

#endif // BOT_BASE_H

