#if !defined( HUD_XP_H )
#define HUD_XP_H
#ifdef _WIN32
#pragma once
#endif

#include "hudelement.h"
#include "hud_numericdisplay.h"
#include <vgui_controls/Panel.h>

//-----------------------------------------------------------------------------
// Purpose: Shows the sprint power bar
//-----------------------------------------------------------------------------
class CHudXP : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHudXP, vgui::Panel );

public:
	CHudXP( const char *pElementName );
	virtual void	Init( void );
	virtual void	Reset( void );
	virtual void	OnThink( void );
	bool			ShouldDraw( void );
	virtual void	PaintBackground( void );

protected:
	virtual void	Paint();

private:
	CPanelAnimationVar( Color, m_AuxPowerColor, "AuxPowerColor", "255 255 255 255" );
	CPanelAnimationVar( Color, m_AuxPowerColorShadow, "AuxPowerColorShadow", "0 0 0 150" );
	//CPanelAnimationVar( int, m_iAuxPowerDisabledAlpha, "AuxPowerDisabledAlpha", "70" );

	CPanelAnimationVarAliasType( float, m_flBarInsetX, "BarInsetX", "8", "proportional_float" );
	//CPanelAnimationVarAliasType( float, m_flBarInsetY, "BarInsetY", "8", "proportional_float" );
	//CPanelAnimationVarAliasType( float, m_flBarWidth, "BarWidth", "80", "proportional_float" );
	//CPanelAnimationVarAliasType( float, m_flBarHeight, "BarHeight", "10", "proportional_float" );
	//CPanelAnimationVarAliasType( float, m_flBarChunkWidth, "BarChunkWidth", "10", "proportional_float" );
	//CPanelAnimationVarAliasType( float, m_flBarChunkGap, "BarChunkGap", "2", "proportional_float" );

	CPanelAnimationVar( vgui::HFont, m_hTextFont, "TextFont", "CovenHUDBold" );
	//CPanelAnimationVarAliasType( float, text_xpos, "text_xpos", "8", "proportional_float" );
	//CPanelAnimationVarAliasType( float, text_ypos, "text_ypos", "20", "proportional_float" );
	//CPanelAnimationVarAliasType( float, text2_xpos, "text2_xpos", "8", "proportional_float" );
	//CPanelAnimationVarAliasType( float, text2_ypos, "text2_ypos", "40", "proportional_float" );
	//CPanelAnimationVarAliasType( float, text2_gap, "text2_gap", "10", "proportional_float" );

	int m_XP;
	int m_nGlassTex[2];
	int m_nBlipTex;
};	

#endif
