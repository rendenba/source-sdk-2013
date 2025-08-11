#ifndef CHL2MPCLIENTITEMMENUDIALOG_H
#define CHL2MPCLIENTITEMMENUDIALOG_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include <game/client/iviewport.h>
#include "hl2mpwheelmenu.h"

class CHL2MPClientItemMenuDialog : public CHL2MPClientWheelMenuDialog
{
private:
	DECLARE_CLASS_SIMPLE(CHL2MPClientItemMenuDialog, CHL2MPClientWheelMenuDialog);

public:
	CHL2MPClientItemMenuDialog(IViewPort *pViewPort);

	virtual const char *GetName(void) { return PANEL_ITEM; }

};


#endif // CHL2MPCLIENTITEMMENUDIALOG_H
