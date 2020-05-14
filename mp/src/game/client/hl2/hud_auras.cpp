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

struct aura_pic
{
	int aura;
	int text;
	float timer;
};

class CHudAuras : public CHudElement, public Panel
{
   DECLARE_CLASS_SIMPLE( CHudAuras, Panel );
 
   public:
   CHudAuras( const char *pElementName );
   void togglePrint();
   virtual void OnThink();
 
   protected:
   CUtlVector<aura_pic *> active_auras;
   virtual void Paint();

   void DrawCircleSegment( int x, int y, int wide, int tall, float flEndDegrees, bool clockwise /* = true */ );

   //void PaintMiniBackground(int x, int y, int wide, int tall);

   int m_nImportBlips[3];
   int m_nImportBlanks[2];

   int m_nImportCapPoint[2];
   int m_nImportLevel[2];
   int m_nImportSprint[2];
   int m_nImportFury[2];
   int m_nImportStats[2];
   int m_nImportBerserk[2];
   int m_nImportMasochist[2];
   int m_nImportGCheck[2];
   int m_nImportDodge[2];
   int m_nImportHH[2];
   int m_nImportBLust[2];
   int m_nImportSlow[2];
   int m_nImportStun[2];
   int m_nImportPhase[2];
   int m_nImportCTS[2];

   int m_nShadowTex;

   float humtime;

   float max_duration[COVEN_MAX_BUFFS];

	CPanelAnimationVar( vgui::HFont, m_hTextFont, "TextFont", "CovenHUD" );
};

DECLARE_HUDELEMENT( CHudAuras );

