#include "cbase.h"
#include "hudelement.h"
#include "hud_numericdisplay.h"
#include <vgui_controls/Panel.h>
#include "hud.h"
#include "hud_suitpower.h"
#include "hud_macros.h"
#include "iclientmode.h"
#include <vgui_controls/AnimationController.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include "c_basehlplayer.h"
#include "coven_parse.h"

#include "tier0/memdbgon.h" 

using namespace vgui;


class CHudBuildings : public Panel, public CHudElement
{
	DECLARE_CLASS_SIMPLE(CHudBuildings, Panel);

public:
	//CHudBuildings(Panel *parent, const char *name);
	CHudBuildings(const char *name);
	//void togglePrint();
	virtual void OnThink();

protected:
	virtual void	Paint();
	void			PaintBarStat(int x, int y, int value, int maxValue, float barWidth, float barHeight, float barChunkWidth, float barChunkGap);
	void			PaintBarStatContinuous(int x, int y, int value, int maxValue, float barWidth, float barHeight, Color color);
	virtual void	PaintBackground();

	CPanelAnimationVar(vgui::HFont, m_hTextFontTitle, "TextFont", "CovenHUD");
	CPanelAnimationVar(Color, m_AuxPowerColor, "AuxPowerColor", "255 255 255 255");
	CPanelAnimationVar(Color, m_AuxTippedColor, "AuxTippedColor", "255 0 0 255");
	CPanelAnimationVar(Color, m_AuxPowerColorGray, "AuxPowerColor", "125 125 125 255");
	int m_iType;
	CPanelAnimationVar(vgui::HFont, m_hTextFont, "TextFont", "CovenHUDBold");
	CPanelAnimationVar(vgui::HFont, m_hTextFontNumerals, "TextFont", "WeaponIconsCoven");
	CPanelAnimationVar(vgui::HFont, m_hTextFontIcons, "TextFont", "WeaponIconsBuilding");
};

DECLARE_HUDELEMENT(CHudBuildings);

CHudBuildings::CHudBuildings(const char *name) : BaseClass(NULL, name), CHudElement(name)
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent(pParent);

	SetPaintEnabled(true);
	SetPaintBackgroundEnabled(true);
	m_iType = 0;

	SetHiddenBits(HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT);
	SetBgColor(Color(0, 0, 0, 255));
}

void CHudBuildings::PaintBackground()
{
	C_BaseHLPlayer *pPlayer = (C_BaseHLPlayer *)C_BasePlayer::GetLocalPlayer();
	if (!pPlayer)
		return;

	int wide, tall;
	GetSize(wide, tall);

	int team = 0;
	if (pPlayer->GetTeamNumber() == COVEN_TEAMID_VAMPIRES)
		team = 1;

	BaseClass::PaintBackground();
}

void CHudBuildings::PaintBarStatContinuous(int x, int y, int value, int maxValue, float barWidth, float barHeight, Color color)
{
	float factor = ((float)value / (float)maxValue);
	float endpos = x + barWidth * factor;
	surface()->DrawSetColor(Color(255, 255, 255, color.a()));
	//surface()->DrawSetColor(color);
	surface()->DrawFilledRect(x, y, endpos, y + barHeight);
	surface()->DrawSetColor(color);
	//surface()->DrawSetColor(Color(color.r() / 2, color.g() / 2, color.b() / 2, color.a()));
	surface()->DrawFilledRect(endpos, y, x + barWidth, y + barHeight);
}

void CHudBuildings::PaintBarStat(int x, int y, int value, int maxValue, float barWidth, float barHeight, float barChunkWidth, float barChunkGap)
{
	// get bar chunks
	int chunkCount = barWidth / (barChunkWidth + barChunkGap);
	int enabledChunks = (int)((float)chunkCount * (value * 1.0f / maxValue) + 0.5f);
	// see if we've changed power state
	int lowPower = 0;
	if (enabledChunks <= (chunkCount / 4))
	{
		lowPower = 1;
	}
	/*if (m_nSuitPowerLow != lowPower)
	{
	if (m_iActiveSuitDevices || m_flSuitPower < max)
	{
	if (lowPower)
	{
	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("SuitAuxPowerDecreasedBelow25");
	}
	else
	{
	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("SuitAuxPowerIncreasedAbove25");
	}
	m_nSuitPowerLow = lowPower;
	}
	}*/
	surface()->DrawSetColor(Color(255, 0, 0, 255/*m_iAuxPowerDisabledAlpha*/));
	for (int i = 0; i < enabledChunks; i++)
	{
		surface()->DrawFilledRect(x, y, x + barChunkWidth, y + barHeight);
		x += (barChunkWidth + barChunkGap);
	}
	// draw the exhausted portion of the bar.
	surface()->DrawSetColor(Color(255, 255, 255, 255/*m_iAuxPowerDisabledAlpha*/));
	for (int i = enabledChunks; i < chunkCount; i++)
	{
		surface()->DrawFilledRect(x, y, x + barChunkWidth, y + barHeight);
		x += (barChunkWidth + barChunkGap);
	}
}

