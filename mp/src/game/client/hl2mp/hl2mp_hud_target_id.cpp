//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: HUD Target ID element
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "hud.h"
#include "hudelement.h"
#include "c_hl2mp_player.h"
#include "c_playerresource.h"
#include "vgui_entitypanel.h"
#include "iclientmode.h"
#include "vgui/ILocalize.h"
#include "hl2mp_gamerules.h"
#include "purchaseitem_shared.h"
#include "coven_parse.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define PLAYER_HINT_DISTANCE	150
#define PLAYER_HINT_DISTANCE_SQ	(PLAYER_HINT_DISTANCE*PLAYER_HINT_DISTANCE)

static ConVar hud_centerid( "hud_centerid", "1" );
static ConVar hud_showtargetid( "hud_showtargetid", "1" );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTargetID : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CTargetID, vgui::Panel );

public:
	CTargetID( const char *pElementName );
	void Init( void );
	virtual void	ApplySchemeSettings( vgui::IScheme *scheme );
	virtual void	Paint( void );
	void VidInit( void );

private:
	Color			GetColorForTargetTeam( int iTeamNumber );

	vgui::HFont		m_hFont;
	int				m_iLastEntIndex;
	CHandle<C_CovenBuilding> lastItem;
	float			m_flLastChangeTime;
};

DECLARE_HUDELEMENT( CTargetID );

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTargetID::CTargetID( const char *pElementName ) :
	CHudElement( pElementName ), BaseClass( NULL, "TargetID" )
{
	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	m_hFont = g_hFontTrebuchet24;
	m_flLastChangeTime = 0;
	m_iLastEntIndex = 0;
	lastItem = NULL;

	SetHiddenBits( HIDEHUD_MISCSTATUS );
}

//-----------------------------------------------------------------------------
// Purpose: Setup
//-----------------------------------------------------------------------------
void CTargetID::Init( void )
{
};

void CTargetID::ApplySchemeSettings( vgui::IScheme *scheme )
{
	BaseClass::ApplySchemeSettings( scheme );

	m_hFont = scheme->GetFont( "TargetID", IsProportional() );

	SetPaintBackgroundEnabled( false );
}

//-----------------------------------------------------------------------------
// Purpose: clear out string etc between levels
//-----------------------------------------------------------------------------
void CTargetID::VidInit()
{
	CHudElement::VidInit();

	m_flLastChangeTime = 0;
	m_iLastEntIndex = 0;
}

Color CTargetID::GetColorForTargetTeam( int iTeamNumber )
{
	return GameResources()->GetTeamColor( iTeamNumber );
} 

