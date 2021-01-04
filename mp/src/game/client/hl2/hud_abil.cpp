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

static ConVar cl_coven_abilitytitles("cl_coven_abilitytitles", "1", FCVAR_ARCHIVE, "Display ability titles.");
static ConVar cl_coven_cooldownblips("cl_coven_cooldownblips", "0", FCVAR_ARCHIVE, "Play cooldown blip sounds.");

struct ability_pic
{
	int abil;
	int text;
	float timer;
};

class CHudAbils : public CHudElement, public Panel
{
	DECLARE_CLASS_SIMPLE( CHudAbils, Panel );
 
	public:
	CHudAbils( const char *pElementName );
	void togglePrint();
	virtual void OnThink();
 
	protected:
	// CUtlVector<ability_pic *> active_abils;
	virtual void Paint();
	virtual void PaintBackground();

	int m_nShadowTex;
	int m_nCircleBlip;

	int m_nBackgrounds[2];
	int m_nAbilBorders[2][COVEN_MAX_CLASSCOUNT][4][3];
	int m_nBorders[4];

	void CheckCooldown(int iAbilityNum);

	float max_duration[4];
	bool cooledDown[4];

	void DrawCircleSegment( int x, int y, int wide, int tall, float flEndDegrees, bool clockwise /* = true */ );
	void DrawTextValue(int wide, int tall, int x, int y, float val);
	void DrawTextTitle(int x, int y, int wide, int tall, wchar_t *title, bool bCooldown, HFont useFont = NULL);

	float DrawAbility(int iAbilityNum, float y, float wide, float inset, float minset);

	CPanelAnimationVar( vgui::HFont, m_hTextFont, "TextFont", "CovenHUDBoldXL" );
	CPanelAnimationVar(vgui::HFont, m_hTextFontTitle, "TextFont", "CovenHUD");
	CPanelAnimationVar(vgui::HFont, m_hFontNumerals, "TextFont", "WeaponIconsTiny");

	CPanelAnimationVar(Color, m_AuxPowerColor, "AuxPowerColor", "255 255 255 255");
	CPanelAnimationVar(Color, m_AuxPowerColorGray, "AuxPowerColor", "125 125 125 255");
	CPanelAnimationVar(Color, m_AuxPowerColorShadow, "AuxPowerColorShadow", "0 0 0 200");//150
	//CPanelAnimationVarAliasType( int, m_nCBgTextureId1, "Texture1", "vgui/hud/800corner1", "textureid" );
	//CPanelAnimationVarAliasType( int, m_nCBgTextureId2, "Texture2", "vgui/hud/800corner2", "textureid" );
	//CPanelAnimationVarAliasType( int, m_nCBgTextureId3, "Texture3", "vgui/hud/800corner3", "textureid" );
	//CPanelAnimationVarAliasType( int, m_nCBgTextureId4, "Texture4", "vgui/hud/800corner4", "textureid" );
};

DECLARE_HUDELEMENT( CHudAbils );

CHudAbils::CHudAbils( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudAbils" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );   
	SetPaintBorderEnabled(false);

	Q_memset(max_duration, 0, sizeof(max_duration));
	Q_memset(cooledDown, 1, sizeof(cooledDown));

	m_nShadowTex = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile( m_nShadowTex, "hud/abilities/cooldown", true, true);

	m_nCircleBlip = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile( m_nCircleBlip, "hud/abilities/circleblip", true, true);

	m_nBackgrounds[0] = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile( m_nBackgrounds[0], "hud/backgrounds/iron", true, true);
	m_nBackgrounds[1] = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile( m_nBackgrounds[1], "hud/backgrounds/stone", true, true);

	m_nBorders[0] = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile( m_nBorders[0], "hud/abilities/gray_border", true, true);
	m_nBorders[1] = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile( m_nBorders[1], "hud/abilities/active_border", true, true);
	m_nBorders[2] = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile( m_nBorders[2], "hud/abilities/cooldown_border", true, true);
	m_nBorders[3] = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile( m_nBorders[3], "hud/abilities/passive_border", true, true);

	SetHiddenBits( HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT );
}

typedef struct
{
	float minProgressRadians;

	float vert1x;
	float vert1y;
	float vert2x;
	float vert2y;

	int swipe_dir_x;
	int swipe_dir_y;
} circular_progress_segment_ability;

