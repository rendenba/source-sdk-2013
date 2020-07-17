#include "cbase.h"
#include <stdio.h>

#include <cdll_client_int.h>
#include <cdll_util.h>
#include <globalvars_base.h>
#include <igameresources.h>
#include "IGameUIFuncs.h" // for key bindings
#include "inputsystem/iinputsystem.h"
#include "clientscoreboarddialog.h"
#include <voice_status.h>

#include <vgui/IScheme.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vstdlib/IKeyValuesSystem.h>
#include <vgui/IInput.h>

#include <KeyValues.h>
#include <vgui_controls/ImageList.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/SectionedListPanel.h>

#include <game/client/iviewport.h>
#include <igameresources.h>

#include "hl2mpwheelmenu.h"

#include "hl2mp_gamerules.h"
#include "c_hl2mp_player.h"
#include "input.h"

#include "hl2mp_gamerules.h"

using namespace vgui;

static char temparray[256];

const int NumSegments = 7;
static int coord[NumSegments + 1] = {
	0,
	1,
	2,
	3,
	4,
	6,
	9,
	10
};

//-----------------------------------------------------------------------------
// Purpose: Konstructor
//-----------------------------------------------------------------------------
CHL2MPClientWheelMenuDialog::CHL2MPClientWheelMenuDialog(IViewPort *pViewPort, const char *panelName) : EditablePanel(NULL, panelName)
{
	m_nCloseKey = BUTTON_CODE_INVALID;

	// initialize dialog
	SetProportional(true);
	SetKeyBoardInputEnabled(false);
	SetMouseInputEnabled(false);

	// set the scheme before any child control is created
	SetScheme("ClientScheme");

	LoadControlSettings("Resource/UI/WheelMenu.res");

	m_iSelected = 0;
	m_iX = m_iY = 0;
	szPanelName = panelName;

	SetVisible(false);
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CHL2MPClientWheelMenuDialog::~CHL2MPClientWheelMenuDialog()
{
}

void CHL2MPClientWheelMenuDialog::LoadWheelTextures(void)
{
	m_iBlipTex = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile(m_iBlipTex, "hud/ui/numeral_blip", true, true);

	m_iBlipTexOn = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile(m_iBlipTexOn, "hud/ui/numeral_blip_on", true, true);

	int texture = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile(texture, "hud/ui/g_wheel_off", true, true);
	m_iSectorTex.AddToTail(texture);

	for (int i = 1; i <= m_Items.Count(); i++)
	{
		char szTextureFile[MAX_PATH];
		texture = surface()->CreateNewTextureID();
		Q_snprintf(szTextureFile, sizeof(szTextureFile), "hud/ui/g_wheel_sector%d", i);
		surface()->DrawSetTextureFile(texture, szTextureFile, true, true);
		m_iSectorTex.AddToTail(texture);
	}
}

void CHL2MPClientWheelMenuDialog::OnThink()
{
	BaseClass::OnThink();

	// NOTE: this is necessary because of the way input works.
	// If a key down message is sent to vgui, then it will get the key up message
	// Sometimes the scoreboard is activated by other vgui menus, 
	// sometimes by console commands. In the case where it's activated by
	// other vgui menus, we lose the key up message because this panel
	// doesn't accept keyboard input. It *can't* accept keyboard input
	// because another feature of the dialog is that if it's triggered
	// from within the game, you should be able to still run around while
	// the scoreboard is up. That feature is impossible if this panel accepts input.
	// because if a vgui panel is up that accepts input, it prevents the engine from
	// receiving that input. So, I'm stuck with a polling solution.
	// 
	// Close key is set to non-invalid when something other than a keybind
	// brings the scoreboard up, and it's set to invalid as soon as the 
	// dialog becomes hidden.
	if (m_nCloseKey != BUTTON_CODE_INVALID)
	{
		if (!g_pInputSystem->IsButtonDown(m_nCloseKey))
		{
			m_nCloseKey = BUTTON_CODE_INVALID;
			gViewPortInterface->ShowPanel(szPanelName, false);
			GetClientVoiceMgr()->StopSquelchMode();
		}
	}

	if (IsVisible())
	{
		int x = g_pInputSystem->GetAnalogDelta(MOUSE_X);
		int y = g_pInputSystem->GetAnalogDelta(MOUSE_Y);

		if (x != 0 || y != 0)
		{
			int wide, tall;
			GetSize(wide, tall);

			m_iX += x;
			m_iY -= y;

			float dist = sqrtf(m_iX*m_iX + m_iY*m_iY);

			if (dist > MIN_PICKUP_DIST * wide)
			{
				float angle = 0.0f;

				if (m_iX == 0)
				{
					if (m_iY < 0)
					{
						angle = 180.0f;
					}
				}
				else if (m_iY != 0)
				{
					int iX = m_iX;
					int iY = m_iY;
					float sector = 0.0f;
					if (iX > 0 && iY < 0)
					{
						int temp = iX;
						iX = -iY;
						iY = temp;
						sector = 90.0f;
					}
					else if (iX < 0 && iY < 0)
						sector = 180.0f;
					else if (iX < 0 && iY > 0)
					{
						int temp = iY;
						iY = -iX;
						iX = temp;
						sector = 270.0f;
					}
					angle = atan(iX / (float)iY) * 180.0f / M_PI + sector;
				}
				else if (m_iX > 0)
					angle = 90.0f;
				else
					angle = 270.0f;

				float width = 360.0f / m_Items.Count();
				m_iSelected = ceil(angle / width);

				float sn, cs;
				SinCos(angle * M_PI / 180.0f, &sn, &cs);
				int clampX = abs(wide * MAX_PICKUP_DIST * sn);
				int clampY = abs(tall * MAX_PICKUP_DIST * cs);

				m_iX = clamp(m_iX, -clampX, clampX);
				m_iY = clamp(m_iY, -clampY, clampY);
			}
			else
				m_iSelected = 0;
		}
	}
}

void CHL2MPClientWheelMenuDialog::ShowPanel(bool bShow)
{
	// Catch the case where we call ShowPanel before ApplySchemeSettings, eg when
	// going from windowed <-> fullscreen

	if (!bShow)
	{
		m_nCloseKey = BUTTON_CODE_INVALID;
	}

	if (BaseClass::IsVisible() == bShow)
		return;

	if (bShow)
	{
		Reset();
		Update();
		SetVisible(true);
		MoveToFront();
	}
	else
	{
		BaseClass::SetVisible(false);
		SetMouseInputEnabled(false);

		CHL2MP_Player *pPlayer = ToHL2MPPlayer(C_BasePlayer::GetLocalPlayer());

		if (!pPlayer)
			return;

		if (m_iSelected > 0)
		{
			CovenItemID_t n = m_Items[m_iSelected - 1];
			if (pPlayer->CovenItemQuantity(n) > 0 && HL2MPRules()->CanUseCovenItem(pPlayer, n))
			{
				if (n < COVEN_ITEM_COUNT)
				{
					char cmdString[8];
					Q_snprintf(cmdString, sizeof(cmdString), "item %d", n);
					engine->ClientCmd(cmdString);
					CovenItemInfo_t *info = GetCovenItemData(n);
					if (info->flUseTime > 0.0f)
						pPlayer->m_bPredictHideHud = true;
				}
				else
				{
					CBaseCombatWeapon *pWeap = pPlayer->GetActiveWeapon();
					switch (n)
					{
					case COVEN_ITEM_GRENADE:
						if (pWeap && !FClassnameIs(pWeap, "weapon_frag"))
						{
							pPlayer->m_bPredictHideHud = true;
							engine->ClientCmd("use weapon_frag");
						}
						break;
					case COVEN_ITEM_STUN_GRENADE:
						if (pWeap && !FClassnameIs(pWeap, "weapon_stunfrag"))
						{
							pPlayer->m_bPredictHideHud = true;
							engine->ClientCmd("use weapon_stunfrag");
						}
						break;
					case COVEN_ITEM_HOLYWATER:
						if (pWeap && !FClassnameIs(pWeap, "weapon_holywater"))
						{
							pPlayer->m_bPredictHideHud = true;
							engine->ClientCmd("use weapon_holywater");
						}
						break;
					}
				}
			}
			else
			{
				pPlayer->EmitSound("HL2Player.UseDeny");
			}
		}
	}
}

bool CHL2MPClientWheelMenuDialog::NeedsUpdate(void)
{
	return (m_fNextUpdateTime < gpGlobals->curtime);
}

void CHL2MPClientWheelMenuDialog::Update(void)
{
	// Reset();

	MoveToCenterOfScreen();

	//int x = 0;
	//int y = 0;
	//vgui::input()->GetCursorPosition(x, y);

	// update every second
	m_fNextUpdateTime = gpGlobals->curtime + 1.0f;
}

void CHL2MPClientWheelMenuDialog::MoveToCenterOfScreen()
{
	int wx, wy, ww, wt;
	surface()->GetWorkspaceBounds(wx, wy, ww, wt);
	SetPos((ww - GetWide()) / 2, (wt - GetTall()) / 2);
}

void CHL2MPClientWheelMenuDialog::AddItemToWheel(CovenItemID_t iItemType)
{
	m_Items.AddToTail(iItemType);
}

//-----------------------------------------------------------------------------
// Purpose: Paint background for rounded corners
//-----------------------------------------------------------------------------
void CHL2MPClientWheelMenuDialog::PaintBackground()
{
	int wide, tall;
	GetSize(wide, tall);

	float angle = 360.0f / m_Items.Count();
	float curangle = angle / 2.0f;

	surface()->DrawSetColor(Color(0, 0, 0, 255));

	CHL2MP_Player *pPlayer = ToHL2MPPlayer(C_BasePlayer::GetLocalPlayer());

	if (!pPlayer)
		return;

	int fntHeight = surface()->GetFontTall(m_hTextNumberFont) * 0.5f;

	surface()->DrawSetTexture(m_iSectorTex[m_iSelected]);
	surface()->DrawTexturedRect(0, 0, wide, tall);

	if (m_iSelected > 0)
	{
		CovenItemInfo_t *info = GetCovenItemData(m_Items[m_iSelected - 1]);
		surface()->DrawSetTextFont(m_hTextFont);
		wchar_t *tempString = NULL;
		tempString = g_pVGuiLocalize->Find(info->szPrintName);
		int tx = center_x - UTIL_ComputeStringWidth(m_hTextFont, tempString) * 0.5f;
		int ty = center_y - surface()->GetFontTall(m_hTextFont) * 0.5f;
		surface()->DrawSetTextPos(tx - 2, ty + 2);
		surface()->DrawSetTextColor(m_PowerColorShadow);
		surface()->DrawUnicodeString(tempString);
		surface()->DrawSetTextPos(tx + 2, ty - 2);
		surface()->DrawUnicodeString(tempString);
		surface()->DrawSetTextPos(tx, ty);
		surface()->DrawSetTextColor(m_PowerColor);
		surface()->DrawUnicodeString(tempString);
	}

	surface()->DrawSetTextFont(m_hTextNumberFont);

	for (int i = 0; i < m_Items.Count(); i++)
	{
		CovenItemInfo_t *info = GetCovenItemData(m_Items[i]);
		float sn, cs;
		SinCos(curangle * M_PI / 180.0f, &sn, &cs);
		int px = center_x + mouse_distance * sn;
		int py = center_y - mouse_distance * cs;
		int amount = pPlayer->CovenItemQuantity(m_Items[i]);
		bool bAllow = HL2MPRules()->CanUseCovenItem(pPlayer, m_Items[i]);
		if (amount > 0 && bAllow)
		{
			surface()->DrawSetTextColor(Color(255, 255, 255, 255));
			surface()->DrawSetTexture(info->hudIcon->textureId);
		}
		else
		{
			surface()->DrawSetTextColor(Color(128, 128, 128, 128));
			surface()->DrawSetTexture(info->hudIconOff->textureId);
		}
		wchar_t szAmt[2];
		V_swprintf_safe(szAmt, L"%d", amount);
		int width, height;
		surface()->DrawGetTextureSize(info->hudIcon->textureId, width, height);
		int hw = item_height * width / height;
		surface()->DrawTexturedRect(px - hw, py - item_height, px + hw, py + item_height);

		int pxn = px + hw * 0.6f;
		int pyn = py + item_height * 0.6f;

		if (amount > 0 && bAllow)
			surface()->DrawSetTexture(m_iBlipTexOn);
		else
			surface()->DrawSetTexture(m_iBlipTex);

		surface()->DrawTexturedRect(pxn, pyn, pxn + bumper, pyn + bumper);

		surface()->DrawSetTextPos(pxn + bumper * 0.5f - UTIL_ComputeStringWidth(m_hTextNumberFont, szAmt) * 0.5f, pyn + bumper * 0.5f - fntHeight);
		surface()->DrawUnicodeString(szAmt);

		curangle += angle;
	}

	//DEBUGGING
	/*surface()->DrawSetTextColor(Color(255, 0, 0, 255));
	int iX = wide / 2.0f + m_iX;
	int iY = tall / 2.0f - m_iY;
	surface()->DrawSetTextPos(iX, iY);
	surface()->DrawUnicodeString(L".");*/
}

//-----------------------------------------------------------------------------
// Purpose: Paint border for rounded corners
//-----------------------------------------------------------------------------
void CHL2MPClientWheelMenuDialog::PaintBorder()
{
	int x1, x2, y1, y2;
	surface()->DrawSetColor(m_borderColor);
	surface()->DrawSetTextColor(m_borderColor);

	int wide, tall;
	GetSize(wide, tall);

	int i;

	// top-left corner --------------------------------------------------------
	int xDir = 1;
	int yDir = -1;
	int xIndex = 0;
	int yIndex = NumSegments - 1;
	int xMult = 1;
	int yMult = 1;
	int x = 0;
	int y = 0;
	for (i = 0; i<NumSegments; ++i)
	{
		x1 = MIN(x + coord[xIndex] * xMult, x + coord[xIndex + 1] * xMult);
		x2 = MAX(x + coord[xIndex] * xMult, x + coord[xIndex + 1] * xMult);
		y1 = MIN(y + coord[yIndex] * yMult, y + coord[yIndex + 1] * yMult);
		y2 = MAX(y + coord[yIndex] * yMult, y + coord[yIndex + 1] * yMult);
		surface()->DrawFilledRect(x1, y1, x2, y2);

		xIndex += xDir;
		yIndex += yDir;
	}

	// top-right corner -------------------------------------------------------
	xDir = 1;
	yDir = -1;
	xIndex = 0;
	yIndex = NumSegments - 1;
	x = wide;
	y = 0;
	xMult = -1;
	yMult = 1;
	for (i = 0; i<NumSegments; ++i)
	{
		x1 = MIN(x + coord[xIndex] * xMult, x + coord[xIndex + 1] * xMult);
		x2 = MAX(x + coord[xIndex] * xMult, x + coord[xIndex + 1] * xMult);
		y1 = MIN(y + coord[yIndex] * yMult, y + coord[yIndex + 1] * yMult);
		y2 = MAX(y + coord[yIndex] * yMult, y + coord[yIndex + 1] * yMult);
		surface()->DrawFilledRect(x1, y1, x2, y2);
		xIndex += xDir;
		yIndex += yDir;
	}

	// bottom-right corner ----------------------------------------------------
	xDir = 1;
	yDir = -1;
	xIndex = 0;
	yIndex = NumSegments - 1;
	x = wide;
	y = tall;
	xMult = -1;
	yMult = -1;
	for (i = 0; i<NumSegments; ++i)
	{
		x1 = MIN(x + coord[xIndex] * xMult, x + coord[xIndex + 1] * xMult);
		x2 = MAX(x + coord[xIndex] * xMult, x + coord[xIndex + 1] * xMult);
		y1 = MIN(y + coord[yIndex] * yMult, y + coord[yIndex + 1] * yMult);
		y2 = MAX(y + coord[yIndex] * yMult, y + coord[yIndex + 1] * yMult);
		surface()->DrawFilledRect(x1, y1, x2, y2);
		xIndex += xDir;
		yIndex += yDir;
	}

	// bottom-left corner -----------------------------------------------------
	xDir = 1;
	yDir = -1;
	xIndex = 0;
	yIndex = NumSegments - 1;
	x = 0;
	y = tall;
	xMult = 1;
	yMult = -1;
	for (i = 0; i<NumSegments; ++i)
	{
		x1 = MIN(x + coord[xIndex] * xMult, x + coord[xIndex + 1] * xMult);
		x2 = MAX(x + coord[xIndex] * xMult, x + coord[xIndex + 1] * xMult);
		y1 = MIN(y + coord[yIndex] * yMult, y + coord[yIndex + 1] * yMult);
		y2 = MAX(y + coord[yIndex] * yMult, y + coord[yIndex + 1] * yMult);
		surface()->DrawFilledRect(x1, y1, x2, y2);
		xIndex += xDir;
		yIndex += yDir;
	}

	// top --------------------------------------------------------------------
	x1 = coord[NumSegments];
	x2 = wide - coord[NumSegments];
	y1 = 0;
	y2 = 1;
	surface()->DrawFilledRect(x1, y1, x2, y2);

	// bottom -----------------------------------------------------------------
	x1 = coord[NumSegments];
	x2 = wide - coord[NumSegments];
	y1 = tall - 1;
	y2 = tall;
	surface()->DrawFilledRect(x1, y1, x2, y2);

	// left -------------------------------------------------------------------
	x1 = 0;
	x2 = 1;
	y1 = coord[NumSegments];
	y2 = tall - coord[NumSegments];
	surface()->DrawFilledRect(x1, y1, x2, y2);

	// right ------------------------------------------------------------------
	x1 = wide - 1;
	x2 = wide;
	y1 = coord[NumSegments];
	y2 = tall - coord[NumSegments];
	surface()->DrawFilledRect(x1, y1, x2, y2);
}

//-----------------------------------------------------------------------------
// Purpose: Apply scheme settings
//-----------------------------------------------------------------------------
void CHL2MPClientWheelMenuDialog::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_bgColor = GetSchemeColor("SectionedListPanel.BgColor", GetBgColor(), pScheme);
	m_borderColor = pScheme->GetColor("FgColor", Color(0, 0, 0, 0));

	SetBgColor(Color(0, 0, 0, 0));
	//SetBorder(pScheme->GetBorder("BaseBorder"));
}

void CHL2MPClientWheelMenuDialog::Reset()
{
	m_fNextUpdateTime = 0;
	m_iSelected = 0;
	m_iX = m_iY = 0;
}
