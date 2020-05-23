//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef MOUSEOVERPANELBUTTON_H
#define MOUSEOVERPANELBUTTON_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui/IScheme.h>
#include <vgui_controls/RichText.h>
#include <vgui_controls/Button.h>
#include <vgui/KeyCode.h>
#include <filesystem.h>
#include <vgui/ILocalize.h>

#include "c_basehlplayer.h"
#include "coven_parse.h"
#include "weapon_parse.h"

extern vgui::Panel *g_lastPanel;
extern vgui::Button *g_lastButton;

extern ConVar sv_coven_hp_per_con;

//-----------------------------------------------------------------------------
// Purpose: Triggers a new panel when the mouse goes over the button
//    
// the new panel has the same dimensions as the passed templatePanel and is of
// the same class.
//
// must at least inherit from vgui::EditablePanel to support LoadControlSettings
//-----------------------------------------------------------------------------

//template <class T>
class MouseOverButton : public vgui::Button
{
private:
	DECLARE_CLASS_SIMPLE( MouseOverButton, vgui::Button );
	
public:
	MouseOverButton(vgui::Panel *parent, const char *panelName, /*T*/vgui::RichText *templatePanel ) :
					Button( parent, panelName, "MouseOverButton")
	{
		m_pPanel = templatePanel;
		/*m_pPanel = new T( parent, NULL );
		m_pPanel ->SetVisible( false );

		// copy size&pos from template panel
		int x,y,wide,tall;
		templatePanel->GetBounds( x, y, wide, tall );
		m_pPanel->SetBounds( x, y, wide, tall );
		int px, py;
		templatePanel->GetPinOffset( px, py );
		int rx, ry;
		templatePanel->GetResizeOffset( rx, ry );
		// Apply pin settings from template, too
		m_pPanel->SetAutoResize( templatePanel->GetPinCorner(), templatePanel->GetAutoResize(), px, py, rx, ry );

		m_bPreserveArmedButtons = false;
		m_bUpdateDefaultButtons = false;*/
	}

	virtual void SetPreserveArmedButtons( bool bPreserve ){ m_bPreserveArmedButtons = bPreserve; }
	virtual void SetUpdateDefaultButtons( bool bUpdate ){ m_bUpdateDefaultButtons = bUpdate; }

