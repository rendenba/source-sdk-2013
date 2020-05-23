//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $NoKeywords: $
//=============================================================================//

#if !defined( C_HL2_PLAYERLOCALDATA_H )
#define C_HL2_PLAYERLOCALDATA_H
#ifdef _WIN32
#pragma once
#endif


#include "dt_recv.h"

#include "hl2/hl_movedata.h"

EXTERN_RECV_TABLE( DT_HL2Local );


class C_HL2PlayerLocalData
{
public:
	DECLARE_PREDICTABLE();
	DECLARE_CLASS_NOBASE( C_HL2PlayerLocalData );
	DECLARE_EMBEDDED_NETWORKVAR();

	C_HL2PlayerLocalData();

	float	m_flSuitPower;
	bool	m_bZooming;
	int		m_bitsActiveDevices;
	int		m_iSquadMemberCount;
	int		m_iSquadMedicCount;
	bool	m_fSquadInFollowMode;
	bool	m_bWeaponLowered;
	EHANDLE m_hAutoAimTarget;
	Vector	m_vecAutoAimPoint;
	bool	m_bDisplayReticle;
	bool	m_bStickyAutoAim;
	bool	m_bAutoAimTarget;
	int		covenXPCounter;
	float	covenStrengthCounter;
	float	covenConstitutionCounter;
	float	covenIntellectCounter;
	int		covenCurrentPointsSpent;
	float	covenGCD;
	CovenAbility_t		covenAbilities[COVEN_MAX_ABILITIES];
	float	covenStatusTimers[COVEN_STATUS_COUNT];
	int		covenStatusMagnitude[COVEN_STATUS_COUNT];
	float	covenCooldownTimers[COVEN_MAX_ABILITIES];
#ifdef HL2_EPISODIC
	float	m_flFlashBattery;
	Vector	m_vecLocatorOrigin;
#endif

	// Ladder related data
	EHANDLE			m_hLadder;
	LadderMove_t	m_LadderMove;
};


#endif
