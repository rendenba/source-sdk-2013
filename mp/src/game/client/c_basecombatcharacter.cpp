//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Client's C_BaseCombatCharacter entity
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "c_basecombatcharacter.h"
#include "dlight.h"
#include "r_efx.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#if defined( CBaseCombatCharacter )
#undef CBaseCombatCharacter	
#endif

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_BaseCombatCharacter::C_BaseCombatCharacter()
{
	for ( int i=0; i < m_iAmmo.Count(); i++ )
	{
		m_iAmmo.Set( i, 0 );
	}
}

void C_BaseCombatCharacter::AddEntity(void)
{
	BaseClass::AddEntity();
	if (IsEffectActive(EF_DIMLIGHT))
	{
		Vector vForward;
		AngleVectors(EyeAngles(), &vForward);
		trace_t tr;
		UTIL_TraceLine(EyePosition(), EyePosition() + (vForward * 250), MASK_SHOT, this, COLLISION_GROUP_NONE, &tr);

		dlight_t *el = effects->CL_AllocDlight(0);
		el->origin = tr.endpos;
		el->radius = 75;
		el->color.r = 200;
		el->color.g = 200;
		el->color.b = 200;
		el->die = gpGlobals->curtime + 0.1;
	}
}

void CFXCharSprite::Update( Vector newpos, Vector newcolor, bool draw )
{
	//TGB: is this a new position/color?
	if ( (newpos != m_FXData.m_vecOrigin) || (newcolor != m_FXData.m_Color))
	{
		//update position
		m_FXData.m_vecOrigin = newpos;
		//and color
		m_FXData.m_Color = newcolor;
	}
/*
	if (m_FXData.m_flYaw < 360)
	{
		m_FXData.m_flYaw = min(360, m_FXData.m_flYaw + 25);
	}
	else
		m_FXData.m_flYaw = 25;
*/	
	if ( draw )
		Draw();
}
void CFXCharSprite::Draw( )
{
	float scale = m_FXData.m_flStartScale;
	float alpha = m_FXData.m_flStartAlpha;
	
	//Bind the material
	CMatRenderContextPtr pRenderContext( materials );
	IMesh* pMesh = pRenderContext->GetDynamicMesh( true, NULL, NULL, m_FXData.m_pMaterial );
	CMeshBuilder meshBuilder;

	meshBuilder.Begin( pMesh, MATERIAL_QUADS, 1 );

	Vector	pos;
	Vector	vRight, vUp;

	float color[4];

	color[0] = m_FXData.m_Color[0];
	color[1] = m_FXData.m_Color[1];
	color[2] = m_FXData.m_Color[2];
	color[3] = alpha;

	VectorVectors( m_FXData.m_vecNormal, vRight, vUp );

	Vector	rRight, rUp;

	rRight	= ( vRight * cos( DEG2RAD( m_FXData.m_flYaw ) ) ) - ( vUp * sin( DEG2RAD( m_FXData.m_flYaw ) ) );
	rUp		= ( vRight * cos( DEG2RAD( m_FXData.m_flYaw+90.0f ) ) ) - ( vUp * sin( DEG2RAD( m_FXData.m_flYaw+90.0f ) ) );

	vRight	= rRight * ( scale * 0.5f );
	vUp		= rUp * ( scale * 0.5f );

	pos = m_FXData.m_vecOrigin + vRight - vUp;

	meshBuilder.Position3fv( pos.Base() );
	meshBuilder.Normal3fv( m_FXData.m_vecNormal.Base() );
	meshBuilder.TexCoord2f( 0, 1.0f, 1.0f );
	meshBuilder.Color4fv( color );
	meshBuilder.AdvanceVertex();

	pos = m_FXData.m_vecOrigin - vRight - vUp;

	meshBuilder.Position3fv( pos.Base() );
	meshBuilder.Normal3fv( m_FXData.m_vecNormal.Base() );
	meshBuilder.TexCoord2f( 0, 0.0f, 1.0f );
	meshBuilder.Color4fv( color );
	meshBuilder.AdvanceVertex();

	pos = m_FXData.m_vecOrigin - vRight + vUp;

	meshBuilder.Position3fv( pos.Base() );
	meshBuilder.Normal3fv( m_FXData.m_vecNormal.Base() );
	meshBuilder.TexCoord2f( 0, 0.0f, 0.0f );
	meshBuilder.Color4fv( color );
	meshBuilder.AdvanceVertex();

	pos = m_FXData.m_vecOrigin + vRight + vUp;

	meshBuilder.Position3fv( pos.Base() );
	meshBuilder.Normal3fv( m_FXData.m_vecNormal.Base() );
	meshBuilder.TexCoord2f( 0, 1.0f, 0.0f );
	meshBuilder.Color4fv( color );
	meshBuilder.AdvanceVertex();

	meshBuilder.End();
	pMesh->Draw();
}

