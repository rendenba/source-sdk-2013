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

#include "c_basehlplayer.h"

extern vgui::Panel *g_lastPanel;
extern vgui::Button *g_lastButton;

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
					if ( !Q_stricmp( "Sprint - Rank 1", temp) || !Q_stricmp( "Sprint - Rank 2", temp) || !Q_stricmp( "Sprint - Rank 3", temp) )
					{
						m_pPanel->SetText("Sprint:\n\nEnergy: 10\nCooldown: 20 seconds\n\nBoosts speed for 8/10/12 seconds.");
					}
					else if ( !Q_stricmp( "Battle Yell - Rank 1", temp) || !Q_stricmp( "Battle Yell - Rank 2", temp) || !Q_stricmp( "Battle Yell - Rank 3", temp) )
					{
						m_pPanel->SetText("Battle Yell:\n\nEnergy: 12\nCooldown: 20 seconds\n\nBoosts damage by 10/20/30% for all team mates within 400 units for 10 seconds.");
					}
					else if ( !Q_stricmp( "Holy Water - Rank 1", temp) || !Q_stricmp( "Holy Water - Rank 2", temp) || !Q_stricmp( "Holy Water - Rank 3", temp) )
					{
						m_pPanel->SetText("Holy Water:\n\nEnergy: 12\nCooldown: 5 seconds\n\nThrows a holy water grenade healing allies and setting vampires on fire. Effect increases with rank.");
					}
					else if ( !Q_stricmp( "Leap - Rank 1", temp) || !Q_stricmp( "Leap - Rank 2", temp) || !Q_stricmp( "Leap - Rank 3", temp) )
					{
						m_pPanel->SetText("Leap:\n\nEnergy: 12/9/6\nCooldown: 3.5/3.25/3.0 seconds\n\nCauses you to accelerate quickly through the air.");
					}
					else if ( !Q_stricmp( "Phase - Rank 1", temp) || !Q_stricmp( "Phase - Rank 2", temp) || !Q_stricmp( "Phase - Rank 3", temp) )
					{
						m_pPanel->SetText("Phase:\n\nEnergy: 4 plus 2.0/1.5/1.0 per second\nCooldown: 3 seconds\n\nDisappears from sight and greatly boosts movement speed. Attacking phases back into sight.");
					}
					else if ( !Q_stricmp( "Dread Scream - Rank 1", temp) || !Q_stricmp( "Dread Scream - Rank 2", temp) || !Q_stricmp( "Dread Scream - Rank 3", temp) )
					{
						m_pPanel->SetText("Dread Scream:\n\nEnergy: 10\nCooldown: 15 seconds\n\nScream instilling fear in Slayers within 400 units. Players afflicted are slowed by 15/30/45% for 8 seconds.");
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
						m_pPanel->SetText("Bandage:\n\nEnergy: 14/12/10\nCooldown: 10 seconds\n\nThrows a med kit for team mates to pick up.");
					}
					else if ( !Q_stricmp( "Sheer Will - Rank 1", temp) || !Q_stricmp( "Sheer Will - Rank 2", temp) || !Q_stricmp( "Sheer Will - Rank 3", temp) )
					{
						m_pPanel->SetText("Sheer Will:\n\nEnergy: 10\nCooldown: 20 seconds\n\nBoosts all stats by 10/20/30% for 10 seconds.");
					}
					else if ( !Q_stricmp( "Trip Mine - Rank 1", temp) || !Q_stricmp( "Trip Mine - Rank 2", temp) || !Q_stricmp( "Trip Mine - Rank 3", temp) )
					{
						m_pPanel->SetText("Trip Mine:\n\nEnergy: 8\nCooldown: 4 seconds\n\nPlants a laser activated trip mine on a wall in front of you. Can have 2/3/4 total trip mines active at once.");
					}
					else if ( !Q_stricmp( "Bloodlust - Rank 1", temp) || !Q_stricmp( "Bloodlust - Rank 2", temp) || !Q_stricmp( "Bloodlust - Rank 3", temp) )
					{
						m_pPanel->SetText("Bloodlust:\n\nEnergy: 10\nCooldown: 18 seconds\n\nCauses all teammates within 500 units to regain 10/20/30% of all damage dealt to enemies for 10 seconds.");
					}
					else if ( !Q_stricmp( "Charge - Rank 1", temp) || !Q_stricmp( "Charge - Rank 2", temp) || !Q_stricmp( "Charge - Rank 3", temp) )
					{
						m_pPanel->SetText("Charge:\n\nEnergy: 4 plus 10/9/8 per second\nCooldown: 3 seconds\n\nCharges straight ahead at immense speed. Usable while phased. Hold the effect to continue charging.");
					}
					else if ( !Q_stricmp( "Dodge - Rank 1", temp) || !Q_stricmp( "Dodge - Rank 2", temp) || !Q_stricmp( "Dodge - Rank 3", temp) )
					{
						m_pPanel->SetText("Dodge:\n\nEnergy: 4 plus 4 per second\nCooldown: 5 seconds\n\nToggles dodging. Damage taken while dodging is reduced by 30/50/70%. 1/2/3 in 6 chance to take no damage.");
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
						m_pPanel->SetText("Sneak:\n\nPassive Bonus\n\nFade invisible over 6/3/2 seconds. Magnitude of invisibility increases with rank. Movement or receiving damage cancels the effect.");
					}
					else if ( !Q_stricmp( "Masochist - Rank 1", temp) || !Q_stricmp( "Masochist - Rank 2", temp) || !Q_stricmp( "Masochist - Rank 3", temp) )
					{
						m_pPanel->SetText("Masochist:\n\nPassive Bonus\n\nGrants a speed bonus for 5/10/15% of all damage taken (up 100% bonus speed) for 6 seconds. Effect is cumulative until no damage is received for 6 seconds.");
					}
					else if ( !Q_stricmp( "Gorge - Rank 1", temp) || !Q_stricmp( "Gorge - Rank 2", temp) || !Q_stricmp( "Gorge - Rank 3", temp) )
					{
						m_pPanel->SetText("Gorge:\n\nPassive Bonus\n\nAllows feeding to grant health 10/20/30% past normal maximum health.");
					}
					else if ( !Q_stricmp( "Intimidating Shout - Rank 1", temp) || !Q_stricmp( "Intimidating Shout - Rank 2", temp) || !Q_stricmp( "Intimidating Shout - Rank 3", temp) )
					{
						m_pPanel->SetText("Intimidating Shout:\n\nEnergy: 12/16/20\nCooldown: 20 seconds\n\nStuns all enemies within 200/300/400 units for 1.5 seconds.");
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
					if ( !Q_stricmp( "Berserk - Rank 1", temp) || !Q_stricmp( "Berserk - Rank 2", temp) || !Q_stricmp( "Berserk - Rank 3", temp) )
					{
						m_pPanel->SetText("Berserk:\n\nEnergy: 15\nCooldown: 20 seconds\n\nIncreases health and max health by 40/60/80% for 15 seconds.");
					}
					else if ( !Q_stricmp( "Gut Check - Rank 1", temp) || !Q_stricmp( "Gut Check - Rank 2", temp) || !Q_stricmp( "Gut Check - Rank 3", temp) )
					{
						m_pPanel->SetText("Gut Check:\n\nPassive Bonus\nCooldown: 15/10/5 seconds\n\nGrants immunity from one damaging effect while active. Resets 15/10/5 seconds after being used.");
					}
					else if ( !Q_stricmp( "Undying - Rank 1", temp) || !Q_stricmp( "Undying - Rank 2", temp) || !Q_stricmp( "Undying - Rank 3", temp) )
					{
						m_pPanel->SetText("Undying:\n\nPassive Bonus\nResurrect 0.66/1.33/2.0 seconds faster and with 10/20/30% more health.");
					}
					else if ( !Q_stricmp( "Detonate Blood - Rank 1", temp) || !Q_stricmp( "Detonate Blood - Rank 2", temp) || !Q_stricmp( "Detonate Blood - Rank 3", temp) )
					{
						m_pPanel->SetText("Detonate Blood:\n\nEnergy: 10\nCooldown: 10 seconds\n\nExplode inflicting 15/30/45% max health damage to self and 25/50/75% max health to enemies. Causes 1.5 second stun.");
					}
					else if ( !Q_stricmp( "Vengeful Soul - Rank 1", temp) || !Q_stricmp( "Vengeful Soul - Rank 2", temp) || !Q_stricmp( "Vengeful Soul - Rank 3", temp) )
					{
						m_pPanel->SetText("Vengeful Soul:\n\nEnergy: 12\nCooldown: N/A\n\nUnleash the vengeance of your soul to smite vampires. Soul energy regenerates at 5/6/7 per 4 seconds. Damage caused is proportional to soul energy spent. Soul energy totally obliterates vampires and their corpses.");
					}
					else if ( !Q_stricmp( "UV Light - Rank 1", temp) || !Q_stricmp( "UV Light - Rank 2", temp) || !Q_stricmp( "UV Light - Rank 3", temp) )
					{
						m_pPanel->SetText("UV Light:\n\nEnergy: 4 plus 7.0/5.5/4.0 per second\nCooldown: 3 seconds\n\nToggle a flashlight emitting high powered UV light. Damages and pushes back vampires. Vampires under the effects of holy water take additional damage.");
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
				if( !Q_stricmp( "reaver", GetName() ) )
				{
					m_pPanel->SetText("Reaver:\nClose range tank, also somewhat effective at medium range. Slow speed.\nMax HP: 120\n\n10 Gauge Double Barrel Shotgun:\nVery slow reload, but backs a close to mid range whallop.\n\nAbilities:\n\nSprint: Boosts speed.\n\nSheer Will: Boosts all stats.\n\nIntimidating Shout: Stuns all enemies within range.\n\nGut Check: Grants immunity from one damaging effect while active.");
				}
				else if( !Q_stricmp( "hellion", GetName() ) )
				{
					m_pPanel->SetText("Hellion:\nLightweight scout and expert at mischief. Fastest speed.\nMax HP: 100\n\n357 Magnum:\nStandard big pistol. Very accurate.\n\nAbilities:\n\nHoly Water: Throws a holy water grenade healing allies and setting vampires on fire.\n\nTrip Mine: Plants a laser activated trip mine on a wall in front of you.\n\nReflexes: Increases safe fall distance.\n\nUV Light: Toggle a flashlight emitting high powered UV light. Damages and pushes back vampires.");
				}
				else if( !Q_stricmp( "avenger", GetName() ) )
				{
					m_pPanel->SetText("Avenger:\nStandard meatshield. Average speed.\nMax HP: 100\n\n12 Gauge Pump Action Shotgun:\nHas a six shot clip.  Packs a decent punch.\n\nAbilities:\n\nBattle Yell: Boosts damage for team mates in range.\n\nBandage: Throws a med kit for team mates to pick up.\n\nRevenge: Increases all stats when a nearby ally dies.\n\nVengeful Soul: Releases a projectile that deals damage to vampires and obliterates them.");
				}
				else
				{
					m_pPanel->SetText(" ");
				}
			}
			else if ( !Q_stricmp( "VClassInfo", m_pPanel->GetName() ))
			{
				if( !Q_stricmp( "fiend", GetName() ) )
				{
					m_pPanel->SetText("Fiend:\nLightweight flyer. Insanely mobile.\nMax HP: 72\n\nAbilities:\n\nLeap: Causes you to accelerate quicky through the air.\n\nDodge: Toggle to reduce damage taken.\n\nSneak: Fade invisible. Movement or receiving damage cancels the effect.\n\nBerserk: Increases health and max health.");
				}
				else if( !Q_stricmp( "gore", GetName() ) )
				{
					m_pPanel->SetText("Gore:\nPretty tough. Pretty slow.\nMax HP: 120\n\nAbilities:\n\nPhase: Disappears from sight and greatly boosts movement speed. Attacking phases back into sight.\n\nCharge: Charges straight ahead at immense speed. Usable while phased. Hold the effect to continue charging.\n\nGorge: Allows feeding to grant health past normal maximum health.\n\nDetonate Blood: Explode inflicting damage to self and enemies. Causes stun.");
				}
				else if( !Q_stricmp( "degen", GetName() ) )
				{
					m_pPanel->SetText("Degenerate:\nSlow and \"non-threatening,\" but excels at one-on-one combat.\nMax HP: 112\n\nAbilities:\n\nDread Scream: Scream instilling fear in Slayers within ranges. Players afflicted are slowed.\n\nBloodlust:  Causes all teammates within range to regain some of damage dealt to enemies.\n\nMasochist: Grants a speed bonus for some of all damage taken.\n\nUndying: Resurrect faster and with more health.");
				}
				else
				{
					m_pPanel->SetText(" ");
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
