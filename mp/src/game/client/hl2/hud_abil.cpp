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
 
#include "tier0/memdbgon.h" 
 
using namespace vgui;

//static wchar_t *abilities[2][COVEN_MAX_CLASSCOUNT][4] =
//{{{L"Battle Yell",L"Bandage",L"Revenge",L"Vengeful Soul"},{L"Sprint",L"Sheer Will",L"Intimidating Shout",L"Gut Check"},{L"Holy Water",L"Trip Mine",L"Reflexes",L"UV Light"}},
//{{L"Leap",L"Dodge",L"Sneak",L"Berserk"},{L"Phase",L"Charge",L"Gorge",L"Detonate Blood"},{L"Dread Scream",L"Bloodlust",L"Masochist",L"Undying"}}};

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
   int m_nAbils[2][COVEN_MAX_CLASSCOUNT][4][2];
   int m_nAbilBorders[2][COVEN_MAX_CLASSCOUNT][4][3];
   int m_nBorders[4];

   float max_duration[4];

   void LoadAbilityTex(int team, int classid, int num, const char *filename, int grayborder, int activeborder, int cooldownborder);

   void DrawCircleSegment( int x, int y, int wide, int tall, float flEndDegrees, bool clockwise /* = true */ );
   void DrawTextValue(int wide, int tall, int x, int y, float val);

	CPanelAnimationVar( vgui::HFont, m_hTextFont, "TextFont", "CovenHUDBoldXL" );
	CPanelAnimationVar( Color, m_AuxPowerColor, "AuxPowerColor", "255 255 255 255" );
	CPanelAnimationVar( Color, m_AuxPowerColorShadow, "AuxPowerColorShadow", "0 0 0 150" );
	//CPanelAnimationVarAliasType( int, m_nCBgTextureId1, "Texture1", "vgui/hud/800corner1", "textureid" );
	//CPanelAnimationVarAliasType( int, m_nCBgTextureId2, "Texture2", "vgui/hud/800corner2", "textureid" );
	//CPanelAnimationVarAliasType( int, m_nCBgTextureId3, "Texture3", "vgui/hud/800corner3", "textureid" );
	//CPanelAnimationVarAliasType( int, m_nCBgTextureId4, "Texture4", "vgui/hud/800corner4", "textureid" );
};

DECLARE_HUDELEMENT( CHudAbils );

void CHudAbils::LoadAbilityTex(int team, int classid, int num, const char *filename, int grayborder, int activeborder, int cooldownborder)
{
	char graytex[64];
	V_sprintf_safe(graytex, "%s_gray", filename);
	m_nAbils[team][classid-1][num][0] = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile( m_nAbils[team][classid-1][num][0], graytex, true, true);
	m_nAbils[team][classid-1][num][1] = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile( m_nAbils[team][classid-1][num][1], filename, true, true);
	m_nAbilBorders[team][classid-1][num][0] = grayborder;
	m_nAbilBorders[team][classid-1][num][1] = activeborder;
	m_nAbilBorders[team][classid-1][num][2] = cooldownborder;
}

