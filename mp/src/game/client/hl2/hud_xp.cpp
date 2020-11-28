//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "hud.h"
#include "hud_xp.h"
#include "hud_macros.h"
#include "c_basehlplayer.h"
#include "iclientmode.h"
#include <vgui_controls/AnimationController.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>

using namespace vgui;

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DECLARE_HUDELEMENT( CHudXP );

#define XP_INIT -1

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudXP::CHudXP( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudXP" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT );

	m_nGlassTex[0] = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile( m_nGlassTex[0], "hud/bars/s_glass_empty", true, true);

	m_nGlassTex[1] = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile( m_nGlassTex[1], "hud/bars/v_glass_empty", true, true);

	m_nBlipTex = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile( m_nBlipTex, "hud/bars/glass_x", true, true);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudXP::Init( void )
{
	m_XP = XP_INIT;
	SetBgColor(Color(0,0,0,250));
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudXP::Reset( void )
{
	Init();
}

//-----------------------------------------------------------------------------
// Purpose: Save CPU cycles by letting the HUD system early cull
// costly traversal.  Called per frame, return true if thinking and 
// painting need to occur.
//-----------------------------------------------------------------------------
bool CHudXP::ShouldDraw()
{
	bool bNeedsDraw = false;

	C_BaseHLPlayer *pPlayer = (C_BaseHLPlayer *)C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return false;

	// needs draw if suit power changed or animation in progress
	bNeedsDraw = ( ( pPlayer->m_HL2Local.covenXPCounter != m_XP ) || ( m_AuxPowerColor[3] > 0 ) );

	return ( bNeedsDraw && CHudElement::ShouldDraw() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudXP::OnThink( void )
{
	C_BaseHLPlayer *pPlayer = (C_BaseHLPlayer *)C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	// Only update if we've changed suit power
	if (pPlayer->m_HL2Local.covenXPCounter == m_XP)
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


	m_XP = pPlayer->m_HL2Local.covenXPCounter;
}

void CHudXP::PaintBackground()
{
}

//-----------------------------------------------------------------------------
// Purpose: draws the power bar
//-----------------------------------------------------------------------------
void CHudXP::Paint()
{
	C_BaseHLPlayer *pPlayer = (C_BaseHLPlayer *)C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	int wide, tall;
	GetSize(wide, tall);

	int max = (COVEN_MAX_XP_PER_LEVEL+(pPlayer->covenLevelCounter-1)*COVEN_XP_INCREASE_PER_LEVEL);

	float maxbar = wide - m_flBarInsetX;

	float perc = ((float)m_XP)/((float)max)*maxbar;
	if (perc > maxbar)
		perc = maxbar;

	int team = 0;
	if (pPlayer->GetTeamNumber() == COVEN_TEAMID_VAMPIRES)
		team = 1;

	surface()->DrawSetColor( m_AuxPowerColor );
	surface()->DrawSetTexture(m_nGlassTex[team]);
	surface()->DrawTexturedRect(0,0, wide, tall);
	surface()->DrawSetTexture(m_nBlipTex);
	surface()->DrawTexturedRect(m_flBarInsetX, 0, perc, tall);

	/*// get bar chunks
	int chunkCount = m_flBarWidth / (m_flBarChunkWidth + m_flBarChunkGap);
	int enabledChunks = (int)((float)chunkCount * (m_XP * 1.0f/(COVEN_MAX_XP_PER_LEVEL+pPlayer->covenLevelCounter*COVEN_XP_INCREASE_PER_LEVEL)) + 0.5f );

	// draw the suit power bar
	surface()->DrawSetColor( m_AuxPowerColor );
	int xpos = m_flBarInsetX, ypos = m_flBarInsetY;
	for (int i = 0; i < enabledChunks; i++)
	{
		surface()->DrawFilledRect( xpos, ypos, xpos + m_flBarChunkWidth, ypos + m_flBarHeight );
		xpos += (m_flBarChunkWidth + m_flBarChunkGap);
	}
	// draw the exhausted portion of the bar.
	surface()->DrawSetColor( Color( m_AuxPowerColor[0], m_AuxPowerColor[1], m_AuxPowerColor[2], m_iAuxPowerDisabledAlpha ) );
	for (int i = enabledChunks; i < chunkCount; i++)
	{
		surface()->DrawFilledRect( xpos, ypos, xpos + m_flBarChunkWidth, ypos + m_flBarHeight );
		xpos += (m_flBarChunkWidth + m_flBarChunkGap);
	}

	wchar_t szText[ 63 ];
	//wchar_t *tempString = L"Level";
	V_swprintf_safe(szText, L"Level %d", pPlayer->covenLevelCounter);
	
	*/
	// draw our name
	/*surface()->DrawSetTextFont(m_hTextFont);
	surface()->DrawSetTextColor(m_AuxPowerColor);
	surface()->DrawSetTextPos(text_xpos, text_ypos);

	surface()->DrawPrintText(szText, wcslen(szText));*/
	//surface()->DrawPrintText(L"Level", wcslen(L"Level"));

	surface()->DrawSetTextFont(m_hTextFont);
	//draw our value
	wchar_t szText[ 32 ];

	if (pPlayer->GetTeamNumber() == COVEN_TEAMID_SLAYERS)
		V_swprintf_safe(szText, L"£ %d / %d", m_XP, max);
	else
		V_swprintf_safe(szText, L"%d / %d", m_XP, max);

	int tx = wide/2-UTIL_ComputeStringWidth(m_hTextFont,szText)/2;
	int ty = tall/2-surface()->GetFontTall(m_hTextFont)/2;
	surface()->DrawSetTextPos(tx-2, ty+2);
	surface()->DrawSetTextColor(m_AuxPowerColorShadow);
	surface()->DrawPrintText(szText, wcslen(szText));
	surface()->DrawSetTextPos(tx+2, ty-2);
	//surface()->DrawSetTextColor(Color(150, 150, 150, 250));
	surface()->DrawPrintText(szText, wcslen(szText));
	surface()->DrawSetTextPos(tx, ty);
	surface()->DrawSetTextColor(m_AuxPowerColor);
	surface()->DrawPrintText(szText, wcslen(szText));
}


