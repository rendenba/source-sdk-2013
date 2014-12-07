//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include <stdio.h>

#include <cdll_client_int.h>
#include <cdll_util.h>
#include <globalvars_base.h>
#include <igameresources.h>
#include "IGameUIFuncs.h" // for key bindings
#include "inputsystem/iinputsystem.h"
#include "clientscoreboarddialog.h"
#include <voice_status.h>

#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vstdlib/IKeyValuesSystem.h>

#include <KeyValues.h>
#include <vgui_controls/ImageList.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/SectionedListPanel.h>

#include <game/client/iviewport.h>
#include <igameresources.h>

#include "hl2mpclientminimap.h"

#include "hl2mp_gamerules.h"
#include "c_team.h"

using namespace vgui;

static char temparray[256];

#define TEAM_MAXCOUNT			5

// id's of sections used in the scoreboard
enum EScoreboardSections
{
	SCORESECTION_COMBINE = 1,
	SCORESECTION_REBELS = 2,
	SCORESECTION_FREEFORALL = 3,
	SCORESECTION_SPECTATOR = 4
};

const int NumSegments = 7;
static int coord[NumSegments+1] = {
	0,
	1,
	2,
	3,
	4,
	6,
	9,
	10
};

//-----------------------------------------------------------------------------
// Purpose: Konstructor
//-----------------------------------------------------------------------------
CHL2MPClientMiniMapDialog::CHL2MPClientMiniMapDialog(IViewPort *pViewPort) : EditablePanel( NULL, PANEL_MINIMAP )
{
	m_nCloseKey = BUTTON_CODE_INVALID;

	
	// initialize dialog
	SetProportional(true);
	SetKeyBoardInputEnabled(false);
	SetMouseInputEnabled(false);

	// set the scheme before any child control is created
	SetScheme("ClientScheme");

	LoadControlSettings("Resource/UI/MiniMap.res");
	m_iDesiredHeight = GetTall();

	
	// update scoreboard instantly if on of these events occure
	ListenForGameEvent( "hltv_status" );
	ListenForGameEvent( "game_newmap" );
	ListenForGameEvent( "server_spawn" );

	m_nGrayDot = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile( m_nGrayDot, "effects/graydot", true, true);

	m_nRedDot = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile( m_nRedDot, "effects/reddot", true, true);

	m_nBlueDot = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile( m_nBlueDot, "effects/bluedot", true, true);

	m_nGoldStar = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile( m_nGoldStar, "effects/goldstar", true, true);
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CHL2MPClientMiniMapDialog::~CHL2MPClientMiniMapDialog()
{
}

void CHL2MPClientMiniMapDialog::OnThink()
{
	BaseClass::OnThink();

	// NOTE: this is necessary because of the way input works.
	// If a key down message is sent to vgui, then it will get the key up message
	// Sometimes the scoreboard is activated by other vgui menus, 
	// sometimes by console commands. In the case where it's activated by
	// other vgui menus, we lose the key up message because this panel
	// doesn't accept keyboard input. It *can't* accept keyboard input
	// because another feature of the dialog is that if it's triggered
	// from within the game, you should be able to still run around while
	// the scoreboard is up. That feature is impossible if this panel accepts input.
	// because if a vgui panel is up that accepts input, it prevents the engine from
	// receiving that input. So, I'm stuck with a polling solution.
	// 
	// Close key is set to non-invalid when something other than a keybind
	// brings the scoreboard up, and it's set to invalid as soon as the 
	// dialog becomes hidden.
	if ( m_nCloseKey != BUTTON_CODE_INVALID )
	{
		if ( !g_pInputSystem->IsButtonDown( m_nCloseKey ) )
		{
			m_nCloseKey = BUTTON_CODE_INVALID;
			gViewPortInterface->ShowPanel( PANEL_MINIMAP, false );
			GetClientVoiceMgr()->StopSquelchMode();
		}
	}
}

void CHL2MPClientMiniMapDialog::ShowPanel(bool bShow)
{
	// Catch the case where we call ShowPanel before ApplySchemeSettings, eg when
	// going from windowed <-> fullscreen

	if ( !bShow )
	{
		m_nCloseKey = BUTTON_CODE_INVALID;
	}

	if ( BaseClass::IsVisible() == bShow )
		return;

	if ( bShow )
	{
		Reset();
		Update();
		SetVisible( true );
		MoveToFront();
	}
	else
	{
		BaseClass::SetVisible( false );
		SetMouseInputEnabled( false );
		SetKeyBoardInputEnabled( false );
	}
}

bool CHL2MPClientMiniMapDialog::NeedsUpdate( void )
{
	return (m_fNextUpdateTime < gpGlobals->curtime);	
}

void CHL2MPClientMiniMapDialog::Update( void )
{
	// Set the title
	
	// Reset();
	
	// grow the scoreboard to fit all the players
	int wide, tall;
	wide = 512;
	tall = 512;
	tall += GetAdditionalHeight();
	if (m_iDesiredHeight < tall)
	{
		SetSize(wide, tall);
	}
	else
	{
		SetSize(wide, m_iDesiredHeight);
	}
	MoveToCenterOfScreen();

	C_Team *team = GetGlobalTeam( COVEN_TEAMID_SLAYERS );
	const char *pDialogVarTeamScore = "s_teamscore";
	if (team)
		SetDialogVariable( pDialogVarTeamScore, team->Get_Score() );
	team = GetGlobalTeam( COVEN_TEAMID_VAMPIRES );
	pDialogVarTeamScore = "v_teamscore";
	if (team)
		SetDialogVariable( pDialogVarTeamScore, team->Get_Score() );

	// update every second
	m_fNextUpdateTime = gpGlobals->curtime + 1.0f; 
}

void CHL2MPClientMiniMapDialog::MoveToCenterOfScreen()
{
	int wx, wy, ww, wt;
	surface()->GetWorkspaceBounds(wx, wy, ww, wt);
	SetPos((ww - GetWide()) / 2, (wt - GetTall()) / 2);
	Panel *control = FindChildByName( "SlayerScoreLabel" );
	if ( control )
	{
		control->SetPos(GetWide()/4-control->GetWide(), GetTall()-50);
		control->MoveToFront();
	}
	control = FindChildByName( "SlayerScoreValue" );
	if ( control )
	{
		control->SetPos(GetWide()/4+10, GetTall()-50);
		control->MoveToFront();
	}
	control = FindChildByName( "VampireScoreLabel" );
	if ( control )
	{
		control->SetPos(GetWide()-GetWide()/4-control->GetWide(), GetTall()-50);
		control->MoveToFront();
	}
	control = FindChildByName( "VampireScoreValue" );
	if ( control )
	{
		control->SetPos(GetWide()-GetWide()/4+10, GetTall()-50);
		control->MoveToFront();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Paint background for rounded corners
//-----------------------------------------------------------------------------
void CHL2MPClientMiniMapDialog::PaintBackground()
{
	int x1, x2, y1, y2;
	surface()->DrawSetColor(m_bgColor);
	surface()->DrawSetTextColor(m_bgColor);

	surface()->DrawSetTexture( m_nMapTex );

	int wide, tall;
	GetSize( wide, tall );

	int i;

	// top-left corner --------------------------------------------------------
	int xDir = 1;
	int yDir = -1;
	int xIndex = 0;
	int yIndex = NumSegments - 1;
	int xMult = 1;
	int yMult = 1;
	int x = 0;
	int y = 0;
	for ( i=0; i<NumSegments; ++i )
	{
		x1 = MIN( x + coord[xIndex]*xMult, x + coord[xIndex+1]*xMult );
		x2 = MAX( x + coord[xIndex]*xMult, x + coord[xIndex+1]*xMult );
		y1 = MAX( y + coord[yIndex]*yMult, y + coord[yIndex+1]*yMult );
		y2 = y + coord[NumSegments];
		surface()->DrawFilledRect( x1, y1, x2, y2 );

		xIndex += xDir;
		yIndex += yDir;
	}

	// top-right corner -------------------------------------------------------
	xDir = 1;
	yDir = -1;
	xIndex = 0;
	yIndex = NumSegments - 1;
	x = wide;
	y = 0;
	xMult = -1;
	yMult = 1;
	for ( i=0; i<NumSegments; ++i )
	{
		x1 = MIN( x + coord[xIndex]*xMult, x + coord[xIndex+1]*xMult );
		x2 = MAX( x + coord[xIndex]*xMult, x + coord[xIndex+1]*xMult );
		y1 = MAX( y + coord[yIndex]*yMult, y + coord[yIndex+1]*yMult );
		y2 = y + coord[NumSegments];
		surface()->DrawFilledRect( x1, y1, x2, y2 );
		xIndex += xDir;
		yIndex += yDir;
	}

	// bottom-right corner ----------------------------------------------------
	xDir = 1;
	yDir = -1;
	xIndex = 0;
	yIndex = NumSegments - 1;
	x = wide;
	y = tall;
	xMult = -1;
	yMult = -1;
	for ( i=0; i<NumSegments; ++i )
	{
		x1 = MIN( x + coord[xIndex]*xMult, x + coord[xIndex+1]*xMult );
		x2 = MAX( x + coord[xIndex]*xMult, x + coord[xIndex+1]*xMult );
		y1 = y - coord[NumSegments];
		y2 = MIN( y + coord[yIndex]*yMult, y + coord[yIndex+1]*yMult );
		surface()->DrawFilledRect( x1, y1, x2, y2 );
		xIndex += xDir;
		yIndex += yDir;
	}

	// bottom-left corner -----------------------------------------------------
	xDir = 1;
	yDir = -1;
	xIndex = 0;
	yIndex = NumSegments - 1;
	x = 0;
	y = tall;
	xMult = 1;
	yMult = -1;
	for ( i=0; i<NumSegments; ++i )
	{
		x1 = MIN( x + coord[xIndex]*xMult, x + coord[xIndex+1]*xMult );
		x2 = MAX( x + coord[xIndex]*xMult, x + coord[xIndex+1]*xMult );
		y1 = y - coord[NumSegments];
		y2 = MIN( y + coord[yIndex]*yMult, y + coord[yIndex+1]*yMult );
		surface()->DrawFilledRect( x1, y1, x2, y2 );
		xIndex += xDir;
		yIndex += yDir;
	}

	// paint between top left and bottom left ---------------------------------
	x1 = 0;
	x2 = coord[NumSegments];
	y1 = coord[NumSegments];
	y2 = tall - coord[NumSegments];
	surface()->DrawFilledRect( x1, y1, x2, y2 );

	// paint between left and right -------------------------------------------
	x1 = coord[NumSegments];
	x2 = wide - coord[NumSegments];
	y1 = 0;
	y2 = tall;
	surface()->DrawFilledRect( x1, y1, x2, y2 );
	
	// paint between top right and bottom right -------------------------------
	x1 = wide - coord[NumSegments];
	x2 = wide;
	y1 = coord[NumSegments];
	y2 = tall - coord[NumSegments];
	surface()->DrawFilledRect( x1, y1, x2, y2 );


	surface()->DrawTexturedRect( 10, 10, wide-10, tall-10 );

	surface()->DrawSetTexture( m_nGrayDot );
	for (int i = 0; i < mapspots.Size(); i++)
	{
		if (HL2MPRules()->cap_point_state[i] == COVEN_TEAMID_SLAYERS)
			surface()->DrawSetTexture( m_nBlueDot );
		else if (HL2MPRules()->cap_point_state[i] == COVEN_TEAMID_VAMPIRES)
			surface()->DrawSetTexture( m_nRedDot );
		else
			surface()->DrawSetTexture( m_nGrayDot );
		surface()->DrawTexturedRect( mapspots[i].x, mapspots[i].y, mapspots[i].x+32, mapspots[i].y+32 );
	}

	if (usingCTS)
	{
		surface()->DrawSetTexture( m_nGoldStar );
		if (HL2MPRules()->cts_net.Get() == NULL)
		{
			surface()->DrawTexturedRect(cts_zone.x, cts_zone.y, cts_zone.x+32, cts_zone.y+32 );
		}
		else if ((HL2MPRules()->cts_net.Get()->GetLocalOrigin()-cts_origin).Length() < 200)
		{
			surface()->DrawTexturedRect(cts.x, cts.y, cts.x+32, cts.y+32 );
		}
		else
		{
			//CTS IS LOST SOMEWHERE
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Paint border for rounded corners
//-----------------------------------------------------------------------------
void CHL2MPClientMiniMapDialog::PaintBorder()
{
	int x1, x2, y1, y2;
	surface()->DrawSetColor(m_borderColor);
	surface()->DrawSetTextColor(m_borderColor);

	int wide, tall;
	GetSize( wide, tall );

	int i;

	// top-left corner --------------------------------------------------------
	int xDir = 1;
	int yDir = -1;
	int xIndex = 0;
	int yIndex = NumSegments - 1;
	int xMult = 1;
	int yMult = 1;
	int x = 0;
	int y = 0;
	for ( i=0; i<NumSegments; ++i )
	{
		x1 = MIN( x + coord[xIndex]*xMult, x + coord[xIndex+1]*xMult );
		x2 = MAX( x + coord[xIndex]*xMult, x + coord[xIndex+1]*xMult );
		y1 = MIN( y + coord[yIndex]*yMult, y + coord[yIndex+1]*yMult );
		y2 = MAX( y + coord[yIndex]*yMult, y + coord[yIndex+1]*yMult );
		surface()->DrawFilledRect( x1, y1, x2, y2 );

		xIndex += xDir;
		yIndex += yDir;
	}

	// top-right corner -------------------------------------------------------
	xDir = 1;
	yDir = -1;
	xIndex = 0;
	yIndex = NumSegments - 1;
	x = wide;
	y = 0;
	xMult = -1;
	yMult = 1;
	for ( i=0; i<NumSegments; ++i )
	{
		x1 = MIN( x + coord[xIndex]*xMult, x + coord[xIndex+1]*xMult );
		x2 = MAX( x + coord[xIndex]*xMult, x + coord[xIndex+1]*xMult );
		y1 = MIN( y + coord[yIndex]*yMult, y + coord[yIndex+1]*yMult );
		y2 = MAX( y + coord[yIndex]*yMult, y + coord[yIndex+1]*yMult );
		surface()->DrawFilledRect( x1, y1, x2, y2 );
		xIndex += xDir;
		yIndex += yDir;
	}

	// bottom-right corner ----------------------------------------------------
	xDir = 1;
	yDir = -1;
	xIndex = 0;
	yIndex = NumSegments - 1;
	x = wide;
	y = tall;
	xMult = -1;
	yMult = -1;
	for ( i=0; i<NumSegments; ++i )
	{
		x1 = MIN( x + coord[xIndex]*xMult, x + coord[xIndex+1]*xMult );
		x2 = MAX( x + coord[xIndex]*xMult, x + coord[xIndex+1]*xMult );
		y1 = MIN( y + coord[yIndex]*yMult, y + coord[yIndex+1]*yMult );
		y2 = MAX( y + coord[yIndex]*yMult, y + coord[yIndex+1]*yMult );
		surface()->DrawFilledRect( x1, y1, x2, y2 );
		xIndex += xDir;
		yIndex += yDir;
	}

	// bottom-left corner -----------------------------------------------------
	xDir = 1;
	yDir = -1;
	xIndex = 0;
	yIndex = NumSegments - 1;
	x = 0;
	y = tall;
	xMult = 1;
	yMult = -1;
	for ( i=0; i<NumSegments; ++i )
	{
		x1 = MIN( x + coord[xIndex]*xMult, x + coord[xIndex+1]*xMult );
		x2 = MAX( x + coord[xIndex]*xMult, x + coord[xIndex+1]*xMult );
		y1 = MIN( y + coord[yIndex]*yMult, y + coord[yIndex+1]*yMult );
		y2 = MAX( y + coord[yIndex]*yMult, y + coord[yIndex+1]*yMult );
		surface()->DrawFilledRect( x1, y1, x2, y2 );
		xIndex += xDir;
		yIndex += yDir;
	}

	// top --------------------------------------------------------------------
	x1 = coord[NumSegments];
	x2 = wide - coord[NumSegments];
	y1 = 0;
	y2 = 1;
	surface()->DrawFilledRect( x1, y1, x2, y2 );

	// bottom -----------------------------------------------------------------
	x1 = coord[NumSegments];
	x2 = wide - coord[NumSegments];
	y1 = tall - 1;
	y2 = tall;
	surface()->DrawFilledRect( x1, y1, x2, y2 );

	// left -------------------------------------------------------------------
	x1 = 0;
	x2 = 1;
	y1 = coord[NumSegments];
	y2 = tall - coord[NumSegments];
	surface()->DrawFilledRect( x1, y1, x2, y2 );

	// right ------------------------------------------------------------------
	x1 = wide - 1;
	x2 = wide;
	y1 = coord[NumSegments];
	y2 = tall - coord[NumSegments];
	surface()->DrawFilledRect( x1, y1, x2, y2 );
}

//-----------------------------------------------------------------------------
// Purpose: Apply scheme settings
//-----------------------------------------------------------------------------
void CHL2MPClientMiniMapDialog::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	m_bgColor = GetSchemeColor("SectionedListPanel.BgColor", GetBgColor(), pScheme);
	m_borderColor = pScheme->GetColor( "FgColor", Color( 0, 0, 0, 0 ) );

	SetBgColor( Color(0, 0, 0, 0) );
	SetBorder( pScheme->GetBorder( "BaseBorder" ) );
}


//-----------------------------------------------------------------------------
// Purpose: sets up base sections
//-----------------------------------------------------------------------------
void CHL2MPClientMiniMapDialog::InitScoreboardSections()
{

}

bool CHL2MPClientMiniMapDialog::LoadFromBuffer( char const *resourceName, const char *pBuffer, IBaseFileSystem* pFileSystem, const char *pPathID )
{
	if ( !pBuffer )
		return true;

	int nLen = Q_strlen( pBuffer );
	CUtlBuffer buf( pBuffer, nLen, CUtlBuffer::READ_ONLY | CUtlBuffer::TEXT_BUFFER );
	return LoadFromBuffer( resourceName, buf, pFileSystem, pPathID );
}

bool CHL2MPClientMiniMapDialog::LoadFromBuffer( char const *resourceName, CUtlBuffer &buf, IBaseFileSystem *pFileSystem, const char *pPathID )
{
	while (buf.IsValid())
	{
		buf.GetDelimitedString( GetNoEscCharConversion(), temparray, 256 );
		const char *s = temparray;
		if (!s || *s == 0 )
		{
			return false;
		}

		if (Q_strcmp(s,"capturepoints") == 0)
		{
			buf.GetDelimitedString( GetNoEscCharConversion(), temparray, 256 );
			const char *t = temparray;
			int n = 0;

			UTIL_StringToIntArray(&n, 1, t);

			for (int i = 0; i < n; i++)
			{
				int x,y;
				buf.GetDelimitedString( GetNoEscCharConversion(), temparray, 256 );
				const char *u = temparray;
				UTIL_StringToIntArray(&x, 1, u);
				buf.GetDelimitedString( GetNoEscCharConversion(), temparray, 256 );
				const char *v = temparray;
				UTIL_StringToIntArray(&y, 1, v);
				Vector temp(x,y,0);
				mapspots.AddToTail(temp);
			}
		}
		else if (Q_strcmp(s,"cts") == 0)
		{
			usingCTS = true;
			int x,y;
			buf.GetDelimitedString( GetNoEscCharConversion(), temparray, 256 );
			const char *u = temparray;
			UTIL_StringToIntArray(&x, 1, u);
			buf.GetDelimitedString( GetNoEscCharConversion(), temparray, 256 );
			const char *v = temparray;
			UTIL_StringToIntArray(&y, 1, v);
			cts = Vector(x,y,0);
			buf.GetDelimitedString( GetNoEscCharConversion(), temparray, 256 );
			const char *t = temparray;
			float locs[3];
			UTIL_StringToVector(locs, t);
			cts_origin = Vector(locs[0], locs[1], locs[2]);
		}
		else if (Q_strcmp(s,"ctszone") == 0)
		{
			int x,y;
			buf.GetDelimitedString( GetNoEscCharConversion(), temparray, 256 );
			const char *u = temparray;
			UTIL_StringToIntArray(&x, 1, u);
			buf.GetDelimitedString( GetNoEscCharConversion(), temparray, 256 );
			const char *v = temparray;
			UTIL_StringToIntArray(&y, 1, v);
			cts_zone = Vector(x,y,0);
		}
	}
	return true;
}

bool CHL2MPClientMiniMapDialog::LoadDotFile( IBaseFileSystem *filesystem, const char *resourceName, const char *pathID )
{
	FileHandle_t f = filesystem->Open(resourceName, "rb", pathID);
	if (!f)
		return false;

	// load file into a null-terminated buffer
	int fileSize = filesystem->Size(f);
	unsigned bufSize = ((IFileSystem *)filesystem)->GetOptimalReadSize( f, fileSize + 1 );

	char *buffer = (char*)((IFileSystem *)filesystem)->AllocOptimalReadBuffer( f, bufSize );
	
	Assert(buffer);
	
	((IFileSystem *)filesystem)->ReadEx(buffer, bufSize, fileSize, f); // read into local buffer

	buffer[fileSize] = 0; // null terminate file as EOF

	filesystem->Close( f );	// close file after reading

	bool retOK = LoadFromBuffer( resourceName, buffer, filesystem );

	((IFileSystem *)filesystem)->FreeOptimalReadBuffer( buffer );

	return retOK;
}

void CHL2MPClientMiniMapDialog::FireGameEvent( IGameEvent *event )
{
	const char * type = event->GetName();

	if ( Q_strcmp(type, "game_newmap") == 0 )
	{
		usingCTS = false;
		mapspots.RemoveAll();
		Q_snprintf( mapname, sizeof( mapname ), "maps/%s", event->GetString("mapname") );

		char tempfile[MAX_PATH];
		Q_snprintf( tempfile, sizeof( tempfile ), "maps/%s_hud.txt", event->GetString("mapname") );
		LoadDotFile(filesystem, tempfile, "GAME");

		m_nMapTex = surface()->CreateNewTextureID();
		surface()->DrawSetTextureFile( m_nMapTex, mapname, true, true);
	}

	if( IsVisible() )
		Update();

}

void CHL2MPClientMiniMapDialog::Reset()
{
	m_fNextUpdateTime = 0;
	// add all the sections
	InitScoreboardSections();
}

//-----------------------------------------------------------------------------
// Purpose: resets the scoreboard team info
//-----------------------------------------------------------------------------
void CHL2MPClientMiniMapDialog::UpdateTeamInfo()
{

}

//-----------------------------------------------------------------------------
// Purpose: adds the top header of the scoreboars
//-----------------------------------------------------------------------------
void CHL2MPClientMiniMapDialog::AddHeader()
{
}

//-----------------------------------------------------------------------------
// Purpose: Adds a new section to the scoreboard (i.e the team header)
//-----------------------------------------------------------------------------
void CHL2MPClientMiniMapDialog::AddSection(int teamType, int teamNumber)
{

}

int CHL2MPClientMiniMapDialog::GetSectionFromTeamNumber( int teamNumber )
{
	return SCORESECTION_FREEFORALL;
}

//-----------------------------------------------------------------------------
// Purpose: Adds a new row to the scoreboard, from the playerinfo structure
//-----------------------------------------------------------------------------
bool CHL2MPClientMiniMapDialog::GetPlayerScoreInfo(int playerIndex, KeyValues *kv)
{
	return true;
}



//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHL2MPClientMiniMapDialog::UpdatePlayerInfo()
{
	
}
