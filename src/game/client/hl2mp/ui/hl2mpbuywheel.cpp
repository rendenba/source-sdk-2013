#include "cbase.h"

#include "hl2mpbuywheel.h"
#include "c_hl2mp_player.h"
#include "coven_parse.h"
#include "inputsystem/iinputsystem.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Konstructor
//-----------------------------------------------------------------------------
CHL2MPClientBuyWheelDialog::CHL2MPClientBuyWheelDialog(IViewPort *pViewPort) : CHL2MPClientWheelMenuDialog(pViewPort, PANEL_BUY)
{
	AddItemToWheel(COVEN_ITEM_STIMPACK);
	AddItemToWheel(COVEN_ITEM_MEDKIT);
	AddItemToWheel(COVEN_ITEM_PILLS);
	AddItemToWheel(COVEN_ITEM_SLAM);
	AddItemToWheel(COVEN_ITEM_HOLYWATER);
	AddItemToWheel(COVEN_ITEM_STUN_GRENADE);
	AddItemToWheel(COVEN_ITEM_GRENADE);
	LoadWheelTextures();
}

int CHL2MPClientBuyWheelDialog::GetIconText(C_HL2MP_Player *pPlayer, int iId)
{
	CovenItemID_t n = m_Items[iId];
	CovenItemInfo_t *info = GetCovenItemData(n);
	int cost = info->iCost;
	if (pPlayer->HasStatus(COVEN_STATUS_BUYZONE))
		cost = cost * pPlayer->GetStatusMagnitude(COVEN_STATUS_BUYZONE) / 100.0f;

	return cost;
}

bool CHL2MPClientBuyWheelDialog::IconActive(C_HL2MP_Player *pPlayer, int iId)
{
	CovenItemID_t n = m_Items[iId];
	CovenItemInfo_t *info = GetCovenItemData(n);
	int cost = info->iCost;
	if (pPlayer->HasStatus(COVEN_STATUS_BUYZONE))
		cost = cost * pPlayer->GetStatusMagnitude(COVEN_STATUS_BUYZONE) / 100.0f;

	return pPlayer->HasStatus(COVEN_STATUS_BUYZONE) && pPlayer->m_HL2Local.covenXPCounter >= cost;
}

bool CHL2MPClientBuyWheelDialog::OnMouseClick()
{
	C_HL2MP_Player *pPlayer = ToHL2MPPlayer(C_BasePlayer::GetLocalPlayer());

	if (pPlayer)
	{
		if (g_pInputSystem->IsButtonDown(MOUSE_RIGHT))
		{
			if (!bMouse2Latch)
			{
				if (m_iSelected > 0)
				{
					CovenItemID_t n = m_Items[m_iSelected - 1];
					char cmdString[10];
					Q_snprintf(cmdString, sizeof(cmdString), "buyall %d", n);
					engine->ClientCmd(cmdString);
				}
			}
		}
		else if (g_pInputSystem->IsButtonDown(MOUSE_LEFT))
		{
			if (!bMouse1Latch)
			{
				if (m_iSelected > 0)
				{
					CovenItemID_t n = m_Items[m_iSelected - 1];
					char cmdString[8];
					Q_snprintf(cmdString, sizeof(cmdString), "buy %d", n);
					engine->ClientCmd(cmdString);
				}
			}
		}
	}

	return false;
}

void CHL2MPClientBuyWheelDialog::HandleSelection(C_HL2MP_Player *pPlayer)
{
	
}

void CHL2MPClientBuyWheelDialog::OnThink()
{
	BaseClass::OnThink();
	bMouse1Latch = g_pInputSystem->IsButtonDown(MOUSE_LEFT);
	bMouse2Latch = g_pInputSystem->IsButtonDown(MOUSE_RIGHT);
}