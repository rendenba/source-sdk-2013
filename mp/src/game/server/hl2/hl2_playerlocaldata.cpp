//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "hl2_playerlocaldata.h"
#include "hl2_player.h"
#include "mathlib/mathlib.h"
#include "entitylist.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_SEND_TABLE_NOBASE( CHL2PlayerLocalData, DT_HL2Local )
	SendPropFloat( SENDINFO(m_flSuitPower), 10, SPROP_UNSIGNED | SPROP_ROUNDUP, 0.0, 200.0 ),
	SendPropInt( SENDINFO(m_bZooming), 1, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(m_bitsActiveDevices), MAX_SUIT_DEVICES, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(m_iSquadMemberCount) ),
	SendPropInt( SENDINFO(m_iSquadMedicCount) ),
	SendPropBool( SENDINFO(m_fSquadInFollowMode) ),
	SendPropBool( SENDINFO(m_bWeaponLowered) ),
	SendPropEHandle( SENDINFO(m_hAutoAimTarget) ),
	SendPropVector( SENDINFO(m_vecAutoAimPoint) ),
	SendPropEHandle( SENDINFO(m_hLadder) ),
	SendPropBool( SENDINFO(m_bDisplayReticle) ),
	SendPropBool( SENDINFO(m_bStickyAutoAim) ),
	SendPropBool( SENDINFO(m_bAutoAimTarget) ),
	SendPropInt( SENDINFO(covenXPCounter) ),
	SendPropInt( SENDINFO(covenStrengthCounter) ),
	SendPropInt( SENDINFO(covenConstitutionCounter) ),
	SendPropInt( SENDINFO(covenIntellectCounter) ),
	SendPropInt( SENDINFO(covenCurrentPointsSpent) ),
	SendPropInt( SENDINFO(covenCurrentLoadout1) ),
	SendPropInt( SENDINFO(covenCurrentLoadout2) ),
	SendPropInt( SENDINFO(covenCurrentLoadout3) ),
	SendPropInt( SENDINFO(covenCurrentLoadout4) ),
	SendPropArray3( SENDINFO_ARRAY3(covenStatusTimers), SendPropFloat( SENDINFO_ARRAY(covenStatusTimers), 0, SPROP_NOSCALE ) ),
	SendPropArray3( SENDINFO_ARRAY3(covenCooldownTimers), SendPropFloat( SENDINFO_ARRAY(covenCooldownTimers), 0, SPROP_NOSCALE ) ),
#ifdef HL2_EPISODIC
	SendPropFloat( SENDINFO(m_flFlashBattery) ),
	SendPropVector( SENDINFO(m_vecLocatorOrigin) ),
#endif
END_SEND_TABLE()

BEGIN_SIMPLE_DATADESC( CHL2PlayerLocalData )
	DEFINE_FIELD( m_flSuitPower, FIELD_FLOAT ),
	DEFINE_FIELD( m_bZooming, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bitsActiveDevices, FIELD_INTEGER ),
	DEFINE_FIELD( m_iSquadMemberCount, FIELD_INTEGER ),
	DEFINE_FIELD( m_iSquadMedicCount, FIELD_INTEGER ),
	DEFINE_FIELD( m_fSquadInFollowMode, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bWeaponLowered, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bDisplayReticle, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bStickyAutoAim, FIELD_BOOLEAN ),
	DEFINE_FIELD( covenXPCounter, FIELD_INTEGER ),
	DEFINE_FIELD( covenStrengthCounter, FIELD_INTEGER ),
	DEFINE_FIELD( covenConstitutionCounter, FIELD_INTEGER ),
	DEFINE_FIELD( covenIntellectCounter, FIELD_INTEGER ),
#ifdef HL2_EPISODIC
	DEFINE_FIELD( m_flFlashBattery, FIELD_FLOAT ),
	DEFINE_FIELD( m_vecLocatorOrigin, FIELD_POSITION_VECTOR ),
#endif
	// Ladder related stuff
	DEFINE_FIELD( m_hLadder, FIELD_EHANDLE ),
	DEFINE_EMBEDDED( m_LadderMove ),
END_DATADESC()

CHL2PlayerLocalData::CHL2PlayerLocalData()
{
	covenCooldownTimers.Set(0,0.0f);
	covenCooldownTimers.Set(1,0.0f);
	covenCooldownTimers.Set(2,0.0f);
	covenCooldownTimers.Set(3,0.0f);
	covenCurrentPointsSpent = 0;
	covenCurrentLoadout1 = 0;
	covenCurrentLoadout2 = 0;
	covenCurrentLoadout3 = 0;
	covenCurrentLoadout4 = 0;
	covenXPCounter = 0;
	covenStrengthCounter = 0;
	covenConstitutionCounter = 0;
	covenIntellectCounter = 0;
	m_flSuitPower = 0.0;
	m_bZooming = false;
	m_bWeaponLowered = false;
	m_hAutoAimTarget.Set(NULL);
	m_hLadder.Set(NULL);
	m_vecAutoAimPoint.GetForModify().Init();
	m_bDisplayReticle = false;
#ifdef HL2_EPISODIC
	m_flFlashBattery = 0.0f;
#endif
}

