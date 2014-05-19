//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=============================================================================
#ifndef BASEMULTIPLAYERPLAYER_H
#define BASEMULTIPLAYERPLAYER_H
#pragma once

#include "player.h"
#include "ai_speech.h"

#define PLAYER_CLASS_UNDEFINED	-1
#define PLAYER_CLASS_NORMAL		1
#define PLAYER_CLASS_SCOUT		2
#define PLAYER_CLASS_HEAVY		3
#define PLAYER_CLASS_FISHER		4
#define PLAYER_CLASS_SARGE		5

#define	HL2_WALK_SPEED 120
#define	HL2_NORMAL_SPEED 190
#define HL2_SARGE_SPEED 200
#define	HL2_NORM_SPEED 210
#define HL2_SCOUT_SPEED 250
#define HL2_FAST_SPEED 320//400//650
#define	HL2_SPRINT_SPEED 320
#define HL2_HEAVY_SPEED 190
#define HL2_FISHER_SPEED 220
#define HL2_GORE_SPEED 190
#define HL2_DEGEN_SPEED 150
#define HL2_NITRO_SPEED 800

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CBaseMultiplayerPlayer : public CAI_ExpresserHost<CBasePlayer>
{

	DECLARE_CLASS( CBaseMultiplayerPlayer, CAI_ExpresserHost<CBasePlayer> );

public:

	CBaseMultiplayerPlayer();
	~CBaseMultiplayerPlayer();

	virtual void		Spawn( void );

	virtual void		PostConstructor( const char *szClassname );
	virtual void		ModifyOrAppendCriteria( AI_CriteriaSet& criteriaSet );

	virtual bool			SpeakIfAllowed( AIConcept_t concept, const char *modifiers = NULL, char *pszOutResponseChosen = NULL, size_t bufsize = 0, IRecipientFilter *filter = NULL );
	virtual IResponseSystem *GetResponseSystem();
	AI_Response				*SpeakConcept( int iConcept );
	virtual bool			SpeakConceptIfAllowed( int iConcept, const char *modifiers = NULL, char *pszOutResponseChosen = NULL, size_t bufsize = 0, IRecipientFilter *filter = NULL );

	virtual bool		CanHearAndReadChatFrom( CBasePlayer *pPlayer );
	virtual bool		CanSpeak( void ) { return true; }
	virtual bool		CanBeAutobalanced() { return true; }

	virtual void		Precache( void )
	{
		PrecacheParticleSystem( "achieved" );

		BaseClass::Precache();
	}

	virtual bool		ClientCommand( const CCommand &args );

	virtual bool		CanSpeakVoiceCommand( void ) { return true; }
	virtual bool		ShouldShowVoiceSubtitleToEnemy( void );
	virtual void		NoteSpokeVoiceCommand( const char *pszScenePlayed ) {}

	virtual void OnAchievementEarned( int iAchievement ) {}

	enum
	{
		CHAT_IGNORE_NONE = 0,
		CHAT_IGNORE_ALL,
		CHAT_IGNORE_TEAM,
	};

	int m_iIgnoreGlobalChat;

	//---------------------------------
	// Speech support
	virtual CAI_Expresser *GetExpresser() { return m_pExpresser; }
	virtual CMultiplayer_Expresser *GetMultiplayerExpresser() { return m_pExpresser; }

	void SetLastForcedChangeTeamTimeToNow( void ) { m_flLastForcedChangeTeamTime = gpGlobals->curtime; }
	float GetLastForcedChangeTeamTime( void ) { return m_flLastForcedChangeTeamTime; }

	void SetTeamBalanceScore( int iScore ) { m_iBalanceScore = iScore; }
	int GetTeamBalanceScore( void ) { return m_iBalanceScore; }

	virtual int	CalculateTeamBalanceScore( void );

	void AwardAchievement( int iAchievement, int iCount = 1 );
	int	GetPerLifeCounterKV( const char *name );
	void SetPerLifeCounterKV( const char *name, int value );
	void ResetPerLifeCounters( void );

	KeyValues *GetPerLifeCounterKeys( void ) { return m_pAchievementKV; }

	void EscortScoringThink( void );
	void StartScoringEscortPoints( float flRate );
	void StopScoringEscortPoints( void );
	float m_flAreaCaptureScoreAccumulator;
	float m_flCapPointScoreRate;

	float GetConnectionTime( void ) { return m_flConnectionTime; }

	// Command rate limiting.
	bool ShouldRunRateLimitedCommand( const CCommand &args );
	bool ShouldRunRateLimitedCommand( const char *pszCommand );

protected:
	virtual CAI_Expresser *CreateExpresser( void );

	int		m_iCurrentConcept;
private:
	//---------------------------------
	CMultiplayer_Expresser		*m_pExpresser;

	float m_flConnectionTime;
	float m_flLastForcedChangeTeamTime;

	int m_iBalanceScore;	// a score used to determine which players are switched to balance the teams

	KeyValues	*m_pAchievementKV;

	// This lets us rate limit the commands the players can execute so they don't overflow things like reliable buffers.
	CUtlDict<float,int>	m_RateLimitLastCommandTimes;
};

//-----------------------------------------------------------------------------
// Inline methods
//-----------------------------------------------------------------------------
inline CBaseMultiplayerPlayer *ToBaseMultiplayerPlayer( CBaseEntity *pEntity )
{
	if ( !pEntity || !pEntity->IsPlayer() )
		return NULL;
#if _DEBUG
	return dynamic_cast<CBaseMultiplayerPlayer *>( pEntity );
#else
	return static_cast<CBaseMultiplayerPlayer *>( pEntity );
#endif
}

#endif	// BASEMULTIPLAYERPLAYER_H
