#ifndef CHL2MPCLIENTGRENADEMENUDIALOG_H
#define CHL2MPCLIENTGRENADEMENUDIALOG_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include <game/client/iviewport.h>
#include "hl2mpwheelmenu.h"

class CHL2MPClientGrenadeMenuDialog : public CHL2MPClientWheelMenuDialog
{
private:
	DECLARE_CLASS_SIMPLE(CHL2MPClientGrenadeMenuDialog, CHL2MPClientWheelMenuDialog);

public:
	CHL2MPClientGrenadeMenuDialog(IViewPort *pViewPort);

	virtual const char *GetName(void) { return PANEL_GRENADE; }

};


#endif // CHL2MPCLIENTGRENADEMENUDIALOG_H
