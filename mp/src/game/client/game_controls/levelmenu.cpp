//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//


#include "cbase.h"
#include <stdio.h>

#include <cdll_client_int.h>

#include "levelmenu.h"

#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <KeyValues.h>
#include <vgui_controls/ImageList.h>
#include <filesystem.h>

#include <vgui_controls/TextEntry.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/Panel.h>


#include "c_basehlplayer.h"


#include "cdll_util.h"
#include "IGameUIFuncs.h" // for key bindings
#ifndef _XBOX
extern IGameUIFuncs *gameuifuncs; // for key binding details
#endif
#include <game/client/iviewport.h>

#include <stdlib.h> // MAX_PATH define

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

#ifdef TF_CLIENT_DLL
#define HUD_CLASSAUTOKILL_FLAGS		( FCVAR_CLIENTDLL | FCVAR_ARCHIVE | FCVAR_USERINFO )
#else
#define HUD_CLASSAUTOKILL_FLAGS		( FCVAR_CLIENTDLL | FCVAR_ARCHIVE )
#endif // !TF_CLIENT_DLL

static char *abilities[2][COVEN_MAX_CLASSCOUNT][4] =
{{{"Battle Yell","Bandage","Revenge",""},{"Sprint","Sheer Will","","Gut Check"},{"Holy Water","Trip Mine","Reflexes",""}},
{{"Leap","","Sneak","Berserk"},{"Phase","","Gorge",""},{"Dread Scream","Bloodlust","Masochist","Undying"}}};

