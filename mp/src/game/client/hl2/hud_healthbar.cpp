//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "hud.h"
#include "hud_healthbar.h"
#include "hud_macros.h"
#include "c_basehlplayer.h"
#include "iclientmode.h"
#include <vgui_controls/AnimationController.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>

using namespace vgui;

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DECLARE_HUDELEMENT( CHudHealthBar );

#define HPBAR_INIT -1

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudHealthBar::CHudHealthBar( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudHPBar" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT );

	m_nGlassTex = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile( m_nGlassTex, "hud/bars/glass_empty_big", true, true);

	m_nBlipTex = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile( m_nBlipTex, "hud/bars/glass_h", true, true);

	m_nBlipRight = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile( m_nBlipRight, "hud/bars/glass_h_r", true, true);

	m_nBlipLeft = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile( m_nBlipLeft, "hud/bars/glass_h_l", true, true);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudHealthBar::Init( void )
{
	m_iHealth = HPBAR_INIT;
	m_iMaxHealth = HPBAR_INIT;
	SetBgColor(Color(0,0,0,250));
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudHealthBar::Reset( void )
{
	Init();
}

//-----------------------------------------------------------------------------
// Purpose: Save CPU cycles by letting the HUD system early cull
// costly traversal.  Called per frame, return true if thinking and 
// painting need to occur.
//-----------------------------------------------------------------------------
bool CHudHealthBar::ShouldDraw()
{
	bool bNeedsDraw = false;

	C_BaseHLPlayer *pPlayer = (C_BaseHLPlayer *)C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return false;

	// needs draw if suit power changed or animation in progress
	bNeedsDraw = ( ( pPlayer->GetHealth() != m_iHealth ) || ( m_AuxPowerColor[3] > 0 ) );

	return ( bNeedsDraw && CHudElement::ShouldDraw() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudHealthBar::OnThink( void )
{
	int newHealth = 0;
	C_BaseHLPlayer *pPlayer = (C_BaseHLPlayer *)C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

		// Never below zero
		newHealth = MAX( pPlayer->GetHealth(), 0 );

	// Only update if we've changed suit power
	if ( newHealth == m_iHealth )
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


	m_iHealth = newHealth;
	m_iMaxHealth = MAX(pPlayer->GetMaxHealth(), 0);
}

void CHudHealthBar::PaintBackground()
{
}

//-----------------------------------------------------------------------------
// Purpose: draws the power bar
//-----------------------------------------------------------------------------
void CHudHealthBar::Paint()
{
	C_BaseHLPlayer *pPlayer = (C_BaseHLPlayer *)C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	int wide, tall;
	GetSize(wide, tall);

	float maxbar = wide - 2.0f*m_flBarInsetX;
	float mid = maxbar/2.0f;
	float midadj = mid + wide*0.05f;
	mid += m_flBarInsetX;
	float ratio = ((float)m_iHealth)/((float)m_iMaxHealth);
	float inset = (1.0f-ratio)*midadj;
	float start = m_flBarInsetX+inset;

	surface()->DrawSetColor( m_AuxPowerColor );
	surface()->DrawSetTexture(m_nGlassTex);
	surface()->DrawTexturedRect(0,0, wide, tall);
	if (inset > 0.0f)
	{
		float tstart = start;
		if (start > mid)
			tstart = mid;
		if (inset > 16.0f)
			inset = 16.0f;
		surface()->DrawSetTexture(m_nBlipLeft);
		surface()->DrawTexturedRect(start-inset, 0, tstart, tall);
	}
	float end = mid+mid-start;
	surface()->DrawSetTexture(m_nBlipTex);
	surface()->DrawTexturedRect(start, 0, end, tall);
	if (inset > 0.0f)
	{
		float tend = end;
		if (start > mid)
			tend = mid;
		surface()->DrawSetTexture(m_nBlipRight);
		surface()->DrawTexturedRect(tend, 0, end+inset, tall);
	}

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
	V_swprintf_safe(szText, L"%d / %d", m_iHealth, m_iMaxHealth);
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


