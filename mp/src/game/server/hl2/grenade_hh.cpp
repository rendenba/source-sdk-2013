//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "basegrenade_shared.h"
#include "grenade_frag.h"
#include "grenade_hh.h"
#include "Sprite.h"
#include "SpriteTrail.h"
#include "smoke_trail.h"
#include "soundent.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

const float GRENADE_COEFFICIENT_OF_RESTITUTION = 0.2f;

ConVar sk_plr_dmg_holyfraggrenade	( "sk_plr_dmg_holyfraggrenade","0");
ConVar sk_npc_dmg_holyfraggrenade	( "sk_npc_dmg_holyfraggrenade","0");
ConVar sk_holyfraggrenade_radius	( "sk_holyfraggrenade_radius", "0");

LINK_ENTITY_TO_CLASS( grenade_hh, CGrenadeHH );

BEGIN_DATADESC( CGrenadeHH )

	// Fields
	DEFINE_FIELD( m_inSolid, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_punted, FIELD_BOOLEAN ),
	
	// Function Pointers
	DEFINE_ENTITYFUNC( GrenadeHHTouch ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CGrenadeHH::~CGrenadeHH( void )
{
}

void CGrenadeHH::Spawn( void )
{
	Precache( );

	SetModel( HHGRENADE_MODEL );

	SetUse( &CGrenadeHH::DetonateUse );
	SetTouch( &CGrenadeHH::GrenadeHHTouch );
	SetNextThink( gpGlobals->curtime + 0.1f );

	if( GetOwnerEntity() && GetOwnerEntity()->IsPlayer() )
	{
		m_flDamage = 10.0f;
	}
	else
	{
		m_flDamage = 10.0f;
	}

	m_DmgRadius		= 10.0f;//RADIUS!!! sk_smg1_grenade_radius.GetFloat();
	m_takedamage	= DAMAGE_YES;
	m_bIsLive		= true;
	m_iHealth		= 1;

	SetSize( -Vector(4,4,4), Vector(4,4,4) );
	SetCollisionGroup( COLLISION_GROUP_WEAPON );
	CreateVPhysics();

	SetGravity( 1.0f );
	SetFriction( 0.8 );
	SetSequence( 0 );

	//m_fDangerRadius = 100;

	//m_fSpawnTime = gpGlobals->curtime;

	AddSolidFlags( FSOLID_NOT_STANDABLE );

	m_punted			= false;

	BaseClass::Spawn();
}

void CGrenadeHH::Detonate()
{
	CPASFilter filter( GetAbsOrigin() );
	m_hSmokeTrail = SmokeTrail::CreateSmokeTrail();
	if( m_hSmokeTrail )
	{
		m_hSmokeTrail->m_SpawnRate = 28;
		m_hSmokeTrail->m_ParticleLifetime = .5f;
		m_hSmokeTrail->m_StartColor.Init(0.0f, 0.3f, 0.8f);
		m_hSmokeTrail->m_EndColor.Init(0.0f,0.4f,1.0f);
		m_hSmokeTrail->m_StartSize = 12;
		m_hSmokeTrail->m_EndSize = m_hSmokeTrail->m_StartSize * 15;
		m_hSmokeTrail->m_SpawnRadius = 4;
		m_hSmokeTrail->m_MinSpeed = 46;
		m_hSmokeTrail->m_MaxSpeed = 55;
		m_hSmokeTrail->m_Opacity = 0.05f;

		m_hSmokeTrail->SetLifetime(0.3f);
		m_hSmokeTrail->SetAbsOrigin(GetAbsOrigin());
		//m_hSmokeTrail->FollowEntity(this);
	}
	Vector vecForward = GetAbsVelocity();
	VectorNormalize(vecForward);
	trace_t		tr;
	UTIL_TraceLine ( GetAbsOrigin(), GetAbsOrigin() + 60*vecForward, MASK_SHOT, 
		this, COLLISION_GROUP_NONE, &tr);

	UTIL_DecalTrace( &tr, "PaintSplatBlue" );

	EmitSound("HHGlassBreak");
	//BB: we do this wonky with the inflictor to get around team damage
	RadiusDamage ( CTakeDamageInfo( GetThrower(), this, m_flDamage, DMG_HOLY ), GetAbsOrigin(), m_DmgRadius, CLASS_NONE, NULL );

	UTIL_Remove( this );
}

void CGrenadeHH::GrenadeHHTouch( CBaseEntity *pOther )
{
	Assert( pOther );
	if ( !pOther->IsSolid() )
		return;

	// If I'm live go ahead and blow up
	if (m_bIsLive)
	{
		Detonate();
	}
	else
	{
		// If I'm not live, only blow up if I'm hitting an chacter that
		// is not the owner of the weapon
		CBaseCombatCharacter *pBCC = ToBaseCombatCharacter( pOther );
		if (pBCC && GetThrower() != pBCC)
		{
			m_bIsLive = true;
			Detonate();
		}
	}
}

bool CGrenadeHH::CreateVPhysics()
{
	// Create the object in the physics system
	VPhysicsInitNormal( SOLID_BBOX, 0, false );
	return true;
}

// this will hit only things that are in newCollisionGroup, but NOT in collisionGroupAlreadyChecked
class CTraceFilterCollisionGroupDelta : public CTraceFilterEntitiesOnly
{
public:
	// It does have a base, but we'll never network anything below here..
	DECLARE_CLASS_NOBASE( CTraceFilterCollisionGroupDelta );
	
	CTraceFilterCollisionGroupDelta( const IHandleEntity *passentity, int collisionGroupAlreadyChecked, int newCollisionGroup )
		: m_pPassEnt(passentity), m_collisionGroupAlreadyChecked( collisionGroupAlreadyChecked ), m_newCollisionGroup( newCollisionGroup )
	{
	}
	
	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
	{
		if ( !PassServerEntityFilter( pHandleEntity, m_pPassEnt ) )
			return false;
		CBaseEntity *pEntity = EntityFromEntityHandle( pHandleEntity );

		if ( pEntity )
		{
			if ( g_pGameRules->ShouldCollide( m_collisionGroupAlreadyChecked, pEntity->GetCollisionGroup() ) )
				return false;
			if ( g_pGameRules->ShouldCollide( m_newCollisionGroup, pEntity->GetCollisionGroup() ) )
				return true;
		}

		return false;
	}

protected:
	const IHandleEntity *m_pPassEnt;
	int		m_collisionGroupAlreadyChecked;
	int		m_newCollisionGroup;
};

void CGrenadeHH::VPhysicsUpdate( IPhysicsObject *pPhysics )
{
	BaseClass::VPhysicsUpdate( pPhysics );
	Vector vel;
	AngularImpulse angVel;
	pPhysics->GetVelocity( &vel, &angVel );
	
	Vector start = GetAbsOrigin();
	// find all entities that my collision group wouldn't hit, but COLLISION_GROUP_NONE would and bounce off of them as a ray cast
	CTraceFilterCollisionGroupDelta filter( this, GetCollisionGroup(), COLLISION_GROUP_NONE );
	trace_t tr;

	// UNDONE: Hull won't work with hitboxes - hits outer hull.  But the whole point of this test is to hit hitboxes.
#if 0
	UTIL_TraceHull( start, start + vel * gpGlobals->frametime, CollisionProp()->OBBMins(), CollisionProp()->OBBMaxs(), CONTENTS_HITBOX|CONTENTS_MONSTER|CONTENTS_SOLID, &filter, &tr );
#else
	UTIL_TraceLine( start, start + vel * gpGlobals->frametime, CONTENTS_HITBOX|CONTENTS_MONSTER|CONTENTS_SOLID, &filter, &tr );
#endif
	if ( tr.startsolid )
	{
		if ( !m_inSolid )
		{
			// UNDONE: Do a better contact solution that uses relative velocity?
			vel *= -GRENADE_COEFFICIENT_OF_RESTITUTION; // bounce backwards
			pPhysics->SetVelocity( &vel, NULL );
		}
		m_inSolid = true;
		return;
	}
	m_inSolid = false;
	if ( tr.DidHit() )
	{
		Vector dir = vel;
		VectorNormalize(dir);
		// send a tiny amount of damage so the character will react to getting bonked
		CTakeDamageInfo info( this, GetThrower(), pPhysics->GetMass() * vel, GetAbsOrigin(), 0.1f, DMG_CRUSH );
		tr.m_pEnt->TakeDamage( info );

		// reflect velocity around normal
		vel = -2.0f * tr.plane.normal * DotProduct(vel,tr.plane.normal) + vel;
		
		// absorb 80% in impact
		vel *= GRENADE_COEFFICIENT_OF_RESTITUTION;
		angVel *= -0.5f;
		pPhysics->SetVelocity( &vel, &angVel );
	}
}


void CGrenadeHH::Precache( void )
{
	PrecacheModel( HHGRENADE_MODEL );

	PrecacheScriptSound( "Grenade.Blip" );
	PrecacheScriptSound("HHGlassBreak");

	BaseClass::Precache();
}

void CGrenadeHH::SetVelocity( const Vector &velocity, const AngularImpulse &angVelocity )
{
	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();
	if ( pPhysicsObject )
	{
		pPhysicsObject->AddVelocity( &velocity, &angVelocity );
	}
}

int CGrenadeHH::OnTakeDamage( const CTakeDamageInfo &inputInfo )
{
	// Manually apply vphysics because BaseCombatCharacter takedamage doesn't call back to CBaseEntity OnTakeDamage
	VPhysicsTakeDamage( inputInfo );

	// Grenades only suffer blast damage and burn damage.
	if( !(inputInfo.GetDamageType() & (DMG_BLAST|DMG_BURN) ) )
		return 0;

	return BaseClass::OnTakeDamage( inputInfo );
}