circular_progress_segment_ability abilSegments[8] = 
{
	{ M_PI * 1.75,	1.0, 0.0, 0.5, 0.0, -1, 0 },
	{ M_PI * 1.5,	1.0, 0.5, 1.0, 0.0, 0, -1 },
	{ M_PI * 1.25,	1.0, 1.0, 1.0, 0.5, 0, -1 },
	{ M_PI,			0.5, 1.0, 1.0, 1.0, 1, 0 },
	{ M_PI * 0.75,	0.0, 1.0, 0.5, 1.0, 1, 0 },
	{ M_PI * 0.5,	0.0, 0.5, 0.0, 1.0, 0, 1 },
	{ M_PI * 0.25,	0.0, 0.0, 0.0, 0.5, 0, 1 },
	{ 0.0,			0.5, 0.0, 0.0, 0.0, -1, 0 },
};

void CHudAbils::DrawTextValue(int wide, int tall, int x, int y, float val)
{
	surface()->DrawSetTextFont(m_hTextFont);
	//draw our value
	wchar_t szText[ 8 ];
	if (val < 3.0f)
		V_swprintf_safe(szText, L"%.01f", val);
	else
		V_swprintf_safe(szText, L"%.00f", val);
	int tx = x+wide/2-UTIL_ComputeStringWidth(m_hTextFont,szText)/2;
	int ty = y+tall/2-surface()->GetFontTall(m_hTextFont)/2;
	surface()->DrawSetTextPos(tx-2, ty+2);
	surface()->DrawSetTextColor(m_AuxPowerColorShadow);
	surface()->DrawPrintText(szText, wcslen(szText));
	surface()->DrawSetTextPos(tx+2, ty-2);
	surface()->DrawPrintText(szText, wcslen(szText));
	surface()->DrawSetTextPos(tx, ty);
	surface()->DrawSetTextColor(m_AuxPowerColor);
	surface()->DrawPrintText(szText, wcslen(szText));
}

float CHudAbils::DrawAbility(int iAbilityNum, float y, float wide, float inset, float minset)
{
	C_BaseHLPlayer *pPlayer = (C_BaseHLPlayer *)C_BasePlayer::GetLocalPlayer();
	if (!pPlayer)
		return y;

	CovenAbilityInfo_t *info = GetCovenAbilityData((CovenAbility_t)pPlayer->m_HL2Local.covenAbilities[iAbilityNum]);
	if (info->abilityIconActive)
	{
		surface()->DrawSetTexture(info->abilityIconActive->textureId);
		//info->abilityIconActive->DrawSelf(minset, y + minset, wide - minset, y + wide - minset, Color(255, 255, 255, 255));
	}
	else
		surface()->DrawSetTexture(-1);
	surface()->DrawTexturedRect(minset, y + minset, wide - minset, y + wide - minset);
	//BB: no blips for now
	/*
	surface()->DrawSetTexture(m_nCircleBlip);
	for (int i=1; i <= pPlayer->m_HL2Local.covenCurrentLoadout1; i++)
	{
		surface()->DrawTexturedRect(wide-minset-10*i-2,y+wide-minset-10,wide-minset-10*i+6,y+wide-minset-2);
	}*/
	bool bNotGCD = true;
	float teatime = pPlayer->m_HL2Local.covenCooldownTimers[iAbilityNum];
	if (teatime <= pPlayer->m_HL2Local.covenGCD && !info->bPassive)
	{
		bNotGCD = false;
		teatime = pPlayer->m_HL2Local.covenGCD;
	}
	teatime -= gpGlobals->curtime;

	if (pPlayer->m_HL2Local.m_flSuitPower < info->flCost)
	{
		surface()->DrawSetTexture(m_nShadowTex);
		surface()->DrawTexturedRect(inset, y + inset, wide - inset, y + wide - inset);
		surface()->DrawSetTexture(m_nBorders[2]);
	}
	if (teatime > 0.0f)
	{
		if (pPlayer->m_HL2Local.m_flSuitPower >= info->flCost)
			DrawCircleSegment(minset, y + minset, wide - minset * 2.0f, wide - minset * 2.0f, teatime / max_duration[iAbilityNum], false);
		if (bNotGCD)
			DrawTextValue(wide, wide, 0, y, teatime);
		surface()->DrawSetTexture(m_nBorders[2]);
	}
	else if (!info->bPassive && pPlayer->m_HL2Local.m_flSuitPower >= info->flCost)
	{
		surface()->DrawSetTexture(m_nBorders[1]);
	}

	if (info->bPassive)
		surface()->DrawSetTexture(m_nBorders[3]);

	surface()->DrawTexturedRect(inset, y + inset, wide - inset, y + wide - inset);
	if (info->flCost > 0.0f)
	{
		wchar_t wNumTM[3];
		swprintf(wNumTM, sizeof(wNumTM), L"%.0f", info->flCost);
		DrawTextTitle(1.25f * inset, y, UTIL_ComputeStringWidth(m_hFontNumerals, wNumTM), wide - 1.7f * minset, wNumTM, false, m_hFontNumerals);
	}
	else if (info->flDrain > 0.0f)
	{
		wchar_t wNumTM[3];
		swprintf(wNumTM, sizeof(wNumTM), L"%.0f", info->flDrain);
		DrawTextTitle(1.25f * inset, y, UTIL_ComputeStringWidth(m_hFontNumerals, wNumTM), wide - 1.7f * minset, wNumTM, false, m_hFontNumerals);
	}
	//BB: TODO: HACK! fix this? This has moved to an item now.
	/*if (pPlayer->m_HL2Local.covenAbilities[iAbilityNum] == COVEN_ABILITY_TRIPMINE && pPlayer->m_HL2Local.m_iNumTripmines > 0)
	{
		wchar_t wNumTM[3];
		swprintf(wNumTM, sizeof(wNumTM), L"%d", pPlayer->m_HL2Local.m_iNumTripmines);
		//Q_snprintf(numTM, sizeof(numTM), "%d", pPlayer->m_CovenBuilderLocal.m_iNumTripmines);
		//g_pVGuiLocalize->ConvertANSIToUnicode(numTM, wNumTM, sizeof(wNumTM));
		DrawTextTitle(wide - minset, y, UTIL_ComputeStringWidth(m_hFontNumerals, wNumTM) - minset, minset, wNumTM, false, m_hFontNumerals);
	}*/

	if (cl_coven_abilitytitles.GetBool())
	{
		wchar_t *tempString = g_pVGuiLocalize->Find(info->szPrintName);
		if (tempString)
			DrawTextTitle(0, y, wide, wide - inset, tempString, !info->bPassive && (teatime > 0.0f || pPlayer->m_HL2Local.m_flSuitPower < info->flCost));
		else
			DrawTextTitle(0, y, wide, wide - inset, L"MISSING_DATA", !info->bPassive && (teatime > 0.0f || pPlayer->m_HL2Local.m_flSuitPower < info->flCost));
	}
	return y + wide - minset;
}

