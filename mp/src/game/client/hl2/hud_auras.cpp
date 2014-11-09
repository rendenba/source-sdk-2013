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

   int m_nImportCapPoint;
   int m_nImportStar;
   int m_nImportSprint;
   int m_nImportFury;
   int m_nImportStats;
   int m_nImportBerserk;
   int m_nImportCoffin;
   int m_nImportBomb;

	CPanelAnimationVar( vgui::HFont, m_hTextFont, "TextFont", "Default" );
};

DECLARE_HUDELEMENT( CHudAuras );

CHudAuras::CHudAuras( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudAuras" )
{
   Panel *pParent = g_pClientMode->GetViewport();
   SetParent( pParent );   
 
   SetVisible( false );
   SetAlpha( 255 );
 
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

   SetHiddenBits( HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT );
}

void CHudAuras::PaintBackground()
{
	int w,t;
	GetSize(w,t);
	t -= 24;
	surface()->DrawSetColor( Color(0,0,0,125) );
	surface()->DrawFilledRect(0,0,w,t);
}

void CHudAuras::Paint()
{
	int x = 0;
	int w,t;
	GetSize(w,t);
	t -= 24;
	int y = t;
	SetPaintBorderEnabled(false);
	for (int i = 0; i < active_auras.Count(); i++)
	{
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

		surface()->DrawTexturedRect( x, 0, x+t, t );

		int n = 0;

		if (active_auras[i]->text)
		{
			wchar_t uc_text[4];
			swprintf(uc_text, L"%d", active_auras[i]->text);
			surface()->DrawSetTextFont(m_hTextFont);
			//BB: maybe do something like this later.... helps with contrast!
			//surface()->DrawSetColor(Color(0,0,0,255));
			//surface()->DrawFilledRect(x+t/2-UTIL_ComputeStringWidth(m_hTextFont,uc_text)/2,t,x+t/2+UTIL_ComputeStringWidth(m_hTextFont,uc_text)/2,t+12);
			surface()->DrawSetTextPos( x+t/2-UTIL_ComputeStringWidth(m_hTextFont,uc_text)/2, t );
			surface()->DrawUnicodeString(uc_text);
			n = 12;
		}
		if (active_auras[i]->timer > 0.0f)
		{
			wchar_t uc_text[8];
			swprintf(uc_text, L"%.1f", active_auras[i]->timer);
			surface()->DrawSetTextFont(m_hTextFont);
			//BB: maybe do something like this later.... helps with contrast!
			//surface()->DrawSetColor(Color(0,0,0,255));
			//surface()->DrawFilledRect(x+t/2-UTIL_ComputeStringWidth(m_hTextFont,uc_text)/2,t,x+t/2+UTIL_ComputeStringWidth(m_hTextFont,uc_text)/2,t+12);
			surface()->DrawSetTextPos( x+t/2-UTIL_ComputeStringWidth(m_hTextFont,uc_text)/2, t+n );
			surface()->DrawUnicodeString(uc_text);
		}

		x += 42;//t=32
		y += 42;//t=32
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

	if (active_auras.Count() == 0)
		SetVisible(false);
	else
		SetVisible(true);

	SetSize(32*active_auras.Count()+10*(active_auras.Count()-1),32+24);
 
   BaseClass::OnThink();
}