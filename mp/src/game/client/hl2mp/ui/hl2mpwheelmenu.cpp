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

typedef struct
{
	float minProgressRadians;

	float vert1x;
	float vert1y;
	float vert2x;
	float vert2y;

	int swipe_dir_x;
	int swipe_dir_y;
} circular_progress_segment_wheel;

circular_progress_segment_wheel wheelSegments[8] =
{
	{ 0.0f,			1.0f, 0.0f, 0.5f, 0.0f, 1, 0 },
	{ M_PI * 0.25f,	1.0f, 0.5f, 1.0f, 0.0f, 0, 1 },
	{ M_PI * 0.5f,	1.0f, 1.0f, 1.0f, 0.5f, 0, 1 },
	{ M_PI * 0.75f,	0.5f, 1.0f, 1.0f, 1.0f, -1, 0 },
	{ M_PI,			0.0f, 1.0f, 0.5f, 1.0f, -1, 0 },
	{ M_PI * 1.25f,	0.0f, 0.5f, 0.0f, 1.0f, 0, -1 },
	{ M_PI * 1.5f,	0.0f, 0.0f, 0.0f, 0.5f, 0, -1 },
	{ M_PI * 1.75f,	0.5f, 0.0f, 0.0f, 0.0f, 1, 0 },
};

extern ConVar sv_coven_max_slam;

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

	m_iArrowTex = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile(m_iArrowTex, "hud/ui/arrow", true, true);
	m_iShadowTex = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile(m_iShadowTex, "hud/ui/grenade_wheel_black", true, true);
	m_iForeTex = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile(m_iForeTex, "hud/ui/grenade_wheel_white", true, true);
}

bool CHL2MPClientWheelMenuDialog::OnMouseClick()
{
	m_iSelected = 0;
	m_nCloseKey = BUTTON_CODE_INVALID;
	gViewPortInterface->ShowPanel(szPanelName, false);
	GetClientVoiceMgr()->StopSquelchMode();
	return true;
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
		if (g_pInputSystem->IsButtonDown(MOUSE_LEFT) || g_pInputSystem->IsButtonDown(MOUSE_RIGHT))
		{
			if (OnMouseClick())
				return;
		}

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
				m_flAngle = 0.0f;

				if (m_iX == 0)
				{
					if (m_iY < 0)
					{
						m_flAngle = 180.0f;
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
					m_flAngle = atan(iX / (float)iY) * 180.0f / M_PI + sector;
				}
				else if (m_iX > 0)
					m_flAngle = 90.0f;
				else
					m_flAngle = 270.0f;

				float width = 360.0f / m_Items.Count();
				float selection = m_flAngle / width;
				int selected = ceil(selection);
				if (selected == 0)
					selected = 1;

				if (m_iSelected > 0 && selected != m_iSelected)
				{
					float value = selected - selection;
					if (value < 0.95f || value > 1.05f)
					{
						m_iSelected = selected;
					}
				}
				else
					m_iSelected = selected;

				float sn, cs;
				SinCos(m_flAngle * M_PI / 180.0f, &sn, &cs);
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

	CHL2MP_Player *pPlayer = ToHL2MPPlayer(C_BasePlayer::GetLocalPlayer());

	if (!pPlayer)
		return;

	//BB: HACK HACK! Vamps dont use this!
	if (!pPlayer->IsAlive() || pPlayer->GetTeamNumber() == COVEN_TEAMID_VAMPIRES)
		bShow = false;

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
		gHUD.m_flMouseSensitivityFactor = 0.2f;
		gHUD.m_flMaxRotation = 10.0f;
		gHUD.m_angLockAngle = pPlayer->EyeAngles();
		m_flOnTime = gpGlobals->curtime;
	}
	else
	{
		BaseClass::SetVisible(false);
		SetMouseInputEnabled(false);
		gHUD.m_flMouseSensitivityFactor = 0.0f;
		gHUD.m_flMaxRotation = 0.0f;

		HandleSelection(pPlayer);
	}
}

