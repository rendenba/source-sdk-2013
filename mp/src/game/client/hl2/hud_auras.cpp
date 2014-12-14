#include "hudelement.h"
#include "hud_numericdisplay.h"
#include <vgui_controls/Panel.h>
#include "cbase.h"
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
   virtual void PaintBackground();

   void PaintMiniBackground(int x, int y, int wide, int tall);

   int m_nImportCapPoint;
   int m_nImportStar;
   int m_nImportSprint;
   int m_nImportFury;
   int m_nImportStats;
   int m_nImportBerserk;
   int m_nImportCoffin;
   int m_nImportBomb;
   int m_nImportHH;
   int m_nImportHH_de;
   int m_nImportSlow;
   int m_nImportStun;

   float humtime;

	CPanelAnimationVar( vgui::HFont, m_hTextFont, "TextFont", "Default" );
	CPanelAnimationVarAliasType( int, m_nCBgTextureId1, "Texture1", "vgui/hud/800corner1", "textureid" );
	CPanelAnimationVarAliasType( int, m_nCBgTextureId2, "Texture2", "vgui/hud/800corner2", "textureid" );
	CPanelAnimationVarAliasType( int, m_nCBgTextureId3, "Texture3", "vgui/hud/800corner3", "textureid" );
	CPanelAnimationVarAliasType( int, m_nCBgTextureId4, "Texture4", "vgui/hud/800corner4", "textureid" );
};

DECLARE_HUDELEMENT( CHudAuras );

CHudAuras::CHudAuras( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudAuras" )
{
   Panel *pParent = g_pClientMode->GetViewport();
   SetParent( pParent );   
 
   SetVisible( false );
   SetAlpha( 255 );

   humtime = 0.0f;
 
   //AW Create Texture for Looking around
   m_nImportCapPoint = surface()->CreateNewTextureID();
   surface()->DrawSetTextureFile( m_nImportCapPoint, "effects/prot", true, true);

   m_nImportStar = surface()->CreateNewTextureID();
   surface()->DrawSetTextureFile( m_nImportStar, "effects/hudstar", true, true);

   m_nImportSprint = surface()->CreateNewTextureID();
   surface()->DrawSetTextureFile( m_nImportSprint, "effects/guard", true, true);

   m_nImportFury = surface()->CreateNewTextureID();
   surface()->DrawSetTextureFile( m_nImportFury, "effects/fury", true, true);

   m_nImportStats = surface()->CreateNewTextureID();
   surface()->DrawSetTextureFile( m_nImportStats, "effects/banner", true, true);

   m_nImportBerserk = surface()->CreateNewTextureID();
   surface()->DrawSetTextureFile( m_nImportBerserk, "effects/berserk", true, true);

   m_nImportCoffin = surface()->CreateNewTextureID();
   surface()->DrawSetTextureFile( m_nImportCoffin, "effects/undying", true, true);

   m_nImportBomb = surface()->CreateNewTextureID();
   surface()->DrawSetTextureFile( m_nImportBomb, "effects/cowbomb", true, true);

   m_nImportHH = surface()->CreateNewTextureID();
   surface()->DrawSetTextureFile( m_nImportHH, "effects/holy_heal" , true, true);

   m_nImportHH_de = surface()->CreateNewTextureID();
   surface()->DrawSetTextureFile( m_nImportHH_de, "effects/holy_water" , true, true);

   m_nImportSlow = surface()->CreateNewTextureID();
   surface()->DrawSetTextureFile( m_nImportSlow, "effects/slow", true, true);

   m_nImportStun = surface()->CreateNewTextureID();
   surface()->DrawSetTextureFile( m_nImportStun, "effects/cowpain", true, true);

   SetHiddenBits( HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT );
	SetBgColor(Color(0,0,0,250));
}

void CHudAuras::PaintBackground()
{
	/*int w,t;
	GetSize(w,t);
	t -= 24;
	surface()->DrawSetColor( Color(0,0,0,125) );
	surface()->DrawFilledRect(0,0,w,t);*/
}

void CHudAuras::PaintMiniBackground(int x, int y, int wide, int tall)
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
}