	virtual void ShowPage()
	{
		if( m_pPanel )
		{
			//BB: code: Sacrifice nothing changed but this is where this shit is located
			//BB: VERY hacky but it works and its easy...
			//BB: change EditablePanel to RichText and do everything text only...

			if ( !Q_stricmp( "LClassInfo", m_pPanel->GetName() ) )
			{
				if( !Q_stricmp( "abil1", GetName() ) )
				{
					char temp[64];
					GetText(temp, sizeof(temp));
					if ( !Q_stricmp( "Haste - Rank 1", temp) || !Q_stricmp( "Haste - Rank 2", temp) || !Q_stricmp( "Haste - Rank 3", temp) )
					{
						m_pPanel->SetText("Sprint:\n\nEnergy: 2.0/sec\n\nBoosts speed by 15/30/45%.");
					}
					else if ( !Q_stricmp( "Battle Yell - Rank 1", temp) || !Q_stricmp( "Battle Yell - Rank 2", temp) || !Q_stricmp( "Battle Yell - Rank 3", temp) )
					{
						m_pPanel->SetText("Battle Yell:\n\nCooldown: 20 seconds\n\nBoosts damage by 10/20/30% for all team mates within 500 units for 10 seconds.");
					}
					else if ( !Q_stricmp( "Holy Water - Rank 1", temp) || !Q_stricmp( "Holy Water - Rank 2", temp) || !Q_stricmp( "Holy Water - Rank 3", temp) )
					{
						m_pPanel->SetText("Holy Water:\n\nCooldown: 6 seconds\n\nThrows a holy water grenade healing allies and setting vampires on fire. Effect increases with rank.");
					}
					else if ( !Q_stricmp( "Leap - Rank 1", temp) || !Q_stricmp( "Leap - Rank 2", temp) || !Q_stricmp( "Leap - Rank 3", temp) )
					{
						m_pPanel->SetText("Leap:\n\nCooldown: 3.5/3.25/3.0 seconds\n\nCauses you to accelerate quickly through the air. Effect increases with rank.");
					}
					else if ( !Q_stricmp( "Bloodlust - Rank 1", temp) || !Q_stricmp( "Bloodlust - Rank 2", temp) || !Q_stricmp( "Bloodlust - Rank 3", temp) )
					{
						m_pPanel->SetText("Bloodlust:\n\nCooldown: 18 seconds\n\nCauses all teammates within 500 units to regain 10/20/30% of all damage dealt to enemies for 10 seconds.");
					}
					else if ( !Q_stricmp( "Dread Scream - Rank 1", temp) || !Q_stricmp( "Dread Scream - Rank 2", temp) || !Q_stricmp( "Dread Scream - Rank 3", temp) )
					{
						m_pPanel->SetText("Dread Scream:\n\nCooldown: 15 seconds\n\nScream instilling fear in Slayers within 500 units. Players afflicted are slowed by 15/30/45% for 8 seconds.");
					}
					else
					{
						m_pPanel->SetText(" ");
					}
				}
				else if( !Q_stricmp( "abil2", GetName() ) )
				{
					char temp[64];
					GetText(temp, sizeof(temp));
					if ( !Q_stricmp( "Bandage - Rank 1", temp) || !Q_stricmp( "Bandage - Rank 2", temp) || !Q_stricmp( "Bandage - Rank 3", temp) )
					{
						m_pPanel->SetText("Bandage:\n\nCooldown: 6 seconds\n\nThrows a med kit for team mates to pick up. Maximum 1/2/3 med kits active at once.");
					}
					else if ( !Q_stricmp( "Sheer Will - Rank 1", temp) || !Q_stricmp( "Sheer Will - Rank 2", temp) || !Q_stricmp( "Sheer Will - Rank 3", temp) )
					{
						m_pPanel->SetText("Sheer Will:\n\nCooldown: 16 seconds\n\nBoosts all stats by 10/20/30% for 10 seconds.");
					}
					else if (!Q_stricmp("Dark Will - Rank 1", temp) || !Q_stricmp("Dark Will - Rank 2", temp) || !Q_stricmp("Dark Will - Rank 3", temp))
					{
						m_pPanel->SetText("Dark Will:\n\nCooldown: 16 seconds\n\nBoosts all stats by 10/20/30% for 10 seconds. Improved feeding while in use.");
					}
					else if ( !Q_stricmp( "Trip Mine - Rank 1", temp) || !Q_stricmp( "Trip Mine - Rank 2", temp) || !Q_stricmp( "Trip Mine - Rank 3", temp) )
					{
						m_pPanel->SetText("Trip Mine:\n\nCooldown: 2 seconds\n\nPlants a laser activated trip mine on a wall in front of you. Can have 2/3/4 total trip mines active at once.");
					}
					else if ( !Q_stricmp( "Charge - Rank 1", temp) || !Q_stricmp( "Charge - Rank 2", temp) || !Q_stricmp( "Charge - Rank 3", temp) )
					{
						m_pPanel->SetText("Charge:\n\nEnergy: 10/8/6 per second\nCooldown: 5/4/3 seconds\n\nCharges straight ahead at immense speed. Usable while phased. Hold the effect to continue charging.");
					}
					else if ( !Q_stricmp( "Berserk - Rank 1", temp) || !Q_stricmp( "Berserk - Rank 2", temp) || !Q_stricmp( "Berserk - Rank 3", temp) )
					{
						m_pPanel->SetText("Berserk:\n\nCooldown: 20 seconds\n\nIncreases health and max health by 40/60/80% for 10 seconds.");
					}
					else if ( !Q_stricmp( "Undying - Rank 1", temp) || !Q_stricmp( "Undying - Rank 2", temp) || !Q_stricmp( "Undying - Rank 3", temp) )
					{
						m_pPanel->SetText("Undying:\n\nPassive Bonus\nResurrect 0.66/1.33/2.0 seconds faster and with 10/20/30% more health.");
					}
					else if (!Q_stricmp("Gorge - Rank 1", temp) || !Q_stricmp("Gorge - Rank 2", temp) || !Q_stricmp("Gorge - Rank 3", temp))
					{
						m_pPanel->SetText("Gorge:\n\nPassive Bonus\n\nAllows feeding to grant health 10/20/30% past normal maximum health. Feeding speed is increased.");
					}
					else
					{
						m_pPanel->SetText(" ");
					}
				}
				else if( !Q_stricmp( "abil3", GetName() ) )
				{
					char temp[64];
					GetText(temp, sizeof(temp));
					if ( !Q_stricmp( "Revenge - Rank 1", temp) || !Q_stricmp( "Revenge - Rank 2", temp) || !Q_stricmp( "Revenge - Rank 3", temp) )
					{
						m_pPanel->SetText("Revenge:\n\nPassive Bonus\n\nIncreases all stats by 10/20/30% for 10 seconds when an ally dies within 600 units.");
					}
					else if ( !Q_stricmp( "Reflexes - Rank 1", temp) || !Q_stricmp( "Reflexes - Rank 2", temp) || !Q_stricmp( "Reflexes - Rank 3", temp) )
					{
						m_pPanel->SetText("Reflexes:\n\nPassive Bonus\n\nIncreases safe fall distance by 15/30/45%.");
					}
					else if ( !Q_stricmp( "Sneak - Rank 1", temp) || !Q_stricmp( "Sneak - Rank 2", temp) || !Q_stricmp( "Sneak - Rank 3", temp) )
					{
						m_pPanel->SetText("Sneak:\n\nPassive Bonus\n\nFade invisible over 5/2.5/1.67 seconds. Magnitude of invisibility increases with rank. Regernates health up to 66.67/83.33/100%. Running or receiving damage cancels the effect.");
					}
					else if ( !Q_stricmp( "Masochist - Rank 1", temp) || !Q_stricmp( "Masochist - Rank 2", temp) || !Q_stricmp( "Masochist - Rank 3", temp) )
					{
						m_pPanel->SetText("Masochist:\n\nPassive Bonus\n\nGrants a speed bonus for 50/100/150% of all damage taken (up to 50/100/150% bonus speed) for 6/7/8 seconds. Effect is cumulative until no damage is received for 6/7/8 seconds.");
					}
					else if ( !Q_stricmp( "Gut Check - Rank 1", temp) || !Q_stricmp( "Gut Check - Rank 2", temp) || !Q_stricmp( "Gut Check - Rank 3", temp) )
					{
						m_pPanel->SetText("Gut Check:\n\nPassive Bonus\nCooldown: 14/10/6 seconds\n\nGrants immunity from one vampire melee attack while active. Resets 14/10/6 seconds after being used.");
					}
					else
					{
						m_pPanel->SetText(" ");
					}
				}
				else if( !Q_stricmp( "abil4", GetName() ) )
				{
					char temp[64];
					GetText(temp, sizeof(temp));
					if ( !Q_stricmp( "Dodge - Rank 1", temp) || !Q_stricmp( "Dodge - Rank 2", temp) || !Q_stricmp( "Dodge - Rank 3", temp) )
					{
						m_pPanel->SetText("Dodge:\n\nEnergy: 4 per second\nCooldown: 5 seconds\n\nToggles dodging. Damage taken while dodging is reduced by 30/50/70%. 1/2/3 in 6 chance to take no damage. Attacking forces you to stop dodging.");
					}
					else if ( !Q_stricmp( "Phase - Rank 1", temp) || !Q_stricmp( "Phase - Rank 2", temp) || !Q_stricmp( "Phase - Rank 3", temp) )
					{
						m_pPanel->SetText("Phase:\n\nEnergy: 2/1.5/1 per second\nCooldown: 16/15/14 seconds\n\nDisappears from sight and greatly boosts movement speed for 8/10/12 seconds. Regenerates health up to 66.67/83.33/100%. Attacking phases back into sight.");
					}
					else if ( !Q_stricmp( "Intimidating Shout - Rank 1", temp) || !Q_stricmp( "Intimidating Shout - Rank 2", temp) || !Q_stricmp( "Intimidating Shout - Rank 3", temp) )
					{
						m_pPanel->SetText("Intimidating Shout:\n\nCooldown: 20 seconds\n\nStuns all enemies within 300/400/500 units for 1.0/1.5/2.0 seconds.");
					}
					else if ( !Q_stricmp( "Detonate Blood - Rank 1", temp) || !Q_stricmp( "Detonate Blood - Rank 2", temp) || !Q_stricmp( "Detonate Blood - Rank 3", temp) )
					{
						m_pPanel->SetText("Detonate Blood:\n\nEnergy: 5/10/15\nCooldown: 6/5/4 seconds\n\nUnleash to explode inflicting 100% of energy spent damage to self and 300% per energy spent damage to enemies. Counts double for Masochist.");
					}
					else if ( !Q_stricmp( "Vengeful Soul - Rank 1", temp) || !Q_stricmp( "Vengeful Soul - Rank 2", temp) || !Q_stricmp( "Vengeful Soul - Rank 3", temp) )
					{
						m_pPanel->SetText("Vengeful Soul:\n\nEnergy: 4 per second\nCooldown: 3 seconds\n\nHold to charge 4/8/12 soul energy per second. Unleash the vengeance of your soul to smite vampires. Damage caused is proportional to soul energy spent. Soul energy totally obliterates vampires and their corpses.");
					}
					else if ( !Q_stricmp( "UV Light - Rank 1", temp) || !Q_stricmp( "UV Light - Rank 2", temp) || !Q_stricmp( "UV Light - Rank 3", temp) )
					{
						m_pPanel->SetText("UV Light:\n\nEnergy: 5/4/3 per second\nCooldown: 3 seconds\n\nToggle a flashlight emitting high powered UV light. Damages and pushes back vampires. Vampires under the effects of holy water take additional damage.");
					}
					else
					{
						m_pPanel->SetText(" ");
					}
				}
				else
				{
					m_pPanel->SetText(" ");
				}
			}
			else if ( !Q_stricmp( "SClassInfo", m_pPanel->GetName() ))
			{
				for (int i = 1; i <= COVEN_CLASSCOUNT_SLAYERS; i++)
				{
					CovenClassInfo_t *info = GetCovenClassData(i, COVEN_TEAMID_SLAYERS);
					if (!Q_stricmp(info->szName, GetName()))
					{
						wchar_t statsHP[4];
						swprintf(statsHP, sizeof(statsHP), L"%d", (int)(info->flConstitution * sv_coven_hp_per_con.GetFloat()));
						wchar_t statsCon[4];
						swprintf(statsCon, sizeof(statsCon), L"%d", (int)(info->flConstitution));
						wchar_t statsStr[4];
						swprintf(statsStr, sizeof(statsStr), L"%d", (int)(info->flStrength));
						wchar_t statsInt[4];
						swprintf(statsInt, sizeof(statsInt), L"%d", (int)(info->flIntellect));
						wchar_t statsSpeed[4];
						swprintf(statsSpeed, sizeof(statsSpeed), L"%d", (int)(info->flBaseSpeed));

						wchar_t statsString[128];
						g_pVGuiLocalize->ConstructString(statsString, sizeof(statsString), g_pVGuiLocalize->Find("#class_selection_stats"), 5, statsHP, statsSpeed, statsCon, statsStr, statsInt);

						wchar_t abilitiesString[COVEN_MAX_ABILITIES][256];
						for (int j = 0; j < COVEN_MAX_ABILITIES; j++)
						{
							CovenAbilityInfo_t *abilityInfo = GetCovenAbilityData(info->iAbilities[j]);
							swprintf(abilitiesString[j], sizeof(abilitiesString[j]), L"%s:\n%s\n\n", g_pVGuiLocalize->Find(abilityInfo->szPrintName), g_pVGuiLocalize->Find(abilityInfo->szDescription));
						}

						wchar_t weaponsString[128];
						weaponsString[0] = 0;
						for (int j = 0; j < info->szWeapons.Count(); j++)
						{
							FileWeaponInfo_t *weapInfo = GetFileWeaponInfoFromHandle(LookupWeaponInfoSlot(info->szWeapons[j]));
							swprintf(weaponsString, sizeof(weaponsString), L"%s%s\n", weaponsString, g_pVGuiLocalize->Find(weapInfo->szPrintName));
						}

						wchar_t string[1024];
						g_pVGuiLocalize->ConstructString(string, sizeof(string), g_pVGuiLocalize->Find("#class_selection_slayers"), 8, g_pVGuiLocalize->Find(info->szPrintName), g_pVGuiLocalize->Find(info->szDescription), statsString,
							weaponsString, abilitiesString[0], abilitiesString[1], abilitiesString[2], abilitiesString[3]);
						m_pPanel->SetText(string);
					}
				}
			}
			else if ( !Q_stricmp( "VClassInfo", m_pPanel->GetName() ))
			{
				for (int i = 1; i <= COVEN_CLASSCOUNT_VAMPIRES; i++)
				{
					CovenClassInfo_t *info = GetCovenClassData(i, COVEN_TEAMID_VAMPIRES);
					if (!Q_stricmp(info->szName, GetName()))
					{
						wchar_t statsHP[4];
						swprintf(statsHP, sizeof(statsHP), L"%d", (int)(info->flConstitution * sv_coven_hp_per_con.GetFloat()));
						wchar_t statsCon[4];
						swprintf(statsCon, sizeof(statsCon), L"%d", (int)(info->flConstitution));
						wchar_t statsStr[4];
						swprintf(statsStr, sizeof(statsStr), L"%d", (int)(info->flStrength));
						wchar_t statsInt[4];
						swprintf(statsInt, sizeof(statsInt), L"%d", (int)(info->flIntellect));
						wchar_t statsSpeed[4];
						swprintf(statsSpeed, sizeof(statsSpeed), L"%d", (int)(info->flBaseSpeed));

						wchar_t statsString[128];
						g_pVGuiLocalize->ConstructString(statsString, sizeof(statsString), g_pVGuiLocalize->Find("#class_selection_stats"), 5, statsHP, statsSpeed, statsCon, statsStr, statsInt);

						wchar_t abilitiesString[COVEN_MAX_ABILITIES][256];
						for (int j = 0; j < COVEN_MAX_ABILITIES; j++)
						{
							CovenAbilityInfo_t *abilityInfo = GetCovenAbilityData(info->iAbilities[j]);
							swprintf(abilitiesString[j], sizeof(abilitiesString[j]), L"%s:\n%s\n\n", g_pVGuiLocalize->Find(abilityInfo->szPrintName), g_pVGuiLocalize->Find(abilityInfo->szDescription));
						}

						wchar_t string[1024];
						g_pVGuiLocalize->ConstructString(string, sizeof(string), g_pVGuiLocalize->Find("#class_selection_vampires"), 7, g_pVGuiLocalize->Find(info->szPrintName), g_pVGuiLocalize->Find(info->szDescription), statsString,
							abilitiesString[0], abilitiesString[1], abilitiesString[2], abilitiesString[3]);
						m_pPanel->SetText(string);
					}
				}
			}

			//m_pPanel->SetVisible( true );
			//m_pPanel->MoveToFront();
			//g_lastPanel = m_pPanel;
		}
	}
	
