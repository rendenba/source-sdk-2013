#include "cbase.h"

#include "coven_ammocrate.h"



LINK_ENTITY_TO_CLASS(coven_ammocrate, CCoven_AmmoCrate);
LINK_ENTITY_TO_CLASS(coven_ammocrate_infinite, CCoven_AmmoCrate);

BEGIN_DATADESC(CCoven_AmmoCrate)

DEFINE_KEYFIELD(m_iAmmoType, FIELD_INTEGER, "AmmoType"),

DEFINE_FIELD(m_flCloseTime, FIELD_FLOAT),
DEFINE_FIELD(m_hActivator, FIELD_EHANDLE),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCoven_AmmoCrate::Spawn(void)
{
	BaseClass::Spawn();
	SetBodygroup(1, false);

	ResetSequence(LookupSequence("Idle"));

	m_flCloseTime = 0.0f;

	m_iMetal = 0;
	m_iMaxMetal = 400;
	m_flMetalTimer = gpGlobals->curtime + BuildTime();
	m_iAmmoType = 1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCoven_AmmoCrate::Precache(void)
{
	SetModelName(AllocPooledString(COVEN_CRATE_MODELNAME));

	PrecacheScriptSound("AmmoCrate.Open");
	PrecacheScriptSound("AmmoCrate.Close");

	BaseClass::Precache();
}

void CCoven_AmmoCrate::SelfDestructThink(void)
{
	BaseClass::SelfDestructThink();

	if (m_flCloseTime > 0.0f)
	{
		// Start closing if we're not already
		if (GetSequence() != LookupSequence("Close"))
		{
			// Ready to close?
			if (gpGlobals->curtime >= m_flCloseTime)
			{
				m_hActivator = NULL;

				ResetSequence(LookupSequence("Close"));
			}
		}
		else
		{
			// See if we're fully closed
			if (IsSequenceFinished())
			{
				// Stop thinking
				m_flCloseTime = 0.0f;

				CPASAttenuationFilter sndFilter(this, "AmmoCrate.Close");
				EmitSound(sndFilter, entindex(), "AmmoCrate.Close");

				ResetSequence(LookupSequence("Idle"));
				SetBodygroup(1, true);
			}
		}
	}
}

const Vector CCoven_AmmoCrate::GetPlayerMidPoint(void) const
{
	return EyePosition();
}

void CCoven_AmmoCrate::Enable(void)
{
	SetNextThink(gpGlobals->curtime + 0.1f);
	BaseClass::Enable();
}

void CCoven_AmmoCrate::OnBuildingComplete(void)
{
	SetBodygroup(1, true);
	SetThink(&CCoven_AmmoCrate::AmmoCrateThink);

	BaseClass::OnBuildingComplete();
}

void CCoven_AmmoCrate::AddMetal(void)
{
	m_iMetal = min(m_iMetal + 40 + 10 * m_iLevel, m_iMaxMetal);
}

int CCoven_AmmoCrate::GetMetal(int have)
{
	if (HasSpawnFlags(SF_COVEN_CRATE_INFINITE))
		return 200 - have;

	int need = 200 - have;
	int metal = min(min(m_iMetal, 50 + 20 * m_iLevel), need);
	m_iMetal -= metal;
	return metal;
}

bool CCoven_AmmoCrate::Open(CBasePlayer *pPlayer)
{
	// See if we're not opening already
	if (GetSequence() == LookupSequence("Idle"))
	{
		//BB: too troll-y
		/*Vector mins, maxs;
		trace_t tr;

		CollisionProp()->WorldSpaceAABB(&mins, &maxs);

		Vector vOrigin = GetAbsOrigin();
		vOrigin.z += (maxs.z - mins.z);
		mins = (mins - GetAbsOrigin()) * 0.2f;
		maxs = (maxs - GetAbsOrigin()) * 0.2f;
		mins.z = (GetAbsOrigin().z - vOrigin.z);

		UTIL_TraceHull(vOrigin, vOrigin, mins, maxs, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr);
		
		if (!(tr.startsolid || tr.allsolid))
		{*/
		m_hActivator = pPlayer;

		// Animate!
		ResetSequence(LookupSequence("Open"));

		// Make sound
		CPASAttenuationFilter sndFilter(this, "AmmoCrate.Open");
		EmitSound(sndFilter, entindex(), "AmmoCrate.Open");
		//}

		// Start thinking to make it return
		SetNextThink(gpGlobals->curtime + 0.1f);

		// Don't close again for two seconds
		m_flCloseTime = gpGlobals->curtime + COVEN_CRATE_CLOSE_DELAY;
	}
	else
		return false;

	return true;
}

int CCoven_AmmoCrate::GetAmmo(int index)
{
	if (index == 1)
		return m_iMetal;

	return -1;
}

int CCoven_AmmoCrate::GetMaxAmmo(int index)
{
	if (index == 1)
		return m_iMaxMetal;

	return -1;
}

bool CCoven_AmmoCrate::GiveMetal(CHL2MP_Player *pPlayer)
{
	if (pPlayer && pPlayer->IsBuilderClass())
	{
		float percentage = pPlayer->SuitPower_GetCurrentPercentage();
		if (percentage < 200.0f)
		{
			int metal = GetMetal(pPlayer->SuitPower_GetCurrentPercentage());
			if (metal > 0)
			{
				pPlayer->SuitPower_Charge(metal, true);
				return true;
			}
		}
	}
	return false;
}

bool CCoven_AmmoCrate::GiveAmmo(int index, bool &gaveMetal)
{
	CHL2MP_Player *pPlayer = ToHL2MPPlayer(UTIL_PlayerByIndex(index));
	if (pPlayer && pPlayer->GetTeamNumber() == GetTeamNumber() && pPlayer->IsAlive() && !pPlayer->KO)
	{
		if ((GetAbsOrigin() - pPlayer->GetAbsOrigin()).Length() < PickupRadius())
		{
			trace_t trace;
			UTIL_TraceLine(GetAbsOrigin(), pPlayer->EyePosition(), MASK_PLAYERSOLID, this, COLLISION_GROUP_NONE, &trace);
			if (!trace.DidHitWorld())
			{
				if (!gaveMetal)
					gaveMetal |= GiveMetal(pPlayer);

				//BB: another hacky hack... just give ammo to the first weapon we find that takes ammo. Coven only has one ammo-y weapon right now
				for (int i = 0; i < pPlayer->WeaponCount(); i++)
				{
					CBaseCombatWeapon *pWeap = pPlayer->GetWeapon(i);
					if (pWeap && pWeap->UsesPrimaryAmmo())
					{
						if (HasSpawnFlags(SF_COVEN_CRATE_INFINITE))
						{
							pPlayer->GiveAmmo(100, pWeap->m_iPrimaryAmmoType, false);
						}
						else
						{
							
							if (pPlayer->GiveAmmo(GetAmmoDef()->MaxCarry(pWeap->GetPrimaryAmmoType()) * 0.1f * (m_iLevel + 1), pWeap->m_iPrimaryAmmoType, false) != 0)
							{
								return true;
							}
						}
					}
				}
			}
		}
	}
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Catches the monster-specific messages that occur when tagged
//			animation frames are played.
// Input  : *pEvent - 
//-----------------------------------------------------------------------------
void CCoven_AmmoCrate::HandleAnimEvent(animevent_t *pEvent)
{
	//BB: this seems like a strange way of doing this, but I guess it works...
	if (pEvent->event == AE_AMMOCRATE_PICKUP_AMMO)
	{
		bool doPlayer[MAX_PLAYERS];
		Q_memset(doPlayer, 1, sizeof(doPlayer));
		bool didPickup = false;
		bool didPickupMetal = false;
		//BB: always prioritize owner->activator->everyone else
		if (mOwner != NULL)
		{
			doPlayer[mOwner->entindex()] = false;
			didPickup = GiveAmmo(mOwner->entindex(), didPickupMetal);
		}
		if (m_hActivator != NULL)
		{
			doPlayer[m_hActivator->entindex()] = false;
			didPickup = GiveAmmo(m_hActivator->entindex(), didPickupMetal);
		}
		for (int pN = 0; pN < gpGlobals->maxClients; pN++)
		{
			if (doPlayer[pN])
				didPickup |= GiveAmmo(pN, didPickupMetal);
		}
		if (didPickup || didPickupMetal)
			SetBodygroup(1, false);
		m_hActivator = NULL;

		return;
	}

	BaseClass::HandleAnimEvent(pEvent);
}

void CCoven_AmmoCrate::SelfDestruct(void)
{
	BaseClass::SelfDestruct();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCoven_AmmoCrate::AmmoCrateThink(void)
{
	if (PreThink())
		return;

	if (m_flMetalTimer == 0.0f && m_iMetal != m_iMaxMetal)
		m_flMetalTimer = gpGlobals->curtime + 5.0f;
	else if (gpGlobals->curtime > m_flMetalTimer)
	{
		if (m_iMetal == m_iMaxMetal)
			m_flMetalTimer = 0.0f;
		else
		{
			AddMetal();
			m_flMetalTimer = gpGlobals->curtime + 5.0f;
		}
	}

	if (m_flCloseTime > 0.0f)
	{
		SetNextThink(gpGlobals->curtime + 0.1f);

		// Start closing if we're not already
		if (GetSequence() != LookupSequence("Close"))
		{
			// Ready to close?
			if (gpGlobals->curtime >= m_flCloseTime)
			{
				m_hActivator = NULL;

				ResetSequence(LookupSequence("Close"));
			}
		}
		else
		{
			// See if we're fully closed
			if (IsSequenceFinished())
			{
				// Stop thinking
				m_flCloseTime = 0.0f;

				CPASAttenuationFilter sndFilter(this, "AmmoCrate.Close");
				EmitSound(sndFilter, entindex(), "AmmoCrate.Close");

				ResetSequence(LookupSequence("Idle"));
				SetBodygroup(1, true);

				SetNextThink(gpGlobals->curtime + 0.8f);
			}
		}
	}
	else if (m_bEnabled)
	{
		int pN;
		for (pN = 0; pN < gpGlobals->maxClients; pN++)
		{
			CHL2MP_Player *pPlayer = ToHL2MPPlayer(UTIL_PlayerByIndex(pN));
			if (pPlayer && pPlayer->GetTeamNumber() == GetTeamNumber() && pPlayer->IsAlive() && !pPlayer->KO)
			{
				if ((GetAbsOrigin() - pPlayer->GetAbsOrigin()).Length() < PickupRadius())
				{
					trace_t trace;
					UTIL_TraceLine(GetAbsOrigin(), pPlayer->EyePosition(), MASK_PLAYERSOLID, this, COLLISION_GROUP_NONE, &trace);
					if (!trace.DidHitWorld())
					{
						Open(pPlayer);
						break;
					}
				}
			}
		}

		if (pN == gpGlobals->maxClients)
			SetNextThink(gpGlobals->curtime + 0.2f);
	}
}
