//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef CHL2MPCLIENTMINIMAPDIALOG_H
#define CHL2MPCLIENTMINIMAPDIALOG_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include <game/client/iviewport.h>
#include "GameEventListener.h"
#include "filesystem.h"

//-----------------------------------------------------------------------------
// Purpose: Game ScoreBoard
//-----------------------------------------------------------------------------
class CHL2MPClientMiniMapDialog : public vgui::EditablePanel, public IViewPortPanel, public CGameEventListener
{
private:
	DECLARE_CLASS_SIMPLE(CHL2MPClientMiniMapDialog, vgui::EditablePanel);
	
public:
	CHL2MPClientMiniMapDialog(IViewPort *pViewPort);
	~CHL2MPClientMiniMapDialog();

	virtual const char *GetName( void ) { return PANEL_MINIMAP; }
	virtual void SetData(KeyValues *data) {};
	virtual void Reset();
	virtual void Update();
	virtual bool NeedsUpdate( void );
	virtual bool HasInputElements( void ) { return true; }
	virtual void ShowPanel( bool bShow );

	vgui::VPANEL GetVPanel( void ) { return BaseClass::GetVPanel(); }
  	virtual bool IsVisible() { return BaseClass::IsVisible(); }
  	virtual void SetParent( vgui::VPANEL parent ) { BaseClass::SetParent( parent ); }
 	
	// IGameEventListener interface:
	virtual void FireGameEvent( IGameEvent *event);


protected:
	// scoreboard overrides
	virtual void InitScoreboardSections();
	virtual void UpdateTeamInfo();
	virtual bool GetPlayerScoreInfo(int playerIndex, KeyValues *outPlayerInfo);
	virtual void UpdatePlayerInfo();

	// vgui overrides for rounded corner background
	virtual void PaintBackground();
	virtual void PaintBorder();
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	void MoveToCenterOfScreen();


	// functions to override
	virtual void OnThink();
	virtual void AddHeader(); // add the start header of the scoreboard
	virtual void AddSection(int teamType, int teamNumber); // add a new section header for a team
	virtual int GetAdditionalHeight() { return 0; }

	float m_fNextUpdateTime;


private:

	bool LoadFromBuffer( char const *resourceName, CUtlBuffer &buf, IBaseFileSystem *pFileSystem, const char *pPathID );
	bool LoadFromBuffer( char const *resourceName, const char *pBuffer, IBaseFileSystem* pFileSystem, const char *pPathID = NULL );
	bool LoadDotFile( IBaseFileSystem *filesystem, const char *resourceName, const char *pathID );
	bool LoadDotFile( IBaseFileSystem *filesystem, const char *resourceName );

	int m_nMapTex;
	int m_nGrayDot;
	int m_nRedDot;
	int m_nBlueDot;
	int m_nGoldStar;
	char mapname[MAX_PATH];

	CUtlVector<Vector2D> mapspots;
	CUtlVector<Vector2D> textspots;
	char text_names[COVEN_MAX_CAP_POINTS][MAX_PLAYER_NAME_LENGTH];
	Vector2D cts;
	Vector2D cts_zone;
	int			m_iDesiredHeight;
	IViewPort	*m_pViewPort;
	ButtonCode_t m_nCloseKey;

	int GetSectionFromTeamNumber( int teamNumber );
	enum 
	{ 
		CSTRIKE_NAME_WIDTH = 300,
		CSTRIKE_CLASS_WIDTH = 66,
		CSTRIKE_SCORE_WIDTH = 40,
		CSTRIKE_DEATH_WIDTH = 46,
		CSTRIKE_PING_WIDTH = 46,
		CSTRIKE_LEVELS_WIDTH = 46,
		CSTRIKE_STATUS_WIDTH = 40,
//		CSTRIKE_VOICE_WIDTH = 40, 
//		CSTRIKE_FRIENDS_WIDTH = 24,
	};

	// rounded corners
	Color					 m_bgColor;
	Color					 m_borderColor;
	CPanelAnimationVar( vgui::HFont, m_hTextFont, "TextFont", "Default" );
};


#endif // CHL2MPCLIENTSCOREBOARDDIALOG_H