	virtual void HidePage()
	{
		if ( m_pPanel )
		{
			m_pPanel->SetVisible( false );
		}
	}

	const char *GetClassPage( const char *className )
	{
		static char classPanel[ _MAX_PATH ];
		Q_snprintf( classPanel, sizeof( classPanel ), "classes/%s.res", className);

		if ( g_pFullFileSystem->FileExists( classPanel, IsX360() ? "MOD" : "GAME" ) )
		{
		}
		else if (g_pFullFileSystem->FileExists( "classes/default.res", IsX360() ? "MOD" : "GAME" ) )
		{
			Q_snprintf ( classPanel, sizeof( classPanel ), "classes/default.res" );
		}
		else
		{
			return NULL;
		}

		return classPanel;
	}

#ifdef REFRESH_CLASSMENU_TOOL

	void RefreshClassPage( void )
	{
		m_pPanel->LoadControlSettings( GetClassPage( GetName() ) );
	}

#endif

	virtual void ApplySettings( KeyValues *resourceData ) 
	{
		BaseClass::ApplySettings( resourceData );

		// name, position etc of button is set, now load matching
		// resource file for associated info panel:
		//m_pPanel->LoadControlSettings( GetClassPage( GetName() ) );
	}		

	/*T*/vgui::RichText *GetClassPanel( void ) { return m_pPanel; }

