#include "cbase.h"

#include "hl2mpclientitemmenu.h"

using namespace vgui; 

//-----------------------------------------------------------------------------
// Purpose: Konstructor
//-----------------------------------------------------------------------------
CHL2MPClientItemMenuDialog::CHL2MPClientItemMenuDialog(IViewPort *pViewPort) : CHL2MPClientWheelMenuDialog(pViewPort, PANEL_ITEM)
{
	AddItemToWheel(COVEN_ITEM_STIMPACK);
	AddItemToWheel(COVEN_ITEM_MEDKIT);
	AddItemToWheel(COVEN_ITEM_PILLS);
	LoadWheelTextures();
}