void CHL2MPClientWheelMenuDialog::HandleSelection(CHL2MP_Player *pPlayer)
{
	if (m_iSelected > 0)
	{
		CovenItemID_t n = m_Items[m_iSelected - 1];
		if ((n == COVEN_ITEM_SLAM && ((pPlayer->CovenItemQuantity(n) > 0 && pPlayer->m_HL2Local.m_iNumSatchel + pPlayer->m_HL2Local.m_iNumTripmines < sv_coven_max_slam.GetInt()) || HL2MPRules()->CanUseCovenItem(pPlayer, n))) || (pPlayer->CovenItemQuantity(n) > 0 && HL2MPRules()->CanUseCovenItem(pPlayer, n)))
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
				case COVEN_ITEM_SLAM:
					if (pWeap && !FClassnameIs(pWeap, "weapon_slam"))
					{
						pPlayer->m_bPredictHideHud = true;
						engine->ClientCmd("use weapon_slam");
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
	DrawCircle();
}

bool CHL2MPClientWheelMenuDialog::IconActive(CHL2MP_Player *pPlayer, int iId)
{
	int amount = pPlayer->CovenItemQuantity(m_Items[iId]);
	bool bAllow = HL2MPRules()->CanUseCovenItem(pPlayer, m_Items[iId]);

	return (amount > 0 && bAllow) || (m_Items[iId] == COVEN_ITEM_SLAM && ((amount > 0 && pPlayer->m_HL2Local.m_iNumSatchel + pPlayer->m_HL2Local.m_iNumTripmines < sv_coven_max_slam.GetInt()) || bAllow));
}

int CHL2MPClientWheelMenuDialog::GetIconText(CHL2MP_Player *pPlayer, int iId)
{
	return pPlayer->CovenItemQuantity(m_Items[iId]);
}

void CHL2MPClientWheelMenuDialog::DrawCircle()
{
	CHL2MP_Player *pPlayer = ToHL2MPPlayer(C_BasePlayer::GetLocalPlayer());

	if (!pPlayer)
		return;

	int wide, tall;
	GetSize(wide, tall);

	float angle = 2.0f * M_PI / m_Items.Count();
	float flStart = 0.0f;
	float curangle = angle / 2.0f;

	surface()->DrawSetColor(Color(0, 0, 0, 255));

	float deltaTime = (gpGlobals->curtime - m_flOnTime);
	float scale = clamp(deltaTime / 0.2f, 0.0f, 0.7f) + 0.3f;

	int fntHeight = surface()->GetFontTall(m_hTextNumberFont) * 0.5f;

	//DRAW TITLE
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
		//DRAW CIRCLE
		if ((i + 1) == m_iSelected)
			surface()->DrawSetTexture(m_iForeTex);
		else
			surface()->DrawSetTexture(m_iShadowTex);

		DrawSegment(flStart, flStart + angle, scale);

		//DRAW ICON
		CovenItemInfo_t *info = GetCovenItemData(m_Items[i]);
		float sn, cs;
		SinCos(curangle, &sn, &cs);
		int px = center_x + scale * mouse_distance * sn;
		int py = center_y - scale * mouse_distance * cs;

		bool bAllow = IconActive(pPlayer, i);
		
		if (bAllow)
		{
			surface()->DrawSetTextColor(Color(255, 255, 255, 255));
			surface()->DrawSetTexture(info->hudIcon->textureId);
		}
		else
		{
			if ((i + 1) == m_iSelected)
				surface()->DrawSetTextColor(Color(0, 0, 0, 255));
			else
				surface()->DrawSetTextColor(Color(128, 128, 128, 255));
			surface()->DrawSetTexture(info->hudIconOff->textureId);
		}
		int width, height;
		surface()->DrawGetTextureSize(info->hudIcon->textureId, width, height);
		int hw = item_height * width / height;
		surface()->DrawTexturedRect(px - hw, py - item_height, px + hw, py + item_height);

		//DRAW BLIP
		if (scale >= 1.0f)
		{
			wchar_t szAmt[3];
			V_swprintf_safe(szAmt, L"%d", GetIconText(pPlayer, i));
			int pxn = px + hw * 0.6f;
			int pyn = py + item_height * 0.6f;

			if (bAllow)
				surface()->DrawSetTexture(m_iBlipTexOn);
			else
				surface()->DrawSetTexture(m_iBlipTex);

			surface()->DrawTexturedRect(pxn, pyn, pxn + bumper, pyn + bumper);

		
			surface()->DrawSetTextPos(pxn + bumper * 0.5f - UTIL_ComputeStringWidth(m_hTextNumberFont, szAmt) * 0.5f, pyn + bumper * 0.5f - fntHeight);
			surface()->DrawUnicodeString(szAmt);
		}

		curangle += angle;
		flStart += angle;
	}

	//DRAW ARROW
	DrawArrow();

	//DEBUGGING
	/*surface()->DrawSetTextColor(Color(255, 0, 0, 255));
	int iX = wide / 2.0f + m_iX;
	int iY = tall / 2.0f - m_iY;
	surface()->DrawSetTextPos(iX, iY);
	surface()->DrawUnicodeString(L".");*/
}

void CHL2MPClientWheelMenuDialog::DrawArrow()
{
	int wide, tall;
	GetSize(wide, tall);
	float dist = sqrtf(m_iX*m_iX + m_iY*m_iY);

	if (dist > MIN_PICKUP_DIST * wide)
	{
		int iX = wide / 2.0f;
		int iY = tall / 2.0f;
		float hSize = arrow_size / 2.0f;
		float sn, cs;

		SinCos(m_flAngle * M_PI / 180.0f, &sn, &cs);

		//xout = x * cs - y * sn;
		//yout = x * sn + y * cs;
		vgui::Vertex_t v[4];
		surface()->DrawSetTextColor(Color(255, 0, 0, 255));

		//Bottom Right
		v[2].m_Position.Init(iX + arrow_dist * sn, iY - arrow_dist * cs);
		v[2].m_TexCoord.Init(1, 1);

		//Top left
		v[0].m_Position.Init(iX + (arrow_dist + arrow_size) * sn, iY - (arrow_dist + arrow_size) * cs);
		v[0].m_TexCoord.Init(0, 0);

		//Bottom Left
		v[3].m_Position.Init(iX + (-hSize) * cs + (arrow_dist + hSize) * sn, iY + (-hSize) * sn - (arrow_dist + hSize) * cs);
		v[3].m_TexCoord.Init(0, 1);

		//Top Right
		v[1].m_Position.Init(iX + hSize * cs + (arrow_dist + hSize) * sn, iY + hSize * sn - (arrow_dist + hSize) * cs);
		v[1].m_TexCoord.Init(1, 0);

		surface()->DrawSetTexture(m_iArrowTex);
		surface()->DrawTexturedPolygon(4, v);
	}
}

void CHL2MPClientWheelMenuDialog::DrawSegment(float flStartRad, float flEndRad, float scale)
{
	int wide, tall;
	GetSize(wide, tall);

	float flWide = (float)wide;
	float flTall = (float)tall;

	float flHalfWide = wide / 2.0f;
	float flHalfTall = tall / 2.0f;

	float c_x = flHalfWide;
	float c_y = flHalfTall;

	int iStartSegment = (floor(flStartRad / (M_PI / 400.0f)) / 100.0f);
	int iEndSegment = ceil(floor(flEndRad / (M_PI / 400.0f)) / 100.0f);

	for (int i = iStartSegment; i < iEndSegment; i++)
	{
		vgui::Vertex_t v[3];

		// vert 0 is ( 0.5, 0.5 )
		v[0].m_Position.Init(c_x, c_y);
		v[0].m_TexCoord.Init(0.5f, 0.5f);

		float flInternalProgress = flEndRad - wheelSegments[i].minProgressRadians;

		//int x_sign = (i < 4) ? 1 : -1;
		//int y_sign = (i > 1 && i < 6) ? 1 : -1;

		// vert 2
		if (flInternalProgress < (M_PI / 4.0f))
		{
			// Calc how much of this slice we should be drawing

			if (i % 2 == 1)
			{
				flInternalProgress = (M_PI / 4.0f) - flInternalProgress;
			}

			float flTan = tan(flInternalProgress);

			float flDeltaX, flDeltaY;

			if (i % 2 == 1)
			{
				flDeltaX = (flHalfWide - flHalfTall * flTan) * wheelSegments[i].swipe_dir_x;
				flDeltaY = (flHalfTall - flHalfWide * flTan) * wheelSegments[i].swipe_dir_y;
			}
			else
			{
				flDeltaX = flHalfTall * flTan * wheelSegments[i].swipe_dir_x;
				flDeltaY = flHalfWide * flTan * wheelSegments[i].swipe_dir_y;
			}
			//c_x + x_sign * scale * 
			//v[2].m_Position.Init((flWide * wheelSegments[i].vert1x - flDeltaX), (flTall * wheelSegments[i].vert1y - flDeltaY));
			v[2].m_Position.Init(c_x + scale * (flWide * (wheelSegments[i].vert2x - 0.5f) + flDeltaX), c_y + scale * (flTall * (wheelSegments[i].vert2y - 0.5f) + flDeltaY));
			v[2].m_TexCoord.Init(wheelSegments[i].vert2x + (flDeltaX / flHalfWide) * 0.5f, wheelSegments[i].vert2y + (flDeltaY / flHalfTall) * 0.5f);
		}
		else
		{
			// full segment, easy calculation
			v[2].m_Position.Init(c_x + scale * flWide * (wheelSegments[i].vert1x - 0.5f), c_y + scale * flTall * (wheelSegments[i].vert1y - 0.5f));
			v[2].m_TexCoord.Init(wheelSegments[i].vert1x, wheelSegments[i].vert1y);
		}

		flInternalProgress = flStartRad - wheelSegments[i].minProgressRadians;
		
		// vert 1
		if (flInternalProgress > 0.0f)
		{
			// Calc how much of this slice we should be drawing

			if (i % 2 == 1)
			{
				flInternalProgress = (M_PI / 4.0f) - flInternalProgress;
			}

			float flTan = tan(flInternalProgress);

			float flDeltaX, flDeltaY;

			if (i % 2 == 1)
			{
				flDeltaX = (flHalfWide - flHalfTall * flTan) * wheelSegments[i].swipe_dir_x;
				flDeltaY = (flHalfTall - flHalfWide * flTan) * wheelSegments[i].swipe_dir_y;
			}
			else
			{
				flDeltaX = flHalfTall * flTan * wheelSegments[i].swipe_dir_x;
				flDeltaY = flHalfWide * flTan * wheelSegments[i].swipe_dir_y;
			}
			v[1].m_Position.Init(c_x + scale * (flWide * (wheelSegments[i].vert2x - 0.5f) + flDeltaX), c_y + scale * (flTall * (wheelSegments[i].vert2y - 0.5f) + flDeltaY));
			v[1].m_TexCoord.Init(wheelSegments[i].vert2x + (flDeltaX / flHalfWide) * 0.5f, wheelSegments[i].vert2y + (flDeltaY / flHalfTall) * 0.5f);
		}
		else
		{
			//full segment, easy calculation
			v[1].m_Position.Init(c_x + scale * flWide * (wheelSegments[i].vert2x - 0.5f), c_y + scale * flTall * (wheelSegments[i].vert2y - 0.5f));
			v[1].m_TexCoord.Init(wheelSegments[i].vert2x, wheelSegments[i].vert2y);
		}

		surface()->DrawTexturedPolygon(3, v);
	}

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