void CHudAbils::DrawTextTitle(int x, int y, int wide, int tall, wchar_t *title, bool bCooldown, HFont useFont)
{
	if (useFont == NULL)
		surface()->DrawSetTextFont(m_hTextFontTitle);
	else
		surface()->DrawSetTextFont(useFont);
	int tx = x + wide * 0.5f - UTIL_ComputeStringWidth(m_hTextFontTitle, title) * 0.5f;
	int ty = y + tall/* + surface()->GetFontTall(m_hTextFontTitle) / 2*/;
	
	if (bCooldown)
		surface()->DrawSetTextColor(m_AuxPowerColorShadow);
	else
		surface()->DrawSetTextColor(m_AuxPowerColorShadow);

	surface()->DrawSetTextPos(tx - 1, ty + 1);
	surface()->DrawUnicodeString(title);
	surface()->DrawSetTextPos(tx + 1, ty - 1);
	surface()->DrawUnicodeString(title);
	
	if (bCooldown)
		surface()->DrawSetTextColor(m_AuxPowerColorGray);
	else
		surface()->DrawSetTextColor(m_AuxPowerColor);

	surface()->DrawSetTextPos(tx, ty);
	surface()->DrawUnicodeString(title);

}

void CHudAbils::DrawCircleSegment( int x, int y, int wide, int tall, float flEndProgress, bool bClockwise )
{
	float flWide = (float)wide;
	float flTall = (float)tall;

	float flHalfWide = (float)wide / 2;
	float flHalfTall = (float)tall / 2;

	float c_x = (float)x+flHalfWide;
	float c_y = (float)y+flHalfTall;

	surface()->DrawSetTexture( m_nShadowTex );

	// TODO - if we want to progress CCW, reverse a few things

	float flEndProgressRadians = flEndProgress * M_PI * 2;

	int cur_wedge = 0;
	for ( int i=0;i<8;i++ )
	{
		if ( flEndProgressRadians > abilSegments[cur_wedge].minProgressRadians)
		{
			vgui::Vertex_t v[3];

			// vert 0 is ( 0.5, 0.5 )
			v[0].m_Position.Init( c_x, c_y );
			v[0].m_TexCoord.Init( 0.5f, 0.5f );

			float flInternalProgress = flEndProgressRadians - abilSegments[cur_wedge].minProgressRadians;

			if ( flInternalProgress < ( M_PI / 4 ) )
			{
				// Calc how much of this slice we should be drawing

				if ( i % 2 == 1 )
				{
					flInternalProgress = ( M_PI / 4 ) - flInternalProgress;
				}

				float flTan = tan(flInternalProgress);
	
				float flDeltaX, flDeltaY;

				if ( i % 2 == 1 )
				{
					flDeltaX = ( flHalfWide - flHalfTall * flTan ) * abilSegments[i].swipe_dir_x;
					flDeltaY = ( flHalfTall - flHalfWide * flTan ) * abilSegments[i].swipe_dir_y;
				}
				else
				{
					flDeltaX = flHalfTall * flTan * abilSegments[i].swipe_dir_x;
					flDeltaY = flHalfWide * flTan * abilSegments[i].swipe_dir_y;
				}

				v[1].m_Position.Init( x + abilSegments[i].vert1x * flWide + flDeltaX, y + abilSegments[i].vert1y * flTall + flDeltaY );
				v[1].m_TexCoord.Init( abilSegments[i].vert1x + ( flDeltaX / flHalfWide ) * 0.5, abilSegments[i].vert1y + ( flDeltaY / flHalfTall ) * 0.5 );
			}
			else
			{
				// full segment, easy calculation
				v[1].m_Position.Init( c_x + flWide * ( abilSegments[i].vert2x - 0.5 ), c_y + flTall * ( abilSegments[i].vert2y - 0.5 ) );
				v[1].m_TexCoord.Init( abilSegments[i].vert2x, abilSegments[i].vert2y );
			}

			// vert 2 is ( abilSegments[i].vert1x, abilSegments[i].vert1y )
			v[2].m_Position.Init( c_x + flWide * ( abilSegments[i].vert1x - 0.5 ), c_y + flTall * ( abilSegments[i].vert1y - 0.5 ) );
			v[2].m_TexCoord.Init( abilSegments[i].vert1x, abilSegments[i].vert1y );

			surface()->DrawTexturedPolygon( 3, v );
		}

		cur_wedge++;
		if ( cur_wedge >= 8)
			cur_wedge = 0;
	}
}