void CFXCharSprite::Destroy( void )
{
	//Release the material
	if ( m_FXData.m_pMaterial != NULL )
	{
		m_FXData.m_pMaterial->DecrementReferenceCount();
		m_FXData.m_pMaterial = NULL;
	}
}

void C_BaseCombatCharacter::CreateObjectiveCircle()
{
	//only if we don't already have one
	if (m_objectiveCircle) return;

	//doubling brightness because the sprite is less visible now that it's drawn only once
	const float brightness = clamp((0.25f * 3), 0.1, 1.0);

	//TGB: this defines a colour that moves from green to red depending on character's health
	//used in selection material and dedicated health circle
	//color should be redder as this guy approaches death
	const float redness = 1.0f;
	//as the ring gets redder it should become less green, or we'll end up yellow when near death
	const float greenness = 0.0f;

	//make quad data
	FXQuadData_t data;
	//only using start values
	data.SetAlpha( 0.5, 0 );
	data.SetScale( 25.0f, 0 );
	data.SetMaterial( "effects/objective_marker" );
	data.SetNormal( Vector(0,0,1) );
	data.SetOrigin( GetAbsOrigin() + Vector(0,0,3) );
	data.SetColor( redness*brightness, greenness*brightness, 0 );
	data.SetScaleBias( 0 );
	data.SetAlphaBias( 0 );
	data.SetYaw( 0, 0 );

	m_objectiveCircle = new CFXCharSprite( data );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_BaseCombatCharacter::~C_BaseCombatCharacter()
{
}

/*
//-----------------------------------------------------------------------------
// Purpose: Returns the amount of ammunition of the specified type the character's carrying
//-----------------------------------------------------------------------------
int	C_BaseCombatCharacter::GetAmmoCount( char *szName ) const
{
	return GetAmmoCount( g_pGameRules->GetAmmoDef()->Index(szName) );
}
*/

#ifdef GLOWS_ENABLE
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_BaseCombatCharacter::GetGlowEffectColor(float *r, float *g, float *b)
{
	if (GetCovenStatusEffects() & (1 << COVEN_STATUS_HAS_CTS))
	{
		*r = 1.0f;
		*g = 1.0f;
		*b = 0.0f;
	}
	else if (GetTeamNumber() == COVEN_TEAMID_VAMPIRES && GetTeamNumber() == C_BasePlayer::GetLocalPlayer()->GetTeamNumber())
	{
		*r = 0.0f;
		*g = 1.0f;
		*b = 0.0f;
	}
	else
	{
		*r = 0.76f;
		*g = 0.76f;
		*b = 0.76f;
	}
}
#endif // GLOWS_ENABLE

//-----------------------------------------------------------------------------
// Purpose: Overload our muzzle flash and send it to any actively held weapon
//-----------------------------------------------------------------------------
void C_BaseCombatCharacter::DoMuzzleFlash()
{
	// Our weapon takes our muzzle flash command
	C_BaseCombatWeapon *pWeapon = GetActiveWeapon();
	if ( pWeapon )
	{
		pWeapon->DoMuzzleFlash();
		//NOTENOTE: We do not chain to the base here
	}
	else
	{
		BaseClass::DoMuzzleFlash();
	}
}

IMPLEMENT_CLIENTCLASS(C_BaseCombatCharacter, DT_BaseCombatCharacter, CBaseCombatCharacter);

// Only send active weapon index to local player
BEGIN_RECV_TABLE_NOBASE( C_BaseCombatCharacter, DT_BCCLocalPlayerExclusive )
	RecvPropTime( RECVINFO( m_flNextAttack ) ),
END_RECV_TABLE();


BEGIN_RECV_TABLE(C_BaseCombatCharacter, DT_BaseCombatCharacter)
	RecvPropDataTable( "bcc_localdata", 0, 0, &REFERENCE_RECV_TABLE(DT_BCCLocalPlayerExclusive) ),
	RecvPropEHandle( RECVINFO( m_hActiveWeapon ) ),
	RecvPropArray3( RECVINFO_ARRAY(m_hMyWeapons), RecvPropEHandle( RECVINFO( m_hMyWeapons[0] ) ) ),
#ifdef INVASION_CLIENT_DLL
	RecvPropInt( RECVINFO( m_iPowerups ) ),
#endif

END_RECV_TABLE()


BEGIN_PREDICTION_DATA( C_BaseCombatCharacter )

	DEFINE_PRED_ARRAY( m_iAmmo, FIELD_INTEGER,  MAX_AMMO_TYPES, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flNextAttack, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_hActiveWeapon, FIELD_EHANDLE, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_ARRAY( m_hMyWeapons, FIELD_EHANDLE, MAX_WEAPONS, FTYPEDESC_INSENDTABLE ),

END_PREDICTION_DATA()
