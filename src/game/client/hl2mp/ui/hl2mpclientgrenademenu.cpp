#include "cbase.h"

#include "hl2mpclientgrenademenu.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Konstructor
//-----------------------------------------------------------------------------
CHL2MPClientGrenadeMenuDialog::CHL2MPClientGrenadeMenuDialog(IViewPort *pViewPort) : CHL2MPClientWheelMenuDialog(pViewPort, PANEL_GRENADE)
{
	AddItemToWheel(COVEN_ITEM_SLAM);
	AddItemToWheel(COVEN_ITEM_HOLYWATER);
	AddItemToWheel(COVEN_ITEM_STUN_GRENADE);
	AddItemToWheel(COVEN_ITEM_GRENADE);
	LoadWheelTextures();
}
