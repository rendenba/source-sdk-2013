#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "hud_numericdisplay.h"
#include "iclientmode.h"
#include "iclientvehicle.h"
#include <vgui_controls/AnimationController.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include "ihudlcd.h"
#include "hl2mp_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

extern ConVar sv_coven_alarm_time;

class CHudRoundTimer : public CHudNumericDisplay, public CHudElement
{
	DECLARE_CLASS_SIMPLE(CHudRoundTimer, CHudNumericDisplay);

public:
	CHudRoundTimer(const char *pElementName);

protected:
	virtual void OnThink();
	void SetTime(float flTime);
	void UpdateTimerDisplay();

private:
	float		m_flTimer;
	float		m_flTick;
	bool		m_bTicked20;
};

DECLARE_HUDELEMENT(CHudRoundTimer);

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CHudRoundTimer::CHudRoundTimer(const char *pElementName) : BaseClass(NULL, "HudRoundTimer"), CHudElement(pElementName)
{
	SetHiddenBits(HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT);
	m_flTimer = 0.0f;
	m_flTick = 0.0f;
	m_bTicked20 = false;
	m_bIsTime = true;
	m_bIndent = true;
}

void CHudRoundTimer::UpdateTimerDisplay()
{
	CHL2MPRules *pRules = HL2MPRules();

	if (!pRules || pRules->flRoundTimer < 0.0f)
	{
		SetPaintEnabled(false);
		SetPaintBackgroundEnabled(false);
		return;
	}

	SetPaintEnabled(true);
	SetPaintBackgroundEnabled(true);

	SetTime(pRules->flRoundTimer);
}

void CHudRoundTimer::OnThink()
{
	UpdateTimerDisplay();
}

void CHudRoundTimer::SetTime(float flTime)
{
	float flTimeleft = ceil(flTime - gpGlobals->curtime);
	if (flTime != m_flTimer)
	{
		m_bTicked20 = false;
		m_flTimer = flTime;
		if (flTimeleft > sv_coven_alarm_time.GetFloat())
		{
			g_pClientMode->GetViewportAnimationController()->CancelAllAnimations();
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("RoundTimerInit");
		}
	}
	else
	{
		
		if (flTimeleft <= 10.0f && flTimeleft >= 0.0f && gpGlobals->curtime > m_flTick)
		{
			m_flTick = gpGlobals->curtime + 1.0f;
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("RoundTimerPulse");
		}
		if (flTimeleft <= sv_coven_alarm_time.GetFloat() && !m_bTicked20)
		{
			m_flTick = 10.0f;
			m_bTicked20 = true;
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("RoundTimerBelow20");
		}
	}
	if (flTimeleft < 0.0f)
		flTimeleft = 0.0f;
	SetDisplayValue(flTimeleft);
}