CHudAbils::CHudAbils( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudAbils" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );   
 
	SetVisible( false );
	SetAlpha( 255 );

	Q_memset(max_duration, 0, sizeof(max_duration));

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

	LoadAbilityTex(0, COVEN_CLASSID_REAVER, 0, "hud/abilities/haste", m_nBorders[0], m_nBorders[1], m_nBorders[2]);
	LoadAbilityTex(0, COVEN_CLASSID_REAVER, 1, "hud/abilities/sheerwill", m_nBorders[0], m_nBorders[1], m_nBorders[2]);
	LoadAbilityTex(0, COVEN_CLASSID_REAVER, 2, "hud/abilities/gutcheck", m_nBorders[3], m_nBorders[3], m_nBorders[3]);
	LoadAbilityTex(0, COVEN_CLASSID_REAVER, 3, "hud/abilities/intshout", m_nBorders[0], m_nBorders[1], m_nBorders[2]);

	LoadAbilityTex(0, COVEN_CLASSID_HELLION, 0, "hud/abilities/holywater", m_nBorders[0], m_nBorders[1], m_nBorders[2]);
	LoadAbilityTex(0, COVEN_CLASSID_HELLION, 1, "hud/abilities/tripmine", m_nBorders[0], m_nBorders[1], m_nBorders[2]);
	LoadAbilityTex(0, COVEN_CLASSID_HELLION, 2, "hud/abilities/reflexes", m_nBorders[3], m_nBorders[3], m_nBorders[3]);
	LoadAbilityTex(0, COVEN_CLASSID_HELLION, 3, "hud/abilities/uvlight", m_nBorders[0], m_nBorders[1], m_nBorders[2]);

	LoadAbilityTex(0, COVEN_CLASSID_AVENGER, 0, "hud/abilities/battleyell", m_nBorders[0], m_nBorders[1], m_nBorders[2]);
	LoadAbilityTex(0, COVEN_CLASSID_AVENGER, 1, "hud/abilities/bandage", m_nBorders[0], m_nBorders[1], m_nBorders[2]);
	LoadAbilityTex(0, COVEN_CLASSID_AVENGER, 2, "hud/abilities/revenge", m_nBorders[3], m_nBorders[3], m_nBorders[3]);
	LoadAbilityTex(0, COVEN_CLASSID_AVENGER, 3, "hud/abilities/vengesoul", m_nBorders[0], m_nBorders[1], m_nBorders[2]);

	LoadAbilityTex(1, COVEN_CLASSID_FIEND, 0, "hud/abilities/leap", m_nBorders[0], m_nBorders[1], m_nBorders[2]);
	LoadAbilityTex(1, COVEN_CLASSID_FIEND, 1, "hud/abilities/berserk", m_nBorders[0], m_nBorders[1], m_nBorders[2]);
	LoadAbilityTex(1, COVEN_CLASSID_FIEND, 2, "hud/abilities/sneak", m_nBorders[3], m_nBorders[3], m_nBorders[3]);
	LoadAbilityTex(1, COVEN_CLASSID_FIEND, 3, "hud/abilities/dodge", m_nBorders[0], m_nBorders[1], m_nBorders[2]);

	LoadAbilityTex(1, COVEN_CLASSID_GORE, 0, "hud/abilities/scream", m_nBorders[0], m_nBorders[1], m_nBorders[2]);
	LoadAbilityTex(1, COVEN_CLASSID_GORE, 1, "hud/abilities/charge", m_nBorders[0], m_nBorders[1], m_nBorders[2]);
	LoadAbilityTex(1, COVEN_CLASSID_GORE, 2, "hud/abilities/gorge", m_nBorders[3], m_nBorders[3], m_nBorders[3]);
	LoadAbilityTex(1, COVEN_CLASSID_GORE, 3, "hud/abilities/phase", m_nBorders[0], m_nBorders[1], m_nBorders[2]);

	LoadAbilityTex(1, COVEN_CLASSID_DEGEN, 0, "hud/abilities/bloodlust", m_nBorders[0], m_nBorders[1], m_nBorders[2]);
	LoadAbilityTex(1, COVEN_CLASSID_DEGEN, 1, "hud/abilities/undying", m_nBorders[3], m_nBorders[3], m_nBorders[3]);
	LoadAbilityTex(1, COVEN_CLASSID_DEGEN, 2, "hud/abilities/masochist", m_nBorders[3], m_nBorders[3], m_nBorders[3]);
	LoadAbilityTex(1, COVEN_CLASSID_DEGEN, 3, "hud/abilities/detonate", m_nBorders[0], m_nBorders[1], m_nBorders[2]);

	SetHiddenBits( HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT );
	SetBgColor(Color(0,0,0,250));
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

	surface()->DrawSetColor( Color(0,0,0,250) );
	surface()->DrawSetTexture(m_nBackgrounds[team]);
	surface()->DrawTexturedRect(0,0,wide,tall);

	//surface()->DrawFilledRect(0,0,w,t+32);
	/*int cornerWide, cornerTall;
	GetCornerTextureSize( cornerWide, cornerTall );
	surface()->DrawFilledRect(x + cornerWide, y, x + wide - cornerWide,	y + cornerTall);
	surface()->DrawFilledRect(x, y + cornerTall, x + wide, y + tall - cornerTall);
	surface()->DrawFilledRect(x + cornerWide, y + tall - cornerTall, x + wide - cornerWide, y + tall);
	//TOP-LEFT
		surface()->DrawSetTexture(m_nCBgTextureId1);
		surface()->DrawTexturedRect(x, y, x + cornerWide, y + cornerTall);



	//TOP-RIGHT
		surface()->DrawSetTexture(m_nCBgTextureId2);
		surface()->DrawTexturedRect(x + wide - cornerWide, y, x + wide, y + cornerTall);


	//BOTTOM-LEFT
		surface()->DrawSetTexture(m_nCBgTextureId4);
		surface()->DrawTexturedRect(x + 0, y + tall - cornerTall, x + cornerWide, y + tall);



	//BOTTOM-RIGHT
		surface()->DrawSetTexture(m_nCBgTextureId3);
		surface()->DrawTexturedRect(x + wide - cornerWide, y + tall - cornerTall, x + wide, y + tall);*/
}

