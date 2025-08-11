//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef HL2_PLAYERLOCALDATA_H
#define HL2_PLAYERLOCALDATA_H
#ifdef _WIN32
#pragma once
#endif

#include "networkvar.h"

#include "hl_movedata.h"

//-----------------------------------------------------------------------------
// Purpose: Player specific data for HL2 ( sent only to local player, too )
//-----------------------------------------------------------------------------
class CHL2PlayerLocalData
{
public:
	// Save/restore
	DECLARE_SIMPLE_DATADESC();
	DECLARE_CLASS_NOBASE( CHL2PlayerLocalData );
	DECLARE_EMBEDDED_NETWORKVAR();

	CHL2PlayerLocalData();

	CNetworkVar( float, m_flSuitPower );
	CNetworkVar( float, m_flSuitPowerLoad );
	CNetworkVar( float, m_flTimeAllSuitDevicesOff );
	CNetworkVar( bool,  m_bNewSprinting );
	CNetworkVar( bool,	m_bZooming );
	CNetworkVar( int,	m_bitsActiveDevices );
	CNetworkVar( int,	m_iSquadMemberCount );
	CNetworkVar( int,	m_iSquadMedicCount );
	CNetworkVar( bool,	m_fSquadInFollowMode );
	CNetworkVar( bool,	m_bWeaponLowered );
	CNetworkVar( EHANDLE, m_hAutoAimTarget );
	CNetworkVar( Vector, m_vecAutoAimPoint );
	CNetworkVar( bool,	m_bDisplayReticle );
	CNetworkVar( bool,	m_bStickyAutoAim );
	CNetworkVar( bool,	m_bAutoAimTarget );
	CNetworkVar( int, covenXPCounter );
	CNetworkVar( float, covenStrengthCounter );
	CNetworkVar( float, covenConstitutionCounter );
	CNetworkVar( float, covenIntellectCounter );
	CNetworkVar( int, covenCurrentPointsSpent );
	CNetworkVar( float, covenGCD );
	CNetworkVar( float, covenActionTimer );
	CNetworkVar( CovenDeferredAction_t, covenAction);
	CNetworkArray( CovenAbility_t, covenAbilities, COVEN_MAX_ABILITIES );
	CNetworkArray( float, covenStatusTimers, COVEN_STATUS_COUNT );
	CNetworkArray( int, covenStatusMagnitude, COVEN_STATUS_COUNT );
	CNetworkArray( float, covenCooldownTimers, COVEN_MAX_ABILITIES );
	CNetworkArray( int, m_iItems, COVEN_ITEM_COUNT );
	CNetworkArray( int, m_iDollHP, COVEN_MAX_RAGDOLLS );
	CNetworkVar(int,	m_iNumTripmines);
	CNetworkVar(int,	m_iNumSatchel);
#ifdef HL2_EPISODIC
	CNetworkVar( float, m_flFlashBattery );
	CNetworkVar( Vector, m_vecLocatorOrigin );
#endif

	// Ladder related data
	CNetworkHandle( CBaseEntity, m_hLadder );
	LadderMove_t			m_LadderMove;
};

EXTERN_SEND_TABLE(DT_HL2Local);


#endif // HL2_PLAYERLOCALDATA_H
