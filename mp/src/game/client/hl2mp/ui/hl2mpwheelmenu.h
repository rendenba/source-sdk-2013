#ifndef CHL2MPCLIENTWHEELMENUDIALOG_H
#define CHL2MPCLIENTWHEELMENUDIALOG_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include <game/client/iviewport.h>
#include "filesystem.h"

#define MIN_PICKUP_DIST 0.05f //percent
#define MAX_PICKUP_DIST 0.08f //percent

class CHL2MPClientWheelMenuDialog : public vgui::EditablePanel, public IViewPortPanel
{
private:
	DECLARE_CLASS_SIMPLE(CHL2MPClientWheelMenuDialog, vgui::EditablePanel);

public:
	CHL2MPClientWheelMenuDialog(IViewPort *pViewPort, const char *panelName);
	~CHL2MPClientWheelMenuDialog();

	virtual const char *GetName(void) { return "wheelmenu"; }
	virtual void SetData(KeyValues *data) {};
	virtual void Reset();
	virtual void Update();
	virtual bool NeedsUpdate(void);
	virtual bool HasInputElements(void) { return true; }
	virtual void ShowPanel(bool bShow);
	void AddItemToWheel(CovenItemID_t iItemType);
	void LoadWheelTextures(void);

	vgui::VPANEL GetVPanel(void) { return BaseClass::GetVPanel(); }
	virtual bool IsVisible() { return BaseClass::IsVisible(); }
	virtual void SetParent(vgui::VPANEL parent) { BaseClass::SetParent(parent); }

protected:
	// vgui overrides for rounded corner background
	virtual void PaintBackground();
	virtual void PaintBorder();
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	void MoveToCenterOfScreen();


	// functions to override
	virtual void OnThink();

	float m_fNextUpdateTime;

private:
	CUtlVector<CovenItemID_t> m_Items;
	const char *szPanelName;
	int m_iSelected;
	int m_iX;
	int m_iY;
	CUtlVector<int> m_iSectorTex;
	int m_iBlipTex;
	int m_iBlipTexOn;

	IViewPort	*m_pViewPort;
	ButtonCode_t m_nCloseKey;

	// rounded corners
	Color					 m_bgColor;
	Color					 m_borderColor;
	CPanelAnimationVarAliasType(int, item_height, "item_height", "40", "proportional_int");
	CPanelAnimationVarAliasType(float, mouse_distance, "mouse_distance", "140.0", "proportional_float");
	CPanelAnimationVarAliasType(float, bumper, "bumper", "12.0", "proportional_float");
	CPanelAnimationVarAliasType(int, center_x, "center_x", "50.0", "proportional_int");
	CPanelAnimationVarAliasType(int, center_y, "center_y", "50.0", "proportional_int");
	CPanelAnimationVar(vgui::HFont, m_hTextNumberFont, "TextNumberFont", "WeaponIconsTiny");
	CPanelAnimationVar(vgui::HFont, m_hTextFont, "TextFont", "CovenHUDBoldXL");
	CPanelAnimationVar(Color, m_PowerColor, "PowerColor", "255 255 255 255");
	CPanelAnimationVar(Color, m_PowerColorShadow, "PowerColorShadow", "0 0 0 150");
};


#endif // CHL2MPCLIENTWHEELMENUDIALOG_H