void CHudAbils::Paint()
{
	C_BaseHLPlayer *pPlayer = (C_BaseHLPlayer *)C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;
	if (pPlayer->GetTeamNumber() < 2 || pPlayer->covenClassID < 1)
		return;

	int wide,tall;
	GetSize(wide,tall);
	SetPaintBorderEnabled(false);

	float inset = wide * 0.15f;
	float minset = inset*1.3f;
	float y = 0.0f;

	int team = 0;
	if (pPlayer->GetTeamNumber() == COVEN_TEAMID_VAMPIRES)
		team = 1;

	int lev = 0;
	if (pPlayer->m_HL2Local.covenCurrentLoadout1 > 0)
		lev = 1;
	surface()->DrawSetTexture(m_nAbils[team][pPlayer->covenClassID-1][0][lev]);
	surface()->DrawTexturedRect(minset,y+minset,wide-minset,y+wide-minset);
	surface()->DrawSetTexture(m_nCircleBlip);
	for (int i=1; i <= pPlayer->m_HL2Local.covenCurrentLoadout1; i++)
	{
		surface()->DrawTexturedRect(wide-minset-10*i-2,y+wide-minset-10,wide-minset-10*i+6,y+wide-minset-2);
	}
	float teatime = pPlayer->m_HL2Local.covenCooldownTimers[0] - gpGlobals->curtime;
	if (teatime > 0.0f)
	{
		lev = 2;
		DrawCircleSegment(minset, y+minset, wide-minset*2.0f, wide-minset*2.0f, teatime/max_duration[0], false);
		DrawTextValue(wide, wide, 0, y, teatime);
	}
	surface()->DrawSetTexture(m_nAbilBorders[team][pPlayer->covenClassID-1][0][lev]);
	surface()->DrawTexturedRect(inset,y+inset,wide-inset,y+wide-inset);
	y += wide-minset;

	lev = 0;
	if (pPlayer->m_HL2Local.covenCurrentLoadout2 > 0)
		lev = 1;
	surface()->DrawSetTexture(m_nAbils[team][pPlayer->covenClassID-1][1][lev]);
	surface()->DrawTexturedRect(minset,y+minset,wide-minset,y+wide-minset);
	surface()->DrawSetTexture(m_nCircleBlip);
	for (int i=1; i <= pPlayer->m_HL2Local.covenCurrentLoadout2; i++)
	{
		surface()->DrawTexturedRect(wide-minset-10*i-2,y+wide-minset-10,wide-minset-10*i+6,y+wide-minset-2);
	}
	teatime = pPlayer->m_HL2Local.covenCooldownTimers[1] - gpGlobals->curtime;
	if (teatime > 0.0f)
	{
		lev = 2;
		DrawCircleSegment(minset, y+minset, wide-minset*2.0f, wide-minset*2.0f, teatime/max_duration[1], false);
		DrawTextValue(wide, wide, 0, y, teatime);
	}
	surface()->DrawSetTexture(m_nAbilBorders[team][pPlayer->covenClassID-1][1][lev]);
	surface()->DrawTexturedRect(inset,y+inset,wide-inset,y+wide-inset);
	y += wide-minset;

	lev = 0;
	if (pPlayer->m_HL2Local.covenCurrentLoadout3 > 0)
		lev = 1;
	surface()->DrawSetTexture(m_nAbils[team][pPlayer->covenClassID-1][2][lev]);
	surface()->DrawTexturedRect(minset,y+minset,wide-minset,y+wide-minset);
	surface()->DrawSetTexture(m_nCircleBlip);
	for (int i=1; i <= pPlayer->m_HL2Local.covenCurrentLoadout3; i++)
	{
		surface()->DrawTexturedRect(wide-minset-10*i-2,y+wide-minset-10,wide-minset-10*i+6,y+wide-minset-2);
	}
	teatime = pPlayer->m_HL2Local.covenCooldownTimers[2] - gpGlobals->curtime;
	if (teatime > 0.0f)
	{
		lev = 2;
		DrawCircleSegment(minset, y+minset, wide-minset*2.0f, wide-minset*2.0f, teatime/max_duration[2], false);
		DrawTextValue(wide, wide, 0, y, teatime);
	}
	surface()->DrawSetTexture(m_nAbilBorders[team][pPlayer->covenClassID-1][2][lev]);
	surface()->DrawTexturedRect(inset,y+inset,wide-inset,y+wide-inset);
	y += wide-minset;

	lev = 0;
	if (pPlayer->m_HL2Local.covenCurrentLoadout4 > 0)
		lev = 1;
	surface()->DrawSetTexture(m_nAbils[team][pPlayer->covenClassID-1][3][lev]);
	surface()->DrawTexturedRect(minset,y+minset,wide-minset,y+wide-minset);
	surface()->DrawSetTexture(m_nCircleBlip);
	for (int i=1; i <= pPlayer->m_HL2Local.covenCurrentLoadout4; i++)
	{
		surface()->DrawTexturedRect(wide-minset-10*i-2,y+wide-minset-10,wide-minset-10*i+6,y+wide-minset-2);
	}
	teatime = pPlayer->m_HL2Local.covenCooldownTimers[3] - gpGlobals->curtime;
	if (teatime > 0.0f)
	{
		lev = 2;
		DrawCircleSegment(minset, y+minset, wide-minset*2.0f, wide-minset*2.0f, teatime/max_duration[3], false);
		DrawTextValue(wide, wide, 0, y, teatime);
	}
	surface()->DrawSetTexture(m_nAbilBorders[team][pPlayer->covenClassID-1][3][lev]);
	surface()->DrawTexturedRect(inset,y+inset,wide-inset,y+wide-inset);
	y += wide-minset;
	/*for (int i = 0; i < active_abils.Count(); i++)
	{
		//surface()->DrawSetTexture( m_nImportAbil );

		//surface()->DrawTexturedRect( x, 0, x+t, t );

		wchar_t uc_texts[16];
		swprintf(uc_texts, sizeof(uc_texts), L"%s", abilities[pPlayer->GetTeamNumber()-2][pPlayer->covenClassID-1][active_abils[i]->abil-1]);
		surface()->DrawSetTextFont(m_hTextFont);
		surface()->DrawSetTextColor(Color(120,120,120,250));
		surface()->DrawSetTextPos( x+arti_w/2-UTIL_ComputeStringWidth(m_hTextFont,uc_texts)/2, 0 );
		surface()->DrawUnicodeString(uc_texts);

		int n = 0;

		if (active_abils[i]->text)
		{
			wchar_t uc_text[10];
			swprintf(uc_text, sizeof(uc_text), L"Rank %d", active_abils[i]->text);
			surface()->DrawSetTextFont(m_hTextFont);
			//BB: maybe do something like this later.... helps with contrast!
			//surface()->DrawSetColor(Color(0,0,0,255));
			//surface()->DrawFilledRect(x+t/2-UTIL_ComputeStringWidth(m_hTextFont,uc_text)/2,t,x+t/2+UTIL_ComputeStringWidth(m_hTextFont,uc_text)/2,t+12);
			surface()->DrawSetTextPos( x+arti_w/2-UTIL_ComputeStringWidth(m_hTextFont,uc_text)/2, t );
			surface()->DrawUnicodeString(uc_text);
			n = 16;
		}
		if (active_abils[i]->timer > 0.0f)
		{
			wchar_t uc_text[8];
			swprintf(uc_text, sizeof(uc_text), L"%.1f", active_abils[i]->timer);
			surface()->DrawSetTextFont(m_hTextFont);
			//BB: maybe do something like this later.... helps with contrast!
			//surface()->DrawSetColor(Color(0,0,0,255));
			//surface()->DrawFilledRect(x+t/2-UTIL_ComputeStringWidth(m_hTextFont,uc_text)/2,t,x+t/2+UTIL_ComputeStringWidth(m_hTextFont,uc_text)/2,t+12);
			surface()->DrawSetTextPos( x+arti_w/2-UTIL_ComputeStringWidth(m_hTextFont,uc_text)/2, t+n );
			surface()->DrawUnicodeString(uc_text);
		}

		x += 128;//t=32
		y += 128;//t=32
	}*/
}

void CHudAbils::OnThink()
{
	C_BaseHLPlayer *pPlayer = (C_BaseHLPlayer *)C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	float timer = pPlayer->m_HL2Local.covenCooldownTimers[0] - gpGlobals->curtime;
	if (timer > max_duration[0])
			max_duration[0] = timer;

	timer = pPlayer->m_HL2Local.covenCooldownTimers[1] - gpGlobals->curtime;
	if (timer > max_duration[1])
			max_duration[1] = timer;

	timer = pPlayer->m_HL2Local.covenCooldownTimers[2] - gpGlobals->curtime;
	if (timer > max_duration[2])
			max_duration[2] = timer;

	timer = pPlayer->m_HL2Local.covenCooldownTimers[3] - gpGlobals->curtime;
	if (timer > max_duration[3])
			max_duration[3] = timer;

	/*if (active_abils.Count() == 0)
	{
		SetPaintEnabled(false);
		SetVisible(false);
	}
	else
	{
		SetPaintEnabled(true);
		SetVisible(true);
	}*/

	//SetSize(84*active_abils.Count()+20*(active_abils.Count()-1),84+32);
	//SetSize(128*active_abils.Count(),84+36);
 
   BaseClass::OnThink();
}
