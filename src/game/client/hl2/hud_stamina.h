#if !defined( HUD_STAMINA_H )
#define HUD_STAMINA_H
#ifdef _WIN32
#pragma once
#endif

#include "hudelement.h"
#include "hud_numericdisplay.h"
#include <vgui_controls/Panel.h>

//-----------------------------------------------------------------------------
// Purpose: Shows the sprint power bar
//-----------------------------------------------------------------------------
class CHudStamina : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CHudStamina, vgui::Panel);

public:
	CHudStamina(const char *pElementName);
	virtual void	Init(void);
	virtual void	Reset(void);
	virtual void	OnThink(void);
	bool			ShouldDraw(void);

protected:
	virtual void	Paint();

private:
	CPanelAnimationVar(Color, m_AuxPowerColor, "AuxPowerColor", "120 255 120 220");
	CPanelAnimationVar(Color, m_AuxOffColor, "AuxPowerOffColor", "50 50 50 70");

	CPanelAnimationVarAliasType(float, m_flBarInsetX, "BarInsetX", "10", "proportional_float");
	CPanelAnimationVarAliasType(float, m_flBarInsetY, "BarInsetY", "0", "proportional_float");
	CPanelAnimationVarAliasType(float, m_flBarWidth, "BarWidth", "280", "proportional_float");
	CPanelAnimationVarAliasType(float, m_flBarHeight, "BarHeight", "3", "proportional_float");
	CPanelAnimationVarAliasType(float, m_flBarChunkWidth, "BarChunkWidth", "8", "proportional_float");
	CPanelAnimationVarAliasType(float, m_flBarChunkGap, "BarChunkGap", "3", "proportional_float");

	CPanelAnimationVar(vgui::HFont, m_hTextFont, "TextFont", "Default");
	CPanelAnimationVarAliasType(float, text_xpos, "text_xpos", "8", "proportional_float");
	CPanelAnimationVarAliasType(float, text_ypos, "text_ypos", "4", "proportional_float");

	CPanelAnimationVar(float, m_flHoldTime, "HoldTime", "3");

	float m_flTimer;
	float m_flStamina;
	int m_flStaminaLow;
};

#endif // HUD_STAMINA_H