CHudAuras::CHudAuras( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudAuras" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );   

	SetPaintBorderEnabled(false);
	SetPaintBackgroundEnabled(false);

	humtime = 0.0f;

	Q_memset(max_duration, 0, sizeof(max_duration));
 
   //AW Create Texture for Looking around
   m_nImportBlips[0] = surface()->CreateNewTextureID();
   surface()->DrawSetTextureFile( m_nImportBlips[0], "hud/statuseffects/blip_left", true, true);

   m_nImportBlips[1] = surface()->CreateNewTextureID();
   surface()->DrawSetTextureFile( m_nImportBlips[1], "hud/statuseffects/blip_center", true, true);

   m_nImportBlips[2] = surface()->CreateNewTextureID();
   surface()->DrawSetTextureFile( m_nImportBlips[2], "hud/statuseffects/blip_right", true, true);

   m_nShadowTex = surface()->CreateNewTextureID();
   surface()->DrawSetTextureFile( m_nShadowTex, "hud/statuseffects/cooldown", true, true);

   m_nImportBlanks[0] = surface()->CreateNewTextureID();
   surface()->DrawSetTextureFile(m_nImportBlanks[0], "hud/statuseffects/s_blank", true, true);

   m_nImportBlanks[1] = surface()->CreateNewTextureID();
   surface()->DrawSetTextureFile(m_nImportBlanks[1], "hud/statuseffects/v_blank", true, true);



   m_nImportCapPoint[0] = surface()->CreateNewTextureID();
   surface()->DrawSetTextureFile( m_nImportCapPoint[0], "hud/statuseffects/star", true, true);

   m_nImportCapPoint[1] = surface()->CreateNewTextureID();
   surface()->DrawSetTextureFile( m_nImportCapPoint[1], "hud/statuseffects/star", true, true);

   m_nImportLevel[0] = surface()->CreateNewTextureID();
   surface()->DrawSetTextureFile( m_nImportLevel[0], "hud/statuseffects/s_level", true, true);

   m_nImportLevel[1] = surface()->CreateNewTextureID();
   surface()->DrawSetTextureFile( m_nImportLevel[1], "hud/statuseffects/v_level", true, true);

   m_nImportSprint[0] = surface()->CreateNewTextureID();
   surface()->DrawSetTextureFile( m_nImportSprint[0], "hud/statuseffects/haste", true, true);

   m_nImportFury[0] = surface()->CreateNewTextureID();
   surface()->DrawSetTextureFile( m_nImportFury[0], "hud/statuseffects/battleyell", true, true);

   m_nImportStats[0] = surface()->CreateNewTextureID();
   surface()->DrawSetTextureFile( m_nImportStats[0], "hud/statuseffects/stats", true, true);

   m_nImportStats[1] = surface()->CreateNewTextureID();
   surface()->DrawSetTextureFile(m_nImportStats[1], "hud/statuseffects/stats", true, true);

   m_nImportBerserk[1] = surface()->CreateNewTextureID();
   surface()->DrawSetTextureFile( m_nImportBerserk[1], "hud/statuseffects/berserk", true, true);

   m_nImportMasochist[1] = surface()->CreateNewTextureID();
   surface()->DrawSetTextureFile( m_nImportMasochist[1], "hud/statuseffects/masochist", true, true);

   m_nImportGCheck[0] = surface()->CreateNewTextureID();
   surface()->DrawSetTextureFile( m_nImportGCheck[0], "hud/statuseffects/gutcheck", true, true);

   m_nImportDodge[1] = surface()->CreateNewTextureID();
   surface()->DrawSetTextureFile( m_nImportDodge[1], "hud/statuseffects/ethereal", true, true);

   m_nImportHH[0] = surface()->CreateNewTextureID();
   surface()->DrawSetTextureFile( m_nImportHH[0], "hud/statuseffects/s_holywater" , true, true);

   m_nImportHH[1] = surface()->CreateNewTextureID();
   surface()->DrawSetTextureFile( m_nImportHH[1], "hud/statuseffects/v_holywater" , true, true);

   m_nImportBLust[1] = surface()->CreateNewTextureID();
   surface()->DrawSetTextureFile( m_nImportBLust[1], "hud/statuseffects/bloodlust" , true, true);

   m_nImportSlow[0] = surface()->CreateNewTextureID();
   surface()->DrawSetTextureFile( m_nImportSlow[0], "hud/statuseffects/slow", true, true);

   m_nImportSlow[1] = surface()->CreateNewTextureID();
   surface()->DrawSetTextureFile(m_nImportSlow[1], "hud/statuseffects/slow", true, true);

   m_nImportStun[0] = surface()->CreateNewTextureID();
   surface()->DrawSetTextureFile( m_nImportStun[0], "hud/statuseffects/stun", true, true);

   m_nImportStun[1] = surface()->CreateNewTextureID();
   surface()->DrawSetTextureFile( m_nImportStun[1], "hud/statuseffects/stun", true, true);

   m_nImportPhase[1] = surface()->CreateNewTextureID();
   surface()->DrawSetTextureFile( m_nImportPhase[1], "hud/statuseffects/phase", true, true);

   m_nImportCTS[0] = surface()->CreateNewTextureID();
   surface()->DrawSetTextureFile( m_nImportCTS[0], "hud/statuseffects/s_cts", true, true);

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
} circular_progress_segment_coven;

circular_progress_segment_coven auraSegments[8] = 
{
	{ 0.0,			0.5, 0.0, 1.0, 0.0, 1, 0 },
	{ M_PI * 0.25,	1.0, 0.0, 1.0, 0.5, 0, 1 },
	{ M_PI * 0.5,	1.0, 0.5, 1.0, 1.0, 0, 1 },
	{ M_PI * 0.75,	1.0, 1.0, 0.5, 1.0, -1, 0 },
	{ M_PI,			0.5, 1.0, 0.0, 1.0, -1, 0 },
	{ M_PI * 1.25,	0.0, 1.0, 0.0, 0.5, 0, -1 },
	{ M_PI * 1.5,	0.0, 0.5, 0.0, 0.0, 0, -1 },
	{ M_PI * 1.75,	0.0, 0.0, 0.5, 0.0, 1, 0 },
};

