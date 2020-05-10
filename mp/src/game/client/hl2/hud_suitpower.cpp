//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "hud.h"
#include "hud_suitpower.h"
#include "hud_macros.h"
#include "c_basehlplayer.h"
#include "iclientmode.h"
#include <vgui_controls/AnimationController.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>

using namespace vgui;

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DECLARE_HUDELEMENT( CHudSuitPower );

#define SUITPOWER_INIT -1

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudSuitPower::CHudSuitPower( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudSuitPower" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT );

	m_nGlassTex[0] = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile( m_nGlassTex[0], "hud/bars/s_glass_empty", true, true);

	m_nGlassTex[1] = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile( m_nGlassTex[1], "hud/bars/v_glass_empty", true, true);

	m_nBlipTex = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile( m_nBlipTex, "hud/bars/glass_e", true, true);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudSuitPower::Init( void )
{
	m_flSuitPower = SUITPOWER_INIT;
	//m_nSuitPowerLow = -1;
	//m_iActiveSuitDevices = 0;

	//SetSize(256, 32);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudSuitPower::Reset( void )
{
	Init();
}

//-----------------------------------------------------------------------------
// Purpose: Save CPU cycles by letting the HUD system early cull
// costly traversal.  Called per frame, return true if thinking and 
// painting need to occur.
//-----------------------------------------------------------------------------
bool CHudSuitPower::ShouldDraw()
{
	bool bNeedsDraw = false;

	C_BaseHLPlayer *pPlayer = (C_BaseHLPlayer *)C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return false;

	// needs draw if suit power changed or animation in progress
	bNeedsDraw = ( ( pPlayer->m_HL2Local.m_flSuitPower != m_flSuitPower ) || ( m_AuxPowerColor[3] > 0 ) );

	return ( bNeedsDraw && CHudElement::ShouldDraw() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudSuitPower::OnThink( void )
{
	float flCurrentPower = 0;
	C_BaseHLPlayer *pPlayer = (C_BaseHLPlayer *)C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	flCurrentPower = pPlayer->m_HL2Local.m_flSuitPower;

	//float max = pPlayer->myIntellect()*10.0f;

	// Only update if we've changed suit power
	if ( flCurrentPower == m_flSuitPower )
		return;

	/*if ( flCurrentPower >= max && m_flSuitPower < max )
	{
		// we've reached max power
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("SuitAuxPowerMax");
	}
	else if ( flCurrentPower < max && (m_flSuitPower >= max || m_flSuitPower == SUITPOWER_INIT) )
	{
		// we've lost power
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("SuitAuxPowerNotMax");
	}*/

	/*bool flashlightActive = pPlayer->IsFlashlightActive();
	bool sprintActive = pPlayer->IsSprinting();
	bool breatherActive = pPlayer->IsBreatherActive();
	int activeDevices = (int)flashlightActive + (int)sprintActive + (int)breatherActive;*/

	/*if (activeDevices != m_iActiveSuitDevices)
	{
		m_iActiveSuitDevices = activeDevices;

		switch ( m_iActiveSuitDevices )
		{
		default:
		case 3:
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("SuitAuxPowerThreeItemsActive");
			break;
		case 2:
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("SuitAuxPowerTwoItemsActive");
			break;
		case 1:
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("SuitAuxPowerOneItemActive");
			break;
		case 0:
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("SuitAuxPowerNoItemsActive");
			break;
		}
	}*/

	m_flSuitPower = flCurrentPower;
	BaseClass::OnThink();
}

void CHudSuitPower::PaintBackground()
{
}

//-----------------------------------------------------------------------------
// Purpose: draws the power bar
//-----------------------------------------------------------------------------
void CHudSuitPower::Paint()
{
	C_BaseHLPlayer *pPlayer = (C_BaseHLPlayer *)C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	float max = pPlayer->myIntellect() * COVEN_MANA_PER_INT;

	//BB: HACK builder needs a lot
	if (pPlayer->GetTeamNumber() == COVEN_TEAMID_SLAYERS && pPlayer->covenClassID == COVEN_CLASSID_HELLION)
		max = 200.0f;

	int wide, tall;
	GetSize(wide, tall);

	float maxbar = wide - m_flBarInsetX;

	float perc = m_flSuitPower/max*maxbar;
	if (perc > maxbar)
		perc = maxbar;

	int team = 0;
	if (pPlayer->GetTeamNumber() == COVEN_TEAMID_VAMPIRES)
		team = 1;

	/*
	// get bar chunks
	int chunkCount = m_flBarWidth / (m_flBarChunkWidth + m_flBarChunkGap);
	int enabledChunks = (int)((float)chunkCount * (m_flSuitPower * 1.0f/max) + 0.5f );

	// see if we've changed power state
	int lowPower = 0;
	if (enabledChunks <= (chunkCount / 4))
	{
		lowPower = 1;
	}
	if (m_nSuitPowerLow != lowPower)
	{
		if (m_iActiveSuitDevices || m_flSuitPower < max)
		{
			if (lowPower)
			{
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("SuitAuxPowerDecreasedBelow25");
			}
			else
			{
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("SuitAuxPowerIncreasedAbove25");
			}
			m_nSuitPowerLow = lowPower;
		}
	}*/

	// draw the suit power bar
	surface()->DrawSetColor( m_AuxPowerColor );
	surface()->DrawSetTexture(m_nGlassTex[team]);
	surface()->DrawTexturedRect(0,0, wide, tall);
	surface()->DrawSetTexture(m_nBlipTex);
	surface()->DrawTexturedRect(m_flBarInsetX, 0, perc, tall);

	/*int xpos = m_flBarInsetX, ypos = m_flBarInsetY;
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
	}*/

	// draw our name
	surface()->DrawSetTextFont(m_hTextFont);

	/*wchar_t *tempString = g_pVGuiLocalize->Find("#Valve_Hud_AUX_POWER");

	if (tempString)
	{
		surface()->DrawPrintText(tempString, wcslen(tempString));
	}
	else
	{
		surface()->DrawPrintText(L"MANA", wcslen(L"MANA"));
	}*/

	//draw our value
	wchar_t szText[ 32 ];
	V_swprintf_safe(szText, L"%.00f / %.00f", m_flSuitPower, max);
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
	


	/*if ( m_iActiveSuitDevices )
	{
		// draw the additional text
		int ypos = text2_ypos;

		if (pPlayer->IsBreatherActive())
		{
			tempString = g_pVGuiLocalize->Find("#Valve_Hud_OXYGEN");

			surface()->DrawSetTextPos(text2_xpos, ypos);

			if (tempString)
			{
				surface()->DrawPrintText(tempString, wcslen(tempString));
			}
			else
			{
				surface()->DrawPrintText(L"OXYGEN", wcslen(L"OXYGEN"));
			}
			ypos += text2_gap;
		}

		if (pPlayer->IsFlashlightActive())
		{
			tempString = g_pVGuiLocalize->Find("#Valve_Hud_FLASHLIGHT");

			surface()->DrawSetTextPos(text2_xpos, ypos);

			if (tempString)
			{
				surface()->DrawPrintText(tempString, wcslen(tempString));
			}
			else
			{
				surface()->DrawPrintText(L"FLASHLIGHT", wcslen(L"FLASHLIGHT"));
			}
			ypos += text2_gap;
		}

		if (pPlayer->IsSprinting())
		{
			tempString = g_pVGuiLocalize->Find("#Valve_Hud_SPRINT");

			surface()->DrawSetTextPos(text2_xpos, ypos);

			if (tempString)
			{
				surface()->DrawPrintText(tempString, wcslen(tempString));
			}
			else
			{
				surface()->DrawPrintText(L"SPRINT", wcslen(L"SPRINT"));
			}
			ypos += text2_gap;
		}
	}*/
}


