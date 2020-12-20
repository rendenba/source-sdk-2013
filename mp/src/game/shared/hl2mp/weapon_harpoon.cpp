//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"

#ifdef CLIENT_DLL
	#include "c_hl2mp_player.h"
	#include "c_te_effect_dispatch.h"
#else
	#include "hl2mp_player.h"
	#include "te_effect_dispatch.h"
	#include "rope.h"
	#include "IEffects.h"
	#include "Sprite.h"
	#include "SpriteTrail.h"
	#include "beam_shared.h"
#endif

#include "weapon_hl2mpbasehlmpcombatweapon.h"
#include "effect_dispatch_data.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//#define BOLT_MODEL			"models/crossbow_bolt.mdl"
#define BOLT_MODEL	"models/weapons/w_missile_closed.mdl"

#define HP_AIR_VELOCITY	2500 //3500
#define HP_WATER_VELOCITY	1500
#define	HP_SKIN_NORMAL	0
//#define BOLT_SKIN_GLOW		1

#define MAX_HARPOON_LENGTH 300 //this used to be 600 in version 2.5

#ifndef CLIENT_DLL

extern ConVar sk_plr_dmg_crossbow;
extern ConVar sk_npc_dmg_crossbow;

void TE_StickyBolt( IRecipientFilter& filter, float delay,	Vector vecDirection, const Vector *origin );

//-----------------------------------------------------------------------------
// Crossbow Bolt
//-----------------------------------------------------------------------------
class CHarpoonBolt : public CBaseCombatCharacter
{
	DECLARE_CLASS( CHarpoonBolt, CBaseCombatCharacter );

public:
	CHarpoonBolt() { };
	~CHarpoonBolt();

	Class_T Classify( void ) { return CLASS_NONE; }

public:
	void Spawn( void );
	void Precache( void );
	void BubbleThink( void );
	void BoltTouch( CBaseEntity *pOther );
	bool CreateVPhysics( void );
	unsigned int PhysicsSolidMaskForEntity() const;
	static CHarpoonBolt *BoltCreate( const Vector &vecOrigin, const QAngle &angAngles, int iDamage, CBasePlayer *pentOwner = NULL );

	CHandle<CRopeKeyframe> pRope;

protected:

	bool	CreateSprites( void );

	//CHandle<CSprite>		m_pGlowSprite;
	//CHandle<CSpriteTrail>	m_pGlowTrail;
	
	int		m_iDamage;

	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();
};
LINK_ENTITY_TO_CLASS( harpoon_bolt, CHarpoonBolt );

BEGIN_DATADESC( CHarpoonBolt )
	// Function Pointers
	DEFINE_FUNCTION( BubbleThink ),
	DEFINE_FUNCTION( BoltTouch ),

	// These are recreated on reload, they don't need storage
	//DEFINE_FIELD( m_pGlowSprite, FIELD_EHANDLE ),
	//DEFINE_FIELD( m_pGlowTrail, FIELD_EHANDLE ),

END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CHarpoonBolt, DT_HarpoonBolt )
END_SEND_TABLE()