void CHudAuras::DrawCircleSegment( int x, int y, int wide, int tall, float flEndProgress, bool bClockwise )
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
		if ( flEndProgressRadians > auraSegments[cur_wedge].minProgressRadians)
		{
			vgui::Vertex_t v[3];

			// vert 0 is ( 0.5, 0.5 )
			v[0].m_Position.Init( c_x, c_y );
			v[0].m_TexCoord.Init( 0.5f, 0.5f );

			float flInternalProgress = flEndProgressRadians - auraSegments[cur_wedge].minProgressRadians;

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
					flDeltaX = ( flHalfWide - flHalfTall * flTan ) * auraSegments[i].swipe_dir_x;
					flDeltaY = ( flHalfTall - flHalfWide * flTan ) * auraSegments[i].swipe_dir_y;
				}
				else
				{
					flDeltaX = flHalfTall * flTan * auraSegments[i].swipe_dir_x;
					flDeltaY = flHalfWide * flTan * auraSegments[i].swipe_dir_y;
				}

				v[2].m_Position.Init( x + auraSegments[i].vert1x * flWide + flDeltaX, y + auraSegments[i].vert1y * flTall + flDeltaY );
				v[2].m_TexCoord.Init( auraSegments[i].vert1x + ( flDeltaX / flHalfWide ) * 0.5, auraSegments[i].vert1y + ( flDeltaY / flHalfTall ) * 0.5 );
			}
			else
			{
				// full segment, easy calculation
				v[2].m_Position.Init( c_x + flWide * ( auraSegments[i].vert2x - 0.5 ), c_y + flTall * ( auraSegments[i].vert2y - 0.5 ) );
				v[2].m_TexCoord.Init( auraSegments[i].vert2x, auraSegments[i].vert2y );
			}

			// vert 2 is ( auraSegments[i].vert1x, auraSegments[i].vert1y )
			v[1].m_Position.Init( c_x + flWide * ( auraSegments[i].vert1x - 0.5 ), c_y + flTall * ( auraSegments[i].vert1y - 0.5 ) );
			v[1].m_TexCoord.Init( auraSegments[i].vert1x, auraSegments[i].vert1y );

			surface()->DrawTexturedPolygon( 3, v );
		}

		cur_wedge++;
		if ( cur_wedge >= 8)
			cur_wedge = 0;
	}
}

/*void CHudAuras::PaintMiniBackground(int x, int y, int wide, int tall)
{
	int cornerWide, cornerTall;
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
		surface()->DrawTexturedRect(x + wide - cornerWide, y + tall - cornerTall, x + wide, y + tall);
}*/