void CHudAuras::Paint()
{
	C_BaseHLPlayer *pPlayer = (C_BaseHLPlayer *)C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;
	int x = 12;
	int w,t;
	GetSize(w,t);
	t -= 44;
	int y = t;
	SetPaintBorderEnabled(false);
	for (int i = 0; i < active_auras.Count(); i++)
	{
		surface()->DrawSetTextColor(Color(120,120,120,250));
		surface()->DrawSetColor( Color(0,0,0,250) );
		PaintMiniBackground(x-6,0,44,t+44);
		if (active_auras[i]->aura == COVEN_BUFF_CAPPOINT)
		{
			surface()->DrawSetTexture( m_nImportCapPoint );
		}
		else if (active_auras[i]->aura == COVEN_BUFF_LEVEL)
		{
			surface()->DrawSetTexture( m_nImportStar );
		}
		else if (active_auras[i]->aura == COVEN_BUFF_SPRINT)
		{
			surface()->DrawSetTexture( m_nImportSprint );
		}
		else if (active_auras[i]->aura == COVEN_BUFF_BYELL)
		{
			surface()->DrawSetTexture( m_nImportFury );
		}
		else if (active_auras[i]->aura == COVEN_BUFF_STATS)
		{
			surface()->DrawSetTexture( m_nImportStats );
		}
		else if (active_auras[i]->aura == COVEN_BUFF_BERSERK)
		{
			surface()->DrawSetTexture( m_nImportBerserk );
		}
		else if (active_auras[i]->aura == COVEN_BUFF_MASOCHIST)
		{
			surface()->DrawSetTexture( m_nImportCoffin );
		}
		else if (active_auras[i]->aura == COVEN_BUFF_GCHECK)
		{
			surface()->DrawSetTexture( m_nImportBomb );
		}
		else if (active_auras[i]->aura == COVEN_BUFF_HOLYWATER)
		{
			if (pPlayer->GetTeamNumber() == COVEN_TEAMID_SLAYERS)
				surface()->DrawSetTexture( m_nImportHH );
			else
				surface()->DrawSetTexture( m_nImportHH_de );
		}
		else if (active_auras[i]->aura == COVEN_BUFF_BLUST)
		{
			surface()->DrawSetTexture( m_nImportCoffin );
		}
		else if (active_auras[i]->aura == COVEN_BUFF_SLOW)
		{
			surface()->DrawSetTexture( m_nImportSlow );
		}
		else if (active_auras[i]->aura == COVEN_BUFF_STUN)
		{
			surface()->DrawSetTexture( m_nImportStun );
		}
		else if (active_auras[i]->aura == COVEN_BUFF_SOULENERGY)
		{
			surface()->DrawSetTexture( m_nImportBomb );
		}
		else if (active_auras[i]->aura == COVEN_BUFF_CTS)
		{
			surface()->DrawSetTexture( m_nImportStats );
		}

		surface()->DrawTexturedRect( x, 4, x+t, t+4 );

		int n = 0;

		if (active_auras[i]->text)
		{
			wchar_t uc_text[4];
			swprintf(uc_text, sizeof(uc_text), L"%d", active_auras[i]->text);
			surface()->DrawSetTextFont(m_hTextFont);
			//BB: maybe do something like this later.... helps with contrast!
			//surface()->DrawSetColor(Color(0,0,0,255));
			//surface()->DrawFilledRect(x+t/2-UTIL_ComputeStringWidth(m_hTextFont,uc_text)/2,t,x+t/2+UTIL_ComputeStringWidth(m_hTextFont,uc_text)/2,t+12);
			surface()->DrawSetTextPos( x+t/2-UTIL_ComputeStringWidth(m_hTextFont,uc_text)/2, t+4 );
			surface()->DrawUnicodeString(uc_text);
			n = 16;
		}
		if (active_auras[i]->timer > 0.0f)
		{
			wchar_t uc_text[8];
			swprintf(uc_text, sizeof(uc_text), L"%.1f", active_auras[i]->timer);
			surface()->DrawSetTextFont(m_hTextFont);
			//BB: maybe do something like this later.... helps with contrast!
			//surface()->DrawSetColor(Color(0,0,0,255));
			//surface()->DrawFilledRect(x+t/2-UTIL_ComputeStringWidth(m_hTextFont,uc_text)/2,t,x+t/2+UTIL_ComputeStringWidth(m_hTextFont,uc_text)/2,t+12);
			surface()->DrawSetTextPos( x+t/2-UTIL_ComputeStringWidth(m_hTextFont,uc_text)/2, t+n+4 );
			surface()->DrawUnicodeString(uc_text);
		}

		x += 56;//t=32
		y += 56;//t=32
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
		temp->text = 0;
		temp->timer = pPlayer->m_HL2Local.covenStatusTimers[COVEN_BUFF_SPRINT]-gpGlobals->curtime;
		active_auras.AddToTail(temp);
	}
	if (pPlayer->covenStatusEffects & COVEN_FLAG_BYELL)
	{
		aura_pic *temp;
		temp = new aura_pic;
		temp->aura = COVEN_BUFF_BYELL;
		temp->text = pPlayer->m_HL2Local.covenStatusMagnitude[COVEN_BUFF_BYELL];
		temp->timer = pPlayer->m_HL2Local.covenStatusTimers[COVEN_BUFF_BYELL]-gpGlobals->curtime;
		active_auras.AddToTail(temp);
	}
	if (pPlayer->covenStatusEffects & COVEN_FLAG_STATS)
	{
		aura_pic *temp;
		temp = new aura_pic;
		temp->aura = COVEN_BUFF_STATS;
		temp->text = pPlayer->m_HL2Local.covenStatusMagnitude[COVEN_BUFF_STATS];
		temp->timer = pPlayer->m_HL2Local.covenStatusTimers[COVEN_BUFF_STATS]-gpGlobals->curtime;
		active_auras.AddToTail(temp);
	}
	if (pPlayer->covenStatusEffects & COVEN_FLAG_BERSERK)
	{
		aura_pic *temp;
		temp = new aura_pic;
		temp->aura = COVEN_BUFF_BERSERK;
		temp->text = pPlayer->m_HL2Local.covenStatusMagnitude[COVEN_BUFF_BERSERK];
		temp->timer = pPlayer->m_HL2Local.covenStatusTimers[COVEN_BUFF_BERSERK]-gpGlobals->curtime;
		active_auras.AddToTail(temp);
	}
	if (pPlayer->covenStatusEffects & COVEN_FLAG_MASOCHIST)
	{
		aura_pic *temp;
		temp = new aura_pic;
		temp->aura = COVEN_BUFF_MASOCHIST;
		temp->text = pPlayer->m_HL2Local.covenStatusMagnitude[COVEN_BUFF_MASOCHIST];
		temp->timer = pPlayer->m_HL2Local.covenStatusTimers[COVEN_BUFF_MASOCHIST]-gpGlobals->curtime;
		active_auras.AddToTail(temp);
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
	}
	if (pPlayer->covenStatusEffects & COVEN_FLAG_SLOW)
	{
		aura_pic *temp;
		temp = new aura_pic;
		temp->aura = COVEN_BUFF_SLOW;
		temp->text = pPlayer->m_HL2Local.covenStatusMagnitude[COVEN_BUFF_SLOW];
		temp->timer = pPlayer->m_HL2Local.covenStatusTimers[COVEN_BUFF_SLOW]-gpGlobals->curtime;
		active_auras.AddToTail(temp);
	}
	if (pPlayer->covenStatusEffects & COVEN_FLAG_STUN)
	{
		aura_pic *temp;
		temp = new aura_pic;
		temp->aura = COVEN_BUFF_STUN;
		temp->text = 0;
		temp->timer = pPlayer->m_HL2Local.covenStatusTimers[COVEN_BUFF_STUN]-gpGlobals->curtime;
		active_auras.AddToTail(temp);
	}
	if (pPlayer->covenStatusEffects & COVEN_FLAG_SOULENERGY)
	{
		aura_pic *temp;
		temp = new aura_pic;
		temp->aura = COVEN_BUFF_SOULENERGY;
		temp->text = pPlayer->m_HL2Local.covenStatusMagnitude[COVEN_BUFF_SOULENERGY];
		temp->timer = 0;
		active_auras.AddToTail(temp);
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
		SetVisible(false);
	else
		SetVisible(true);

	//SetSize(52*active_auras.Count()+10*(active_auras.Count()-1),32+32);
	SetSize(56*active_auras.Count(),32+44);
 
   BaseClass::OnThink();
}