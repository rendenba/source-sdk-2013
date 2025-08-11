#ifndef CHL2MPCLIENTBUYWHEEL_H
#define CHL2MPCLIENTBUYWHEEL_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include <game/client/iviewport.h>
#include "hl2mpwheelmenu.h"

class CHL2MPClientBuyWheelDialog : public CHL2MPClientWheelMenuDialog
{
private:
	DECLARE_CLASS_SIMPLE(CHL2MPClientBuyWheelDialog, CHL2MPClientWheelMenuDialog);

public:
	CHL2MPClientBuyWheelDialog(IViewPort *pViewPort);

	virtual const char *GetName(void) { return PANEL_BUY; }
protected:
	void HandleSelection(C_HL2MP_Player *pPlayer);
	bool IconActive(C_HL2MP_Player *pPlayer, int iId);
	int GetIconText(C_HL2MP_Player *pPlayer, int iId);
	bool OnMouseClick();
	void OnThink();
	bool bMouse1Latch;
	bool bMouse2Latch;
};


#endif // CHL2MPCLIENTBUYWHEEL_H
