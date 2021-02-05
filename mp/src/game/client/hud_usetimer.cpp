#include "cbase.h"
#include "hud.h"
#include "hud_usetimer.h"
#include "iclientmode.h"
#include "view.h"
#include <vgui/ISurface.h>
#include "c_basehlplayer.h"
#include "vgui/ILocalize.h"
#include "coven_parse.h"

DECLARE_HUDELEMENT(CHudUseTimer);

using namespace vgui;

typedef struct
{
	float minProgressRadians;

	float vert1x;
	float vert1y;
	float vert2x;
	float vert2y;

	int swipe_dir_x;
	int swipe_dir_y;
} circular_progress_segment_coven;

circular_progress_segment_coven timerSegments[8] =
{
	{ 0.0, 0.5, 0.0, 1.0, 0.0, 1, 0 },
	{ M_PI * 0.25, 1.0, 0.0, 1.0, 0.5, 0, 1 },
	{ M_PI * 0.5, 1.0, 0.5, 1.0, 1.0, 0, 1 },
	{ M_PI * 0.75, 1.0, 1.0, 0.5, 1.0, -1, 0 },
	{ M_PI, 0.5, 1.0, 0.0, 1.0, -1, 0 },
	{ M_PI * 1.25, 0.0, 1.0, 0.0, 0.5, 0, -1 },
	{ M_PI * 1.5, 0.0, 0.5, 0.0, 0.0, 0, -1 },
	{ M_PI * 1.75, 0.0, 0.0, 0.5, 0.0, 1, 0 },
};


CHudUseTimer::CHudUseTimer(const char *pElementName) :
CHudElement(pElementName), BaseClass(NULL, "HudUseTimer")
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent(pParent);
	SetPaintBorderEnabled(false);

	m_flMaxTime = 0.0f;
	m_flMaxTimer = 0.0f;
	m_wszItemName[0] = 0;

	m_iEmptyTex = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile(m_iEmptyTex, "hud/ui/use_empty", true, true);

	m_iFullTex = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile(m_iFullTex, "hud/ui/use_full", true, true);
	SetHiddenBits(HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT);
}

CHudUseTimer::~CHudUseTimer()
{
}

void CHudUseTimer::OnThink()
{
	C_BaseHLPlayer* pPlayer = dynamic_cast<C_BaseHLPlayer *>(C_BasePlayer::GetLocalPlayer());
	if (pPlayer)
	{
		if (pPlayer->m_HL2Local.covenActionTimer > 0.0f)
		{
			if (m_flMaxTime == 0.0f || m_flMaxTimer != pPlayer->m_HL2Local.covenActionTimer)
			{
				m_flMaxTimer = pPlayer->m_HL2Local.covenActionTimer;
				m_flMaxTime = pPlayer->m_HL2Local.covenActionTimer - gpGlobals->curtime;
				if (pPlayer->m_HL2Local.covenAction < COVEN_ACTION_ITEMS)
				{
					CovenItemInfo_t *info = GetCovenItemData(CovenItemID_t(pPlayer->m_HL2Local.covenAction));
					V_swprintf_safe(m_wszItemName, L"%s", g_pVGuiLocalize->Find(info->szPrintName));
				}
				else if (pPlayer->m_HL2Local.covenAction < COVEN_ACTION_ABILITIES)
				{
					CovenAbilityInfo_t *info = GetCovenAbilityData(CovenAbility_t(pPlayer->m_HL2Local.covenAction - COVEN_ACTION_ITEMS));
					V_swprintf_safe(m_wszItemName, L"%s", g_pVGuiLocalize->Find(info->szPrintName));
				}
				else
				{
					switch (pPlayer->m_HL2Local.covenAction)
					{
					case COVEN_ACTION_REFUEL:
						V_swprintf_safe(m_wszItemName, L"%s", g_pVGuiLocalize->Find("#cvn_action_refuel"));
						break;
					default:
						V_swprintf_safe(m_wszItemName, L"%s", "INVALID ACTION");
					}
				}
			}
		}
		else
		{
			m_flMaxTimer = m_flMaxTime = 0.0f;
		}
	}

	BaseClass::OnThink();
}

bool CHudUseTimer::ShouldDraw()
{
	C_BaseHLPlayer* pPlayer =  dynamic_cast<C_BaseHLPlayer *>(C_BasePlayer::GetLocalPlayer());
	if (pPlayer && pPlayer->m_HL2Local.covenActionTimer == 0.0f)
		return false;

	return CHudElement::ShouldDraw();
}