void CHudAbils::PaintBackground()
{
	C_BaseHLPlayer *pPlayer = (C_BaseHLPlayer *)C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	int wide,tall;
	GetSize(wide,tall);

	int team = 0;
	if (pPlayer->GetTeamNumber() == COVEN_TEAMID_VAMPIRES)
		team = 1;

	surface()->DrawSetColor( Color(0,0,0,255) );
	surface()->DrawSetTexture(m_nBackgrounds[team]);
	surface()->DrawTexturedRect(0,0,wide,tall);
}

void CHudAbils::Paint()
{
	C_BaseHLPlayer *pPlayer = (C_BaseHLPlayer *)C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	int wide,tall;
	GetSize(wide,tall);

	float inset = wide * 0.15f;
	float minset = inset*1.3f;
	float y = 0.0f;

	int team = 0;
	if (pPlayer->GetTeamNumber() == COVEN_TEAMID_VAMPIRES)
		team = 1;

	for (int i = 0; i < COVEN_MAX_ABILITIES; i++)
		y = DrawAbility(i, y, wide, inset, minset);
}

void CHudAbils::CheckCooldown(int iAbilityNum)
{
	C_BaseHLPlayer *pPlayer = (C_BaseHLPlayer *)C_BasePlayer::GetLocalPlayer();
	if (!pPlayer)
		return;

	float timer = pPlayer->m_HL2Local.covenCooldownTimers[iAbilityNum];
	CovenAbilityInfo_t *info = GetCovenAbilityData((CovenAbility_t)pPlayer->m_HL2Local.covenAbilities[iAbilityNum]);
	if (timer < pPlayer->m_HL2Local.covenGCD && !info->bPassive)
		timer = pPlayer->m_HL2Local.covenGCD;
	timer -= gpGlobals->curtime;
	if (timer > max_duration[iAbilityNum])
	{
		cooledDown[iAbilityNum] = false;
		max_duration[iAbilityNum] = timer;
	}
	else if (timer <= 0.0f && !cooledDown[iAbilityNum])
	{
		max_duration[iAbilityNum] = 0.0f;
		cooledDown[iAbilityNum] = true;
		if (pPlayer->IsAlive() && cl_coven_cooldownblips.GetBool())
		{
			CLocalPlayerFilter filter;
			EmitSound_t params;
			params.m_pSoundName = "CooldownBlip";
			params.m_bIgnoreDSP = false;
			C_BaseEntity::EmitSound(filter, -1, params);
		}
	}
}

void CHudAbils::OnThink()
{
	C_BaseHLPlayer *pPlayer = (C_BaseHLPlayer *)C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	if (pPlayer->covenClassID < 1 || pPlayer->GetTeamNumber() < 2)
	{
		SetPaintEnabled(false);
		SetPaintBackgroundEnabled(false);
	}
	else
	{
		SetPaintEnabled(true);
		SetPaintBackgroundEnabled(true);
	}

	for (int i = 0; i < COVEN_MAX_ABILITIES; i++)
		CheckCooldown(i);
 
   BaseClass::OnThink();
}
