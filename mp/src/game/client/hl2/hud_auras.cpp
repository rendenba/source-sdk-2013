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

		surface()->DrawTexturedRect( x, 0, y, t );

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
		}

		x += 32;
		y += 32;
	}
}

void CHudAuras::OnThink()
{
	C_BaseHLPlayer *pPlayer = (C_BaseHLPlayer *)C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer )
		return;
	active_auras.RemoveAll();

	if (pPlayer->covenStatusEffects & COVEN_FLAG_CAPPOINT)
	{
		aura_pic *temp;
		temp = new aura_pic;
		temp->aura = COVEN_BUFF_CAPPOINT;
		temp->text = 0;
		active_auras.AddToTail(temp);
	}

	if (active_auras.Count() == 0)
		SetVisible(false);
	else
		SetVisible(true);

	SetSize(32*active_auras.Count(),32+24);
 
   BaseClass::OnThink();
}