void CHudBuildings::Paint()
{
	C_BaseHLPlayer *pPlayer = (C_BaseHLPlayer *)C_BasePlayer::GetLocalPlayer();
	if (!pPlayer)
		return;

	int wide, tall;
	GetSize(wide, tall);
	SetPaintBorderEnabled(true);

	int team = 0;
	if (pPlayer->GetTeamNumber() == COVEN_TEAMID_VAMPIRES)
		team = 1;

	int barXpos = wide * 0.15f;
	int ypos = 0.12f * tall;
	int fontTall = surface()->GetFontTall(m_hTextFontIcons);
	int yfontoffsetnormal = fontTall * 0.3333f;
	int yfontoffset = fontTall * 0.48f;
	float barWidth = wide * 0.8f;
	float barHeight = fontTall * 0.3f;
	int yfontoffset2 = surface()->GetFontTall(m_hTextFontNumerals) * 0.3f;
	float barRowGap = fontTall * 0.25f;//tall * 0.12f;
	int xpos = barXpos * 0.5f - UTIL_ComputeStringWidth(m_hTextFontIcons, L"+") * 0.5f;

	surface()->DrawSetTextFont(m_hTextFontIcons);

	Color red(128, 0, 0, 50);
	Color yellow(128, 0, 0, 50);
	Color blue(0, 64, 128, 50);
	//Color purple(128, 0, 128, 50);
	Color orange(255, 128, 0, 50);

	if (pPlayer->m_CovenBuilderLocal.m_iTurretHP > 0 && m_iType == 1)
	{
		int xpos3 = barXpos / 2.0f - UTIL_ComputeStringWidth(m_hTextFontIcons, L"r") / 2.0f;
		int actualLevel = pPlayer->m_CovenBuilderLocal.m_iTurretLevel + 1;
		wchar_t wLevel[3];
		//Q_snprintf(level, sizeof(level), "%d", actualLevel);
		swprintf(wLevel, sizeof(wLevel), L"%d", actualLevel);
		//wcsncpy(localizeText, ansiLocal, sizeof(localizeText) / sizeof(wchar_t));
		//g_pVGuiLocalize->ConvertANSIToUnicode(level, wLevel, sizeof(wLevel));
		int xpos2 = barXpos / 2.0f - UTIL_ComputeStringWidth(m_hTextFontNumerals, wLevel) / 2.0f;
		surface()->DrawSetTextPos(xpos, ypos - yfontoffset);
		if (pPlayer->m_CovenBuilderLocal.m_bTurretTipped)
			surface()->DrawSetTextColor(m_AuxTippedColor);
		else
			surface()->DrawSetTextColor(m_AuxPowerColor);
		surface()->DrawUnicodeString(L"+");
		PaintBarStatContinuous(barXpos, ypos, pPlayer->m_CovenBuilderLocal.m_iTurretHP, pPlayer->m_CovenBuilderLocal.m_iTurretMaxHP, barWidth, barHeight, red);
		ypos += barHeight + barRowGap;

		surface()->DrawSetTextPos(xpos3, ypos - yfontoffsetnormal);
		surface()->DrawUnicodeString(L"r");
		PaintBarStatContinuous(barXpos, ypos, pPlayer->m_CovenBuilderLocal.m_iTurretAmmo, pPlayer->m_CovenBuilderLocal.m_iTurretMaxAmmo, barWidth, barHeight, yellow);
		ypos += barHeight + barRowGap;

		surface()->DrawSetTextPos(xpos, ypos - yfontoffset);
		surface()->DrawUnicodeString(L"*");
		PaintBarStatContinuous(barXpos, ypos, pPlayer->m_CovenBuilderLocal.m_iTurretEnergy, pPlayer->m_CovenBuilderLocal.m_iTurretMaxEnergy, barWidth, barHeight, orange);
		ypos += barHeight + barRowGap;

		surface()->DrawSetTextFont(m_hTextFontNumerals);
		surface()->DrawSetTextPos(xpos2, ypos - yfontoffset2);
		surface()->DrawUnicodeString(wLevel);
		CovenBuildingInfo_t *bldgInfo = GetCovenBuildingData(BUILDING_TURRET);
		if (actualLevel < bldgInfo->iMaxLevel)
			PaintBarStatContinuous(barXpos, ypos, pPlayer->m_CovenBuilderLocal.m_iTurretXP, pPlayer->m_CovenBuilderLocal.m_iTurretMaxXP, barWidth, barHeight, blue);
	}
	else if (pPlayer->m_CovenBuilderLocal.m_iDispenserHP > 0 && m_iType == 2)
	{
		int actualLevel = pPlayer->m_CovenBuilderLocal.m_iDispenserLevel + 1;
		wchar_t wLevel[3];
		swprintf(wLevel, sizeof(wLevel), L"%d", actualLevel);
		//Q_snprintf(level, sizeof(level), "%d", actualLevel);
		//wcsncpy(localizeText, ansiLocal, sizeof(localizeText) / sizeof(wchar_t));
		//g_pVGuiLocalize->ConvertANSIToUnicode(level, wLevel, sizeof(wLevel));
		surface()->DrawSetTextColor(m_AuxPowerColor);
		int xpos2 = barXpos / 2.0f - UTIL_ComputeStringWidth(m_hTextFontNumerals, wLevel) / 2.0f;
		surface()->DrawSetTextPos(xpos, ypos - yfontoffset);
		surface()->DrawUnicodeString(L"+");
		PaintBarStatContinuous(barXpos, ypos, pPlayer->m_CovenBuilderLocal.m_iDispenserHP, pPlayer->m_CovenBuilderLocal.m_iDispenserMaxHP, barWidth, barHeight, red);
		ypos += barHeight + barRowGap;

		surface()->DrawSetTextPos(xpos, ypos - yfontoffset);
		surface()->DrawUnicodeString(L"*");
		PaintBarStatContinuous(barXpos, ypos, pPlayer->m_CovenBuilderLocal.m_iDispenserMetal, pPlayer->m_CovenBuilderLocal.m_iDispenserMaxMetal, barWidth, barHeight, orange);
		ypos += barHeight + barRowGap;

		surface()->DrawSetTextFont(m_hTextFontNumerals);
		surface()->DrawSetTextPos(xpos2, ypos - yfontoffset2);
		surface()->DrawUnicodeString(wLevel);
		CovenBuildingInfo_t *bldgInfo = GetCovenBuildingData(BUILDING_AMMOCRATE);
		if (actualLevel < bldgInfo->iMaxLevel)
			PaintBarStatContinuous(barXpos, ypos, pPlayer->m_CovenBuilderLocal.m_iDispenserXP, pPlayer->m_CovenBuilderLocal.m_iDispenserMaxXP, barWidth, barHeight, blue);
	}
}

