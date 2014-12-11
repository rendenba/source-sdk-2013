//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "hud.h"
#include "hud_teamscore.h"
#include "hud_macros.h"
#include "c_basehlplayer.h"
#include "iclientmode.h"
#include <vgui_controls/AnimationController.h>
#include <vgui/ISurface.h>
#include <igameresources.h>
#include <vgui/ILocalize.h>
#include "c_team.h"
#include "hl2mp_gamerules.h"

using namespace vgui;

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DECLARE_HUDELEMENT( CHudTeamScore );

#define TEAMSCORE_INIT -1

extern ConVar fraglimit;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudTeamScore::CHudTeamScore( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudTeamScore" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT );

	m_nGrayDot = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile( m_nGrayDot, "effects/graydot", true, true);

	m_nRedDot = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile( m_nRedDot, "effects/reddot", true, true);

	m_nBlueDot = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile( m_nBlueDot, "effects/bluedot", true, true);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudTeamScore::Init( void )
{
	m_TeamSScore = TEAMSCORE_INIT;
	m_TeamVScore = TEAMSCORE_INIT;
	SetBgColor(Color(0,0,0,250));
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudTeamScore::Reset( void )
{
	Init();
}

//-----------------------------------------------------------------------------
// Purpose: Save CPU cycles by letting the HUD system early cull
// costly traversal.  Called per frame, return true if thinking and 
// painting need to occur.
//-----------------------------------------------------------------------------
bool CHudTeamScore::ShouldDraw()
{
	bool bNeedsDraw = false;

	C_BaseHLPlayer *pPlayer = (C_BaseHLPlayer *)C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return false;

	C_Team *team = GetGlobalTeam( COVEN_TEAMID_SLAYERS );
	int sscore = team->Get_Score();
	team = GetGlobalTeam( COVEN_TEAMID_VAMPIRES );
	int vscore = team->Get_Score();

	// needs draw if suit power changed or animation in progress
	bNeedsDraw = ( ( sscore != m_TeamSScore ) || ( vscore != m_TeamVScore ) || ( m_AuxPowerColor[3] > 0 ));

	return ( bNeedsDraw && CHudElement::ShouldDraw() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudTeamScore::OnThink( void )
{
	C_BaseHLPlayer *pPlayer = (C_BaseHLPlayer *)C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	C_Team *team = GetGlobalTeam( COVEN_TEAMID_SLAYERS );
	int sscore = team->Get_Score();
	team = GetGlobalTeam( COVEN_TEAMID_VAMPIRES );
	int vscore = team->Get_Score();

	// Only update if we've changed suit power
	if ( ( sscore == m_TeamSScore ) && ( vscore == m_TeamVScore ) )
		return;

	/*if ( currentXP >= 100.0f && m_XP < 100.0f )
	{
		// we've reached max power
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("SuitAuxPowerMax");
	}
	else if ( flCurrentPower < 100.0f && (m_flSuitPower >= 100.0f || m_flSuitPower == SUITPOWER_INIT) )
	{
		// we've lost power
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("SuitAuxPowerNotMax");
	}*/


	m_TeamSScore = sscore;
	m_TeamVScore = vscore;
}

//-----------------------------------------------------------------------------
// Purpose: draws the power bar
//-----------------------------------------------------------------------------
void CHudTeamScore::Paint()
{
	C_BaseHLPlayer *pPlayer = (C_BaseHLPlayer *)C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;
	wchar_t szText[ 63 ];
	int w,t;
	GetSize(w,t);
	Color c;
	c = GameResources()->GetTeamColor( COVEN_TEAMID_SLAYERS );
	// get bar chunks
	int chunkCount = m_flBarWidth / (m_flBarChunkWidth + m_flBarChunkGap) / 2;
	int enabledChunks = (int)((float)chunkCount * (m_TeamSScore * 1.0f/(fraglimit.GetFloat())) + 0.5f );

	// draw the suit power bar
	surface()->DrawSetColor( c );
	int xpos = w/2 + m_flBarChunkGap;
	int ypos = m_flBarInsetY;
	for (int i = 0; i < enabledChunks; i++)
	{
		surface()->DrawFilledRect( xpos, ypos, xpos + m_flBarChunkWidth, ypos + m_flBarHeight );
		xpos += (m_flBarChunkWidth + m_flBarChunkGap);
	}
	// draw the exhausted portion of the bar.
	surface()->DrawSetColor( Color( c[0], c[1], c[2], m_iAuxPowerDisabledAlpha ) );
	for (int i = enabledChunks; i < chunkCount; i++)
	{
		surface()->DrawFilledRect( xpos, ypos, xpos + m_flBarChunkWidth, ypos + m_flBarHeight );
		xpos += (m_flBarChunkWidth + m_flBarChunkGap);
	}

	V_swprintf_safe(szText, L"%d", m_TeamSScore);

	int charWidth = surface()->GetCharacterWidth(m_hTextFont, '0');
	// draw our name
	surface()->DrawSetTextFont(m_hTextFont);
	surface()->DrawSetTextColor(c);
	int xpos_mod = text2_xpos;
	if (m_TeamSScore > 9)
		xpos_mod -= charWidth;
	if (m_TeamSScore > 99)
		xpos_mod -= charWidth;
	if (m_TeamSScore > 999)
		xpos_mod -= charWidth;
	surface()->DrawSetTextPos(xpos_mod, text2_ypos);
	surface()->DrawPrintText(szText, wcslen(szText));

	c = GameResources()->GetTeamColor( COVEN_TEAMID_VAMPIRES );
	// get bar chunks
	enabledChunks = (int)((float)chunkCount * (m_TeamVScore * 1.0f/(fraglimit.GetFloat())) + 0.5f );

	// draw the suit power bar
	surface()->DrawSetColor( c );
	xpos = w/2 - m_flBarChunkGap;
	ypos = m_flBarInsetY;
	for (int i = 0; i < enabledChunks; i++)
	{
		surface()->DrawFilledRect( xpos - m_flBarChunkWidth, ypos, xpos, ypos + m_flBarHeight );
		xpos -= (m_flBarChunkWidth + m_flBarChunkGap);
	}
	// draw the exhausted portion of the bar.
	surface()->DrawSetColor( Color( c[0], c[1], c[2], m_iAuxPowerDisabledAlpha ) );
	for (int i = enabledChunks; i < chunkCount; i++)
	{
		surface()->DrawFilledRect( xpos - m_flBarChunkWidth, ypos, xpos, ypos + m_flBarHeight );
		xpos -= (m_flBarChunkWidth + m_flBarChunkGap);
	}

	V_swprintf_safe(szText, L"%d", m_TeamVScore);

	// draw our name
	surface()->DrawSetTextFont(m_hTextFont);
	surface()->DrawSetTextColor(c);
	surface()->DrawSetTextPos(text_xpos, text_ypos);

	surface()->DrawPrintText(szText, wcslen(szText));
	//surface()->DrawPrintText(L"Level", wcslen(L"Level"));


	xpos = (w-m_flBarChunkWidth*HL2MPRules()->num_cap_points-m_flBarChunkGap*(max(HL2MPRules()->num_cap_points,2)-1))/2;
	ypos = m_flBarInsetY+m_flBarHeight*2;
	for (int i = 0; i < HL2MPRules()->num_cap_points; i++)
	{
		if (HL2MPRules()->cap_point_state[i] == COVEN_TEAMID_SLAYERS)
			surface()->DrawSetTexture( m_nBlueDot );
		else if (HL2MPRules()->cap_point_state[i] == COVEN_TEAMID_VAMPIRES)
			surface()->DrawSetTexture( m_nRedDot );
		else
			surface()->DrawSetTexture( m_nGrayDot );

		surface()->DrawTexturedRect(xpos, ypos, xpos+m_flBarHeight, ypos+m_flBarHeight );
		xpos += (m_flBarChunkWidth+m_flBarChunkGap);
	}
}


