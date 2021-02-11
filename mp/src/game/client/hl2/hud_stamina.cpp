#include "cbase.h"
#include "hud.h"
#include "hud_stamina.h"
#include "hud_macros.h"
#include "c_basehlplayer.h"
#include "iclientmode.h"
#include <vgui_controls/AnimationController.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>

using namespace vgui;

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DECLARE_HUDELEMENT(CHudStamina);

#define SUITSTAMINA_INIT -1

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudStamina::CHudStamina(const char *pElementName) : CHudElement(pElementName), BaseClass(NULL, "HudStamina")
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent(pParent);
	SetPaintBackgroundEnabled(false);

	SetHiddenBits(HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudStamina::Init(void)
{
	m_flStamina = SUITSTAMINA_INIT;
	m_flStaminaLow = -1;
	m_flTimer = gpGlobals->curtime + 1.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudStamina::Reset(void)
{
	Init();
}

//-----------------------------------------------------------------------------
// Purpose: Save CPU cycles by letting the HUD system early cull
// costly traversal.  Called per frame, return true if thinking and 
// painting need to occur.
//-----------------------------------------------------------------------------
bool CHudStamina::ShouldDraw()
{
	bool bNeedsDraw = false;

	C_BaseHLPlayer *pPlayer = (C_BaseHLPlayer *)C_BasePlayer::GetLocalPlayer();
	if (!pPlayer)
		return false;

	if (pPlayer->GetTeamNumber() == COVEN_TEAMID_VAMPIRES)
		return false;

	// needs draw if suit power changed or animation in progress
	bNeedsDraw = ((pPlayer->m_Local.m_flStamina != m_flStamina) || (m_AuxPowerColor[3] > 0));

	return (bNeedsDraw && CHudElement::ShouldDraw());
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudStamina::OnThink(void)
{
	float flCurrentStamina = 0;
	C_BaseHLPlayer *pPlayer = (C_BaseHLPlayer *)C_BasePlayer::GetLocalPlayer();
	if (!pPlayer)
		return;

	flCurrentStamina = pPlayer->m_Local.m_flStamina;

	if (m_flTimer > 0.0f && gpGlobals->curtime > m_flTimer)
	{
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("SuitAuxPowerMax");
		m_flTimer = 0.0f;
	}

	// Only update if we've changed suit power
	if (flCurrentStamina == m_flStamina)
		return;

	if (flCurrentStamina >= MAX_STAMINA && m_flStamina < MAX_STAMINA)
	{
		// we've reached max power
		m_flTimer = gpGlobals->curtime + m_flHoldTime;
	}
	else if (flCurrentStamina < MAX_STAMINA && (m_flStamina >= MAX_STAMINA || m_flStamina == SUITSTAMINA_INIT))
	{
		// we've lost power
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("SuitAuxPowerNotMax");
		m_flTimer = 0.0f;
	}

	

	m_flStamina = flCurrentStamina;
}

//-----------------------------------------------------------------------------
// Purpose: draws the power bar
//-----------------------------------------------------------------------------
void CHudStamina::Paint()
{
	C_BaseHLPlayer *pPlayer = (C_BaseHLPlayer *)C_BasePlayer::GetLocalPlayer();
	if (!pPlayer)
		return;

	// get bar chunks
	int chunkCount = m_flBarWidth / (m_flBarChunkWidth + m_flBarChunkGap);
	int enabledChunks = (int)((float)chunkCount * (m_flStamina * 1.0f / 100.0f) + 0.5f);
	if (enabledChunks % 2 != chunkCount % 2)
		enabledChunks--;
	int disabledChunks = (chunkCount - enabledChunks) / 2.0f;

	// see if we've changed power state
	int lowPower = 0;
	if (m_flStamina < MIN_STAMINA)
	{
		lowPower = 2;
	}
	else if (m_flStamina < DODGE_COST)
	{
		lowPower = 1;
	}
	if (m_flStaminaLow != lowPower)
	{
		if (m_flStamina < MAX_STAMINA)
		{
			if (lowPower == 2)
			{
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("SuitAuxPowerDecreasedBelow25");
			}
			else if (lowPower == 1)
			{
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("SuitAuxPowerDecreasedBelow75");
			}
			else
			{
				g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("SuitAuxPowerIncreasedAbove75");
			}
			m_flStaminaLow = lowPower;
		}
	}

	int xpos = m_flBarInsetX, ypos = m_flBarInsetY;
	// draw the exhausted portion of the bar.
	surface()->DrawSetColor(m_AuxOffColor);
	for (int i = 0; i < disabledChunks; i++)
	{
		surface()->DrawFilledRect(xpos, ypos, xpos + m_flBarChunkWidth, ypos + m_flBarHeight);
		xpos += (m_flBarChunkWidth + m_flBarChunkGap);
	}
	// draw the suit power bar
	surface()->DrawSetColor(m_AuxPowerColor);
	for (int i = 0; i < enabledChunks; i++)
	{
		surface()->DrawFilledRect(xpos, ypos, xpos + m_flBarChunkWidth, ypos + m_flBarHeight);
		xpos += (m_flBarChunkWidth + m_flBarChunkGap);
	}
	// draw the exhausted portion of the bar.
	surface()->DrawSetColor(m_AuxOffColor);
	for (int i = 0; i < disabledChunks; i++)
	{
		surface()->DrawFilledRect(xpos, ypos, xpos + m_flBarChunkWidth, ypos + m_flBarHeight);
		xpos += (m_flBarChunkWidth + m_flBarChunkGap);
	}

	// draw our name
	/*surface()->DrawSetTextFont(m_hTextFont);
	surface()->DrawSetTextColor(m_AuxPowerColor);
	surface()->DrawSetTextPos(text_xpos, text_ypos);

	wchar_t *tempString = g_pVGuiLocalize->Find("#Valve_Hud_AUX_POWER");

	if (tempString)
	{
		surface()->DrawPrintText(tempString, wcslen(tempString));
	}
	else
	{
		surface()->DrawPrintText(L"AUX POWER", wcslen(L"AUX POWER"));
	}*/

}