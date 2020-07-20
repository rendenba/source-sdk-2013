//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: The various ammo types for HL2	
//
//=============================================================================//

#include "cbase.h"
#include "item_itemcrate.h"
#include "item_dynamic_resupply.h"
#include "hl2mp_gamerules.h"
#include "weapon_hl2mpbase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

const char *pszItemCrateModelName[] =
{
	"models/items/item_item_crate.mdl",
	"models/items/item_beacon_crate.mdl",
};

LINK_ENTITY_TO_CLASS(item_item_crate, CItem_ItemCrate);

extern ConVar sv_coven_dropboxtime;

//-----------------------------------------------------------------------------
// Save/load: 
//-----------------------------------------------------------------------------
BEGIN_DATADESC( CItem_ItemCrate )

	DEFINE_KEYFIELD( m_CrateType, FIELD_INTEGER, "CrateType" ),	
	DEFINE_KEYFIELD( m_strItemClass, FIELD_STRING, "ItemClass" ),	
	DEFINE_KEYFIELD( m_nItemCount, FIELD_INTEGER, "ItemCount" ),	
	DEFINE_KEYFIELD( m_strAlternateMaster, FIELD_STRING, "SpecificResupply" ),	
	DEFINE_KEYFIELD( m_CrateAppearance, FIELD_INTEGER, "CrateAppearance" ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Kill", InputKill ),
	DEFINE_OUTPUT( m_OnCacheInteraction, "OnCacheInteraction" ),

END_DATADESC()


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItem_ItemCrate::Precache( void )
{
	// Set this here to quiet base prop warnings
	PrecacheModel( pszItemCrateModelName[m_CrateAppearance] );
	SetModel( pszItemCrateModelName[m_CrateAppearance] );

	BaseClass::Precache();
	if ( m_CrateType == CRATE_SPECIFIC_ITEM )
	{
		if ( NULL_STRING != m_strItemClass )
		{
			// Don't precache if this is a null string. 
			UTIL_PrecacheOther( STRING(m_strItemClass) );
		}
	}
}

void CItem_ItemCrate::AddCovenItem(CovenItemID_t iItemType, int iCount)
{
	CovenItemCollection *collect = new CovenItemCollection();
	collect->iItemType = iItemType;
	collect->iCount = iCount;
	m_CovenItems.AddToTail(collect);
	m_nItemCount = m_CovenItems.Count();
}

void CItem_ItemCrate::FallThink(void)
{
	SetNextThink(gpGlobals->curtime + 0.1f);

	bool shouldMaterialize = false;
	IPhysicsObject *pPhysics = VPhysicsGetObject();
	if (pPhysics)
	{
		shouldMaterialize = pPhysics->IsAsleep();
	}
	else
	{
		shouldMaterialize = (GetFlags() & FL_ONGROUND) ? true : false;
	}

	if (shouldMaterialize)
	{
		SetThink(NULL);

		m_vOriginalSpawnOrigin = GetAbsOrigin();
		m_vOriginalSpawnAngles = GetAbsAngles();

		if (m_flLifetime > 0.0f)
		{
			SetThink(&CItem_ItemCrate::SUB_FadeOut);
			SetNextThink(m_flLifetime);
		}
		HL2MPRules()->AddLevelDesignerPlacedObject(this);
	}
}

void CItem_ItemCrate::UpdateOnRemove()
{
	HL2MPRules()->RemoveLevelDesignerPlacedObject(this);
	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItem_ItemCrate::Spawn( void )
{ 
	if ( g_pGameRules->IsAllowedToSpawn( this ) == false )
	{
		UTIL_Remove( this );
		return;
	}

	m_flLifetime = 0.0f;

	DisableAutoFade();
	SetModelName( AllocPooledString( pszItemCrateModelName[m_CrateAppearance] ) );

	if ( (NULL_STRING == m_strItemClass && m_CrateType == CRATE_SPECIFIC_ITEM) || (m_CrateType == CRATE_COVEN && m_CovenItems.Count() == 0))
	{
		Warning( "CItem_ItemCrate(%i):  CRATE_SPECIFIC_ITEM with NULL ItemClass string (deleted)!!!\n", entindex() );
		UTIL_Remove( this );
		return;
	}

	Precache( );
	SetModel( pszItemCrateModelName[m_CrateAppearance] );
	AddEFlags( EFL_NO_ROTORWASH_PUSH );
	BaseClass::Spawn( );

	SetThink(&CItem_ItemCrate::FallThink);
	SetNextThink(gpGlobals->curtime + 0.1f);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &data - 
//-----------------------------------------------------------------------------
void CItem_ItemCrate::InputKill( inputdata_t &data )
{
	UTIL_Remove( this );
}


//-----------------------------------------------------------------------------
// Item crates blow up immediately
//-----------------------------------------------------------------------------
int CItem_ItemCrate::OnTakeDamage( const CTakeDamageInfo &info )
{
	if ( info.GetDamageType() & DMG_AIRBOAT )
	{
		CTakeDamageInfo dmgInfo = info;
		dmgInfo.ScaleDamage( 10.0 );
		return BaseClass::OnTakeDamage( dmgInfo );
	}

	if (info.GetAttacker() == NULL || !info.GetAttacker()->IsPlayer() || (info.GetAttacker()->IsPlayer() && info.GetAttacker()->GetTeamNumber() != GetTeamNumber()))
	{
		CTakeDamageInfo dmgInfo = info;
		dmgInfo.SetDamage(0.0f);
		return BaseClass::OnTakeDamage(dmgInfo);
	}

	return BaseClass::OnTakeDamage( info );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItem_ItemCrate::VPhysicsCollision( int index, gamevcollisionevent_t *pEvent )
{
	float flDamageScale = 1.0f;
	if ( FClassnameIs( pEvent->pEntities[!index], "prop_vehicle_airboat" ) ||
		 FClassnameIs( pEvent->pEntities[!index], "prop_vehicle_jeep" ) )
	{
		flDamageScale = 100.0f;
	}

	m_impactEnergyScale *= flDamageScale;
	BaseClass::VPhysicsCollision( index, pEvent );
	m_impactEnergyScale /= flDamageScale;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CItem_ItemCrate::OnBreak( const Vector &vecVelocity, const AngularImpulse &angImpulse, CBaseEntity *pBreaker )
{
	// FIXME: We could simply store the name of an entity to put into the crate 
	// as a string entered in by worldcraft. Should we?	I'd do it for sure
	// if it was easy to get a dropdown with all entity types in it.

	m_OnCacheInteraction.FireOutput(pBreaker,this);

	for ( int i = 0; i < m_nItemCount; ++i )
	{
		CBaseEntity *pSpawn = NULL;
		switch( m_CrateType )
		{
		case CRATE_SPECIFIC_ITEM:
			pSpawn = CreateEntityByName(STRING(m_strItemClass));
			break;

		case CRATE_COVEN:
			switch (m_CovenItems[i]->iItemType)
			{
				case COVEN_ITEM_GRENADE:
				{
					pSpawn = CreateEntityByName("weapon_frag");
					break;
				}
				case COVEN_ITEM_STUN_GRENADE:
				{
					pSpawn = CreateEntityByName("weapon_stunfrag");
					break;
				}
				case COVEN_ITEM_HOLYWATER:
				{
					pSpawn = CreateEntityByName("weapon_holywater");
					break;
				}
				case COVEN_ITEM_STIMPACK:
				{
					pSpawn = CreateEntityByName("item_healthvial");
					break;
				}
				case COVEN_ITEM_MEDKIT:
				{
					pSpawn = CreateEntityByName("item_healthkit");
					break;
				}
				case COVEN_ITEM_PILLS:
				{
					pSpawn = CreateEntityByName("item_pills");
					break;
				}
			}
			if (pSpawn)
			{
				//BB: TODO: are dropboxes always slayers? We might be able to simplify item pickup rules...
				pSpawn->ChangeTeam(COVEN_TEAMID_SLAYERS);
				pSpawn->AddSpawnFlags(SF_NORESPAWN);
				//pSpawn->KeyValue("RestActivate", "1");
			}
			break;

		default:
			break;
		}

		if ( !pSpawn )
			return;

		// Give a little randomness...
		Vector vecOrigin;
		CollisionProp()->RandomPointInBounds( Vector(0.25, 0.25, 0.25), Vector( 0.75, 0.75, 0.75 ), &vecOrigin );
		pSpawn->SetAbsOrigin( vecOrigin );

		QAngle vecAngles;
		vecAngles.x = random->RandomFloat( -20.0f, 20.0f );
		vecAngles.y = random->RandomFloat( 0.0f, 360.0f );
		vecAngles.z = random->RandomFloat( -20.0f, 20.0f );
		pSpawn->SetAbsAngles( vecAngles );

		Vector vecActualVelocity;
		vecActualVelocity.Random( -10.0f, 10.0f );
//		vecActualVelocity += vecVelocity;
		pSpawn->SetAbsVelocity( vecActualVelocity );

		QAngle angVel;
		AngularImpulseToQAngle( angImpulse, angVel );
		pSpawn->SetLocalAngularVelocity( angVel );

		// If we're creating an item, it can't be picked up until it comes to rest
		// But only if it wasn't broken by a vehicle
		/*CItem *pItem = dynamic_cast<CItem*>(pSpawn);
		if ( pItem && !pBreaker->GetServerVehicle())
		{
			pItem->ActivateWhenAtRest();
		}*/

		pSpawn->Spawn();

		if (m_CrateType == CRATE_COVEN)
		{
			CWeaponHL2MPBase *pWeap = dynamic_cast<CWeaponHL2MPBase *>(pSpawn);
			if (pWeap)
			{
				pWeap->SetPrimaryAmmoCount(m_CovenItems[i]->iCount);
				pWeap->SetOriginalSpawnOrigin(pSpawn->GetAbsOrigin());
				pWeap->SetOriginalSpawnAngles(pSpawn->GetAbsAngles());
				pWeap->m_flLifetime = gpGlobals->curtime + sv_coven_dropboxtime.GetFloat();
				pWeap->AddGlowEffect(false, true, true, true, true, 2000.0f);
			}
			else
			{
				CItem *pItem = dynamic_cast<CItem *>(pSpawn);
				if (pItem)
				{
					pItem->SetCount(m_CovenItems[i]->iCount);
					pItem->SetOriginalSpawnOrigin(pSpawn->GetAbsOrigin());
					pItem->SetOriginalSpawnAngles(pSpawn->GetAbsAngles());
					pItem->m_flLifetime = gpGlobals->curtime + sv_coven_dropboxtime.GetFloat();
					pItem->AddGlowEffect(false, true, true, true, true, 2000.0f);
				}
			}
		}

		// Avoid missing items drops by a dynamic resupply because they don't think immediately
		if ( FClassnameIs( pSpawn, "item_dynamic_resupply" ) )
		{
			if ( m_strAlternateMaster != NULL_STRING )
			{
				DynamicResupply_InitFromAlternateMaster( pSpawn, m_strAlternateMaster );
			}
			if ( i == 0 )
			{
				pSpawn->AddSpawnFlags( SF_DYNAMICRESUPPLY_ALWAYS_SPAWN );
			}
			pSpawn->SetNextThink( gpGlobals->curtime );
		}
	}

	if (m_CrateType == CRATE_COVEN)
		HL2MPRules()->RemoveLevelDesignerPlacedObject(this);
}

void CItem_ItemCrate::OnPhysGunPickup( CBasePlayer *pPhysGunUser, PhysGunPickup_t reason )
{
	BaseClass::OnPhysGunPickup( pPhysGunUser, reason );

	m_OnCacheInteraction.FireOutput( pPhysGunUser, this );

	if ( reason == PUNTED_BY_CANNON && m_CrateAppearance != CRATE_APPEARANCE_RADAR_BEACON )
	{
		Vector vForward;
		AngleVectors( pPhysGunUser->EyeAngles(), &vForward, NULL, NULL );
		Vector vForce = Pickup_PhysGunLaunchVelocity( this, vForward, PHYSGUN_FORCE_PUNTED );
		AngularImpulse angular = AngularImpulse( 0, 0, 0 );

		IPhysicsObject *pPhysics = VPhysicsGetObject();

		if ( pPhysics )
		{
			pPhysics->AddVelocity( &vForce, &angular );
		}

		TakeDamage( CTakeDamageInfo( pPhysGunUser, pPhysGunUser, GetHealth(), DMG_GENERIC ) );
	}
}