	virtual void OnCursorExited()
	{
		if ( !m_bPreserveArmedButtons )
		{
			BaseClass::OnCursorExited();
		}
	}

	void ChangePanel(vgui::RichText *text)
	{
		m_pPanel = text;
	}

	virtual void OnCursorEntered() 
	{
		BaseClass::OnCursorEntered();

		//BB: temp hack... we want to show for disabled stuff...
		//if ( !IsEnabled() || !m_pPanel)
		if (!m_pPanel)
			return;

		// are we updating the default buttons?
		/*if ( m_bUpdateDefaultButtons )
		{
			SetAsDefaultButton( 1 );
		}*/

		// are we preserving the armed state (and need to turn off the old button)?
		/*if ( m_bPreserveArmedButtons )
		{
			if ( g_lastButton && g_lastButton != this )
			{
				g_lastButton->SetArmed( false );
			}

			g_lastButton = this;
		}*/

		// turn on our panel (if it isn't already)
		/*if ( m_pPanel && ( !m_pPanel->IsVisible() ) )
		{
			// turn off the previous panel
			if ( g_lastPanel && g_lastPanel->IsVisible() )
			{
				g_lastPanel->SetVisible( false );
			}
*/
			ShowPage();
		/*}*/
	}

	virtual void OnKeyCodeReleased( vgui::KeyCode code )
	{
		BaseClass::OnKeyCodeReleased( code );

		if ( m_bPreserveArmedButtons )
		{
			if ( g_lastButton )
			{
				g_lastButton->SetArmed( true );
			}
		}
	}

private:

	/*T*/vgui::RichText *m_pPanel;
	bool m_bPreserveArmedButtons;
	bool m_bUpdateDefaultButtons;
};

//#define MouseOverPanelButton MouseOverButton<vgui::EditablePanel>
#define MouseOverPanelButton MouseOverButton

#endif // MOUSEOVERPANELBUTTON_H
