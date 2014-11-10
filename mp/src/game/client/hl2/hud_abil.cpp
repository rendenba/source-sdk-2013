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

static wchar_t *abilities[2][COVEN_MAX_CLASSCOUNT][4] =
{{{L"Battle Yell",L"Bandage",L"Revenge",L""},{L"Sprint",L"Sheer Will",L"",L"Gut Check"},{L"Holy Water",L"Trip Mine",L"Reflexes",L""}},
{{L"Leap",L"",L"Sneak",L"Berserk"},{L"Phase",L"",L"Gorge",L""},{L"",L"Bloodlust",L"Masochist",L"Undying"}}};

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
   CUtlVector<ability_pic *> active_abils;
   virtual void Paint();
   virtual void PaintBackground();

   int m_nImportAbil;

	CPanelAnimationVar( vgui::HFont, m_hTextFont, "TextFont", "Default" );
};

DECLARE_HUDELEMENT( CHudAbils );

CHudAbils::CHudAbils( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudAbils" )
{
   Panel *pParent = g_pClientMode->GetViewport();
   SetParent( pParent );   
 
   SetVisible( false );
   SetAlpha( 255 );
 
   //AW Create Texture for Looking around
   m_nImportAbil = surface()->CreateNewTextureID();
   surface()->DrawSetTextureFile( m_nImportAbil, "effects/cowbomb", true, true);

   SetHiddenBits( HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT );
}

void CHudAbils::PaintBackground()
{
	int w,t;
	GetSize(w,t);
	t -= 36;
	surface()->DrawSetColor( Color(0,0,0,125) );
	surface()->DrawFilledRect(0,0,w,t);
}

void CHudAbils::Paint()
{
	C_BaseHLPlayer *pPlayer = (C_BaseHLPlayer *)C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;
	int x = 0;
	int w,t;
	GetSize(w,t);
	t -= 36;
	int y = t;
	int arti_w = 128;
	SetPaintBorderEnabled(false);
	for (int i = 0; i < active_abils.Count(); i++)
	{
		//surface()->DrawSetTexture( m_nImportAbil );

		//surface()->DrawTexturedRect( x, 0, x+t, t );

		wchar_t uc_texts[16];
		swprintf(uc_texts, L"%s", abilities[pPlayer->GetTeamNumber()-2][pPlayer->covenClassID-1][active_abils[i]->abil-1]);
		surface()->DrawSetTextFont(m_hTextFont);
		surface()->DrawSetTextPos( x+arti_w/2-UTIL_ComputeStringWidth(m_hTextFont,uc_texts)/2, 0 );
		surface()->DrawUnicodeString(uc_texts);

		int n = 0;

		if (active_abils[i]->text)
		{
			wchar_t uc_text[10];
			swprintf(uc_text, L"Rank %d", active_abils[i]->text);
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
			swprintf(uc_text, L"%.1f", active_abils[i]->timer);
			surface()->DrawSetTextFont(m_hTextFont);
			//BB: maybe do something like this later.... helps with contrast!
			//surface()->DrawSetColor(Color(0,0,0,255));
			//surface()->DrawFilledRect(x+t/2-UTIL_ComputeStringWidth(m_hTextFont,uc_text)/2,t,x+t/2+UTIL_ComputeStringWidth(m_hTextFont,uc_text)/2,t+12);
			surface()->DrawSetTextPos( x+arti_w/2-UTIL_ComputeStringWidth(m_hTextFont,uc_text)/2, t+n );
			surface()->DrawUnicodeString(uc_text);
		}

		x += 128;//t=32
		y += 128;//t=32
	}
}

void CHudAbils::OnThink()
{
	C_BaseHLPlayer *pPlayer = (C_BaseHLPlayer *)C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;
	active_abils.RemoveAll();

	if (pPlayer->m_HL2Local.covenCurrentLoadout1 > 0)
	{
		ability_pic *temp;
		temp = new ability_pic;
		temp->abil = 1;
		temp->text = pPlayer->m_HL2Local.covenCurrentLoadout1;
		temp->timer = pPlayer->m_HL2Local.covenCooldownTimers[0] - gpGlobals->curtime;
		active_abils.AddToTail(temp);
	}
	if (pPlayer->m_HL2Local.covenCurrentLoadout2 > 0)
	{
		ability_pic *temp;
		temp = new ability_pic;
		temp->abil = 2;
		temp->text = pPlayer->m_HL2Local.covenCurrentLoadout2;
		temp->timer = pPlayer->m_HL2Local.covenCooldownTimers[1] - gpGlobals->curtime;
		active_abils.AddToTail(temp);
	}
	if (pPlayer->m_HL2Local.covenCurrentLoadout3 > 0)
	{
		ability_pic *temp;
		temp = new ability_pic;
		temp->abil = 3;
		temp->text = pPlayer->m_HL2Local.covenCurrentLoadout3;
		temp->timer = pPlayer->m_HL2Local.covenCooldownTimers[2] - gpGlobals->curtime;
		active_abils.AddToTail(temp);
	}
	if (pPlayer->m_HL2Local.covenCurrentLoadout4 > 0)
	{
		ability_pic *temp;
		temp = new ability_pic;
		temp->abil = 4;
		temp->text = pPlayer->m_HL2Local.covenCurrentLoadout4;
		temp->timer = pPlayer->m_HL2Local.covenCooldownTimers[3] - gpGlobals->curtime;
		active_abils.AddToTail(temp);
	}

	if (active_abils.Count() == 0)
		SetVisible(false);
	else
		SetVisible(true);

	//SetSize(84*active_abils.Count()+20*(active_abils.Count()-1),84+32);
	SetSize(128*active_abils.Count(),84+36);
 
   BaseClass::OnThink();
}