CHarpoonBolt *CHarpoonBolt::BoltCreate( const Vector &vecOrigin, const QAngle &angAngles, int iDamage, CBasePlayer *pentOwner )
{
	// Create a new entity with CHarpoonBolt private data
	CHarpoonBolt *pBolt = (CHarpoonBolt *)CreateEntityByName( "harpoon_bolt" );
	UTIL_SetOrigin( pBolt, vecOrigin );
	pBolt->SetAbsAngles( angAngles );
	pBolt->Spawn();
	pBolt->SetOwnerEntity( pentOwner );

	pBolt->m_iDamage = iDamage;

	return pBolt;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHarpoonBolt::~CHarpoonBolt( void )
{
	/*if ( m_pGlowSprite )
	{
		UTIL_Remove( m_pGlowSprite );
	}*/
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CHarpoonBolt::CreateVPhysics( void )
{
	// Create the object in the physics system
	VPhysicsInitNormal( SOLID_BBOX, FSOLID_NOT_STANDABLE, false );

	return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
unsigned int CHarpoonBolt::PhysicsSolidMaskForEntity() const
{
	return ( BaseClass::PhysicsSolidMaskForEntity() | CONTENTS_HITBOX ) & ~CONTENTS_GRATE;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CHarpoonBolt::CreateSprites( void )
{
	// Start up the eye glow
	/*m_pGlowSprite = CSprite::SpriteCreate( "sprites/light_glow02_noz.vmt", GetLocalOrigin(), false );

	if ( m_pGlowSprite != NULL )
	{
		m_pGlowSprite->FollowEntity( this );
		m_pGlowSprite->SetTransparency( kRenderGlow, 255, 255, 255, 128, kRenderFxNoDissipation );
		m_pGlowSprite->SetScale( 0.2f );
		m_pGlowSprite->TurnOff();
	}*/

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHarpoonBolt::Spawn( void )
{
	Precache( );

	SetModel( "models/crossbow_bolt.mdl" );
	SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_CUSTOM );
	UTIL_SetSize( this, -Vector(0.25f,0.25f,0.25f), Vector(0.25f,0.25f,0.25f) ); //111x111
	SetSolid( SOLID_BBOX );
	SetGravity( 0.75f ); //0.05f
	
	// Make sure we're updated if we're underwater
	UpdateWaterState();

	SetTouch( &CHarpoonBolt::BoltTouch );

	SetThink( &CHarpoonBolt::BubbleThink );
	SetNextThink( gpGlobals->curtime + 0.1f );
	
	CreateSprites();

	// Make us glow until we've hit the wall
	m_nSkin = HP_SKIN_NORMAL;
}


void CHarpoonBolt::Precache( void )
{
	PrecacheModel( BOLT_MODEL );

	// This is used by C_TEStickyBolt, despte being different from above!!!
	PrecacheModel( "models/crossbow_bolt.mdl" );

	//PrecacheModel( "sprites/light_glow02_noz.vmt" );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOther - 
//-----------------------------------------------------------------------------
void CHarpoonBolt::BoltTouch( CBaseEntity *pOther )
{
	if ( !pOther->IsSolid() || pOther->IsSolidFlagSet(FSOLID_VOLUME_CONTENTS) )
		return;

	if ( pOther->m_takedamage != DAMAGE_NO )
	{
		trace_t	tr, tr2;
		tr = BaseClass::GetTouchTrace();
		Vector	vecNormalizedVel = GetAbsVelocity();

		ClearMultiDamage();
		VectorNormalize( vecNormalizedVel );

		if( GetOwnerEntity() && GetOwnerEntity()->IsPlayer() && pOther->IsServerdoll() )
		{
			//RAGDOLL
			//we have hit a ragdoll... see if it has an alive player
			if (((CRagdollProp *)pOther)->myBody != NULL && ((CRagdollProp *)pOther)->GetTeamNumber() == COVEN_TEAMID_VAMPIRES)
			{
				//kill the player
				CTakeDamageInfo	dmgInfo( this, GetOwnerEntity(), 999.0f, DMG_CLUB );
				CalculateMeleeDamageForce( &dmgInfo, vecNormalizedVel, tr.endpos, 0.7f );
				dmgInfo.SetDamagePosition( tr.endpos );
				pOther->DispatchTraceAttack( dmgInfo, vecNormalizedVel, &tr );
				((CRagdollProp *)pOther)->myBody->OnTakeDamage( dmgInfo );
			}
		}
		else
		{
			//NOT RAGDOLL
			CTakeDamageInfo	dmgInfo( this, GetOwnerEntity(), m_iDamage, DMG_BULLET | DMG_NEVERGIB );
			CalculateMeleeDamageForce( &dmgInfo, vecNormalizedVel, tr.endpos, 0.7f );
			dmgInfo.SetDamagePosition( tr.endpos );
			pOther->DispatchTraceAttack( dmgInfo, vecNormalizedVel, &tr );
		}

		CHL2MP_Player *pHL2MPOwner = ToHL2MPPlayer(GetOwnerEntity());
		if (pRope != NULL && pHL2MPOwner)
		{
			pHL2MPOwner->coven_hook_state = COVEN_HOOK_NONE;
			pRope->DetachPoint(0);
			pRope->SetThink( &CRopeKeyframe::SUB_Remove );
			pRope->SetNextThink( gpGlobals->curtime + 0.1f );
			pHL2MPOwner->pCovenRope = NULL;
			pRope = NULL;
		}

		ApplyMultiDamage();

		//Adrian: keep going through the glass.
		if ( pOther->GetCollisionGroup() == COLLISION_GROUP_BREAKABLE_GLASS )
			 return;

		SetAbsVelocity( Vector( 0, 0, 0 ) );

		// play body "thwack" sound
		EmitSound( "Weapon_Crossbow.BoltHitBody" );

		Vector vForward;

		AngleVectors( GetAbsAngles(), &vForward );
		VectorNormalize ( vForward );

		UTIL_TraceLine( GetAbsOrigin(),	GetAbsOrigin() + vForward * 128, MASK_OPAQUE, pOther, COLLISION_GROUP_NONE, &tr2 );

		if ( tr2.fraction != 1.0f )
		{
//			NDebugOverlay::Box( tr2.endpos, Vector( -16, -16, -16 ), Vector( 16, 16, 16 ), 0, 255, 0, 0, 10 );
//			NDebugOverlay::Box( GetAbsOrigin(), Vector( -16, -16, -16 ), Vector( 16, 16, 16 ), 0, 0, 255, 0, 10 );

			if ( tr2.m_pEnt == NULL || ( tr2.m_pEnt && tr2.m_pEnt->GetMoveType() == MOVETYPE_NONE ) )
			{
				CEffectData	data;

				data.m_vOrigin = tr2.endpos;
				data.m_vNormal = vForward;
				data.m_nEntIndex = tr2.fraction != 1.0f;
			
				DispatchEffect( "BoltImpact", data );
			}
		}
		
		SetTouch( NULL );
		SetThink( NULL );

		UTIL_Remove( this );
	}
	else
	{
		//DAMAGE_NO means sky or wall

		trace_t	tr;
		tr = BaseClass::GetTouchTrace();

		// See if we struck the world
		if ( pOther->GetMoveType() == MOVETYPE_NONE && !( tr.surface.flags & SURF_SKY ) )
		{
			//Hit good old earth

			EmitSound( "Weapon_Crossbow.BoltHitWorld" );

			// if what we hit is static architecture, can stay around for a while.
			Vector vecDir = GetAbsVelocity();
			float speed = VectorNormalize( vecDir );

			// See if we should reflect off this surface
			float hitDot = DotProduct( tr.plane.normal, -vecDir );
			
			if ( ( hitDot < 0.5f ) && ( speed > 100 ) )
			{
				//Bounce

				Vector vReflection = 2.0f * tr.plane.normal * hitDot + vecDir;
				
				QAngle reflectAngles;

				VectorAngles( vReflection, reflectAngles );

				SetLocalAngles( reflectAngles );

				SetAbsVelocity( vReflection * speed * 0.75f );

				// Start to sink faster
				SetGravity( 1.0f );
			}
			else
			{
				if (pRope != NULL && (pRope->m_fLockedPoints.Get() & 0x1))
				{
					CHL2MP_Player *pHL2MPOwner = ToHL2MPPlayer(GetOwnerEntity());
					if (pHL2MPOwner)
					{
						pHL2MPOwner->coven_hook_state = COVEN_HOOK_WORLD;
						pHL2MPOwner->coven_hook_anchor = GetAbsOrigin();
						pHL2MPOwner->coven_rope_constant = 0.0f;
					}
				}
				
				//FIXME: We actually want to stick (with hierarchy) to what we've hit
				SetMoveType( MOVETYPE_NONE );
			
				Vector vForward;

				AngleVectors( GetAbsAngles(), &vForward );
				VectorNormalize ( vForward );

				CEffectData	data;

				data.m_vOrigin = tr.endpos;
				data.m_vNormal = vForward;
				data.m_nEntIndex = 0;
			
				DispatchEffect( "BoltImpact", data );
				
				UTIL_ImpactTrace( &tr, DMG_BULLET );

				AddEffects( EF_NODRAW );
				SetTouch( NULL );
				SetThink( &CHarpoonBolt::SUB_Remove );
				SetNextThink( gpGlobals->curtime + 1.0f );

				//BB: no glow
				/*if ( m_pGlowSprite != NULL )
				{
					m_pGlowSprite->TurnOn();
					m_pGlowSprite->FadeAndDie( 3.0f );
				}*/
			}
			
			// Shoot some sparks
			if ( UTIL_PointContents( GetAbsOrigin() ) != CONTENTS_WATER)
			{
				g_pEffects->Sparks( GetAbsOrigin() );
			}
		}
		else
		{
			// Put a mark unless we've hit the sky
			if ( ( tr.surface.flags & SURF_SKY ) == false )
			{
				UTIL_ImpactTrace( &tr, DMG_BULLET );
			}

			UTIL_Remove( this );
		}
	}

	if ( g_pGameRules->IsMultiplayer() )
	{
//		SetThink( &CHarpoonBolt::ExplodeThink );
//		SetNextThink( gpGlobals->curtime + 0.1f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHarpoonBolt::BubbleThink( void )
{
	QAngle angNewAngles;

	VectorAngles( GetAbsVelocity(), angNewAngles );
	SetAbsAngles( angNewAngles );

	SetNextThink( gpGlobals->curtime + 0.1f );

	//check rope length and break if too long
	CHL2MP_Player *pHL2MPOwner = ToHL2MPPlayer(GetOwnerEntity());
	if (pHL2MPOwner && pHL2MPOwner->coven_hook_state == COVEN_HOOK_FIRED && pRope != NULL && (pRope->GetStartPoint()->GetAbsOrigin() - pRope->GetEndPoint()->GetAbsOrigin()).Length() > MAX_HARPOON_LENGTH * 0.9f)
	{
		pRope->SetThink( &CRopeKeyframe::SUB_Remove );
		pRope->SetNextThink(gpGlobals->curtime + 5.0f);
		pHL2MPOwner->coven_hook_state = COVEN_HOOK_NONE;
		pHL2MPOwner->pCovenRope = NULL;
		pRope->DetachPoint(0);
		SetThink( NULL );
	}

	if ( GetWaterLevel()  == 0 )
		return;

	UTIL_BubbleTrail( GetAbsOrigin() - GetAbsVelocity() * 0.1f, GetAbsOrigin(), 5 );
}

#endif

//-----------------------------------------------------------------------------
// CWeaponHarpoon
//-----------------------------------------------------------------------------

#ifdef CLIENT_DLL
#define CWeaponHarpoon C_WeaponHarpoon
#endif

class CWeaponHarpoon : public CBaseHL2MPCombatWeapon
{
	DECLARE_CLASS( CWeaponHarpoon, CBaseHL2MPCombatWeapon );
public:

	CWeaponHarpoon( void );
	
	virtual void	Precache( void );
	virtual void	PrimaryAttack( void );
	virtual void	SecondaryAttack( void );
	virtual bool	Deploy( void );
	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo = NULL );
	virtual bool	Reload( void );
	virtual void	ItemPostFrame( void );
	virtual void	ItemBusyFrame( void );
	virtual bool	SendWeaponAnim( int iActivity );
	virtual bool	ReloadOrSwitchWeapons( void );

#ifndef CLIENT_DLL
	virtual void Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
#endif

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

private:
	
	void	SetSkin( int skinNum );
	void	CheckZoomToggle( void );
	void	FireBolt( void );
	void	ToggleZoom( void );
	
	// Various states for the crossbow's charger
	enum ChargerState_t
	{
		CHARGER_STATE_START_LOAD,
		CHARGER_STATE_START_CHARGE,
		CHARGER_STATE_READY,
		CHARGER_STATE_DISCHARGE,
		CHARGER_STATE_OFF,
	};

	void	CreateChargerEffects( void );
	void	SetChargerState( ChargerState_t state );
	void	DoLoadEffect( void );

#ifndef CLIENT_DLL
	DECLARE_ACTTABLE();
#endif

private:
	
	// Charger effects
	ChargerState_t		m_nChargeState;

#ifndef CLIENT_DLL
	CHandle<CSprite>	m_hChargerSprite;
#endif

	CNetworkVar( bool,	m_bInZoom );
	CNetworkVar( bool,	m_bMustReload );

	CWeaponHarpoon( const CWeaponHarpoon & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponHarpoon, DT_WeaponHarpoon )

BEGIN_NETWORK_TABLE( CWeaponHarpoon, DT_WeaponHarpoon )
#ifdef CLIENT_DLL
	RecvPropBool( RECVINFO( m_bInZoom ) ),
	RecvPropBool( RECVINFO( m_bMustReload ) ),
#else
	SendPropBool( SENDINFO( m_bInZoom ) ),
	SendPropBool( SENDINFO( m_bMustReload ) ),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CWeaponHarpoon )
	DEFINE_PRED_FIELD( m_bInZoom, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bMustReload, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_harpoon, CWeaponHarpoon );

PRECACHE_WEAPON_REGISTER( weapon_harpoon );

#ifndef CLIENT_DLL

acttable_t	CWeaponHarpoon::m_acttable[] = 
{
	{ ACT_HL2MP_IDLE,					ACT_HL2MP_IDLE_CROSSBOW,					false },
	{ ACT_HL2MP_RUN,					ACT_HL2MP_RUN_CROSSBOW,						false },
	{ ACT_HL2MP_IDLE_CROUCH,			ACT_HL2MP_IDLE_CROUCH_CROSSBOW,				false },
	{ ACT_HL2MP_WALK_CROUCH,			ACT_HL2MP_WALK_CROUCH_CROSSBOW,				false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK,	ACT_HL2MP_GESTURE_RANGE_ATTACK_CROSSBOW,	false },
	{ ACT_HL2MP_GESTURE_RELOAD,			ACT_HL2MP_GESTURE_RELOAD_CROSSBOW,			false },
	{ ACT_HL2MP_JUMP,					ACT_HL2MP_JUMP_CROSSBOW,					false },
};

IMPLEMENT_ACTTABLE(CWeaponHarpoon);

#endif

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponHarpoon::CWeaponHarpoon( void )
{
	m_bReloadsSingly	= true;
	m_bFiresUnderwater	= true;
	m_bInZoom			= false;
	m_bMustReload		= false;
}

//#define	Harpoon_GLOW_SPRITE	"sprites/light_glow02_noz.vmt"
//#define	Harpoon_GLOW_SPRITE2	"sprites/blueflare1.vmt"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponHarpoon::Precache( void )
{
#ifndef CLIENT_DLL
	UTIL_PrecacheOther( "crossbow_bolt" );
#endif

	PrecacheScriptSound( "Weapon_Crossbow.BoltHitBody" );
	PrecacheScriptSound( "Weapon_Crossbow.BoltHitWorld" );
	PrecacheScriptSound( "Weapon_Crossbow.BoltSkewer" );

	//PrecacheModel( Harpoon_GLOW_SPRITE );
	//PrecacheModel( Harpoon_GLOW_SPRITE2 );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponHarpoon::PrimaryAttack( void )
{
#ifndef CLIENT_DLL
	CHL2MP_Player *pHL2MPOwner = ToHL2MPPlayer(GetOwnerEntity());
	if (pHL2MPOwner && pHL2MPOwner->coven_hook_state != COVEN_HOOK_NONE)
	{
		pHL2MPOwner->coven_hook_state = COVEN_HOOK_NONE;
		if (pHL2MPOwner->pCovenRope != NULL)
		{
			pHL2MPOwner->pCovenRope->DetachPoint(0);
			pHL2MPOwner->pCovenRope->SetThink(&CRopeKeyframe::SUB_Remove);
			pHL2MPOwner->pCovenRope->SetNextThink(gpGlobals->curtime + 5.0f);
			pHL2MPOwner->pCovenRope = NULL;
		}
	}
#endif
	if ( m_bInZoom && g_pGameRules->IsMultiplayer() )
	{
//		FireSniperBolt();
		FireBolt();
	}
	else
	{
		FireBolt();
	}

	// Signal a reload
	m_bMustReload = true;

	SetWeaponIdleTime( gpGlobals->curtime + SequenceDuration( ACT_VM_PRIMARYATTACK ) );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponHarpoon::SecondaryAttack( void )
{
	//NOTENOTE: The zooming is handled by the post/busy frames
#ifndef CLIENT_DLL
	CHL2MP_Player *pHL2MPOwner = ToHL2MPPlayer(GetOwnerEntity());
	if (pHL2MPOwner->coven_hook_state != COVEN_HOOK_NONE)
	{
		pHL2MPOwner->coven_hook_state = COVEN_HOOK_NONE;
		if (pHL2MPOwner->pCovenRope != NULL)
		{
			pHL2MPOwner->pCovenRope->DetachPoint(0);
			pHL2MPOwner->pCovenRope->SetThink(&CRopeKeyframe::SUB_Remove);
			pHL2MPOwner->pCovenRope->SetNextThink(gpGlobals->curtime + 5.0f);
			pHL2MPOwner->pCovenRope = NULL;
		}
	}
#endif
}

bool CWeaponHarpoon::ReloadOrSwitchWeapons( void )
{
#ifndef CLIENT_DLL
	CHL2MP_Player *pHL2MPOwner = ToHL2MPPlayer(GetOwnerEntity());
	if (pHL2MPOwner->coven_hook_state != COVEN_HOOK_NONE)
		return false;
#endif
	return BaseClass::ReloadOrSwitchWeapons();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponHarpoon::Reload( void )
{
	if ( BaseClass::Reload() )
	{
		m_bMustReload = false;
		CBaseCombatCharacter *pOwner = GetOwner();
		if (!pOwner)
			return false;
		float factor = 1.0f;
		if (ToHL2MPPlayer(pOwner)->HasStatus(COVEN_STATUS_HASTE))
			factor = 1.0f / (1.0f + (ToHL2MPPlayer(pOwner)->GetStatusMagnitude(COVEN_STATUS_HASTE) * 0.01f));

		m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration() * factor;
		pOwner->m_flNextAttack = m_flNextSecondaryAttack = gpGlobals->curtime + 0.1f;

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponHarpoon::CheckZoomToggle( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	
	if ( pPlayer->m_afButtonPressed & IN_ATTACK2 )
	{
		ToggleZoom();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponHarpoon::ItemBusyFrame( void )
{
	// Allow zoom toggling even when we're reloading
	//BB: DONT DO THIS PLZ
	//CheckZoomToggle();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponHarpoon::ItemPostFrame( void )
{
	// Allow zoom toggling
	//BB: DONT DO THIS
	//CheckZoomToggle();

	if ( m_bMustReload && HasWeaponIdleTimeElapsed() )
	{
		Reload();
	}

	BaseClass::ItemPostFrame();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponHarpoon::FireBolt( void )
{
	if ( m_iClip1 <= 0 )
	{
		if ( !m_bFireOnEmpty )
		{
			Reload();
		}
		else
		{
			WeaponSound( EMPTY );
			m_flNextPrimaryAttack = 0.15f;
		}

		return;
	}

	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	
	if ( pOwner == NULL )
		return;

	CHL2MP_Player *pHL2Player = ToHL2MPPlayer(pOwner);

#ifndef CLIENT_DLL
	Vector vecAiming	= pOwner->GetAutoaimVector( 0 );	
	Vector vecSrc		= pOwner->Weapon_ShootPosition();

	QAngle angAiming;
	VectorAngles( vecAiming, angAiming );

	CHarpoonBolt *pBolt = CHarpoonBolt::BoltCreate( vecSrc, angAiming, GetHL2MPWpnData().m_iPlayerDamage, pOwner );

	if ( pOwner->GetWaterLevel() == 3 )
	{
		pBolt->SetAbsVelocity( vecAiming * HP_WATER_VELOCITY );
	}
	else
	{
		pBolt->SetAbsVelocity( vecAiming * HP_AIR_VELOCITY );
	}

	pBolt->pRope = CRopeKeyframe::Create(GetOwner(), pBolt, GetOwner()->LookupAttachment("ValveBiped.Bip01_L_Hand"));
	if (pBolt->pRope != NULL)
	{
		CHL2MP_Player *pHL2MPOwner = ToHL2MPPlayer(GetOwner());
		pHL2MPOwner->coven_hook_state = COVEN_HOOK_FIRED;
		pHL2MPOwner->pCovenRope = pBolt->pRope;
		pBolt->pRope->m_Width = 1;
		pBolt->pRope->m_nSegments = 10;
		pBolt->pRope->EnableWind(false);
		pBolt->pRope->SetupHangDistance(0);
		pBolt->pRope->m_RopeLength = MAX_HARPOON_LENGTH;
		pBolt->pRope->EnableCollision();
		pBolt->pRope->EnablePlayerWeaponAttach(true);
	}

#endif

	m_iClip1--;

	pOwner->ViewPunch( QAngle( -2, 0, 0 ) );

	WeaponSound( SINGLE );
	WeaponSound( SPECIAL2 );

	SendWeaponAnim( ACT_VM_PRIMARYATTACK );

	if ( !m_iClip1 && pOwner->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 )
	{
		// HEV suit - indicate out of ammo condition
		pOwner->SetSuitUpdate("!HEV_AMO0", FALSE, 0);
	}

	if (pHL2Player->HasStatus(COVEN_STATUS_HASTE))
		m_flNextPrimaryAttack = gpGlobals->curtime + 0.75f - pHL2Player->GetStatusMagnitude(COVEN_STATUS_HASTE) * 0.0075f;
	else
		m_flNextPrimaryAttack = gpGlobals->curtime + 0.75f;

	m_flNextSecondaryAttack = gpGlobals->curtime + 0.1f;

	DoLoadEffect();
	SetChargerState( CHARGER_STATE_DISCHARGE );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponHarpoon::Deploy( void )
{
	if ( m_iClip1 <= 0 )
	{
		return DefaultDeploy( (char*)GetViewModel(), (char*)GetWorldModel(), ACT_CROSSBOW_DRAW_UNLOADED, (char*)GetAnimPrefix() );
	}

	SetSkin( HP_SKIN_NORMAL );

	return BaseClass::Deploy();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pSwitchingTo - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CWeaponHarpoon::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	if ( m_bInZoom )
	{
		ToggleZoom();
	}

	SetChargerState( CHARGER_STATE_OFF );

	return BaseClass::Holster( pSwitchingTo );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponHarpoon::ToggleZoom( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	
	if ( pPlayer == NULL )
		return;

#ifndef CLIENT_DLL

	if ( m_bInZoom )
	{
		if ( pPlayer->SetFOV( this, 0, 0.2f ) )
		{
			m_bInZoom = false;
		}
	}
	else
	{
		if ( pPlayer->SetFOV( this, 20, 0.1f ) )
		{
			m_bInZoom = true;
		}
	}
#endif
}

#define	BOLT_TIP_ATTACHMENT	2

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponHarpoon::CreateChargerEffects( void )
{
#ifndef CLIENT_DLL
	/*CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( m_hChargerSprite != NULL )
		return;

	m_hChargerSprite = CSprite::SpriteCreate( Harpoon_GLOW_SPRITE, GetAbsOrigin(), false );

	if ( m_hChargerSprite )
	{
		m_hChargerSprite->SetAttachment( pOwner->GetViewModel(), BOLT_TIP_ATTACHMENT );
		m_hChargerSprite->SetTransparency( kRenderTransAdd, 255, 128, 0, 255, kRenderFxNoDissipation );
		m_hChargerSprite->SetBrightness( 0 );
		m_hChargerSprite->SetScale( 0.1f );
		m_hChargerSprite->TurnOff();
	}*/
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : skinNum - 
//-----------------------------------------------------------------------------
void CWeaponHarpoon::SetSkin( int skinNum )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	
	if ( pOwner == NULL )
		return;

	CBaseViewModel *pViewModel = pOwner->GetViewModel();

	if ( pViewModel == NULL )
		return;

	pViewModel->m_nSkin = skinNum;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponHarpoon::DoLoadEffect( void )
{
	SetSkin( HP_SKIN_NORMAL );

	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( pOwner == NULL )
		return;

	CBaseViewModel *pViewModel = pOwner->GetViewModel();

	if ( pViewModel == NULL )
		return;

	CEffectData	data;

#ifdef CLIENT_DLL
	data.m_hEntity = pViewModel->GetRefEHandle();
#else
	data.m_nEntIndex = pViewModel->entindex();
#endif
	data.m_nAttachmentIndex = 1;

	DispatchEffect( "HarpoonLoad", data );

#ifndef CLIENT_DLL

	/*CSprite *pBlast = CSprite::SpriteCreate( Harpoon_GLOW_SPRITE2, GetAbsOrigin(), false );

	if ( pBlast )
	{
		pBlast->SetAttachment( pOwner->GetViewModel(), 1 );
		pBlast->SetTransparency( kRenderTransAdd, 255, 255, 255, 255, kRenderFxNone );
		pBlast->SetBrightness( 128 );
		pBlast->SetScale( 0.2f );
		pBlast->FadeOutFromSpawn();
	}*/
#endif
	
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : state - 
//-----------------------------------------------------------------------------
void CWeaponHarpoon::SetChargerState( ChargerState_t state )
{
	// Make sure we're setup
	CreateChargerEffects();

	// Don't do this twice
	if ( state == m_nChargeState )
		return;

	m_nChargeState = state;

	switch( m_nChargeState )
	{
	case CHARGER_STATE_START_LOAD:
	
		WeaponSound( SPECIAL1 );
		
		// Shoot some sparks and draw a beam between the two outer points
		DoLoadEffect();
		
		break;
#ifndef CLIENT_DLL
	case CHARGER_STATE_START_CHARGE:
		{
			if ( m_hChargerSprite == NULL )
				break;
			
			m_hChargerSprite->SetBrightness( 32, 0.5f );
			m_hChargerSprite->SetScale( 0.025f, 0.5f );
			m_hChargerSprite->TurnOn();
		}

		break;

	case CHARGER_STATE_READY:
		{
			// Get fully charged
			if ( m_hChargerSprite == NULL )
				break;
			
			m_hChargerSprite->SetBrightness( 80, 1.0f );
			m_hChargerSprite->SetScale( 0.1f, 0.5f );
			m_hChargerSprite->TurnOn();
		}

		break;

	case CHARGER_STATE_DISCHARGE:
		{
			SetSkin( HP_SKIN_NORMAL );
			
			if ( m_hChargerSprite == NULL )
				break;
			
			m_hChargerSprite->SetBrightness( 0 );
			m_hChargerSprite->TurnOff();
		}

		break;
#endif
	case CHARGER_STATE_OFF:
		{
			SetSkin( HP_SKIN_NORMAL );

#ifndef CLIENT_DLL
			if ( m_hChargerSprite == NULL )
				break;
			
			m_hChargerSprite->SetBrightness( 0 );
			m_hChargerSprite->TurnOff();
#endif
		}
		break;

	default:
		break;
	}
}

#ifndef CLIENT_DLL
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEvent - 
//			*pOperator - 
//-----------------------------------------------------------------------------
void CWeaponHarpoon::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	switch( pEvent->event )
	{
	case EVENT_WEAPON_THROW:
		SetChargerState( CHARGER_STATE_START_LOAD );
		break;

	case EVENT_WEAPON_THROW2:
		SetChargerState( CHARGER_STATE_START_CHARGE );
		break;
	
	case EVENT_WEAPON_THROW3:
		SetChargerState( CHARGER_STATE_READY );
		break;

	default:
		BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
		break;
	}
}

#endif

//-----------------------------------------------------------------------------
// Purpose: Set the desired activity for the weapon and its viewmodel counterpart
// Input  : iActivity - activity to play
//-----------------------------------------------------------------------------
bool CWeaponHarpoon::SendWeaponAnim( int iActivity )
{
	int newActivity = iActivity;

	// The last shot needs a non-loaded activity
	if ( ( newActivity == ACT_VM_IDLE ) && ( m_iClip1 <= 0 ) )
	{
		newActivity = ACT_VM_FIDGET;
	}

	//For now, just set the ideal activity and be done with it
	return BaseClass::SendWeaponAnim( newActivity );
}
