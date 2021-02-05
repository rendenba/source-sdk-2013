#ifndef HUD_USETIMER_H
#define HUD_USETIMER_H
#ifdef _WIN32
#pragma once
#endif

#include "hudelement.h"
#include <vgui_controls/Panel.h>

namespace vgui
{
	class IScheme;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudUseTimer : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE(CHudUseTimer, vgui::Panel);
public:
	CHudUseTimer(const char *pElementName);
	virtual ~CHudUseTimer();

	virtual void	OnThink();
	virtual bool	ShouldDraw();

protected:
	virtual void	Paint();
	virtual void	PaintBackground();
	void			DrawCircleSegment(int x, int y, int wide, int tall, float flEndDegrees, bool clockwise /* = true */);
	void			DrawTextTitle(int x, int y, int wide, int tall, wchar_t *title);

	int		m_iEmptyTex;
	int		m_iFullTex;
	float	m_flMaxTime;
	float	m_flMaxTimer;
	wchar_t	m_wszItemName[MAX_PLAYER_NAME_LENGTH];

	CPanelAnimationVar(Color, m_Color, "Color", "255 255 255 255");
	CPanelAnimationVar(Color, m_ColorShadow, "ColorShadow", "0 0 0 200");//150
	CPanelAnimationVar(vgui::HFont, m_hTextFont, "TextFont", "CovenHUDBoldXL");
};

#endif // HUD_USETIMER_H