void CHudAuras::Paint()
{
	C_BaseHLPlayer *pPlayer = (C_BaseHLPlayer *)C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;
	int x = 0;
	int w,t;
	GetSize(w,t);

	int teamnum = 0;
	if (pPlayer->GetTeamNumber() == COVEN_TEAMID_VAMPIRES)
		teamnum = 1;

	for (int i = 0; i < active_auras.Count(); i++)
	{
		Color blk(0,0,0,250);
		surface()->DrawSetTextColor(Color(120,120,120,250));
		surface()->DrawSetColor( blk );
		//PaintMiniBackground(x-6,0,44,t+44);

		if (active_auras[i]->aura == COVEN_BUFF_CAPPOINT)
		{
			surface()->DrawSetTexture( m_nImportCapPoint[teamnum] );
		}
		else if (active_auras[i]->aura == COVEN_BUFF_LEVEL)
		{
			surface()->DrawSetTexture( m_nImportLevel[teamnum] );
		}
		else if (active_auras[i]->aura == COVEN_BUFF_SPRINT)
		{
			surface()->DrawSetTexture( m_nImportSprint[teamnum] );
		}
		else if (active_auras[i]->aura == COVEN_BUFF_BYELL)
		{
			surface()->DrawSetTexture( m_nImportFury[teamnum] );
		}
		else if (active_auras[i]->aura == COVEN_BUFF_STATS)
		{
			surface()->DrawSetTexture( m_nImportStats[teamnum] );
		}
		else if (active_auras[i]->aura == COVEN_BUFF_BERSERK)
		{
			surface()->DrawSetTexture( m_nImportBerserk[teamnum] );
		}
		else if (active_auras[i]->aura == COVEN_BUFF_MASOCHIST)
		{
			surface()->DrawSetTexture(m_nImportMasochist[teamnum]);
		}
		else if (active_auras[i]->aura == COVEN_BUFF_GCHECK)
		{
			surface()->DrawSetTexture( m_nImportGCheck[teamnum] );
		}
		else if (active_auras[i]->aura == COVEN_BUFF_DODGE)
		{
			surface()->DrawSetTexture(m_nImportDodge[teamnum]);
		}
		else if (active_auras[i]->aura == COVEN_BUFF_HOLYWATER)
		{
			surface()->DrawSetTexture( m_nImportHH[teamnum] );
		}
		else if (active_auras[i]->aura == COVEN_BUFF_BLUST)
		{
			surface()->DrawSetTexture(m_nImportBLust[teamnum]);
		}
		else if (active_auras[i]->aura == COVEN_BUFF_SLOW)
		{
			surface()->DrawSetTexture( m_nImportSlow[teamnum] );
		}
		else if (active_auras[i]->aura == COVEN_BUFF_STUN)
		{
			surface()->DrawSetTexture( m_nImportStun[teamnum] );
		}
		else if (active_auras[i]->aura == COVEN_BUFF_PHASE)
		{
			surface()->DrawSetTexture( m_nImportPhase[teamnum] );
		}
		else if (active_auras[i]->aura == COVEN_BUFF_CTS)
		{
			surface()->DrawSetTexture( m_nImportCapPoint[teamnum] );
		}

		int size = 0;
		if (active_auras[i]->timer > 0.0f && active_auras[i]->timer < 4.2f && active_auras[i]->timer > 0.5f)
		{
			float part = active_auras[i]->timer - (int)active_auras[i]->timer;
			if (part == 0.0f)
				size = 6;
			else if (part < 0.2f)
				size = 0.2f/part;
			else if (part > 0.6f)
				size = (part-0.6f)/0.08f;

			if (size > 6)
				size = 6;
			if (size < 0)
				size = 0;
		}

		surface()->DrawTexturedRect( x+6-size, 7-size, x+70+size, 71+size );

		if (active_auras[i]->timer > 0.0f)
			DrawCircleSegment(x+6-size, 7-size, 64+2*size, 64+2*size, 1.0f-active_auras[i]->timer/max_duration[active_auras[i]->aura], true);

		//BB: TODO: use surface()->GetFontTall
		if (active_auras[i]->text)
		{
			surface()->DrawSetTexture( m_nImportBlips[0] );
			surface()->DrawTexturedRect( x+57, 0, x+69, 24 );
			surface()->DrawSetTexture( m_nImportBlips[1] );
			surface()->DrawTexturedRect( x+69, 0, x+77, 24 );
			surface()->DrawSetTexture( m_nImportBlips[2] );
			surface()->DrawTexturedRect( x+77, 0, x+89, 24 );
			wchar_t uc_text[4];
			swprintf(uc_text, sizeof(uc_text), L"%d", active_auras[i]->text);
			surface()->DrawSetTextFont(m_hTextFont);
			surface()->DrawSetTextPos( x+73-UTIL_ComputeStringWidth(m_hTextFont,uc_text)/2, 12-surface()->GetFontTall(m_hTextFont)/2 );
			surface()->DrawUnicodeString(uc_text);
		}
		if (active_auras[i]->timer > 0.0f)
		{
			surface()->DrawSetTexture( m_nImportBlips[0] );
			surface()->DrawTexturedRect( x+32, 65, x+44, 89 );
			surface()->DrawSetTexture( m_nImportBlips[1] );
			surface()->DrawTexturedRect( x+44, 65, x+68, 89 );
			surface()->DrawSetTexture( m_nImportBlips[2] );
			surface()->DrawTexturedRect( x+68, 65, x+80, 89 );
			wchar_t uc_text[8];
			swprintf(uc_text, sizeof(uc_text), L"%.1f", active_auras[i]->timer);
			surface()->DrawSetTextFont(m_hTextFont);
			surface()->DrawSetTextPos( x+56-UTIL_ComputeStringWidth(m_hTextFont,uc_text)/2, 77-surface()->GetFontTall(m_hTextFont)/2 );
			surface()->DrawUnicodeString(uc_text);
		}

		x += 89;//t=32
	}
}