void CHudBuildings::OnThink()
{
	C_BaseHLPlayer *pPlayer = (C_BaseHLPlayer *)C_BasePlayer::GetLocalPlayer();
	if (!pPlayer)
		return;

	if (pPlayer->m_CovenBuilderLocal.m_iDispenserHP > 0 && m_iType == 2)
	{
		SetPaintEnabled(true);
		SetPaintBackgroundEnabled(true);
	}
	else if (pPlayer->m_CovenBuilderLocal.m_iTurretHP > 0 && m_iType == 1)
	{
		SetPaintEnabled(true);
		SetPaintBackgroundEnabled(true);
	}
	else
	{
		SetPaintEnabled(false);
		SetPaintBackgroundEnabled(false);
	}
	BaseClass::OnThink();
}

class CHudDispenser : public CHudBuildings
{
	DECLARE_CLASS_SIMPLE(CHudDispenser, CHudBuildings);

public:
	CHudDispenser(const char *name);

};

DECLARE_HUDELEMENT(CHudDispenser);

CHudDispenser::CHudDispenser(const char *name) : BaseClass("HudDispenser")
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent(pParent);

	SetPaintEnabled(true);
	SetPaintBackgroundEnabled(true);
	m_iType = 2;

	CHudBuildings::SetHiddenBits(HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT);
	SetBgColor(Color(0, 0, 0, 255));
}

class CHudTurret : public CHudBuildings
{
	DECLARE_CLASS_SIMPLE(CHudTurret, CHudBuildings);

public:
	CHudTurret(const char *name);

};

DECLARE_HUDELEMENT(CHudTurret);

CHudTurret::CHudTurret(const char *name) : BaseClass("HudTurret")
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent(pParent);

	SetPaintEnabled(true);
	SetPaintBackgroundEnabled(true);
	m_iType = 1;

	CHudBuildings::SetHiddenBits(HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT);
	SetBgColor(Color(0, 0, 0, 255));
}