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

static ConVar cl_coven_statuseffecttitles("cl_coven_statuseffecttitles", "1", FCVAR_ARCHIVE, "Display status effect titles.");

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
   virtual void Paint();

   void DrawCircleSegment( int x, int y, int wide, int tall, float flEndDegrees, bool clockwise /* = true */ );
   void DrawBlipText(int x, int y, wchar_t *text, int fixedwidth = -1, int margin = 4);

   //void PaintMiniBackground(int x, int y, int wide, int tall);

   int m_nImportBlips[3];
   int m_nImportBlanks[2];

   int m_nShadowTex;

   float humtime;

   float max_duration[COVEN_STATUS_COUNT];

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

void CHudAuras::DrawBlipText(int x, int y, wchar_t *text, int fixedwidth, int margin)
{
	int stringwidth = UTIL_ComputeStringWidth(m_hTextFont, text) + margin * 2;
	int height = surface()->GetFontTall(m_hTextFont) * 1.5f;
	if (fixedwidth > 0)
	{
		stringwidth = fixedwidth + margin * 2;
	}
	int midwidth = stringwidth - 24;
	surface()->DrawSetTexture(m_nImportBlips[0]);
	surface()->DrawTexturedRect(x, y, x + 12, y + height);
	surface()->DrawSetTexture(m_nImportBlips[1]);
	surface()->DrawTexturedRect(x + 12, y, x + 12 + midwidth, y + height);
	surface()->DrawSetTexture(m_nImportBlips[2]);
	surface()->DrawTexturedRect(x + 12 + midwidth, y, x + 24 + midwidth, y + height);
	surface()->DrawSetTextFont(m_hTextFont);
	surface()->DrawSetTextPos(x + (stringwidth) / 2.0f - UTIL_ComputeStringWidth(m_hTextFont, text) / 2.0f, y + height / 2.0f - surface()->GetFontTall(m_hTextFont) / 2.0f);
	surface()->DrawUnicodeString(text);
}

void CHudAuras::Paint()
{
	C_BaseHLPlayer *pPlayer = (C_BaseHLPlayer *)C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	int x = 0;
	int w,t;
	GetSize(w,t);
	float picheight = 0.5f * t;
	float inset = 0.15f * picheight;
	float maxpicwidth = picheight + 2 * inset;
	int maxwidth = 0;
	int thiswidth = 0;
	int margin = UTIL_ComputeStringWidth(m_hTextFont, "0");

	int teamnum = 0;
	if (pPlayer->GetTeamNumber() == COVEN_TEAMID_VAMPIRES)
		teamnum = 1;

	for (int i = 0; i < COVEN_STATUS_COUNT; i++)
	{
		CovenStatus_t iStatus = (CovenStatus_t)i;
		if (pPlayer->HasStatus(iStatus))
		{
			thiswidth = maxpicwidth;
			CovenStatusEffectInfo_t *info = GetCovenStatusEffectData(iStatus);
			Color blk(0, 0, 0, 250);
			surface()->DrawSetTextColor(Color(120, 120, 120, 250));
			surface()->DrawSetColor(blk);
			int size = 0;
			float timer = 0.0f;
			if (info->bShowTimer)
			{
				timer = pPlayer->m_HL2Local.covenStatusTimers[iStatus] - gpGlobals->curtime;
				if (timer < 4.2f && timer > 0.5f)
				{
					float part = timer - (int)timer;
					if (part == 0.0f)
						size = inset;
					else if (part < 0.2f) //growing fast
						size = 0.2f / part;
					else if (part > 0.6f) //slow fade out
						size = (part - 0.6f) / 0.08f;

					size = clamp(size, 0.0f, inset);
				}
			}
			if (cl_coven_statuseffecttitles.GetInt() > 0)
			{
				wchar_t *tempString = g_pVGuiLocalize->Find(info->szPrintName);
				if (tempString)
				{
					int strwidth = UTIL_ComputeStringWidth(m_hTextFont, tempString);
					int width = strwidth + 2 * margin;
					if (width > thiswidth)
						thiswidth = width + 2 * inset;
					DrawBlipText(x + thiswidth / 2.0f - width / 2.0f, picheight + inset + surface()->GetFontTall(m_hTextFont), tempString, strwidth, margin);
				}
			}
			surface()->DrawSetTexture(info->statusIcon->textureId);
			int offsetx = inset - size + (thiswidth - maxpicwidth) / 2.0f;
			int offsety = inset - size;
			surface()->DrawTexturedRect(x + offsetx, offsety, x + offsetx + picheight + 2 * size, offsety + picheight + 2 * size);

			if (info->bShowTimer)
				if (timer > 0.0f)
					DrawCircleSegment(x + offsetx, offsety, picheight + 2 * size, picheight + 2 * size, 1.0f - timer / max_duration[iStatus], true);

			if (info->bShowMagnitude)
			{
				wchar_t uc_text[4];
				swprintf(uc_text, sizeof(uc_text), L"%d", pPlayer->m_HL2Local.covenStatusMagnitude[iStatus]);
				int width = UTIL_ComputeStringWidth(m_hTextFont, L"100");
				DrawBlipText(x + picheight - width, 0, uc_text, width, margin);
			}
			if (info->bShowTimer)
			{
				wchar_t uc_text[8];
				if (timer > 60.0f)
					swprintf(uc_text, sizeof(uc_text), L"%.0fm", timer / 60.0f);
				else if (timer > 10.0f)
					swprintf(uc_text, sizeof(uc_text), L"%.0f", timer);
				else
					swprintf(uc_text, sizeof(uc_text), L"%.1f", timer);
				int width = UTIL_ComputeStringWidth(m_hTextFont, L"100.0");
				DrawBlipText(x + picheight - width, picheight, uc_text, width, margin);
			}
			
			x += thiswidth;
			maxwidth += thiswidth;
		}
	}
	SetSize(maxwidth, t);
}

void CHudAuras::OnThink()
{
	C_BaseHLPlayer *pPlayer = (C_BaseHLPlayer *)C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;

	int iNumActiveAuras = 0;

	if (pPlayer->HasStatus(COVEN_STATUS_CAPPOINT))
	{
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

	for (int i = 0; i < COVEN_STATUS_COUNT; i++)
	{
		CovenStatus_t iStatus = (CovenStatus_t)i;
		if (pPlayer->HasStatus(iStatus))
		{
			iNumActiveAuras++;
			float timer = pPlayer->m_HL2Local.covenStatusTimers[iStatus] - gpGlobals->curtime;
			if (timer > max_duration[iStatus])
				max_duration[iStatus] = timer;
		}
	}

	if (iNumActiveAuras == 0)
		SetPaintEnabled(false);
	else
		SetPaintEnabled(true);

	//int wide, tall;
	//GetSize(wide, tall);
	//float scale = tall / 90.0f;
	//float picheight = tall * 0.575f; //50% + 15%
	//SetSize((picheight * iNumActiveAuras), tall);
 
   BaseClass::OnThink();
}
