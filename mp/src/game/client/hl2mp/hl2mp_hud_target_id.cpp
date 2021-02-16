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
#include "c_serverragdoll.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define PLAYER_HINT_DISTANCE	150
#define PLAYER_HINT_DISTANCE_SQ	(PLAYER_HINT_DISTANCE*PLAYER_HINT_DISTANCE)

static ConVar hud_centerid( "hud_centerid", "1" );
static ConVar hud_showtargetid( "hud_showtargetid", "1" );


extern ConVar sv_coven_hp_per_ragdoll;

class C_ServerRagdoll;

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
	vgui::HFont		m_hFontSmall;
	int				m_iLastEntIndex;
	CHandle<CBaseAnimating>			lastGlowObject;
	byte			m_GlowGoalColor;
	byte			m_GlowColor;
	float			m_flGlowTimer;
	float			m_flLastChangeTime;
	CPanelAnimationVarAliasType(float, m_flIconSize, "icon_size", "16.0", "proportional_float");
	CPanelAnimationVarAliasType(float, m_flIconPadding, "icon_padding", "2.0", "proportional_float");

	CPanelAnimationVar(Color, m_AuxPowerColor, "AuxPowerColor", "255 255 255 255");
	CPanelAnimationVar(Color, m_AuxPowerColorShadow, "AuxPowerColorShadow", "0 0 0 150");

	CPanelAnimationVarAliasType(float, m_flBarInsetX, "BarInsetX", "2.0", "proportional_float");
	CPanelAnimationVarAliasType(float, m_flBarHeight, "BarHeight", "5.0", "proportional_float");
	CPanelAnimationVarAliasType(float, m_flBarWidth, "BarWidth", "175.0", "proportional_float");

	CPanelAnimationVar(vgui::HFont, m_hTextFont, "TextFont", "CovenHUDBold");

	int m_nGlassTex[2];
	int m_nBlipTex;
	int m_nBlipLeft;
	int m_nBlipRight;
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
	m_hFontSmall = g_hFontTrebuchet24;
	m_flLastChangeTime = 0;
	m_iLastEntIndex = 0;
	lastGlowObject = NULL;
	m_GlowGoalColor = 255;
	m_GlowColor = 255;
	m_flGlowTimer = 0.0f;

	SetHiddenBits( HIDEHUD_MISCSTATUS );

	m_nGlassTex[0] = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile(m_nGlassTex[0], "hud/bars/s_glass_empty_big", true, true);

	m_nGlassTex[1] = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile(m_nGlassTex[1], "hud/bars/v_glass_empty_big", true, true);

	m_nBlipTex = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile(m_nBlipTex, "hud/bars/glass_h", true, true);

	m_nBlipRight = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile(m_nBlipRight, "hud/bars/glass_h_r", true, true);

	m_nBlipLeft = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile(m_nBlipLeft, "hud/bars/glass_h_l", true, true);
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
	m_hFontSmall = scheme->GetFont( "TargetIDSmall", IsProportional() );

	SetPaintBackgroundEnabled( true );
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
	wchar_t sIDString_line2[ MAX_ID_STRING ];
	wchar_t sIDString_line3[ MAX_ID_STRING ];
	sIDString[0] = 0;
	sIDString_line2[0] = 0;
	sIDString_line3[0] = 0;

	C_HL2MP_Player *pPlayer = C_HL2MP_Player::GetLocalHL2MPPlayer();

	if ( !pPlayer )
		return;

	Color c;

	// Get our target's ent index
	int iEntIndex = pPlayer->GetIDTarget();

	if (iEntIndex != m_iLastEntIndex && lastGlowObject != NULL)
	{
		lastGlowObject->SetClientSideGlowEnabled(false);
		m_GlowColor = m_GlowGoalColor = 255;
		lastGlowObject = NULL;
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

		if (!pPlayer)
			return;

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
			
			if (pLocalPlayer->GetTeamNumber() < COVEN_TEAMID_SLAYERS || (HL2MPRules()->IsTeamplay() == true && pPlayer->InSameTeam(pLocalPlayer)))
			{
				printFormatString = "#Playerid_sameteam";
				bShowHealth = true;
			}
			//BB: we dont want to see name tags for stealthed people...
			else if (pPlayer->m_floatCloakFactor == 0.0f && !((pPlayer->GetFlags() & EF_NODRAW) > 0))
			{
				printFormatString = "#Playerid_diffteam";
			}
		

			if ( bShowHealth )
			{
				//BB: dont show the % sign in the health... we use HP not %... NOT ANY MORE!
				_snwprintf( wszHealthText, ARRAYSIZE(wszHealthText) - 1, L"%d / %d",  pPlayer->GetHealth(), pPlayer->GetMaxHealth());
				wszHealthText[ ARRAYSIZE(wszHealthText)-1 ] = '\0';
				g_pVGuiLocalize->ConstructString(sIDString_line2, sizeof(sIDString_line2), g_pVGuiLocalize->Find("#Playerid_sameteamL2"), 1, wszHealthText);
			}
		}
		else
		{
			CBaseAnimating *pEnt = dynamic_cast<CBaseAnimating *>(cl_entitylist->GetEnt(iEntIndex));
			if (pEnt != NULL)
			{
				CHL2MP_Player *pLocalPlayer = ToHL2MPPlayer(CBasePlayer::GetLocalPlayer());
				if (pEnt->IsABuilding() && (pEnt->GetTeamNumber() == pLocalPlayer->GetTeamNumber() || pLocalPlayer->GetTeamNumber() < COVEN_TEAMID_SLAYERS))
				{
					CCovenBuilding *bldg = static_cast<CCovenBuilding *>(pEnt);
					CovenBuildingInfo_t *info = GetCovenBuildingData(bldg->m_BuildingType);
					int cost = HL2MPRules()->CovenItemCost(bldg->m_BuildingType);

					if (cost > 0 && pLocalPlayer->HasStatus(COVEN_STATUS_BUYZONE))
						cost = cost * pLocalPlayer->GetStatusMagnitude(COVEN_STATUS_BUYZONE) / 100.0f;

					if (cost >= 0 && pLocalPlayer->GetTeamNumber() > COVEN_TEAMID_SPECTATOR)
					{
						c = GetColorForTargetTeam(pEnt->GetTeamNumber());
						wchar_t wszBuildingName[MAX_PLAYER_NAME_LENGTH];
						wchar_t wszCostText[4];
						V_swprintf_safe(wszBuildingName, L"%s", g_pVGuiLocalize->Find(info->szPrintName));
						V_swprintf_safe(wszCostText, L"%d", cost);
						if (!bldg->IsClientSideGlowEnabled())
						{
							bldg->SetClientSideGlowEnabled(true);
							color32 clr;
							clr.b = clr.r = 0;
							clr.g = clr.a = 255;

							bldg->ForceGlowEffect(clr, true, true, 750.0f);
							lastGlowObject = bldg;
						}
						g_pVGuiLocalize->ConstructString(sIDString, sizeof(sIDString), g_pVGuiLocalize->Find("#ItemID"), 2, wszBuildingName, wszCostText);
					}
					else
					{
						CCovenBuilding *bldg = static_cast<CCovenBuilding *>(pEnt);
						if (bldg->mOwner != NULL)
						{
							c = GetColorForTargetTeam(pEnt->GetTeamNumber());
							wchar_t wszHealthString[MAX_PLAYER_NAME_LENGTH];
							wchar_t wszLevelString[3];
							V_swprintf_safe(wszHealthString, L"%d / %d", bldg->GetHealth(), bldg->GetMaxHealth());
							V_swprintf_safe(wszLevelString, L"%d", bldg->m_iLevel + 1);
							if (!bldg->IsClientSideGlowEnabled() && bldg->mOwner.Get() == pLocalPlayer)
							{
								bldg->SetClientSideGlowEnabled(true);
								color32 clr;
								clr.r = c.r();
								clr.g = c.g();
								clr.b = c.b();
								clr.a = c.a();

								bldg->ForceGlowEffect(clr, true, true, 750.0f);
								lastGlowObject = bldg;
							}
							if (IsPlayerIndex(bldg->mOwner->entindex()))
							{
								C_BasePlayer *pOwner = static_cast<C_BasePlayer*>(cl_entitylist->GetEnt(bldg->mOwner->entindex()));
								g_pVGuiLocalize->ConvertANSIToUnicode(pOwner->GetPlayerName(), wszPlayerName, sizeof(wszPlayerName));
								g_pVGuiLocalize->ConstructString(sIDString, sizeof(sIDString), g_pVGuiLocalize->Find("#BuildingID1"), 1, wszLevelString);
								g_pVGuiLocalize->ConstructString(sIDString_line2, sizeof(sIDString_line2), g_pVGuiLocalize->Find("#BuildingID2"), 1, wszPlayerName);
								g_pVGuiLocalize->ConstructString(sIDString_line3, sizeof(sIDString_line3), g_pVGuiLocalize->Find("#BuildingID3"), 1, wszHealthString);
							}
						}
					}
				}
				else if (pEnt->IsServerdoll() && pEnt->GetTeamNumber() == COVEN_TEAMID_SLAYERS && pLocalPlayer->GetTeamNumber() == COVEN_TEAMID_VAMPIRES)
				{
					C_BaseHLPlayer *pHL2Player = static_cast<C_BaseHLPlayer *>(pLocalPlayer);
					C_ServerRagdoll *pDoll = static_cast<C_ServerRagdoll *>(pEnt);
					int iLeft = sv_coven_hp_per_ragdoll.GetInt() - pHL2Player->m_HL2Local.m_iDollHP[pDoll->iSlot];
					if (iLeft > 0)
					{
						float flPercentLeft = (float)iLeft / sv_coven_hp_per_ragdoll.GetInt() * 100.0f;
						wchar_t wszVitalityText[4];
						V_swprintf_safe(wszVitalityText, L"%.0f", flPercentLeft);
						g_pVGuiLocalize->ConstructString(sIDString, sizeof(sIDString), g_pVGuiLocalize->Find("#FeedID"), 1, wszVitalityText);
						c = GetColorForTargetTeam(pLocalPlayer->GetTeamNumber());
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
			int ypos = YRES(300);
			int xpos = XRES(10);

			vgui::surface()->GetTextSize( m_hFont, sIDString, wide, tall );

			if( hud_centerid.GetInt() == 0 )
			{
				ypos = YRES(422);
			}
			else
			{
				xpos = (ScreenWidth() - wide) / 2;
			}
			
			vgui::surface()->DrawSetTextFont( m_hFont );
			vgui::surface()->DrawSetTextPos( xpos, ypos );
			vgui::surface()->DrawSetTextColor( c );
			vgui::surface()->DrawPrintText( sIDString, wcslen(sIDString) );
			ypos += tall;
			if (sIDString_line2[0])
			{
				vgui::surface()->DrawSetTextFont(m_hFontSmall);
				vgui::surface()->GetTextSize(m_hFontSmall, sIDString_line2, wide, tall);
				if (bShowHealth)
				{
					tall += m_flBarHeight;
					xpos = (ScreenWidth() - m_flBarWidth) / 2;
					int end_y = ypos + tall;
					float maxbar = m_flBarWidth - 2.0f * m_flBarInsetX;
					float mid = maxbar / 2.0f;
					float midadj = mid + m_flBarWidth * 0.01f;
					mid += m_flBarInsetX;
					float ratio = (float)pPlayer->GetHealth() / pPlayer->GetMaxHealth();
					float inset = (1.0f - ratio) * midadj;
					float start = m_flBarInsetX + inset;

					int team = pPlayer->GetTeamNumber() - 2;

					surface()->DrawSetColor(m_AuxPowerColor);
					surface()->DrawSetTexture(m_nGlassTex[team]);
					surface()->DrawTexturedRect(xpos, ypos, xpos + m_flBarWidth, end_y);
					if (inset > 0.0f)
					{
						float tstart = start;
						if (start > mid)
							tstart = mid;
						if (inset > 16.0f)
							inset = 16.0f;
						surface()->DrawSetTexture(m_nBlipLeft);
						surface()->DrawTexturedRect(xpos + start - inset, ypos, xpos + tstart, end_y);
					}
					float end_x = mid + mid - start;
					surface()->DrawSetTexture(m_nBlipTex);
					surface()->DrawTexturedRect(xpos + start, ypos, xpos + end_x, end_y);
					if (inset > 0.0f)
					{
						float tend = end_x;
						if (start > mid)
							tend = mid;
						surface()->DrawSetTexture(m_nBlipRight);
						surface()->DrawTexturedRect(xpos + tend, ypos, xpos + end_x + inset, end_y);
					}
					float halfBarHeight = m_flBarHeight / 2.0f;
					ypos += halfBarHeight;
					xpos = (ScreenWidth() - wide) / 2;
					vgui::surface()->DrawSetTextPos(xpos - 1, ypos + 1);
					vgui::surface()->DrawSetTextColor(m_AuxPowerColorShadow);
					vgui::surface()->DrawPrintText(sIDString_line2, wcslen(sIDString_line2));
					vgui::surface()->DrawSetTextPos(xpos + 1, ypos - 1);
					vgui::surface()->DrawSetTextColor(m_AuxPowerColor);
					vgui::surface()->DrawPrintText(sIDString_line2, wcslen(sIDString_line2));
					ypos += halfBarHeight;
				}
				else
				{
					vgui::surface()->DrawSetTextFont(m_hFontSmall);
					vgui::surface()->GetTextSize(m_hFontSmall, sIDString_line2, wide, tall);
					xpos = (ScreenWidth() - wide) / 2;
					vgui::surface()->DrawSetTextPos(xpos, ypos);
					vgui::surface()->DrawPrintText(sIDString_line2, wcslen(sIDString_line2));
				}
				ypos += tall;
			}
			if (bShowHealth)
			{
				ypos += m_flIconPadding;
				int statusCount = 0;
				for (int i = 0; i < COVEN_STATUS_COUNT; i++)
				{
					if (pPlayer->HasStatus((CovenStatus_t)i))
					{
						statusCount++;
					}
				}
				xpos = (ScreenWidth() - statusCount * (m_flIconSize + m_flIconPadding)) / 2;
				for (int i = 0; i < COVEN_STATUS_COUNT; i++)
				{
					CovenStatus_t iStatus = (CovenStatus_t)i;
					if (pPlayer->HasStatus(iStatus))
					{
						CovenStatusEffectInfo_t *info = GetCovenStatusEffectData(iStatus);
						if ((info->iFlags & EFFECT_FLAG_SPLIT_DEF) > 0)
						{
							if (pPlayer->GetTeamNumber() == COVEN_TEAMID_SLAYERS)
								vgui::surface()->DrawSetTexture(info->statusIcon->textureId);
							else
								vgui::surface()->DrawSetTexture(info->altStatusIcon->textureId);
						}
						else
							vgui::surface()->DrawSetTexture(info->statusIcon->textureId);
						vgui::surface()->DrawTexturedRect(xpos, ypos, xpos + m_flIconSize, ypos + m_flIconSize);
						xpos += m_flIconSize + m_flIconPadding;
					}
				}
			}
			if (sIDString_line3[0])
			{
				vgui::surface()->DrawSetTextFont(m_hFontSmall);
				vgui::surface()->GetTextSize(m_hFontSmall, sIDString_line3, wide, tall);
				xpos = (ScreenWidth() - wide) / 2;
				vgui::surface()->DrawSetTextPos(xpos, ypos);
				vgui::surface()->DrawPrintText(sIDString_line3, wcslen(sIDString_line3));
			}
		}
	}
}