void CHudAuras::OnThink()
{
	C_BaseHLPlayer *pPlayer = (C_BaseHLPlayer *)C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	active_auras.RemoveAll();

	if (pPlayer->covenStatusEffects & COVEN_FLAG_LEVEL)
	{
		int lev = min(pPlayer->covenLevelCounter, COVEN_MAX_LEVEL);
		aura_pic *temp;
		temp = new aura_pic;
		temp->aura = COVEN_BUFF_LEVEL;
		temp->text = lev-pPlayer->m_HL2Local.covenCurrentPointsSpent;
		temp->timer = 0.0f;
		active_auras.AddToTail(temp);
	}
	if (pPlayer->covenStatusEffects & COVEN_FLAG_CAPPOINT)
	{
		aura_pic *temp;
		temp = new aura_pic;
		temp->aura = COVEN_BUFF_CAPPOINT;
		temp->text = 0;
		temp->timer = 0.0f;
		active_auras.AddToTail(temp);

		if (gpGlobals->curtime > humtime)
		{
			humtime = 0.0f;
		}
		if (humtime == 0.0f)
		{
			CLocalPlayerFilter filter;
			C_BaseEntity::EmitSound(filter, -1, "Cappoint.Hum");
			humtime = gpGlobals->curtime + 30.0f;
		}
	}
	else
	{
		if (humtime > 0.0f)
			C_BaseEntity::StopSound(-1, "Cappoint.Hum");
		humtime = 0.0f;
	}

	if (pPlayer->covenStatusEffects & COVEN_FLAG_SPRINT)
	{
		aura_pic *temp;
		temp = new aura_pic;
		temp->aura = COVEN_BUFF_SPRINT;
		temp->text = pPlayer->m_HL2Local.covenStatusMagnitude[COVEN_BUFF_SPRINT];
		temp->timer = pPlayer->m_HL2Local.covenStatusTimers[COVEN_BUFF_SPRINT]-gpGlobals->curtime;
		active_auras.AddToTail(temp);
		if (temp->timer > max_duration[COVEN_BUFF_SPRINT])
			max_duration[COVEN_BUFF_SPRINT] = temp->timer;
	}
	if (pPlayer->covenStatusEffects & COVEN_FLAG_BYELL)
	{
		aura_pic *temp;
		temp = new aura_pic;
		temp->aura = COVEN_BUFF_BYELL;
		temp->text = pPlayer->m_HL2Local.covenStatusMagnitude[COVEN_BUFF_BYELL];
		temp->timer = pPlayer->m_HL2Local.covenStatusTimers[COVEN_BUFF_BYELL]-gpGlobals->curtime;
		active_auras.AddToTail(temp);
		if (temp->timer > max_duration[COVEN_BUFF_BYELL])
			max_duration[COVEN_BUFF_BYELL] = temp->timer;
	}
	if (pPlayer->covenStatusEffects & COVEN_FLAG_STATS)
	{
		aura_pic *temp;
		temp = new aura_pic;
		temp->aura = COVEN_BUFF_STATS;
		temp->text = pPlayer->m_HL2Local.covenStatusMagnitude[COVEN_BUFF_STATS];
		temp->timer = pPlayer->m_HL2Local.covenStatusTimers[COVEN_BUFF_STATS]-gpGlobals->curtime;
		active_auras.AddToTail(temp);
		if (temp->timer > max_duration[COVEN_BUFF_STATS])
			max_duration[COVEN_BUFF_STATS] = temp->timer;
	}
	if (pPlayer->covenStatusEffects & COVEN_FLAG_BERSERK)
	{
		aura_pic *temp;
		temp = new aura_pic;
		temp->aura = COVEN_BUFF_BERSERK;
		temp->text = pPlayer->m_HL2Local.covenStatusMagnitude[COVEN_BUFF_BERSERK];
		temp->timer = pPlayer->m_HL2Local.covenStatusTimers[COVEN_BUFF_BERSERK]-gpGlobals->curtime;
		active_auras.AddToTail(temp);
		if (temp->timer > max_duration[COVEN_BUFF_BERSERK])
			max_duration[COVEN_BUFF_BERSERK] = temp->timer;
	}
	if (pPlayer->covenStatusEffects & COVEN_FLAG_MASOCHIST)
	{
		aura_pic *temp;
		temp = new aura_pic;
		temp->aura = COVEN_BUFF_MASOCHIST;
		temp->text = pPlayer->m_HL2Local.covenStatusMagnitude[COVEN_BUFF_MASOCHIST];
		temp->timer = pPlayer->m_HL2Local.covenStatusTimers[COVEN_BUFF_MASOCHIST]-gpGlobals->curtime;
		active_auras.AddToTail(temp);
		if (temp->timer > max_duration[COVEN_BUFF_MASOCHIST])
			max_duration[COVEN_BUFF_MASOCHIST] = temp->timer;
	}
	if (pPlayer->covenStatusEffects & COVEN_FLAG_GCHECK)
	{
		aura_pic *temp;
		temp = new aura_pic;
		temp->aura = COVEN_BUFF_GCHECK;
		temp->text = 0;
		temp->timer = 0;
		active_auras.AddToTail(temp);
	}
	if (pPlayer->covenStatusEffects & COVEN_FLAG_DODGE)
	{
		aura_pic *temp;
		temp = new aura_pic;
		temp->aura = COVEN_BUFF_DODGE;
		temp->text = 0;
		temp->timer = 0;
		active_auras.AddToTail(temp);
	}
	if (pPlayer->covenStatusEffects & COVEN_FLAG_HOLYWATER)
	{
		aura_pic *temp;
		temp = new aura_pic;
		temp->aura = COVEN_BUFF_HOLYWATER;
		temp->text = pPlayer->m_HL2Local.covenStatusMagnitude[COVEN_BUFF_HOLYWATER];
		temp->timer = 0;
		active_auras.AddToTail(temp);
	}
	if (pPlayer->covenStatusEffects & COVEN_FLAG_BLUST)
	{
		aura_pic *temp;
		temp = new aura_pic;
		temp->aura = COVEN_BUFF_BLUST;
		temp->text = pPlayer->m_HL2Local.covenStatusMagnitude[COVEN_BUFF_BLUST];
		temp->timer = pPlayer->m_HL2Local.covenStatusTimers[COVEN_BUFF_BLUST]-gpGlobals->curtime;
		active_auras.AddToTail(temp);
		if (temp->timer > max_duration[COVEN_BUFF_BLUST])
			max_duration[COVEN_BUFF_BLUST] = temp->timer;
	}
	if (pPlayer->covenStatusEffects & COVEN_FLAG_SLOW)
	{
		aura_pic *temp;
		temp = new aura_pic;
		temp->aura = COVEN_BUFF_SLOW;
		temp->text = pPlayer->m_HL2Local.covenStatusMagnitude[COVEN_BUFF_SLOW];
		temp->timer = pPlayer->m_HL2Local.covenStatusTimers[COVEN_BUFF_SLOW]-gpGlobals->curtime;
		active_auras.AddToTail(temp);
		if (temp->timer > max_duration[COVEN_BUFF_SLOW])
			max_duration[COVEN_BUFF_SLOW] = temp->timer;
	}
	if (pPlayer->covenStatusEffects & COVEN_FLAG_STUN)
	{
		aura_pic *temp;
		temp = new aura_pic;
		temp->aura = COVEN_BUFF_STUN;
		temp->text = 0;
		temp->timer = pPlayer->m_HL2Local.covenStatusTimers[COVEN_BUFF_STUN]-gpGlobals->curtime;
		active_auras.AddToTail(temp);
		if (temp->timer > max_duration[COVEN_BUFF_STUN])
			max_duration[COVEN_BUFF_STUN] = temp->timer;
	}
	if (pPlayer->covenStatusEffects & COVEN_FLAG_PHASE)
	{
		aura_pic *temp;
		temp = new aura_pic;
		temp->aura = COVEN_BUFF_PHASE;
		temp->text = 0;
		temp->timer = pPlayer->m_HL2Local.covenStatusTimers[COVEN_BUFF_PHASE]-gpGlobals->curtime;
		active_auras.AddToTail(temp);
		if (temp->timer > max_duration[COVEN_BUFF_PHASE])
			max_duration[COVEN_BUFF_PHASE] = temp->timer;
	}
	if (pPlayer->covenStatusEffects & COVEN_FLAG_CTS)
	{
		aura_pic *temp;
		temp = new aura_pic;
		temp->aura = COVEN_BUFF_CTS;
		temp->text = 0;
		temp->timer = 0;
		active_auras.AddToTail(temp);
	}

	if (active_auras.Count() == 0)
		SetPaintEnabled(false);
	else
		SetPaintEnabled(true);

	//SetSize(52*active_auras.Count()+10*(active_auras.Count()-1),32+32);
	SetSize(6+89*active_auras.Count(),89);
 
   BaseClass::OnThink();
}