ConVar hideautolevel("hideautolevel", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE);

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CLevelMenu::CLevelMenu(IViewPort *pViewPort) : Frame(NULL, PANEL_LEVEL)
{
	m_pViewPort = pViewPort;
	m_iScoreBoardKey = BUTTON_CODE_INVALID; // this is looked up in Activate()
	m_iTeam = 0;

	// initialize dialog
	SetTitle("", true);
	m_bAuto = false;
	m_iLevel = 0;
	m_iLoad1 = 0;	
	m_iLoad2 = 0;	
	m_iLoad3 = 0;	
	m_iLoad4 = 0;	

	m_iClass = 0;

	// load the new scheme early!!
	SetScheme("ClientScheme");
	SetMoveable(false);
	SetSizeable(false);

	// hide the system buttons
	SetTitleBarVisible( false );
	SetProportional(true);

	// info window about this class
	LoadControlSettings( "Resource/UI/LevelMenu.res" );
	//m_pPanel = new RichText( this, "SClassInfo" );
	m_pPanel = dynamic_cast<RichText *>(FindChildByName("LClassInfo"));
	for (int i = 0; i < m_mouseoverButtons.Count(); i++)
	{
		m_mouseoverButtons[i]->ChangePanel(m_pPanel);
	}
	m_pPanel->SetVerticalScrollbar(false);
	
}
void CLevelMenu::PerformLayout()
{
	int w,h;
	GetHudSize(w, h);
	
	// fill the screen
	SetBounds(0,0,w,h);

	// stretch the bottom bar across the screen
	//m_pBottomBarBlank->GetPos(x,y);
	//m_pBottomBarBlank->SetSize( w, h - y );
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CLevelMenu::CLevelMenu(IViewPort *pViewPort, const char *panelName) : Frame(NULL, panelName)
{
	m_pViewPort = pViewPort;
	m_iScoreBoardKey = BUTTON_CODE_INVALID; // this is looked up in Activate()
	m_iTeam = 0;

	m_iLevel = 0;
	m_iLoad1 = 0;	
	m_iLoad2 = 0;	
	m_iLoad3 = 0;	
	m_iLoad4 = 0;

	m_iClass = 0;

	// initialize dialog
	SetTitle("", true);
	m_bAuto = false;

	// load the new scheme early!!
	SetScheme("ClientScheme");
	SetMoveable(false);
	SetSizeable(false);

	// hide the system buttons
	SetTitleBarVisible( false );
	SetProportional(true);

	// info window about this class
	//m_pPanel = new RichText( this, "SClassInfo" );
	LoadControlSettings("Resource/UI/LevelMenu.res");
	m_pPanel = dynamic_cast<RichText *>(FindChildByName("LClassInfo"));
	for (int i = 0; i < m_mouseoverButtons.Count(); i++)
	{
		m_mouseoverButtons[i]->ChangePanel(m_pPanel);
	}
	m_pPanel->SetVerticalScrollbar(false);

	// Inheriting classes are responsible for calling LoadControlSettings()!
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CLevelMenu::~CLevelMenu()
{
}

MouseOverPanelButton* CLevelMenu::CreateNewMouseOverPanelButton(RichText *panel)
{ 
	return new MouseOverPanelButton(this, "MouseOverPanelButton", panel);
}


Panel *CLevelMenu::CreateControlByName(const char *controlName)
{
	if( !Q_stricmp( "MouseOverPanelButton", controlName ) )
	{
		MouseOverPanelButton *newButton = CreateNewMouseOverPanelButton( m_pPanel );

		m_mouseoverButtons.AddToTail( newButton );
		return newButton;
	}
	else
	{
		return BaseClass::CreateControlByName( controlName );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CLevelMenu::Reset()
{
	/*for ( int i = 0 ; i < GetChildCount() ; ++i )
	{
		// Hide the subpanel for the MouseOverPanelButtons
		MouseOverPanelButton *pPanel = dynamic_cast<MouseOverPanelButton *>( GetChild( i ) );

		if ( pPanel )
		{
			pPanel->HidePage();
		}
	}*/

	// Turn the first button back on again (so we have a default description shown)
	m_pPanel->SetText("Please select an ability by choosing an option to the left or pressing the corresponding number key.");
	Assert( m_mouseoverButtons.Count() );
	/*for ( int i=0; i<m_mouseoverButtons.Count(); ++i )
	{
		if ( i == 0 )
		{
			m_mouseoverButtons[i]->ShowPage();	// Show the first page
		}
		else
		{
			m_mouseoverButtons[i]->HidePage();	// Hide the rest
		}
	}*/
}

//-----------------------------------------------------------------------------
// Purpose: Called when the user picks a class
//-----------------------------------------------------------------------------
void CLevelMenu::OnCommand( const char *command )
{
	if ( Q_stricmp( command, "vguicancel" ) )
	{
		engine->ClientCmd( const_cast<char *>( command ) );

#if !defined( CSTRIKE_DLL ) && !defined( TF_CLIENT_DLL )
		// They entered a command to change their class, kill them so they spawn with 
		// the new class right away

#endif // !CSTRIKE_DLL && !TF_CLIENT_DLL
	}

	Close();

	gViewPortInterface->ShowBackGround( false );

	BaseClass::OnCommand( command );
}

void CLevelMenu::UpdateButtons()
{
	C_BaseHLPlayer *pPlayer = (C_BaseHLPlayer *)C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;
	if (pPlayer->GetTeamNumber() < 2 || m_iClass < 1)
		return;

	char temp[64];
	Button *entry = dynamic_cast<Button *>(FindChildByName("abil1"));
	int n = 1;
	if (m_iLoad1 > 2)
	{
		n = 0;
		entry->SetEnabled(false);
	}
	else if ((m_iLoad1 > 0 && m_iLevel < 3) || (m_iLoad1 > 1 && m_iLevel < 5))
	{
		entry->SetEnabled(false);
	}
	else
		entry->SetEnabled(true);
	Q_snprintf(temp, sizeof(temp), "%s - Rank %d", abilities[pPlayer->GetTeamNumber()-2][m_iClass-1][0], m_iLoad1+n);
	PostMessage( entry, new KeyValues( "SetText", "text", temp ) );
	entry = dynamic_cast<Button *>(FindChildByName("abil2"));
	n = 1;
	if (m_iLoad2 > 2)
	{
		n = 0;
		entry->SetEnabled(false);
	}
	else if ((m_iLoad2 > 0 && m_iLevel < 3) || (m_iLoad2 > 1 && m_iLevel < 5))
	{
		entry->SetEnabled(false);
	}
	else
		entry->SetEnabled(true);
	Q_snprintf(temp, sizeof(temp), "%s - Rank %d", abilities[pPlayer->GetTeamNumber()-2][m_iClass-1][1], m_iLoad2+n);
	PostMessage( entry, new KeyValues( "SetText", "text", temp ) );
	entry = dynamic_cast<Button *>(FindChildByName("abil3"));
	n = 1;
	if (m_iLoad3 > 2)
	{
		n = 0;
		entry->SetEnabled(false);
	}
	else if ((m_iLoad3 > 0 && m_iLevel < 3) || (m_iLoad3 > 1 && m_iLevel < 5))
	{
		entry->SetEnabled(false);
	}
	else
		entry->SetEnabled(true);
	Q_snprintf(temp, sizeof(temp), "%s - Rank %d", abilities[pPlayer->GetTeamNumber()-2][m_iClass-1][2], m_iLoad3+n);
	PostMessage( entry, new KeyValues( "SetText", "text", temp ) );
	entry = dynamic_cast<Button *>(FindChildByName("abil4"));
	n = 1;
	if (m_iLoad4 > 2)
	{
		n = 0;
		entry->SetEnabled(false);
	}
	else if (m_iLevel < 6 || (m_iLoad4 > 1 && m_iLevel < 8) || (m_iLoad4 > 2 && m_iLevel < 10))
	{
		entry->SetEnabled(false);
	}
	else
		entry->SetEnabled(true);
	Q_snprintf(temp, sizeof(temp), "%s - Rank %d", abilities[pPlayer->GetTeamNumber()-2][m_iClass-1][3], m_iLoad4+n);
	PostMessage( entry, new KeyValues( "SetText", "text", temp ) );
}

//-----------------------------------------------------------------------------
// Purpose: shows the class menu
//-----------------------------------------------------------------------------
void CLevelMenu::ShowPanel(bool bShow)
{
	if (hideautolevel.GetInt() != 0 && m_bAuto)
			bShow = false;

	if (m_iLevel > COVEN_MAX_LEVEL && m_bAuto)
		bShow = false;

	if ( bShow )
	{
		Activate();
		SetMouseInputEnabled( true );

		UpdateButtons();

		// load a default class page
		/*for ( int i=0; i<m_mouseoverButtons.Count(); ++i )
		{
			if ( i == 0 )
			{
				m_mouseoverButtons[i]->ShowPage();	// Show the first page
			}
			else
			{
				m_mouseoverButtons[i]->HidePage();	// Hide the rest
			}
		}*/
		
		if ( m_iScoreBoardKey == BUTTON_CODE_INVALID ) 
		{
			m_iScoreBoardKey = gameuifuncs->GetButtonCodeForBind( "showscores" );
		}
	}
	else
	{
		SetVisible( false );
		SetMouseInputEnabled( false );
	}
	
	m_pPanel->SetText("Please select an ability by choosing an option to the left or pressing the corresponding number key.");
	m_pViewPort->ShowBackGround( bShow );
}


void CLevelMenu::SetData(KeyValues *data)
{
	m_iTeam = data->GetInt( "team" );
	m_bAuto = data->GetBool( "auto" );
	m_iLevel = data->GetInt( "level" );
	m_iLoad1 = data->GetInt( "load1" );
	m_iLoad2 = data->GetInt( "load2" );
	m_iLoad3 = data->GetInt( "load3" );
	m_iLoad4 = data->GetInt( "load4" );
	m_iClass = data->GetInt( "class" );
}

//-----------------------------------------------------------------------------
// Purpose: Sets the text of a control by name
//-----------------------------------------------------------------------------
void CLevelMenu::SetLabelText(const char *textEntryName, const char *text)
{
	Label *entry = dynamic_cast<Label *>(FindChildByName(textEntryName));
	if (entry)
	{
		entry->SetText(text);
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sets the visibility of a button by name
//-----------------------------------------------------------------------------
void CLevelMenu::SetVisibleButton(const char *textEntryName, bool state)
{
	Button *entry = dynamic_cast<Button *>(FindChildByName(textEntryName));
	if (entry)
	{
		entry->SetVisible(state);
	}
}

void CLevelMenu::OnKeyCodePressed(KeyCode code)
{
	int nDir = 0;

	switch ( code )
	{
	case KEY_XBUTTON_UP:
	case KEY_XSTICK1_UP:
	case KEY_XSTICK2_UP:
	case KEY_UP:
	case KEY_XBUTTON_LEFT:
	case KEY_XSTICK1_LEFT:
	case KEY_XSTICK2_LEFT:
	case KEY_LEFT:
		nDir = -1;
		break;

	case KEY_XBUTTON_DOWN:
	case KEY_XSTICK1_DOWN:
	case KEY_XSTICK2_DOWN:
	case KEY_DOWN:
	case KEY_XBUTTON_RIGHT:
	case KEY_XSTICK1_RIGHT:
	case KEY_XSTICK2_RIGHT:
	case KEY_RIGHT:
		nDir = 1;
		break;
	}

	if ( m_iScoreBoardKey != BUTTON_CODE_INVALID && m_iScoreBoardKey == code )
	{
		gViewPortInterface->ShowPanel( PANEL_SCOREBOARD, true );
		gViewPortInterface->PostMessageToPanel( PANEL_SCOREBOARD, new KeyValues( "PollHideCode", "code", code ) );
	}
	else if ( nDir != 0 )
	{
		CUtlSortVector< SortedPanel_t, CSortedPanelYLess > vecSortedButtons;
		VguiPanelGetSortedChildButtonList( this, (void*)&vecSortedButtons, "&", 0 );

		int nNewArmed = VguiPanelNavigateSortedChildButtonList( (void*)&vecSortedButtons, nDir );

		if ( nNewArmed != -1 )
		{
			// Handled!
			if ( nNewArmed < m_mouseoverButtons.Count() )
			{
				m_mouseoverButtons[ nNewArmed ]->OnCursorEntered();
			}
			return;
		}
	}
	else
	{
		BaseClass::OnKeyCodePressed( code );
	}
}






