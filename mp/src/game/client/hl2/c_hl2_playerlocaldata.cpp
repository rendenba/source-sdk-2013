//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_hl2_playerlocaldata.h"
#include "dt_recv.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_RECV_TABLE_NOBASE( C_HL2PlayerLocalData, DT_HL2Local )
	RecvPropFloat( RECVINFO(m_flSuitPower) ),
	RecvPropInt( RECVINFO(m_bZooming) ),
	RecvPropInt( RECVINFO(m_bitsActiveDevices) ),
	RecvPropInt( RECVINFO(m_iSquadMemberCount) ),
	RecvPropInt( RECVINFO(m_iSquadMedicCount) ),
	RecvPropBool( RECVINFO(m_fSquadInFollowMode) ),
	RecvPropBool( RECVINFO(m_bWeaponLowered) ),
	RecvPropEHandle( RECVINFO(m_hAutoAimTarget) ),
	RecvPropVector( RECVINFO(m_vecAutoAimPoint) ),
	RecvPropEHandle( RECVINFO(m_hLadder) ),
	RecvPropBool( RECVINFO(m_bDisplayReticle) ),
	RecvPropBool( RECVINFO(m_bStickyAutoAim) ),
	RecvPropBool( RECVINFO(m_bAutoAimTarget) ),
	RecvPropInt( RECVINFO(covenXPCounter) ),
	RecvPropFloat( RECVINFO(covenStrengthCounter) ),
	RecvPropFloat( RECVINFO(covenConstitutionCounter) ),
	RecvPropFloat( RECVINFO(covenIntellectCounter) ),
	RecvPropInt( RECVINFO(covenCurrentPointsSpent) ),
	RecvPropFloat( RECVINFO(covenGCD) ),
	RecvPropFloat( RECVINFO(covenActionTimer) ),
	RecvPropInt( RECVINFO(covenAction) ),
	RecvPropArray3( RECVINFO_ARRAY(covenAbilities), RecvPropInt( RECVINFO(covenAbilities[0]))),
	RecvPropArray3( RECVINFO_ARRAY(covenStatusTimers), RecvPropFloat( RECVINFO(covenStatusTimers[0]))),
	RecvPropArray3( RECVINFO_ARRAY(covenCooldownTimers), RecvPropFloat( RECVINFO(covenCooldownTimers[0]))),
	RecvPropArray3( RECVINFO_ARRAY(covenStatusMagnitude), RecvPropInt( RECVINFO(covenStatusMagnitude[0]))),
	RecvPropArray3( RECVINFO_ARRAY(m_iItems), RecvPropInt( RECVINFO(m_iItems[0])) ),
	RecvPropArray3( RECVINFO_ARRAY(m_iDollHP), RecvPropInt(RECVINFO(m_iDollHP[0])) ),
#ifdef HL2_EPISODIC
	RecvPropFloat( RECVINFO(m_flFlashBattery) ),
	RecvPropVector( RECVINFO(m_vecLocatorOrigin) ),
#endif
END_RECV_TABLE()

BEGIN_PREDICTION_DATA_NO_BASE( C_HL2PlayerLocalData )
	DEFINE_PRED_FIELD( m_hLadder, FIELD_EHANDLE, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()

C_HL2PlayerLocalData::C_HL2PlayerLocalData()
{
	covenCurrentPointsSpent = 0;
	covenXPCounter = 0;
	covenStrengthCounter = 0.0f;
	covenConstitutionCounter = 0.0f;
	covenIntellectCounter = 0.0f;
	covenGCD = 0.0f;
	covenActionTimer = 0.0f;
	covenAction = (CovenDeferredAction_t)0;
	m_flSuitPower = 0.0;
	m_bZooming = false;
	m_iSquadMemberCount = 0;
	m_iSquadMedicCount = 0;
	m_fSquadInFollowMode = false;
	m_bWeaponLowered = false;
	m_hLadder = NULL;
#ifdef HL2_EPISODIC
	m_flFlashBattery = 0.0f;
	m_vecLocatorOrigin = vec3_origin;
#endif
}