void CHudUseTimer::Paint()
{
	C_BaseHLPlayer* pPlayer = dynamic_cast<C_BaseHLPlayer *>(C_BasePlayer::GetLocalPlayer());
	if (!pPlayer || m_flMaxTime == 0.0f)
		return;

	int wide, tall;
	GetSize(wide, tall);

	surface()->DrawSetTexture(m_iFullTex);
	surface()->DrawSetColor(Color(0, 0, 0, 255));
	float timer = pPlayer->m_HL2Local.covenActionTimer - gpGlobals->curtime;
	DrawCircleSegment(0.75f * surface()->GetFontTall(m_hTextFont), 0, wide - 1.5f * surface()->GetFontTall(m_hTextFont), tall - 1.5f * surface()->GetFontTall(m_hTextFont), 1.0f - timer / m_flMaxTime, true);

	if (pPlayer->m_HL2Local.covenAction < COVEN_ITEM_COUNT)
	{
		wchar_t wszIDString[MAX_PLAYER_NAME_LENGTH];
		g_pVGuiLocalize->ConstructString(wszIDString, sizeof(wszIDString), g_pVGuiLocalize->Find("#UseID"), 1, m_wszItemName);
		DrawTextTitle(0, tall, wide, surface()->GetFontTall(m_hTextFont), wszIDString);
	}
	else
		DrawTextTitle(0, tall, wide, surface()->GetFontTall(m_hTextFont), m_wszItemName);
}

void CHudUseTimer::PaintBackground()
{
	int wide, tall;
	GetSize(wide, tall);

	surface()->DrawSetTexture(m_iEmptyTex);
	surface()->DrawSetColor(Color(0, 0, 0, 255));
	surface()->DrawTexturedRect(0.75f * surface()->GetFontTall(m_hTextFont), 0, wide - 0.75f * surface()->GetFontTall(m_hTextFont), tall - 1.5f * surface()->GetFontTall(m_hTextFont));
}

void CHudUseTimer::DrawTextTitle(int x, int y, int wide, int tall, wchar_t *title)
{
	surface()->DrawSetTextFont(m_hTextFont);

	int tx = x + wide * 0.5f - UTIL_ComputeStringWidth(m_hTextFont, title) * 0.5f;
	int ty = y - tall;

	surface()->DrawSetTextColor(m_ColorShadow);

	surface()->DrawSetTextPos(tx - 2, ty + 2);
	surface()->DrawUnicodeString(title);
	surface()->DrawSetTextPos(tx + 2, ty - 2);
	surface()->DrawUnicodeString(title);

	surface()->DrawSetTextColor(m_Color);

	surface()->DrawSetTextPos(tx, ty);
	surface()->DrawUnicodeString(title);

}

void CHudUseTimer::DrawCircleSegment(int x, int y, int wide, int tall, float flEndProgress, bool bClockwise)
{
	float flWide = (float)wide;
	float flTall = (float)tall;

	float flHalfWide = (float)wide / 2;
	float flHalfTall = (float)tall / 2;

	float c_x = (float)x + flHalfWide;
	float c_y = (float)y + flHalfTall;

	// TODO - if we want to progress CCW, reverse a few things

	float flEndProgressRadians = flEndProgress * M_PI * 2;

	int cur_wedge = 0;
	for (int i = 0; i<8; i++)
	{
		if (flEndProgressRadians > timerSegments[cur_wedge].minProgressRadians)
		{
			vgui::Vertex_t v[3];

			// vert 0 is ( 0.5, 0.5 )
			v[0].m_Position.Init(c_x, c_y);
			v[0].m_TexCoord.Init(0.5f, 0.5f);

			float flInternalProgress = flEndProgressRadians - timerSegments[cur_wedge].minProgressRadians;

			if (flInternalProgress < (M_PI / 4))
			{
				// Calc how much of this slice we should be drawing

				if (i % 2 == 1)
				{
					flInternalProgress = (M_PI / 4) - flInternalProgress;
				}

				float flTan = tan(flInternalProgress);

				float flDeltaX, flDeltaY;

				if (i % 2 == 1)
				{
					flDeltaX = (flHalfWide - flHalfTall * flTan) * timerSegments[i].swipe_dir_x;
					flDeltaY = (flHalfTall - flHalfWide * flTan) * timerSegments[i].swipe_dir_y;
				}
				else
				{
					flDeltaX = flHalfTall * flTan * timerSegments[i].swipe_dir_x;
					flDeltaY = flHalfWide * flTan * timerSegments[i].swipe_dir_y;
				}

				v[2].m_Position.Init(x + timerSegments[i].vert1x * flWide + flDeltaX, y + timerSegments[i].vert1y * flTall + flDeltaY);
				v[2].m_TexCoord.Init(timerSegments[i].vert1x + (flDeltaX / flHalfWide) * 0.5, timerSegments[i].vert1y + (flDeltaY / flHalfTall) * 0.5);
			}
			else
			{
				// full segment, easy calculation
				v[2].m_Position.Init(c_x + flWide * (timerSegments[i].vert2x - 0.5), c_y + flTall * (timerSegments[i].vert2y - 0.5));
				v[2].m_TexCoord.Init(timerSegments[i].vert2x, timerSegments[i].vert2y);
			}

			// vert 2 is ( auraSegments[i].vert1x, auraSegments[i].vert1y )
			v[1].m_Position.Init(c_x + flWide * (timerSegments[i].vert1x - 0.5), c_y + flTall * (timerSegments[i].vert1y - 0.5));
			v[1].m_TexCoord.Init(timerSegments[i].vert1x, timerSegments[i].vert1y);

			surface()->DrawTexturedPolygon(3, v);
		}

		cur_wedge++;
		if (cur_wedge >= 8)
			cur_wedge = 0;
	}
}