//-----------------------------------------------------------------------------
// Purpose: Draw function for the element
//-----------------------------------------------------------------------------
void CTargetID::Paint()
{
#define MAX_ID_STRING 256
	wchar_t sIDString[ MAX_ID_STRING ];
	sIDString[0] = 0;

	C_HL2MP_Player *pPlayer = C_HL2MP_Player::GetLocalHL2MPPlayer();

	if ( !pPlayer )
		return;

	Color c;

	// Get our target's ent index
	int iEntIndex = pPlayer->GetIDTarget();

	if (iEntIndex != m_iLastEntIndex && lastItem != NULL)
	{
		lastItem->SetClientSideGlowEnabled(false);
		lastItem = NULL;
	}

	// Didn't find one?
	if ( !iEntIndex )
	{
		// Check to see if we should clear our ID
		if ( m_flLastChangeTime && (gpGlobals->curtime > (m_flLastChangeTime + 0.1f)) )
		{
			m_flLastChangeTime = 0;
			sIDString[0] = 0;
			m_iLastEntIndex = 0;
		}
		else
		{
			// Keep re-using the old one
			iEntIndex = m_iLastEntIndex;
		}
	}
	else
	{
		m_flLastChangeTime = gpGlobals->curtime;
		m_iLastEntIndex = iEntIndex;
	}

	// Is this an entindex sent by the server?
	if ( iEntIndex )
	{
		C_BasePlayer *pPlayer = static_cast<C_BasePlayer*>(cl_entitylist->GetEnt( iEntIndex ));
		C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();

		const char *printFormatString = NULL;
		wchar_t wszPlayerName[ MAX_PLAYER_NAME_LENGTH ];
		wchar_t wszHealthText[ 10 ];
		bool bShowHealth = false;
		bool bShowPlayerName = false;

		// Some entities we always want to check, cause the text may change
		// even while we're looking at it
		// Is it a player?
		if ( IsPlayerIndex( iEntIndex ) )
		{
			c = GetColorForTargetTeam( pPlayer->GetTeamNumber() );

			bShowPlayerName = true;
			g_pVGuiLocalize->ConvertANSIToUnicode( pPlayer->GetPlayerName(),  wszPlayerName, sizeof(wszPlayerName) );
			
			if ( HL2MPRules()->IsTeamplay() == true && pPlayer->InSameTeam(pLocalPlayer) )
			{
				printFormatString = "#Playerid_sameteam";
				bShowHealth = true;
			}
			//BB: we dont want to see name tags for stealthed people...
			else if (pPlayer->m_floatCloakFactor > 0.0f && !((pPlayer->GetFlags() & EF_NODRAW) > 0))
			{
				printFormatString = "#Playerid_diffteam";
			}
		

			if ( bShowHealth )
			{
				//BB: dont show the % sign in the health... we use HP not %... NOT ANY MORE!
				_snwprintf( wszHealthText, ARRAYSIZE(wszHealthText) - 1, L"%.0f",  ((float)pPlayer->GetHealth() / (float)pPlayer->GetMaxHealth()*100.0f ) );
				wszHealthText[ ARRAYSIZE(wszHealthText)-1 ] = '\0';
			}
		}
		else
		{
			CBaseEntity *pEnt = cl_entitylist->GetEnt(iEntIndex);
			if (pEnt != NULL)
			{
				C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
				if (pEnt->IsABuilding() && pEnt->GetTeamNumber() == pLocalPlayer->GetTeamNumber())
				{
					C_CovenBuilding *bldg = static_cast<C_CovenBuilding *>(pEnt);
					CovenBuildingInfo_t *info = GetCovenBuildingData(bldg->m_BuildingType);
					c = GetColorForTargetTeam(pEnt->GetTeamNumber());
					wchar_t wszBuildingName[MAX_PLAYER_NAME_LENGTH];
					wchar_t wszCostText[4];
					V_swprintf_safe(wszCostText, L"???");
					V_swprintf_safe(wszBuildingName, L"%s", g_pVGuiLocalize->Find(info->szPrintName));
					int cost = HL2MPRules()->CovenItemCost(bldg->m_BuildingType);
					if (cost >= 0)
					{
						V_swprintf_safe(wszCostText, L"%d", cost);
						if (!bldg->IsClientSideGlowEnabled())
						{
							bldg->SetClientSideGlowEnabled(true);
							color32 clr;
							clr.b = clr.r = 0;
							clr.g = clr.a = 255;

							bldg->ForceGlowEffect(clr, true, true, 750.0f);
							lastItem = bldg;
						}
						g_pVGuiLocalize->ConstructString(sIDString, sizeof(sIDString), g_pVGuiLocalize->Find("#ItemID"), 2, wszBuildingName, wszCostText);
					}
				}
			}
		}

		if ( printFormatString )
		{
			if ( bShowPlayerName && bShowHealth )
			{
				g_pVGuiLocalize->ConstructString( sIDString, sizeof(sIDString), g_pVGuiLocalize->Find(printFormatString), 2, wszPlayerName, wszHealthText );
			}
			else if ( bShowPlayerName )
			{
				g_pVGuiLocalize->ConstructString( sIDString, sizeof(sIDString), g_pVGuiLocalize->Find(printFormatString), 1, wszPlayerName );
			}
			else if ( bShowHealth )
			{
				g_pVGuiLocalize->ConstructString( sIDString, sizeof(sIDString), g_pVGuiLocalize->Find(printFormatString), 1, wszHealthText );
			}
			else
			{
				g_pVGuiLocalize->ConstructString( sIDString, sizeof(sIDString), g_pVGuiLocalize->Find(printFormatString), 0 );
			}
		}

		if ( sIDString[0] )
		{
			int wide, tall;
			int ypos = YRES(260);
			int xpos = XRES(10);

			vgui::surface()->GetTextSize( m_hFont, sIDString, wide, tall );

			if( hud_centerid.GetInt() == 0 )
			{
				ypos = YRES(420);
			}
			else
			{
				xpos = (ScreenWidth() - wide) / 2;
			}
			
			vgui::surface()->DrawSetTextFont( m_hFont );
			vgui::surface()->DrawSetTextPos( xpos, ypos );
			vgui::surface()->DrawSetTextColor( c );
			vgui::surface()->DrawPrintText( sIDString, wcslen(sIDString) );
		}
	}
}
