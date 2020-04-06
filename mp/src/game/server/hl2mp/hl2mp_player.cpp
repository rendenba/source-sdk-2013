//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Player for HL2.
//
//=============================================================================//

#include "cbase.h"
#include "weapon_hl2mpbasehlmpcombatweapon.h"
#include "hl2mp_player.h"
#include "globalstate.h"
#include "game.h"
#include "explode.h"
#include "gamerules.h"
#include "EntityFlame.h"
#include "hl2mp_player_shared.h"
#include "predicted_viewmodel.h"
#include "viewport_panel_names.h"
#include "in_buttons.h"
#include "hl2mp_gamerules.h"
#include "KeyValues.h"
#include "team.h"
#include "weapon_hl2mpbase.h"
#include "grenade_satchel.h"
#include "eventqueue.h"
#include "items.h"
#include "gamestats.h"
#include "grenade_hh.h"

#include "grenade_tripmine.h"

#include "prop_soul.h"

#include "hl2mp_bot_temp.h"

#include "beam_flags.h"
#include "te_effect_dispatch.h"

#include "physics_prop_ragdoll.h"

#include "engine/IEngineSound.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"

#include "ilagcompensationmanager.h"

int g_iLastCitizenModel = 0;
int g_iLastCombineModel = 0;

extern short	g_sModelIndexFireball;		// (in combatweapon.cpp) holds the index for the fireball 
extern short	g_sModelIndexWExplosion;	// (in combatweapon.cpp) holds the index for the underwater explosion
extern short	g_sModelIndexSmoke;			// (in combatweapon.cpp) holds the index for the smoke cloud

CBaseEntity	 *g_pLastCombineSpawn = NULL;
CBaseEntity	 *g_pLastRebelSpawn = NULL;
extern CBaseEntity				*g_pLastSpawn;
extern ConVar sv_coven_freezetime;
extern ConVar coven_ignore_respawns;
extern ConVar sv_coven_pts_cts;
extern ConVar sv_coven_xp_basekill;
extern ConVar sv_coven_xp_inckill;
extern ConVar sv_coven_xp_diffkill;
extern ConVar sv_coven_cts_returntime;

#define HL2MP_COMMAND_MAX_RATE 0.3

void DropPrimedFragGrenade( CHL2MP_Player *pPlayer, CBaseCombatWeapon *pGrenade );

LINK_ENTITY_TO_CLASS( player, CHL2MP_Player );

LINK_ENTITY_TO_CLASS( info_player_combine, CPointEntity );
LINK_ENTITY_TO_CLASS( info_player_rebel, CPointEntity );

IMPLEMENT_SERVERCLASS_ST(CHL2MP_Player, DT_HL2MP_Player)
	SendPropInt( SENDINFO(m_iClass), 4 ),
	SendPropInt( SENDINFO(m_iLevel)),
	SendPropAngle( SENDINFO_VECTORELEM(m_angEyeAngles, 0), 11, SPROP_CHANGES_OFTEN ),
	SendPropAngle( SENDINFO_VECTORELEM(m_angEyeAngles, 1), 11, SPROP_CHANGES_OFTEN ),
	SendPropEHandle( SENDINFO( m_hRagdoll ) ),
	SendPropInt( SENDINFO( m_iSpawnInterpCounter), 4 ),
	SendPropInt( SENDINFO( m_iPlayerSoundType), 3 ),
	
	SendPropExclude( "DT_BaseAnimating", "m_flPoseParameter" ),
	SendPropExclude( "DT_BaseFlex", "m_viewtarget" ),

//	SendPropExclude( "DT_ServerAnimationData" , "m_flCycle" ),	
//	SendPropExclude( "DT_AnimTimeMustBeFirst" , "m_flAnimTime" ),
	
END_SEND_TABLE()

BEGIN_DATADESC( CHL2MP_Player )
END_DATADESC()

const char *g_ppszRandomCitizenModels[] = 
{
	"models/humans/group03/male_01.mdl",
	"models/humans/group03/male_02.mdl",
	"models/humans/group03/female_01.mdl",
	"models/humans/group03/male_03.mdl",
	"models/humans/group03/female_02.mdl",
	"models/humans/group03/male_04.mdl",
	"models/humans/group03/female_03.mdl",
	"models/humans/group03/male_05.mdl",
	"models/humans/group03/female_04.mdl",
	"models/humans/group03/male_06.mdl",
	"models/humans/group03/female_06.mdl",
	"models/humans/group03/male_07.mdl",
	"models/humans/group03/female_07.mdl",
	"models/humans/group03/male_08.mdl",
	"models/humans/group03/male_09.mdl",
};

const char *g_ppszRandomCombineModels[] =
{
	"models/combine_soldier.mdl",
	"models/combine_soldier_prisonguard.mdl",
	"models/combine_super_soldier.mdl",
	"models/police.mdl",
};


#define MAX_COMBINE_MODELS 4
#define MODEL_CHANGE_INTERVAL 5.0f
#define TEAM_CHANGE_INTERVAL 5.0f

#define HL2MPPLAYER_PHYSDAMAGE_SCALE 4.0f

#pragma warning( disable : 4355 )

CON_COMMAND( chooseclass, "Opens a menu for class choose" )
{
	CHL2MP_Player *pPlayer = ToHL2MPPlayer( UTIL_GetCommandClient() );
	if (!pPlayer)
		return;

	pPlayer->ShowViewPortPanel("team", false, NULL);

	if (pPlayer->GetTeamNumber() == TEAM_COMBINE)
	{
		pPlayer->ShowViewPortPanel( "class", true, NULL );
	}
	else
	{
		pPlayer->ShowViewPortPanel( "class2", true, NULL );
	}
}

CON_COMMAND( levelmenu, "Opens a menu for leveling up")
{
	CHL2MP_Player *pPlayer = ToHL2MPPlayer( UTIL_GetCommandClient() );
	if (!pPlayer)
		return;
	KeyValues *data = new KeyValues("data");
	data->SetBool( "auto", false );
	data->SetInt( "level", pPlayer->covenLevelCounter);
	data->SetInt( "load1", pPlayer->GetLoadout(0));
	data->SetInt( "load2", pPlayer->GetLoadout(1));
	data->SetInt( "load3", pPlayer->GetLoadout(2));
	data->SetInt( "load4", pPlayer->GetLoadout(3));
	data->SetInt( "class", pPlayer->covenClassID);
	pPlayer->ShowViewPortPanel( PANEL_LEVEL, true, data );
	data->deleteThis();
}

CON_COMMAND( tracert, "Trace hit" )
{
	CHL2MP_Player *pPlayer = ToHL2MPPlayer( UTIL_GetCommandClient() );
	if (!pPlayer)
		return;

	trace_t	tr;
	QAngle angle = pPlayer->GetAbsAngles();
	Vector forward;
	AngleVectors( angle, &forward );
	VectorNormalize(forward);
	UTIL_TraceLine(pPlayer->Weapon_ShootPosition(), pPlayer->Weapon_ShootPosition()+forward*200, MASK_SHOT_HULL, pPlayer, COLLISION_GROUP_WEAPON, &tr);
	if (tr.fraction != 1.0)
	{
		Msg("%s\n", tr.m_pEnt->GetClassname());
	}
}

CHL2MP_Player::CHL2MP_Player() : m_PlayerAnimState( this )
{
	m_angEyeAngles.Init();

	m_iLevel = 1;

	coven_debug_nodeloc = -1;
	coven_debug_prevnode = -1;

	num_trip_mines = 0;

	coven_display_autolevel = false;

	m_iLastWeaponFireUsercmd = 0;

	m_flNextModelChangeTime = 0.0f;
	m_flNextTeamChangeTime = 0.0f;

	m_iSpawnInterpCounter = 0;

    m_bEnterObserver = false;
	m_bReady = false;

	BaseClass::ChangeTeam( 0 );

	for(int i = 0; i < 2; i++)
	{
		for(int j = 0; j < COVEN_MAX_CLASSCOUNT; j++)
		{
			covenLevelsSpent[i][j] = 0;
			for (int k = 0; k < 4; k++)
			{
				covenLoadouts[i][j][k] = 0;
			}
		}
	}
	
//	UseClientSideAnimation();
}

CHL2MP_Player::~CHL2MP_Player( void )
{

}

void CHL2MP_Player::DoSlayerAbilityThink()
{
	if (m_afButtonPressed & IN_ABIL1)
	{
		float cd = GetCooldown(0);
		int lev = GetLoadout(0);
		if ((cd > 0.0f && gpGlobals->curtime < cd) || lev == 0)
		{
			EmitSound("HL2Player.UseDeny");
		}
		else
		{
			if (covenClassID == COVEN_CLASSID_REAVER)
			{
				//float mana = 10.0f;//10.0f + 5.0f*lev;
				//float cool = 20.0f;// - 10.0f*lev;
				if (covenStatusEffects & COVEN_FLAG_SPRINT)
				{
					covenStatusEffects &= covenStatusEffects & ~COVEN_FLAG_SPRINT;
					SetGlobalCooldown(0, 3.0f);
					SuitPower_ResetDrain();
					ComputeSpeed();
				}
				else if (SuitPower_GetCurrentPercentage() > 1.0f)
				{
					//SetGlobalCooldown(0, gpGlobals->curtime + cool);
					//SetStatusTime(COVEN_BUFF_SPRINT, gpGlobals->curtime + 6.0f + 2.0f*lev);
					covenStatusEffects |= COVEN_FLAG_SPRINT;
					SetStatusMagnitude(COVEN_BUFF_SPRINT, lev);
					ComputeSpeed();
					EmitSound("HL2Player.SprintStart");
					SuitPower_AddDrain(2.0f);
				}
				else
					EmitSound("HL2Player.UseDeny");
			}
			else if (covenClassID == COVEN_CLASSID_AVENGER)
			{
				//float mana = 12.0f;//+ 5.0f*lev;
				SetGlobalCooldown(0, gpGlobals->curtime + 20.0f);
				DoBattleYell(lev);
				EmitSound("BattleYell");
			}
			else if (covenClassID == COVEN_CLASSID_HELLION)
			{
				//float mana = 12.0f;//+2.0f*lev;
				float cool = 5.0f;//20.0f - 5.0f*lev;
				SetGlobalCooldown(0, gpGlobals->curtime + cool);
				ThrowHolywaterGrenade(lev);
			}
		}
	}
	if (m_afButtonPressed & IN_ABIL2)
	{
		float cd = GetCooldown(1);
		int lev = GetLoadout(1);
		if ((cd > 0.0f && gpGlobals->curtime < cd) || lev == 0)
		{
			EmitSound("HL2Player.UseDeny");
		}
		else
		{
			if (covenClassID == COVEN_CLASSID_REAVER)
			{
				//float mana = 10.0f;//5.0f + 5.0f*lev;
				float cool = 16.0f;
				SetGlobalCooldown(1, gpGlobals->curtime + cool);
				SetStatusTime(COVEN_BUFF_STATS, gpGlobals->curtime + 10.0f);
				covenStatusEffects |= COVEN_FLAG_STATS;
				DoSheerWill(lev);
				EmitSound("SheerWill");
			}
			else if (covenClassID == COVEN_CLASSID_AVENGER)
			{
				//float mana = 16.0f-2.0f*lev;
				float cool = 12.0f-2.0f*lev;//40.0f - 10.0f*lev;
				SetGlobalCooldown(1, gpGlobals->curtime + cool);
				if (medkits.Size() < lev)
				{
					GenerateBandage();
				}
				else
				{
					CItem *ent;
					ent = medkits[medkits.Size()-1];
					UTIL_Remove(ent);
					medkits.FindAndRemove(ent);
					GenerateBandage();
				}
			}
			else if (covenClassID == COVEN_CLASSID_HELLION)
			{
				//float mana = 8.0f;//6.0f + 2.0f*lev;
				float cool = 2.0f;
				if (num_trip_mines < (1+lev) && AttachTripmine())
				{
					SetGlobalCooldown(1, gpGlobals->curtime + cool);
				}
				else
					EmitSound("HL2Player.UseDeny");
			}
		}
	}
	if (m_afButtonPressed & IN_ABIL3)
	{
		float cd = GetCooldown(2);
		int lev = GetLoadout(2);
		if ((cd > 0.0f && gpGlobals->curtime < cd) || lev == 0)
		{
			EmitSound("HL2Player.UseDeny");
		}
		else
		{
		}
	}
	if (m_afButtonPressed & IN_ABIL4)
	{
		float cd = GetCooldown(3);
		int lev = GetLoadout(3);
		if ((cd > 0.0f && gpGlobals->curtime < cd) || lev == 0)
		{
			EmitSound("HL2Player.UseDeny");
		}
		else
		{
			if (covenClassID == COVEN_CLASSID_HELLION)
			{
				//float mana = 4.0f;// - lev;
				float mdrain = 6.0f-1.0f*lev;
				float cool = 3.0f;
				if (IsAlive() && FlashlightIsOn())
				{
					RemoveEffects( EF_DIMLIGHT );
					SuitPower_ResetDrain();
					SetGlobalCooldown(3, gpGlobals->curtime + cool);
	
					if( IsAlive() )
					{
						EmitSound( "HL2Player.FlashlightOff" );
					}
				}
				else if (SuitPower_GetCurrentPercentage() > mdrain)
				{
					if (IsAlive())
					{
						SuitPower_AddDrain(mdrain);
						AddEffects( EF_DIMLIGHT );
						EmitSound( "HL2Player.FlashlightOn" );
					}
				}
				else
					EmitSound("HL2Player.UseDeny");
			}
			else if (covenClassID == COVEN_CLASSID_REAVER)
			{
				//float mana = 12.0f;//8.0f + 4.0f*lev;
				float cool = 20.0f;
				SetGlobalCooldown(3, gpGlobals->curtime + cool);
				EmitSound( "IntShout" );
				DoIntimidatingShout(lev);
			}
			else if (covenClassID == COVEN_CLASSID_AVENGER)
			{
				//float mana = 12.0f;//8.0f + 4.0f*lev;
				if (SuitPower_GetCurrentPercentage() > 2.0f)
				{
					coven_soul_power = 2*lev;
					coven_timer_soul = gpGlobals->curtime + 0.5f;
					SuitPower_Drain(2.0f);
					SuitPower_AddDeviceBits( bits_COVEN_VENGE );
				}
				else
					EmitSound("HL2Player.UseDeny");
			}
		}
	}
}

void CHL2MP_Player::UnleashSoul()
{
	Vector vecSrc	 = Weapon_ShootPosition( );
	Vector vecAiming = ToBasePlayer( this )->GetAutoaimVector( AUTOAIM_2DEGREES );
	Vector impactPoint = vecSrc + ( vecAiming * MAX_TRACE_LENGTH );
	Vector vecVelocity = vecAiming * 1000.0f;
	Vector	vForward, vRight, vUp;

	EyeVectors( &vForward, &vRight, &vUp );

	Vector	muzzlePoint = Weapon_ShootPosition() + vForward * 12.0f;// + vRight * 6.0f;// + vUp * -3.0f;
	QAngle vecAngles;
	VectorAngles( vForward, vecAngles );

	CPropSoul *pMissile = CPropSoul::Create( muzzlePoint, vecAngles, edict() );
	/*trace_t	tr;
	Vector vecEye = EyePosition();
	UTIL_TraceLine( vecEye, vecEye + vForward * 128, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );
	if ( tr.fraction == 1.0 )
	{
		pMissile->SetGracePeriod( 0.3 );
	}*/

	pMissile->SetDamage( coven_soul_power );

	// View effects
	color32 white = {255, 255, 255, 64};
	UTIL_ScreenFade( this, white, 0.1, 0, FFADE_IN  );
}

void CHL2MP_Player::SlayerSoulThink()
{
	if (covenClassID == COVEN_CLASSID_AVENGER && GetLoadout(3) > 0 && IsAlive())
	{
		if (coven_timer_soul > 0.0f && gpGlobals->curtime > coven_timer_soul)
		{
			coven_timer_soul = gpGlobals->curtime + 0.5f;
			coven_soul_power += 2*GetLoadout(3);
			SuitPower_Drain(2.0f);
		}
		if (coven_soul_power > 0 && (SuitPower_GetCurrentPercentage() < 2.0f || m_afButtonReleased & IN_ABIL4))
		{
			coven_timer_soul = -1.0f;
			SuitPower_RemoveDeviceBits( bits_COVEN_VENGE );
			SetGlobalCooldown(3, gpGlobals->curtime + 3.0f);
			UnleashSoul();
			coven_soul_power = 0;
		}
	}
}

void CHL2MP_Player::DoIntimidatingShout(int lev)
{
	GiveBuffInRadius(COVEN_TEAMID_VAMPIRES, COVEN_BUFF_STUN, lev, 0.5f+0.5f*lev, 200.0f+100.0f*lev, 0);
}

void CHL2MP_Player::ThrowHolywaterGrenade(int lev)
{
	Vector	vecEye = EyePosition();
	Vector	vForward, vRight;

	EyeVectors( &vForward, &vRight, NULL );
	Vector vecSrc = vecEye + vForward * 18.0f + vRight * 8.0f + Vector( 0, 0, -8 );
	CheckThrowPosition( vecEye, vecSrc );
	
	Vector vecThrow;
	GetVelocity( &vecThrow, NULL );
	vecThrow = vecThrow * 0.3f;
	vecThrow += vForward * 500 + Vector( 0, 0, 150 );
	CGrenadeHH *pGrenade = (CGrenadeHH*)Create( "grenade_HH", vecSrc, vec3_angle, this );
	pGrenade->SetAbsVelocity( vecThrow );
	pGrenade->SetLocalAngularVelocity( RandomAngle( -200, 200 ) );
	pGrenade->SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE );
	pGrenade->SetThrower( this );
	pGrenade->SetDamage( 2.0f + lev*1.0f );
	pGrenade->SetDamageRadius( 150.0f );
}

void CHL2MP_Player::CheckThrowPosition(const Vector &vecEye, Vector &vecSrc)
{
	trace_t tr;

	float hw_gren_rad = 4.0f;

	UTIL_TraceHull( vecEye, vecSrc, -Vector(hw_gren_rad+2,hw_gren_rad+2,hw_gren_rad+2), Vector(hw_gren_rad+2,hw_gren_rad+2,hw_gren_rad+2), 
		PhysicsSolidMaskForEntity(), this, GetCollisionGroup(), &tr );
	
	if ( tr.DidHit() )
	{
		vecSrc = tr.endpos;
	}
}

void CHL2MP_Player::GenerateBandage()
{
	CBaseEntity *ent = CreateEntityByName( "item_healthkit" );
	Vector	vecEye = EyePosition();
	Vector	vForward, vRight;

	EyeVectors( &vForward, &vRight, NULL );
	Vector vecSrc = vecEye + vForward * 18.0f + vRight * 8.0f;

	
	ent->SetLocalOrigin(GetLocalOrigin()+vForward*18+Vector(0,0,40));
	vForward[2] += 0.3f;

	Vector vecThrow;
	GetVelocity( &vecThrow, NULL );
	vecThrow += vForward * 400;
	((CItem *)ent)->creator = this;
	ent->Spawn();
	ent->SetLocalAngles(QAngle(random->RandomInt(-180,180),random->RandomInt(-180,180),random->RandomInt(-180,180)));
	ent->ApplyAbsVelocityImpulse(vecThrow);//SetLocalVelocity(forward*200);
	ent->AddSpawnFlags(SF_NORESPAWN);
	medkits.AddToHead(((CItem *)ent));
}

void CHL2MP_Player::DoSheerWill(int lev)
{
	int tlev = lev;
	if (GetStatusMagnitude(COVEN_BUFF_STATS) > 0)
		tlev -= GetStatusMagnitude(COVEN_BUFF_STATS);

	int str = myStrength();
	int intel = myIntellect();
	int con = myConstitution();

	//reaver
	int stradd = 2;
	int inteladd = 1;
	int conadd = 3;

	if (covenClassID == COVEN_CLASSID_AVENGER)
	{
		stradd = 1;
		inteladd = 1;
		conadd = 2;
	}

	SetConstitution(con+conadd*tlev);
	SetStrength(str+stradd*tlev);
	SetIntellect(intel+inteladd*tlev);
	ResetVitals();
	SetStatusMagnitude(COVEN_BUFF_STATS, lev);
	SetHealth(GetHealth()+conadd*tlev*COVEN_HP_PER_CON);
}

void CHL2MP_Player::RevengeCheck()
{
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CHL2MP_Player *pPlayer = (CHL2MP_Player *)UTIL_PlayerByIndex( i );
		if (pPlayer && pPlayer->IsAlive() && pPlayer->GetTeamNumber() == COVEN_TEAMID_SLAYERS && pPlayer->covenClassID == COVEN_CLASSID_AVENGER && pPlayer->GetLoadout(2) > 0)
		{
			if ((pPlayer->GetLocalOrigin()-GetLocalOrigin()).Length() > 600.0f)
				continue;

			pPlayer->EmitSound("Revenge");
			pPlayer->DoSheerWill(pPlayer->GetLoadout(2));
			pPlayer->SetStatusTime(COVEN_BUFF_STATS, gpGlobals->curtime + 10.0f);
			pPlayer->covenStatusEffects |= COVEN_FLAG_STATS;
		}
	}
}

void CHL2MP_Player::GiveBuffInRadius(int team, int buff, int mag, float duration, float distance, int classid)
{
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CHL2MP_Player *pPlayer = (CHL2MP_Player *)UTIL_PlayerByIndex( i );
		if (pPlayer && pPlayer->GetTeamNumber() == team && pPlayer->IsAlive())
		{
			if (classid && pPlayer->covenClassID != classid)
				continue;

			if ((pPlayer->GetLocalOrigin()-GetLocalOrigin()).Length() > distance)
				continue;

			pPlayer->SetStatusTime(buff, gpGlobals->curtime + duration);
			if (mag > 0)
				pPlayer->SetStatusMagnitude(buff, mag);
			else
				pPlayer->SetStatusMagnitude(buff, pPlayer->GetLoadout(-mag));

			pPlayer->covenStatusEffects |= (1 << buff);
		}
	}
}

void CHL2MP_Player::DoGoreCharge()
{
	gorelock = true;
	AngleVectors(GetLocalAngles(), &lock_ts);
	VectorNormalize(lock_ts);
	lock_ts.z = 0.0f;
}

void CHL2MP_Player::DoGorePhase()
{
	if (GetRenderMode() != kRenderTransTexture)
				SetRenderMode( kRenderTransTexture );

	if (GetViewModel() && GetViewModel()->GetRenderMode() != kRenderTransTexture)
				GetViewModel()->SetRenderMode( kRenderTransTexture );

	gorephased = !gorephased;
	float cloak = 1.0f;
	if (gorephased)
	{
		//AddEffects(EF_NODRAW);
	}
	else
	{
		cloak = 0.0f;
		//RemoveEffects(EF_NODRAW);
	}

	this->m_floatCloakFactor = cloak;
	//SetRenderColorA(alpha);
	//if (GetViewModel())
	//	GetViewModel()->SetRenderColorA(alpha);

	float m_DmgRadius = 51.2f;
	trace_t		pTrace;
	Vector		vecSpot;// trace starts here!

	vecSpot = GetAbsOrigin() + Vector ( 0 , 0 , 8 );
	UTIL_TraceLine ( vecSpot, vecSpot + Vector ( 0, 0, -32 ), MASK_SHOT_HULL, this, COLLISION_GROUP_NONE, & pTrace);

#if !defined( CLIENT_DLL )

	Vector vecAbsOrigin = GetAbsOrigin();
	int contents = UTIL_PointContents ( vecAbsOrigin );

#if !defined( TF_DLL )
	// Since this code only runs on the server, make sure it shows the tempents it creates.
	// This solves a problem with remote detonating the pipebombs (client wasn't seeing the explosion effect)
	CDisablePredictionFiltering disabler;
#endif

	if ( pTrace.fraction != 1.0 )
	{
		Vector vecNormal = pTrace.plane.normal;
		surfacedata_t *pdata = physprops->GetSurfaceData( pTrace.surface.surfaceProps );	
		CPASFilter filter( vecAbsOrigin );

		te->Explosion( filter, -1.0, // don't apply cl_interp delay
			&vecAbsOrigin,
			!( contents & MASK_WATER ) ? g_sModelIndexFireball : g_sModelIndexWExplosion,
			m_DmgRadius,// * .03, 
			12,
			TE_EXPLFLAG_NONE | TE_EXPLFLAG_NOSOUND,
			m_DmgRadius,
			0.0,
			&vecNormal,
			(char) pdata->game.material );
	}
	else
	{
		CPASFilter filter( vecAbsOrigin );
		te->Explosion( filter, -1.0, // don't apply cl_interp delay
			&vecAbsOrigin, 
			!( contents & MASK_WATER ) ? g_sModelIndexFireball : g_sModelIndexWExplosion,
			m_DmgRadius,// * .03, 
			12,
			TE_EXPLFLAG_NONE | TE_EXPLFLAG_NOSOUND,
			m_DmgRadius,
			0.0 );
	}

	// Use the thrower's position as the reported position
	Vector vecReported = GetAbsOrigin();

	int bits = DMG_BLAST | DMG_NO;
	float damn = 40.0f;
	CBaseEntity *ignore;
	ignore = this;

	CTakeDamageInfo info( this, this, vec3_origin, GetAbsOrigin(), damn, bits, 0, &vecReported );

	RadiusDamage( info, GetAbsOrigin(), 300.0f, CLASS_NONE, ignore );

	UTIL_DecalTrace( &pTrace, "Scorch" );

	ComputeSpeed();

	EmitSound( "Weapon_Mortar.Impact" );
#endif
}

void CHL2MP_Player::DoBattleYell(int lev)
{
	GiveBuffInRadius(COVEN_TEAMID_SLAYERS, COVEN_BUFF_BYELL, lev, 10.0f, 500.0f, 0);
}

void CHL2MP_Player::DoBloodLust(int lev)
{
	GiveBuffInRadius(COVEN_TEAMID_VAMPIRES, COVEN_BUFF_BLUST, lev, 10.0f, 500.0f, 0);
}

void CHL2MP_Player::DoDreadScream(int lev)
{
	GiveBuffInRadius(COVEN_TEAMID_SLAYERS, COVEN_BUFF_SLOW, lev, 8.0f, 500.0f, 0);
}

void CHL2MP_Player::RecalcGoreDrain()
{
	SuitPower_ResetDrain();
	//float mdrain_phase = 2.5f - 0.5f*GetLoadout(0);
	float mdrain_charge = 12.0f - 2.0f*GetLoadout(1);
	//if (gorephased)
	//	SuitPower_AddDrain(mdrain_phase);
	if (gorelock)
		SuitPower_AddDrain(mdrain_charge);
}

void CHL2MP_Player::VampireUnDodge()
{
	if (covenStatusEffects & COVEN_FLAG_DODGE)
	{
		covenStatusEffects &= covenStatusEffects & ~COVEN_FLAG_DODGE;
		SuitPower_ResetDrain();
		SetGlobalCooldown(3, gpGlobals->curtime + 5.0f);
		SetRenderColorA(255);
		if( IsAlive() )
		{
			EmitSound( "Dodge" );
		}
	}
}

void CHL2MP_Player::DoVampireAbilityThink()
{
	if (m_afButtonPressed & IN_ABIL1)
	{
		float cd = GetCooldown(0);
		int lev = GetLoadout(0);
		if ((cd > 0.0f && gpGlobals->curtime < cd) || lev == 0)
		{
			EmitSound("HL2Player.UseDeny");
		}
		else
		{
			if (covenClassID == COVEN_CLASSID_FIEND)
			{
				//BB: JAM request. fixed lowered leap cooldown... mana costs dec
				//float mana = 15.0f-3.0f*lev;
				//float cool = 10.0f - 2.0f*lev;
				SetGlobalCooldown(0, gpGlobals->curtime + 3.75f - 0.25f*lev);
				DoLeap();
			}
			else if (covenClassID == COVEN_CLASSID_GORE)
			{
				
				SetGlobalCooldown(0, gpGlobals->curtime + 15.0f);
				DoDreadScream(lev);
				EmitSound("HL2Player.Sweet");
			}
			else if (covenClassID == COVEN_CLASSID_DEGEN)
			{
				//float mana = 10.0f;//5.0f + 5.0f*lev;
				SetGlobalCooldown(0, gpGlobals->curtime + 18.0f);
				DoBloodLust(lev);
				EmitSound("HL2Player.Sweet");
			}
		}
	}
	if (m_afButtonPressed & IN_ABIL2)
	{
		float cd = GetCooldown(1);
		int lev = GetLoadout(1);
		if ((cd > 0.0f && gpGlobals->curtime < cd) || lev == 0)
		{
			EmitSound("HL2Player.UseDeny");
		}
		else
		{
			if (covenClassID == COVEN_CLASSID_GORE)
			{
				//float mana = 4.0f;//7.0f - 1.0f*lev;
				if (SuitPower_GetCurrentPercentage() > 2.0f)
				{
					//SuitPower_Drain(mana);
					DoGoreCharge();
					RecalcGoreDrain();
					EmitSound("HL2Player.Sweet");
				}
				else
					EmitSound("HL2Player.UseDeny");
			}
			else if (covenClassID == COVEN_CLASSID_FIEND)
			{
				//float mana = 15.0f;//10.0f+5.0f*lev;
				float cool = 20.0f;
				SetGlobalCooldown(1, gpGlobals->curtime + cool);
				SetStatusTime(COVEN_BUFF_BERSERK, gpGlobals->curtime + 10.0f);
				SetStatusMagnitude(COVEN_BUFF_BERSERK, lev);
				covenStatusEffects |= COVEN_FLAG_BERSERK;
				DoBerserk(lev);
				EmitSound("HL2Player.Sweet");
			}
		}
	}
	if (m_afButtonPressed & IN_ABIL4)
	{
		float cd = GetCooldown(3);
		int lev = GetLoadout(3);
		if ((cd > 0.0f && gpGlobals->curtime < cd) || lev == 0)
		{
			EmitSound("HL2Player.UseDeny");
		}
		else
		{
			if (covenClassID == COVEN_CLASSID_DEGEN)
			{
				//float mana = 10.0f;//6.0f+2.0f*lev;
				//float cool = 10.0f;
				if (SuitPower_GetCurrentPercentage() > 2.0f)
				{
					coven_timer_detonate = gpGlobals->curtime + 0.5f;
					coven_detonate_power += 2*lev;
					SuitPower_AddDeviceBits( bits_COVEN_DETONATE );
					SuitPower_Drain(2.0f);
				}
				else
					EmitSound("HL2Player.UseDeny");
			}
			else if (covenClassID == COVEN_CLASSID_FIEND)
			{
				//float mana = 4.0f;
				if (covenStatusEffects & COVEN_FLAG_DODGE)
				{
					VampireUnDodge();
				}
				else if (SuitPower_GetCurrentPercentage() > 1.0f)
				{
					covenStatusEffects |= COVEN_FLAG_DODGE;
					EmitSound("HL2Player.SprintStart");
					SetRenderColorA(100);
					SuitPower_AddDrain(4.0f);
				}
				else
					EmitSound("HL2Player.UseDeny");
			}
			else if (covenClassID == COVEN_CLASSID_GORE)
			{
				if (gorephased)
				{
					covenStatusEffects &= covenStatusEffects & ~COVEN_FLAG_PHASE;
					SetStatusTime(COVEN_BUFF_PHASE, 0.0f);
					DoGorePhase();
				}
				else
				{
					float cool = 17.0f-lev;
					float duration = 6.0f+2.0f*lev;
					//float mana = 4.0f;//7.0f - 1.0f*lev;
					SetGlobalCooldown(3, gpGlobals->curtime + cool);
					covenStatusEffects |= COVEN_FLAG_PHASE;
					SetStatusMagnitude(COVEN_BUFF_PHASE, lev);
					SetStatusTime(COVEN_BUFF_PHASE, gpGlobals->curtime + duration);
					DoGorePhase();
				}
			}
		}
	}
}

void CHL2MP_Player::BloodExplode(int lev)
{
	float m_DmgRadius = 51.2f;
	trace_t		pTrace;
	Vector		vecSpot;// trace starts here!

	vecSpot = GetAbsOrigin() + Vector ( 0 , 0 , 8 );
	UTIL_TraceLine ( vecSpot, vecSpot + Vector ( 0, 0, -32 ), MASK_SHOT_HULL, this, COLLISION_GROUP_NONE, & pTrace);

#if !defined( CLIENT_DLL )

	Vector vecAbsOrigin = GetAbsOrigin();
	int contents = UTIL_PointContents ( vecAbsOrigin );

#if !defined( TF_DLL )
	// Since this code only runs on the server, make sure it shows the tempents it creates.
	// This solves a problem with remote detonating the pipebombs (client wasn't seeing the explosion effect)
	CDisablePredictionFiltering disabler;
#endif

	if ( pTrace.fraction != 1.0 )
	{
		Vector vecNormal = pTrace.plane.normal;
		surfacedata_t *pdata = physprops->GetSurfaceData( pTrace.surface.surfaceProps );	
		CPASFilter filter( vecAbsOrigin );

		te->Explosion( filter, -1.0, // don't apply cl_interp delay
			&vecAbsOrigin,
			!( contents & MASK_WATER ) ? g_sModelIndexFireball : g_sModelIndexWExplosion,
			m_DmgRadius,// * .03, 
			12,
			TE_EXPLFLAG_NONE | TE_EXPLFLAG_NOSOUND,
			m_DmgRadius,
			0.0,
			&vecNormal,
			(char) pdata->game.material );
	}
	else
	{
		CPASFilter filter( vecAbsOrigin );
		te->Explosion( filter, -1.0, // don't apply cl_interp delay
			&vecAbsOrigin, 
			!( contents & MASK_WATER ) ? g_sModelIndexFireball : g_sModelIndexWExplosion,
			m_DmgRadius,// * .03, 
			12,
			TE_EXPLFLAG_NONE | TE_EXPLFLAG_NOSOUND,
			m_DmgRadius,
			0.0 );
	}

	// Use the thrower's position as the reported position
	Vector vecReported = GetAbsOrigin();

	int bits = DMG_PARALYZE;
	float damn = coven_detonate_power;
	//float damn = 0.25f*lev*GetMaxHealth();
	CBaseEntity *ignore;
	ignore = this;

	CTakeDamageInfo info( this, this, vec3_origin, GetAbsOrigin(), damn, bits, 0, &vecReported );

	RadiusDamage( info, GetAbsOrigin(), 400.0f, CLASS_NONE, ignore );

	UTIL_DecalTrace( &pTrace, "Scorch" );

	//bits = DMG_CLUB;
	damn = 0.5f*coven_detonate_power;
	//damn = 0.15f*lev*GetMaxHealth();
	info.SetDamage(damn);
	info.SetDamageType(bits);
	TakeDamage(info);

	EmitSound( "NPC_SScanner.DiveBombFlyby" );
#endif
}

void CHL2MP_Player::DoBerserk(int lev)
{
	int oldmax = GetMaxHealth();
	int newmax = oldmax*(1.2f + 0.2f*lev);
	SetMaxHealth(newmax);
	SetHealth(GetHealth()+(newmax-oldmax));
}

void CHL2MP_Player::UpdateOnRemove( void )
{
	//BB: i think this is causing more trouble than it's worth... we are pretty good about cleaning up after ourselves.
	/*if ( m_hRagdoll )
	{
		UTIL_Remove( m_hRagdoll );
		m_hRagdoll = NULL;
	}*/

	BaseClass::UpdateOnRemove();
}

bool CHL2MP_Player::LevelUp( int lvls )
{
	/*if (GetTeamNumber() == COVEN_TEAMID_SLAYERS)
	{
		switch (covenClassID)
		{
		case COVEN_CLASSID_REAVER:
			//GiveStrength(6);
			//GiveAgility(2);
			//GiveIntellect(1);
			break;
		case COVEN_CLASSID_AVENGER:
			//GiveStrength(4);
			//GiveAgility(4);
			//GiveIntellect(2);
			break;
		case COVEN_CLASSID_HELLION:
			//GiveStrength(3);
			//GiveAgility(6);
			//GiveIntellect(2);
			break;
		default:break;
		}
	}
	else if (GetTeamNumber() == COVEN_TEAMID_VAMPIRES)
	{
		switch (covenClassID)
		{
		case COVEN_CLASSID_FIEND:
			//GiveStrength(3);
			//GiveAgility(5);
			//GiveIntellect(2);
			break;
		case COVEN_CLASSID_GORE:
			//GiveStrength(6);
			//GiveAgility(1);
			//GiveIntellect(1);
			break;
		default:break;
		}
	}*/
	if (IsAlive())
	{
	#if !defined( TF_DLL )
		// Since this code only runs on the server, make sure it shows the tempents it creates.
		// This solves a problem with remote detonating the pipebombs (client wasn't seeing the explosion effect)
		CDisablePredictionFiltering disabler;
#endif

			UTIL_ScreenShake( GetAbsOrigin(), 20.0f, 150.0, 1.0, 1250.0f, SHAKE_START );

			CEffectData data;

			data.m_vOrigin = GetAbsOrigin();

			DispatchEffect( "cball_explode", data );

			CBroadcastRecipientFilter filter2;

			te->BeamRingPoint( filter2, 0, GetAbsOrigin(),	//origin
				50,	//start radius
				1024,		//end radius
				s_nExplosionTexture, //texture
				0,			//halo index
				0,			//start frame
				2,			//framerate
				0.2f,		//life
				64,			//width
				0,			//spread
				0,			//amplitude
				255,	//r
				255,	//g
				225,	//b
				32,		//a
				0,		//speed
				FBEAM_FADEOUT
				);

			//Shockring
			te->BeamRingPoint( filter2, 0, GetAbsOrigin(),	//origin
				50,	//start radius
				1024,		//end radius
				s_nExplosionTexture, //texture
				0,			//halo index
				0,			//start frame
				2,			//framerate
				0.5f,		//life
				64,			//width
				0,			//spread
				0,			//amplitude
				255,	//r
				255,	//g
				225,	//b
				64,		//a
				0,		//speed
				FBEAM_FADEOUT
				);

		EmitSound( "NPC_CombineBall.Explosion" );
	}
	//ResetVitals();
	//BB: too many levels... do not reset health anymore?
	if (GetHealth() <= GetMaxHealth())
	{
		int hp = myConstitution()*COVEN_HP_PER_CON;
		if (GetHealth() < hp)
			SetHealth(hp);
	}

	coven_display_autolevel = true;

	return BaseClass::LevelUp(lvls);
}

void CHL2MP_Player::SetGlobalCooldown(int n, float time)
{
	int t = GetTeamNumber();
	if (t > 1)
	{
		t -= 2;
		covenCooldowns[t][covenClassID-1][n] = time;
		RefreshCooldowns();
	}
}

void CHL2MP_Player::SpendPoint(int on)
{
	int t = GetTeamNumber();
	if (t > 1)
	{
		t -= 2;
		covenLevelsSpent[t][covenClassID-1]++;
		covenLoadouts[t][covenClassID-1][on]++;
		RefreshLoadout();
	}
}

void CHL2MP_Player::RefreshLoadout()
{
	SetCurrentLoadout(0, GetLoadout(0));
	SetCurrentLoadout(1, GetLoadout(1));
	SetCurrentLoadout(2, GetLoadout(2));
	SetCurrentLoadout(3, GetLoadout(3));
	SetPointsSpent(GetLevelsSpent());
}

void CHL2MP_Player::RefreshCooldowns()
{
	SetCooldown(0, GetCooldown(0));
	SetCooldown(1, GetCooldown(1));
	SetCooldown(2, GetCooldown(2));
	SetCooldown(3, GetCooldown(3));
}


float CHL2MP_Player::GetCooldown(int n)
{
	int t = GetTeamNumber();
	if (t > 1 && n >= 0 && n <= 3)
	{
		t -= 2;
		return covenCooldowns[t][covenClassID-1][n];
	}

	return -1;
}

int CHL2MP_Player::GetLoadout(int n)
{
	int t = GetTeamNumber();
	if (t > 1 && n >= 0 && n <= 3)
	{
		t -= 2;
		return covenLoadouts[t][covenClassID-1][n];
	}

	return -1;
}

int CHL2MP_Player::GetLevelsSpent()
{
	int t = GetTeamNumber();
	if (t > 1)
	{
		t -= 2;
		return covenLevelsSpent[t][covenClassID-1];
	}

	return -1;
}

void CHL2MP_Player::ResetStats()
{
	/*int baseHP = 100;*/
	if (GetTeamNumber() == COVEN_TEAMID_SLAYERS)
	{
		switch(covenClassID)
		{
		case COVEN_CLASSID_AVENGER:
			SetConstitution(25);
			SetStrength(COVEN_BASESTR_AVENGER);
			SetIntellect(COVEN_BASEINT_AVENGER);
			break;
		case COVEN_CLASSID_HELLION:
			SetConstitution(25);
			SetStrength(COVEN_BASESTR_HELLION);
			SetIntellect(COVEN_BASEINT_HELLION);
			break;
		case COVEN_CLASSID_REAVER:
			SetConstitution(30);
			SetStrength(COVEN_BASESTR_REAVER);
			SetIntellect(COVEN_BASEINT_REAVER);
			break;
		default:break;
		}
	}
	else if (GetTeamNumber() == COVEN_TEAMID_VAMPIRES)
	{
		switch(covenClassID)
		{
		case COVEN_CLASSID_FIEND:
			SetConstitution(18);
			SetStrength(COVEN_BASESTR_FIEND);
			SetIntellect(COVEN_BASEINT_FIEND);
			break;
		case COVEN_CLASSID_GORE:
			SetConstitution(30);
			SetStrength(COVEN_BASESTR_GORE);
			SetIntellect(COVEN_BASEINT_GORE);
			break;
		case COVEN_CLASSID_DEGEN:
			SetConstitution(28);
			SetStrength(COVEN_BASESTR_DEGEN);
			SetIntellect(COVEN_BASEINT_DEGEN);
			break;
		default:break;
		}
	}
}

bool CHL2MP_Player::AttachTripmine()
{
	Vector vecSrc, vecAiming;

	// Take the eye position and direction
	vecSrc = EyePosition();
	
	QAngle angles = GetLocalAngles();

	AngleVectors( angles, &vecAiming );

	trace_t tr;

	UTIL_TraceLine( vecSrc, vecSrc + (vecAiming * 128), MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );
	
	if (tr.fraction < 1.0)
	{
		CBaseEntity *pEntity = tr.m_pEnt;
		if (pEntity && !(pEntity->GetFlags() & FL_CONVEYOR))
		{

			QAngle angles;
			VectorAngles(tr.plane.normal, angles);

			angles.x += 90;

			CBaseEntity *pEnt = CBaseEntity::Create( "npc_tripmine", tr.endpos + tr.plane.normal * 3, angles, NULL );

			CTripmineGrenade *pMine = (CTripmineGrenade *)pEnt;
			pMine->m_hOwner = this;
			pMine->m_nTeam = GetTeamNumber();

			num_trip_mines++;
		}
		return true;
	}
	return false;
}

void CHL2MP_Player::ResetVitals( void )
{
	SetMaxHealth(myConstitution()*COVEN_HP_PER_CON);
}

void CHL2MP_Player::Precache( void )
{
	BaseClass::Precache();

	s_nExplosionTexture = PrecacheModel( "sprites/lgtning.vmt" );

	PrecacheModel ( "sprites/glow01.vmt" );

	UTIL_PrecacheOther("grenade_hh");
	UTIL_PrecacheOther( "npc_tripmine" );
	UTIL_PrecacheOther( "item_ammo_crate" );
	UTIL_PrecacheOther( "item_xp_slayers" );
	UTIL_PrecacheOther( "item_xp_vampires" );
	UTIL_PrecacheOther( "item_cts" );

	//Precache Citizen models
	int nHeads = ARRAYSIZE( g_ppszRandomCitizenModels );
	int i;	

	for ( i = 0; i < nHeads; ++i )
	   	 PrecacheModel( g_ppszRandomCitizenModels[i] );

	//Precache Combine Models
	nHeads = ARRAYSIZE( g_ppszRandomCombineModels );

	for ( i = 0; i < nHeads; ++i )
	   	 PrecacheModel( g_ppszRandomCombineModels[i] );

	PrecacheModel("models/breen.mdl");
	PrecacheModel("models/mossman.mdl");
	PrecacheModel("models/gman.mdl");
	PrecacheModel("models/Sargexx_Soldier.mdl");
	PrecacheModel("models/Brdeg.mdl");
	PrecacheModel("models/Humans/Group01/male_01.mdl");
	PrecacheModel("models/Humans/Group01/male_02.mdl");
	PrecacheModel("models/Humans/Group01/male_03.mdl");
	PrecacheModel("models/Humans/Group01/male_04.mdl");
	PrecacheModel("models/Humans/Group01/male_05.mdl");
	PrecacheModel("models/Humans/Group01/male_06.mdl");
	PrecacheModel("models/Humans/Group01/male_07.mdl");
	PrecacheModel("models/Humans/Group01/male_08.mdl");
	PrecacheModel("models/Humans/Group01/male_09.mdl");
	PrecacheModel("models/Humans/Group01/female_01.mdl");
	PrecacheModel("models/Humans/Group01/female_02.mdl");
	PrecacheModel("models/Humans/Group01/female_03.mdl");
	PrecacheModel("models/Humans/Group01/female_04.mdl");
	PrecacheModel("models/Humans/Group01/female_06.mdl");
	PrecacheModel("models/Humans/Group01/female_07.mdl");

	PrecacheFootStepSounds();

	PrecacheScriptSound( "NPC_CombineBall.Explosion" );

	PrecacheScriptSound( "HL2Player.Sweet" );
	PrecacheScriptSound( "Weapon_Mortar.Impact" );
	PrecacheScriptSound( "Leap.Impact" );
	PrecacheScriptSound( "Stake.Taunt" );
	PrecacheScriptSound( "Leap.Detect" );
	PrecacheScriptSound( "Vampire.Regen" );
	PrecacheScriptSound( "NPC_MetroPolice.Die" );
	PrecacheScriptSound( "NPC_CombineS.Die" );
	PrecacheScriptSound( "NPC_Citizen.die" );
	PrecacheScriptSound( "Resurrect" );
	PrecacheScriptSound( "Resurrect.Finish" );
	PrecacheScriptSound( "Capture" );
	PrecacheScriptSound( "Cappoint.Hum" );
	PrecacheScriptSound( "SheerWill" );
	PrecacheScriptSound( "IntShout" );
	PrecacheScriptSound( "Revenge" );
	PrecacheScriptSound( "BattleYell" );
	PrecacheScriptSound( "Dodge" );

	PrecacheScriptSound( "Leap" );

	PrecacheScriptSound( "Weapon_StunStick.Activate" );
	PrecacheScriptSound( "NPC_SScanner.DiveBombFlyby" );

	/*PrecacheModel( "models/items/ammocrate_pistol.mdl" );

	PrecacheScriptSound( "AmmoCrate.Open" );
	PrecacheScriptSound( "AmmoCrate.Close" );*/
}

void CHL2MP_Player::GiveAllItems( void )
{
	EquipSuit();

	CBasePlayer::GiveAmmo( 255,	"Pistol");
	CBasePlayer::GiveAmmo( 255,	"AR2" );
	CBasePlayer::GiveAmmo( 5,	"AR2AltFire" );
	CBasePlayer::GiveAmmo( 255,	"SMG1");
	CBasePlayer::GiveAmmo( 1,	"smg1_grenade");
	CBasePlayer::GiveAmmo( 255,	"Buckshot");
	CBasePlayer::GiveAmmo( 32,	"357" );
	CBasePlayer::GiveAmmo( 3,	"rpg_round");

	CBasePlayer::GiveAmmo( 1,	"grenade" );
	CBasePlayer::GiveAmmo( 2,	"slam" );

	GiveNamedItem( "weapon_crowbar" );
	GiveNamedItem( "weapon_stunstick" );
	GiveNamedItem( "weapon_pistol" );
	GiveNamedItem( "weapon_357" );

	GiveNamedItem( "weapon_smg1" );
	GiveNamedItem( "weapon_ar2" );
	
	GiveNamedItem( "weapon_shotgun" );
	GiveNamedItem( "weapon_frag" );
	
	GiveNamedItem( "weapon_crossbow" );
	
	GiveNamedItem( "weapon_rpg" );

	GiveNamedItem( "weapon_slam" );

	GiveNamedItem( "weapon_physcannon" );
	
}

void CHL2MP_Player::GiveDefaultItems( void )
{
	EquipSuit();

	if (GetTeamNumber() == COVEN_TEAMID_SLAYERS)
	{
		switch (covenClassID)
		{
		case COVEN_CLASSID_AVENGER:
			//CBasePlayer::GiveAmmo( 90,	"SMG1");
			//GiveNamedItem( "weapon_smg1" );
			CBasePlayer::GiveAmmo( 16,	"Buckshot");
			GiveNamedItem( "weapon_shotgun" );
			break;
		case COVEN_CLASSID_REAVER:
			CBasePlayer::GiveAmmo( 16,	"Buckshot");
			GiveNamedItem( "weapon_doubleshotgun" );
			break;
		case COVEN_CLASSID_HELLION:
			//CBasePlayer::GiveAmmo( 60,	"Pistol");
			//GiveNamedItem( "weapon_pistol" );
			CBasePlayer::GiveAmmo( 18,	"357");
			GiveNamedItem( "weapon_357" );
		default:
			break;
		}
		GiveNamedItem( "weapon_stake" );
	}
	else if (GetTeamNumber() == COVEN_TEAMID_VAMPIRES)
	{
		GiveNamedItem( "weapon_crowbar" );
	}

	const char *szDefaultWeaponName = engine->GetClientConVarValue( engine->IndexOfEdict( edict() ), "cl_defaultweapon" );

	CBaseCombatWeapon *pDefaultWeapon = Weapon_OwnsThisType( szDefaultWeaponName );

	if ( pDefaultWeapon )
	{
		Weapon_Switch( pDefaultWeapon );
	}
	else
	{
		Weapon_Switch( Weapon_OwnsThisType( "weapon_physcannon" ) );
	}
}

void CHL2MP_Player::PickDefaultSpawnTeam( void )
{
	if ( GetTeamNumber() == 0 )
	{
		if ( HL2MPRules()->IsTeamplay() == false )
		{
			if ( GetModelPtr() == NULL )
			{
				const char *szModelName = NULL;
				szModelName = engine->GetClientConVarValue( engine->IndexOfEdict( edict() ), "cl_playermodel" );

				if ( ValidatePlayerModel( szModelName ) == false )
				{
					char szReturnString[512];

					Q_snprintf( szReturnString, sizeof (szReturnString ), "cl_playermodel models/combine_soldier.mdl\n" );
					engine->ClientCommand ( edict(), szReturnString );
				}

				ChangeTeam( TEAM_UNASSIGNED );
			}
		}
		else
		{
			CTeam *pCombine = g_Teams[TEAM_COMBINE];
			CTeam *pRebels = g_Teams[TEAM_REBELS];

			if ( pCombine == NULL || pRebels == NULL )
			{
				ChangeTeam( random->RandomInt( TEAM_COMBINE, TEAM_REBELS ) );
			}
			else
			{
				if ( pCombine->GetNumPlayers() > pRebels->GetNumPlayers() )
				{
					ChangeTeam( TEAM_REBELS );
				}
				else if ( pCombine->GetNumPlayers() < pRebels->GetNumPlayers() )
				{
					ChangeTeam( TEAM_COMBINE );
				}
				else
				{
					ChangeTeam( random->RandomInt( TEAM_COMBINE, TEAM_REBELS ) );
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Sets HL2 specific defaults.
//-----------------------------------------------------------------------------
void CHL2MP_Player::Spawn(void)
{
	if (covenRespawnTimer > 0.0f && gpGlobals->curtime < covenRespawnTimer)
		return;

	covenRespawnTimer = -1.0f;

	covenStatusEffects = 0;

	if (KO && myServerRagdoll)
		UTIL_Remove(myServerRagdoll);
	m_hRagdoll = NULL;
	myServerRagdoll = NULL;

	gorephased = false;
	SuitPower_ResetDrain();
	KO = false;
	timeofdeath = -1.0f;
	mykiller = NULL;
	rezsound = false;
	solidcooldown = -1.0f;

	gorelock = false;
	/*if (myServerRagdoll)
	{
		UTIL_RemoveImmediate(myServerRagdoll);
		myServerRagdoll = NULL;
	}*/

	SetPlayerTeamModel();

	color32 nothing = {0,0,0,255};
	UTIL_ScreenFade( this, nothing, 0, 0, FFADE_IN | FFADE_PURGE );

	m_flNextModelChangeTime = 0.0f;
	m_flNextTeamChangeTime = 0.0f;

	//BB: We don't do things this way
	//PickDefaultSpawnTeam();

	if (covenClassID <= 0)
	{
		StartObserverMode(OBS_MODE_ROAMING);
		return;
	}

	//BB: first spawn... give us a level
	if (covenLevelCounter == 0)
		LevelUp(1);

#ifndef COVEN_DEVELOPER_MODE
	if (GetTeamNumber() == COVEN_TEAMID_VAMPIRES && covenClassID > COVEN_CLASSCOUNT_VAMPIRES)
		return;

	if (GetTeamNumber() == COVEN_TEAMID_SLAYERS && covenClassID > COVEN_CLASSCOUNT_SLAYERS)
		return;
#endif

	ResetStats();

	BaseClass::Spawn();
	
	if ( !IsObserver() )
	{
		pl.deadflag = false;
		RemoveSolidFlags( FSOLID_NOT_SOLID );

		RemoveEffects( EF_NODRAW );
		
		GiveDefaultItems();
		//BB: do not draw crowbar
		if (GetActiveWeapon() && GetTeamNumber() == COVEN_TEAMID_VAMPIRES)
		{
			GetActiveWeapon()->AddEffects(EF_NODRAW);
			GetActiveWeapon()->AddSolidFlags(FSOLID_NOT_SOLID);
		}
	}

	SetNumAnimOverlays( 3 );
	ResetAnimation();

	m_nRenderFX = kRenderNormal;

	m_Local.m_iHideHUD = 0;
	
	AddFlag(FL_ONGROUND); // set the player on the ground at the start of the round.

	//BB: need to move this to prevent unfreezing accidentally
	if ( HL2MPRules()->IsIntermission() || (HL2MPRules()->covenGameState == COVEN_GAME_STATE_FREEZE && sv_coven_freezetime.GetInt() > 0))
	{
		AddFlag( FL_FROZEN );
		AddEFlags( EFL_BOT_FROZEN );
	}
	else
	{
		RemoveFlag( FL_FROZEN );
		RemoveEFlags( EFL_BOT_FROZEN );
	}

	m_impactEnergyScale = HL2MPPLAYER_PHYSDAMAGE_SCALE;

	m_iSpawnInterpCounter = (m_iSpawnInterpCounter + 1) % 8;

	m_Local.m_bDucked = false;

	SetPlayerUnderwater(false);

	m_bReady = false;

	ResetVitals();
	SetHealth(myConstitution()*COVEN_HP_PER_CON);
	lastCheckedCapPoint = 0;
	lastCapPointTime = 0.0f;

	coven_detonate_power = 0;
	coven_soul_power = 0;

	coven_timer_detonate = -1.0f;
	coven_timer_feed = -1.0f;
	coven_timer_damage = -1.0f;
	coven_timer_leapdetectcooldown = -1.0f;
	coven_timer_regen = 0.0f;
	coven_timer_vstealth = 0.0f;
	//coven_timer_gcheck = 0.0f;
	coven_timer_soul = -1.0f;
	coven_timer_light = 0.0f;
	coven_timer_holywater = -1.0f;
}

void CHL2MP_Player::PickupObject( CBaseEntity *pObject, bool bLimitMassAndSize )
{
	
}

bool CHL2MP_Player::ValidatePlayerModel( const char *pModel )
{
	if (GetTeamNumber() == COVEN_TEAMID_VAMPIRES)
	{
		switch (covenClassID)
			{
		case COVEN_CLASSID_FIEND:
			if ( !Q_stricmp( "models/breen.mdl", pModel ) )
			{
				return true;
			}
			break;
		case COVEN_CLASSID_GORE:
			if ( !Q_stricmp( "models/gman.mdl", pModel ) )
			{
				return true;
			}
			break;
		case COVEN_CLASSID_BLOOD:
			if ( !Q_stricmp( "models/Humans/Group01/male_04.mdl", pModel ) )
			{
				return true;
			}
			break;
		case COVEN_CLASSID_DEGEN:
			if ( !Q_stricmp( "models/brdeg.mdl", pModel ) )
			{
				return true;
			}
			break;
		default:break;
			}
	}
	
	if (GetTeamNumber() == COVEN_TEAMID_SLAYERS)
	{
		switch (covenClassID)
		{
		case COVEN_CLASSID_AVENGER:
			if ( !Q_stricmp( "models/combine_soldier_prisonguard.mdl", pModel ) )
			{
				return true;
			}
			break;
		case COVEN_CLASSID_REAVER:
			if ( !Q_stricmp( "models/combine_soldier.mdl", pModel ) )
			{
				return true;
			}
			break;
		case COVEN_CLASSID_DEADEYE:
			if ( !Q_stricmp( "models/combine_super_soldier.mdl", pModel ) )
			{
				return true;
			}
			break;
		case COVEN_CLASSID_HELLION:
			if ( !Q_stricmp( "models/police.mdl", pModel ) )
			{
				return true;
			}
			break;
		default:break;
		}
		/*
			if ( !Q_stricmp( "models/sargexx_soldier.mdl", pModel ) )
			{
				return true;
			}
		}*/
	}
	return false;
}

void CHL2MP_Player::SetPlayerTeamModel( void )
{
	const char *szModelName = NULL;
	szModelName = engine->GetClientConVarValue( engine->IndexOfEdict( edict() ), "cl_playermodel" );

	int modelIndex = modelinfo->GetModelIndex( szModelName );

	if ( GetTeamNumber() == COVEN_TEAMID_SLAYERS )
	{
		switch (covenClassID)
			{
		case COVEN_CLASSID_REAVER:
			szModelName = "models/combine_soldier.mdl";
			break;
		case COVEN_CLASSID_DEADEYE:
			szModelName = "models/combine_super_soldier.mdl";
			break;
		case COVEN_CLASSID_AVENGER:
			szModelName = "models/combine_soldier_prisonguard.mdl";
			break;
		default:
			szModelName = "models/police.mdl";
			break;
			}

		m_iModelType = TEAM_COMBINE;
	}
	else if ( GetTeamNumber() == COVEN_TEAMID_VAMPIRES )
	{
		switch (covenClassID)
			{
		case COVEN_CLASSID_FIEND:
			szModelName = "models/breen.mdl";
			break;
		case COVEN_CLASSID_GORE:
			szModelName = "models/gman.mdl";
			break;
		case COVEN_CLASSID_BLOOD:
			szModelName = "models/Humans/Group01/male_04.mdl";
			break;
		case COVEN_CLASSID_DEGEN:
			szModelName = "models/brdeg.mdl";
			break;
		default:break;
			}
		m_iModelType = TEAM_REBELS;
	}

	if ( modelIndex == -1 || ValidatePlayerModel( szModelName ) == false )
	{
		szModelName = "models/Combine_Soldier.mdl";
		m_iModelType = TEAM_COMBINE;

		char szReturnString[512];

		Q_snprintf( szReturnString, sizeof (szReturnString ), "cl_playermodel %s\n", szModelName );
		engine->ClientCommand ( edict(), szReturnString );
	}
	
	SetModel( szModelName );
	SetupPlayerSoundsByModel( szModelName );
	//BB: temp get rid of GMan briefcase hacky hack
	if ( !Q_stricmp( "models/gman.mdl", szModelName ) )
		SetBodygroup(1, 1);
	else
		SetBodygroup(1, 0);

	//m_flNextModelChangeTime = gpGlobals->curtime + MODEL_CHANGE_INTERVAL;
}

void CHL2MP_Player::SetPlayerModel( void )
{
	const char *szModelName = NULL;
	const char *pszCurrentModelName = modelinfo->GetModelName( GetModel());

	szModelName = engine->GetClientConVarValue( engine->IndexOfEdict( edict() ), "cl_playermodel" );

	if ( ValidatePlayerModel( szModelName ) == false )
	{
		char szReturnString[512];

		if ( ValidatePlayerModel( pszCurrentModelName ) == false )
		{
			pszCurrentModelName = "models/Combine_Soldier.mdl";
		}

		Q_snprintf( szReturnString, sizeof (szReturnString ), "cl_playermodel %s\n", pszCurrentModelName );
		engine->ClientCommand ( edict(), szReturnString );

		szModelName = pszCurrentModelName;
	}

	if ( GetTeamNumber() == TEAM_COMBINE )
	{
		int nHeads = ARRAYSIZE( g_ppszRandomCombineModels );
		
		g_iLastCombineModel = ( g_iLastCombineModel + 1 ) % nHeads;
		szModelName = g_ppszRandomCombineModels[g_iLastCombineModel];

		m_iModelType = TEAM_COMBINE;
	}
	else if ( GetTeamNumber() == TEAM_REBELS )
	{
		int nHeads = ARRAYSIZE( g_ppszRandomCitizenModels );

		g_iLastCitizenModel = ( g_iLastCitizenModel + 1 ) % nHeads;
		szModelName = g_ppszRandomCitizenModels[g_iLastCitizenModel];

		m_iModelType = TEAM_REBELS;
	}
	else
	{
		if ( Q_strlen( szModelName ) == 0 ) 
		{
			szModelName = g_ppszRandomCitizenModels[0];
		}

		if ( Q_stristr( szModelName, "models/human") )
		{
			m_iModelType = TEAM_REBELS;
		}
		else
		{
			m_iModelType = TEAM_COMBINE;
		}
	}

	int modelIndex = modelinfo->GetModelIndex( szModelName );

	if ( modelIndex == -1 )
	{
		szModelName = "models/Combine_Soldier.mdl";
		m_iModelType = TEAM_COMBINE;

		char szReturnString[512];

		Q_snprintf( szReturnString, sizeof (szReturnString ), "cl_playermodel %s\n", szModelName );
		engine->ClientCommand ( edict(), szReturnString );
	}

	SetModel( szModelName );
	SetupPlayerSoundsByModel( szModelName );

	m_flNextModelChangeTime = gpGlobals->curtime + MODEL_CHANGE_INTERVAL;
}

void CHL2MP_Player::VampireStealthCalc()
{
	if (covenClassID == COVEN_CLASSID_FIEND && GetLoadout(2) > 0)
	{
		if (GetRenderMode() != kRenderTransTexture)
				SetRenderMode( kRenderTransTexture );
		if (GetActiveWeapon() != NULL && GetActiveWeapon()->GetRenderMode() != kRenderTransTexture)
				GetActiveWeapon()->SetRenderMode( kRenderTransTexture );

		int max_velocity = 10;
		float alpha = 1.0f;

		if (/*m_Local.m_bDucked && */coven_timer_vstealth == 0.0f && VectorLength(GetAbsVelocity()) <= max_velocity && !KO)
		{
			coven_timer_vstealth = gpGlobals->curtime;
		}
		else if (/*m_Local.m_bDucked && */coven_timer_vstealth > 0.0f && VectorLength(GetAbsVelocity()) <= max_velocity && !KO)
		{
				alpha = 1.0f - 0.2f * GetLoadout(2) * (gpGlobals->curtime - coven_timer_vstealth);//180
		}
		else if (/*!m_Local.m_bDucked || */VectorLength(GetAbsVelocity()) >= 0 || KO)
		{
			coven_timer_vstealth = 0.0f;
			alpha = 1.0f;
		}
		if (GetFlags() & FL_ONFIRE)
		{
			alpha = 1.0f;
		}

		if (alpha > 1.0f)
		{
			alpha = 1.0f;
		}
		float min = 0.09f - 0.03f*GetLoadout(2);
		if (alpha < min)
		{
			alpha = min;
		}
		m_floatCloakFactor = 1.0f-alpha;
		if (GetActiveWeapon() != NULL)
		{
			//GetActiveWeapon()->SetRenderColorA(alpha);
		}
		if (GetViewModel())
		{
			//GetViewModel()->SetRenderColorA(alpha);
		}
		//if (coven_timer_vstealth > 0.0f)
		//	Msg("Player:%s coven_timer_vstealth:%.02f velocity:%.02f KO:%d cloakfactor:%.02f\n", GetPlayerName(), coven_timer_vstealth, VectorLength(GetAbsVelocity()), KO, alpha);
	}
}

float CHL2MP_Player::Feed()
{
	//BB: JAM request... dont break stealth to feed
	//coven_timer_vstealth = 0.0f;

	if (coven_timer_damage > 0.0f)
		return 0.0f;

	if (coven_timer_feed < 0.0f)
	{
		coven_timer_feed = gpGlobals->curtime + 0.5f;
	}
	else if (gpGlobals->curtime > coven_timer_feed + 0.5f)
	{
		coven_timer_feed = -1.0f;
	}
	else if (gpGlobals->curtime > coven_timer_feed)
	{
		coven_timer_feed = -1.0f;
		int temp = 3;
		//BB: new method...
		if (covenClassID == COVEN_CLASSID_BLOOD)
		{
				temp = 6;
		}
		//BB: Coven GORGE implementation
		if (covenClassID == COVEN_CLASSID_GORE && GetLoadout(2) > 0)
		{
			int newmax = GetMaxHealth() * (1.0f + 0.1f*GetLoadout(2));
			temp = newmax - GetHealth();
			if (temp > 0)
			{
				if (temp > 3+GetLoadout(2))
					temp = 3+GetLoadout(2);
				SetHealth(GetHealth()+temp);
			}
		}
		else
			temp = TakeHealth(temp, DMG_GENERIC );
		//BB: new method... remove this, body only counts as 1x, while BB gets 2x
		if (temp > 3)
			temp = 3;
		if (temp > 0)
			EmitSound("Vampire.Regen");
		return (float)temp;
	}

	return 0.0f;
}

void CHL2MP_Player::VampireEnergyHandler()
{
	if (covenClassID == COVEN_CLASSID_DEGEN)
	{
		if (coven_timer_detonate > 0.0f && gpGlobals->curtime > coven_timer_detonate)
		{
			coven_timer_detonate = gpGlobals->curtime + 0.5f;
			coven_detonate_power += 2*GetLoadout(3);
			SuitPower_Drain(2.0f);
		}
		if (coven_detonate_power > 0 && (SuitPower_GetCurrentPercentage() < 2.0f || m_afButtonReleased & IN_ABIL4))
		{
			coven_timer_detonate = -1.0f;
			SuitPower_RemoveDeviceBits( bits_COVEN_DETONATE );
			SetGlobalCooldown(3, gpGlobals->curtime + 6.0f);
			BloodExplode(GetLoadout(3));
			coven_detonate_power = 0;
		}
	}
}

void CHL2MP_Player::VampireReSolidify()
{
	if (solidcooldown > 0.0f)
	{
		Ray_t ray;
		trace_t pm;
		ray.Init( GetAbsOrigin(), GetAbsOrigin(), GetPlayerMins(), GetPlayerMaxs() );
		UTIL_TraceRay( ray, MASK_PLAYERSOLID, this, COLLISION_GROUP_PLAYER, &pm );
		if ( (pm.contents & MASK_PLAYERSOLID) && pm.m_pEnt && pm.m_pEnt->IsPlayer())
		{
			solidcooldown = gpGlobals->curtime;
		}
	}
	if (solidcooldown > 0.0f && solidcooldown + 0.1f < gpGlobals->curtime)
	{
		SetCollisionGroup(COLLISION_GROUP_PLAYER);
		solidcooldown = -1.0f;
	}
}

void CHL2MP_Player::SetupPlayerSoundsByModel( const char *pModelName )
{
	if ( Q_stristr( pModelName, "models/human") )
	{
		m_iPlayerSoundType = (int)PLAYER_SOUNDS_CITIZEN;
	}
	else if ( Q_stristr( pModelName, "models/breen") )
	{
		m_iPlayerSoundType = (int)PLAYER_SOUNDS_CITIZEN;
	}
	else if ( Q_stristr( pModelName, "models/gman") )
	{
		m_iPlayerSoundType = (int)PLAYER_SOUNDS_CITIZEN;
	}
	else if ( Q_stristr( pModelName, "models/brdeg") )
	{
		m_iPlayerSoundType = (int)PLAYER_SOUNDS_CITIZEN;
	}
	else if ( Q_stristr( pModelName, "models/human") )
	{
		m_iPlayerSoundType = (int)PLAYER_SOUNDS_CITIZEN;
	}
	else if ( Q_stristr(pModelName, "police" ) )
	{
		m_iPlayerSoundType = (int)PLAYER_SOUNDS_METROPOLICE;
	}
	else if ( Q_stristr(pModelName, "combine" ) )
	{
		m_iPlayerSoundType = (int)PLAYER_SOUNDS_COMBINESOLDIER;
	}
}

void CHL2MP_Player::ResetAnimation( void )
{
	if ( IsAlive() )
	{
		SetSequence ( -1 );
		SetActivity( ACT_INVALID );

		if (!GetAbsVelocity().x && !GetAbsVelocity().y)
			SetAnimation( PLAYER_IDLE );
		else if ((GetAbsVelocity().x || GetAbsVelocity().y) && ( GetFlags() & FL_ONGROUND ))
			SetAnimation( PLAYER_WALK );
		else if (GetWaterLevel() > 1)
			SetAnimation( PLAYER_WALK );
	}
}


bool CHL2MP_Player::Weapon_Switch( CBaseCombatWeapon *pWeapon, int viewmodelindex )
{
	bool bRet = BaseClass::Weapon_Switch( pWeapon, viewmodelindex );

	if ( bRet == true )
	{
		ResetAnimation();
	}

	return bRet;
}

void CHL2MP_Player::DoLeap()
{
	Vector force;
	AngleVectors(EyeAngles(), &force );
	//Msg("%f\n", force.z);
	force.z = force.z*0.92f;
	EmitSound("Leap");
	//force = 780 * force;
	//VectorAdd(GetLocalVelocity(), force, force);
	//force.z += 1.5*sqrt(2 * 800.0f * 21.0f);
	//SetLocalVelocity(force);
	ApplyAbsVelocityImpulse( (480+20*GetLoadout(0))*force );//780?? 520
}

float CHL2MP_Player::DamageForce( const Vector &size, float damage )
{ 
	//BB: TODO: knockback for damage
	float force = damage * ((32 * 32 * 72.0) / (size.x * size.y * size.z)) * 10;
	
	if ( force > 1000.0) 
	{
		force = 1000.0;
	}

	//Msg("%.02f",force);

	return force;
}

void CHL2MP_Player::PreThink( void )
{
	if (coven_display_autolevel)
	{
		KeyValues *data = new KeyValues("data");
		data->SetBool( "auto", true );
		data->SetInt( "level", covenLevelCounter);
		data->SetInt( "load1", GetLoadout(0));
		data->SetInt( "load2", GetLoadout(1));
		data->SetInt( "load3", GetLoadout(2));
		data->SetInt( "load4", GetLoadout(3));
		data->SetInt( "class", covenClassID);
		ShowViewPortPanel( PANEL_LEVEL, true, data );
		data->deleteThis();
		coven_display_autolevel = false;
	}
	QAngle vOldAngles = GetLocalAngles();
	QAngle vTempAngles = GetLocalAngles();

	vTempAngles = EyeAngles();

	if ( vTempAngles[PITCH] > 180.0f )
	{
		vTempAngles[PITCH] -= 360.0f;
	}

	SetLocalAngles( vTempAngles );

	if (GetTeamNumber() != TEAM_SPECTATOR)
		DoStatusThink();

	BaseClass::PreThink();
	State_PreThink();

	//Reset bullet force accumulator, only lasts one frame
	m_vecTotalBulletForce = vec3_origin;
	SetLocalAngles( vOldAngles );

	//BB: CTS logic
	if (HL2MPRules()->cts_inplay && covenStatusEffects & COVEN_FLAG_CTS)
	{
		if ((HL2MPRules()->cts_zone-GetLocalOrigin()).Length() < HL2MPRules()->cts_zone_radius)
		{
			covenStatusEffects &= covenStatusEffects & ~COVEN_FLAG_CTS;
			HL2MPRules()->AddScore(COVEN_TEAMID_SLAYERS, sv_coven_pts_cts.GetInt());
			HL2MPRules()->GiveItemXP(COVEN_TEAMID_SLAYERS, sv_coven_xp_basekill.GetInt()+sv_coven_xp_inckill.GetInt()*(covenLevelCounter-1));
			EmitSound( "ItemBattery.Touch" );

			HL2MPRules()->SpawnCTS = gpGlobals->curtime + 5.0f;
			const char *killer_weapon_name = "cap_cts_slay";
			IGameEvent *event = gameeventmanager->CreateEvent( "player_death" );
			if( event )
			{
				event->SetInt("userid",  GetUserID());
				event->SetInt("attacker",  GetUserID());
				event->SetString("weapon", killer_weapon_name );
				event->SetString("point", "Supplies");
				event->SetInt( "priority", 7 );
				gameeventmanager->FireEvent( event );
			}
		}
	}

	//BB: cap point logic
	if (IsAlive() && gpGlobals->curtime > lastCapPointTime) //no spectators allowed
	{
		CHL2MPRules *pRules = HL2MPRules();
		Vector tVec;
		if (pRules)
		{
			int index = 3*lastCheckedCapPoint;
			tVec = Vector(pRules->cap_point_coords.Get(index), pRules->cap_point_coords.Get(index+1), pRules->cap_point_coords.Get(index+2));
		}

		covenStatusEffects &= covenStatusEffects & ~COVEN_FLAG_CAPPOINT;

		if (IsAlive() && ((tVec-GetLocalOrigin()).Length() < pRules->cap_point_distance[lastCheckedCapPoint]))
		{
			bool itsago = true;
			if (pRules->cap_point_sightcheck[lastCheckedCapPoint])
			{
				trace_t tr;
				UTIL_TraceLine( EyePosition(), tVec, MASK_SOLID, this, COLLISION_GROUP_DEBRIS, &tr );
				if ( tr.fraction != 1.0 )
					itsago = false;
			}
			if (itsago)
			{
				covenStatusEffects |= COVEN_FLAG_CAPPOINT;
				int n = pRules->cap_point_status.Get(lastCheckedCapPoint);
				lastCapPointTime = gpGlobals->curtime+0.1f;
				pRules->cap_point_timers[lastCheckedCapPoint] = gpGlobals->curtime+0.2f;
				if (GetTeamNumber() == COVEN_TEAMID_SLAYERS)
				{
					if (n < 120)
					{
						pRules->cap_point_status.Set(lastCheckedCapPoint, n+1);
						if ((n+1)==120 && pRules->cap_point_state[lastCheckedCapPoint] != GetTeamNumber())
						{
							pRules->cap_point_state.Set(lastCheckedCapPoint,COVEN_TEAMID_SLAYERS);
							//GetGlobalTeam( GetTeamNumber() )->AddScore(COVEN_CAP_SCORE);
							pRules->GiveItemXP(COVEN_TEAMID_SLAYERS);
							const char *killer_weapon_name = "cap_slay";
							IGameEvent *event = gameeventmanager->CreateEvent( "player_death" );
							if( event )
							{
								event->SetInt("userid",  GetUserID());
								event->SetInt("attacker",  GetUserID());
								event->SetString("weapon", killer_weapon_name );
								event->SetString("point", pRules->cap_point_names[lastCheckedCapPoint]);
								event->SetInt( "priority", 7 );
								gameeventmanager->FireEvent( event );
							}
						}
					}
				}
				else
				{
					if (n >0)
					{
						pRules->cap_point_status.Set(lastCheckedCapPoint, n-1);
						if ((n-1)==0  && pRules->cap_point_state[lastCheckedCapPoint] != GetTeamNumber())
						{
							pRules->cap_point_state.Set(lastCheckedCapPoint,COVEN_TEAMID_VAMPIRES);
							pRules->GiveItemXP(COVEN_TEAMID_VAMPIRES);
							//GetGlobalTeam( GetTeamNumber() )->AddScore(COVEN_CAP_SCORE);
							const char *killer_weapon_name = "cap_vamp";
							IGameEvent *event = gameeventmanager->CreateEvent( "player_death" );
							if( event )
							{
								event->SetInt("userid",  GetUserID());
								event->SetInt("attacker",  GetUserID());
								event->SetString("weapon", killer_weapon_name );
								event->SetString("point", pRules->cap_point_names[lastCheckedCapPoint]);
								event->SetInt( "priority", 7 );
								gameeventmanager->FireEvent( event );
							}
						}
					}
				}
			}
		}
		else
		{
			lastCheckedCapPoint++;
		}

		if (pRules && lastCheckedCapPoint >= pRules->num_cap_points)
			lastCheckedCapPoint = 0;
	}

	if (GetTeamNumber() == COVEN_TEAMID_VAMPIRES)
	{
		DoVampirePreThink();
	}
	else if (GetTeamNumber() == COVEN_TEAMID_SLAYERS)
	{
		DoSlayerPreThink();
	}
}

void CHL2MP_Player::DoVampirePreThink()
{
	DoVampireAbilityThink();
	VampireCheckGore();
	VampireReSolidify();
	VampireCheckRegen();
	VampireManageRagdoll();
	VampireStealthCalc();
	VampireDodgeHandler();
	VampireEnergyHandler();
}

void CHL2MP_Player::VampireCheckGore()
{
	if (covenClassID == COVEN_CLASSID_GORE)
	{
		if (SuitPower_GetCurrentPercentage() <= 0.0f && gorephased)
		{
			DoGorePhase();
			RecalcGoreDrain();

		}
		else if (gorephased)
		{
			m_floatCloakFactor = 1.0f;
		}

		if (gorelock)
		{
			SetMaxSpeed(HL2_NITRO_SPEED);
			Vector t = GetAbsVelocity();
			t.x = t.y = 0;
			SetAbsVelocity(lock_ts*800 + t);
		}
		if (m_afButtonReleased & IN_ABIL2 && gorelock)
		{
			gorelock = false;
			RecalcGoreDrain();
			ComputeSpeed();
			SetGlobalCooldown(1, gpGlobals->curtime + 4.0f-GetLoadout(1));
		}
		if (gorelock && SuitPower_GetCurrentPercentage() <= 0.0f)
		{
			gorelock = false;
			RecalcGoreDrain();
			ComputeSpeed();
			SetGlobalCooldown(1, gpGlobals->curtime + 4.0f-GetLoadout(1));
		}
	}
}

void CHL2MP_Player::Touch(CBaseEntity *pOther)
{
	if (pOther->IsPlayer())
	{
		if (pOther->GetTeamNumber() == COVEN_TEAMID_SLAYERS && GetTeamNumber() == COVEN_TEAMID_VAMPIRES)
		{
			/*if (covenClassID == COVEN_CLASSID_GORE && gorelock)
			{
				//CTakeDamageInfo info(this, this, 0.0f, DMG_CLUB);
				Vector temp = GetAbsVelocity();
				//trace_t tr;
				//UTIL_TraceLine( GetAbsOrigin(), pOther->GetAbsOrigin(), MASK_SHOT_HULL, pOther, COLLISION_GROUP_NONE, &tr );
				VectorNormalize(temp);
				//pOther->DispatchTraceAttack(info, temp, &tr);
				temp.z = 1;
				//pOther->SetAbsVelocity(Vector(0,0,200));
				pOther->ApplyAbsVelocityImpulse(250*temp);
				Vector temp2 = pOther->GetAbsVelocity();
				if (temp2.z > 250)
					temp2.z = 250;
				pOther->SetAbsVelocity(temp2);
				//VectorAdd(temp2, 450*temp, temp2);
			}*/
		}
	}
	BaseClass::Touch(pOther);
}

void CHL2MP_Player::DoSlayerPreThink()
{
	DoSlayerAbilityThink();
	SlayerGutcheckThink();
	SlayerSoulThink();
	SlayerLightHandler();
	SlayerEnergyHandler();
	SlayerHolywaterThink();
}

void CHL2MP_Player::DoVampirePostThink()
{
	VampireCheckResurrect();
}

void CHL2MP_Player::DoSlayerPostThink()
{
	SlayerVampLeapDetect();
}

void CHL2MP_Player::SlayerVampLeapDetect()
{
	if (IsAlive())
	{
		Vector forward, temp;
		temp = GetAbsOrigin();
		temp.z += 52;
		AngleVectors(GetAbsAngles(), &forward);
		VectorNormalize(forward);
		trace_t tr;
		bool itsago = false;
		CBaseEntity *pEntity = NULL;
		UTIL_TraceLine( temp + forward*10, temp + forward * 1500, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );
		if ( !tr.startsolid && tr.DidHitNonWorldEntity() )
		{
			pEntity = tr.m_pEnt;

			if ( pEntity && (pEntity != this) && pEntity->IsPlayer())
			{
				itsago = true;
			}
		}
		if (itsago && pEntity)
		{
			if (pEntity->GetTeamNumber() == TEAM_REBELS && VectorLength(pEntity->GetAbsVelocity()) > 400 && coven_timer_leapdetectcooldown < 0.0f)
			{
				EmitSound("Leap.Detect");
				coven_timer_leapdetectcooldown = gpGlobals->curtime;
			}
		}
		else
		{
			if (gpGlobals->curtime > coven_timer_leapdetectcooldown + 3.0f)
			{
				coven_timer_leapdetectcooldown = -1.0f;
			}
		}
	}
}

void CHL2MP_Player::Taunt()
{
	int roll = random->RandomInt(0,15);
	//BB: a 4/16 chance to play the taunt... this isn't 1/4 because I don't trust random number generators
	//screw that lets make it (less than) 50%
	if (roll > 5)
		return;
	EmitSound("Stake.Taunt");
}

void CHL2MP_Player::VampireManageRagdoll()
{
	if (myServerRagdoll != NULL && ((CRagdollProp *)myServerRagdoll)->flClearTime > 0.0f)
	{
		if (gpGlobals->curtime > ((CRagdollProp *)myServerRagdoll)->flClearTime)
		{
			if (myServerRagdoll == m_hRagdoll)
			{
				m_hRagdoll = NULL;
			}
			UTIL_Remove(myServerRagdoll);
			myServerRagdoll = NULL;
		}
		else
		{
			if (myServerRagdoll->GetRenderMode() != kRenderTransTexture)
				myServerRagdoll->SetRenderMode( kRenderTransTexture );
			if (((CRagdollProp *)myServerRagdoll)->flClearTime - gpGlobals->curtime < 4.5f)
			{
				myServerRagdoll->SetRenderColorA(255.0f*(((CRagdollProp *)myServerRagdoll)->flClearTime - gpGlobals->curtime)/4.5f);
			}
		}
	}
}

void CHL2MP_Player::VampireCheckResurrect()
{
	float x = 4.268f;
	float y = 5.0f;
	if (covenClassID == COVEN_CLASSID_DEGEN)
	{
		x -= 0.66f*GetLoadout(1);
		y -= 0.66f*GetLoadout(1);
	}
	//BB: vampire ressurect... start the sound early...
	if (timeofdeath > 0 && /*gpGlobals->curtime < timeofdeath + 4.9f &&*/ gpGlobals->curtime > timeofdeath + x && !rezsound)
	{
		EmitSound( "Resurrect" );
		rezsound = true;
		if (covenClassID == COVEN_CLASSID_DEGEN)
			SetHealth(GetMaxHealth()*(0.25f+0.1f*GetLoadout(3)));
	}
	else
	//BB: release the vampire player if enough time has passed (and not freezetime)
	if (timeofdeath > 0 && gpGlobals->curtime > timeofdeath + y)
	{
		if (myServerRagdoll != NULL && ((CRagdollProp *)myServerRagdoll)->flClearTime < 0.0f)
		{
			//BB: move the player to the corpse???
			if (IsAlive())
			{
					SetAbsAngles(myServerRagdoll->GetAbsAngles());
					SetAbsOrigin(myServerRagdoll->GetAbsOrigin());
					SetAbsVelocity(myServerRagdoll->GetAbsVelocity());
			}
			//BB: crude check: check for wall clips
			Ray_t ray;
			trace_t pm;
			ray.Init( GetAbsOrigin(), GetAbsOrigin(), GetPlayerMins(), GetPlayerMaxs() );
			UTIL_TraceRay( ray, MASK_PLAYERSOLID, this, COLLISION_GROUP_DEBRIS, &pm );
			if ( pm.m_pEnt && pm.m_pEnt->GetMoveType() == MOVETYPE_NONE && pm.m_pEnt->m_takedamage == DAMAGE_NO)
			{
				//help we are in something!!! try going +x
				int x = 16;
				int y = 0;
				ray.Init( GetAbsOrigin()+Vector(x,y,0), GetAbsOrigin()+Vector(x,y,0), GetPlayerMins(), GetPlayerMaxs() );
				UTIL_TraceRay( ray, MASK_PLAYERSOLID, this, COLLISION_GROUP_DEBRIS, &pm );
				if ( pm.m_pEnt && pm.m_pEnt->GetMoveType() == MOVETYPE_NONE && pm.m_pEnt->m_takedamage == DAMAGE_NO)
				{
					//fail move up y
					y = 16;
					ray.Init( GetAbsOrigin()+Vector(x,y,0), GetAbsOrigin()+Vector(x,y,0), GetPlayerMins(), GetPlayerMaxs() );
					UTIL_TraceRay( ray, MASK_PLAYERSOLID, this, COLLISION_GROUP_DEBRIS, &pm );
					if ( pm.m_pEnt && pm.m_pEnt->GetMoveType() == MOVETYPE_NONE && pm.m_pEnt->m_takedamage == DAMAGE_NO)
					{
						//fail move down x
						x = -16;
						ray.Init( GetAbsOrigin()+Vector(x,y,0), GetAbsOrigin()+Vector(x,y,0), GetPlayerMins(), GetPlayerMaxs() );
						UTIL_TraceRay( ray, MASK_PLAYERSOLID, this, COLLISION_GROUP_DEBRIS, &pm );
						if ( pm.m_pEnt && pm.m_pEnt->GetMoveType() == MOVETYPE_NONE && pm.m_pEnt->m_takedamage == DAMAGE_NO)
						{
							//fail move down y
							y = -16;
							ray.Init( GetAbsOrigin()+Vector(x,y,0), GetAbsOrigin()+Vector(x,y,0), GetPlayerMins(), GetPlayerMaxs() );
							UTIL_TraceRay( ray, MASK_PLAYERSOLID, this, COLLISION_GROUP_DEBRIS, &pm );
							if ( pm.m_pEnt && pm.m_pEnt->GetMoveType() == MOVETYPE_NONE && pm.m_pEnt->m_takedamage == DAMAGE_NO)
							{
								//fail I GIVE UP!
								//reset x and y to default
								x = y = 0;
							}
						}
					}
				}
				//move me to the result of the above toil
				SetAbsOrigin(GetAbsOrigin() + Vector(x,y,0));
			}

			if (myServerRagdoll == m_hRagdoll)
			{
				m_hRagdoll = NULL;
			}
			UTIL_Remove(myServerRagdoll);
			myServerRagdoll = NULL;


			//BB: reset the faded screen...
			color32 nothing = {75,0,0,200};
			UTIL_ScreenFade( this, nothing, 1, 0, FFADE_IN | FFADE_PURGE );
			RemoveAllDecals();
			float m_DmgRadius = 51.2f;
			trace_t		pTrace;
			Vector		vecSpot;// trace starts here!

			vecSpot = GetAbsOrigin() + Vector ( 0 , 0 , 8 );
			UTIL_TraceLine ( vecSpot, vecSpot + Vector ( 0, 0, -32 ), MASK_SHOT_HULL, this, COLLISION_GROUP_NONE, & pTrace);

			#if !defined( CLIENT_DLL )
				Vector vecAbsOrigin = GetAbsOrigin();
				int contents = UTIL_PointContents ( vecAbsOrigin );

				#if !defined( TF_DLL )
					// Since this code only runs on the server, make sure it shows the tempents it creates.
					// This solves a problem with remote detonating the pipebombs (client wasn't seeing the explosion effect)
					CDisablePredictionFiltering disabler;
				#endif

				if ( pTrace.fraction != 1.0 )
				{
					Vector vecNormal = pTrace.plane.normal;
					surfacedata_t *pdata = physprops->GetSurfaceData( pTrace.surface.surfaceProps );	
					CPASFilter filter( vecAbsOrigin );

					te->Explosion( filter, -1.0, // don't apply cl_interp delay
						&vecAbsOrigin,
						!( contents & MASK_WATER ) ? g_sModelIndexFireball : g_sModelIndexWExplosion,
						m_DmgRadius,// * .03, 
						12,
						TE_EXPLFLAG_NONE | TE_EXPLFLAG_NOSOUND,
						m_DmgRadius,
						0.0,
						&vecNormal,
						(char) pdata->game.material );
				}
				else
				{
					CPASFilter filter( vecAbsOrigin );
					te->Explosion( filter, -1.0, // don't apply cl_interp delay
						&vecAbsOrigin, 
						!( contents & MASK_WATER ) ? g_sModelIndexFireball : g_sModelIndexWExplosion,
						m_DmgRadius,// * .03, 
						12,
						TE_EXPLFLAG_NONE | TE_EXPLFLAG_NOSOUND,
						m_DmgRadius,
						0.0 );
				}

				// Use the thrower's position as the reported position
				Vector vecReported = GetAbsOrigin();
		
				CTakeDamageInfo info( this, this, vec3_origin, GetAbsOrigin(), 40.0, DMG_NO | DMG_BLAST, 0, &vecReported );

				RadiusDamage( info, GetAbsOrigin(), 300.0f, CLASS_NONE, NULL );

				UTIL_DecalTrace( &pTrace, "Scorch" );
			#endif
				//BB: only unfreeze if it is not intermission
				if (!HL2MPRules()->IsIntermission())
				{
					RemoveFlag(FL_FROZEN);
					RemoveEFlags(EFL_BOT_FROZEN);
				}
			RemoveEffects(EF_NODRAW);

			RemoveSolidFlags( FSOLID_NOT_SOLID );
			SetCollisionGroup(COLLISION_GROUP_DEBRIS);
			solidcooldown = gpGlobals->curtime;

			//solidcooldown = gpGlobals->curtime;
			if (GetActiveWeapon() != NULL)
			{
				//GetActiveWeapon()->RemoveEffects(EF_NODRAW);
				//GetActiveWeapon()->RemoveSolidFlags( FSOLID_NOT_SOLID );
			}
			if (GetViewModel() != NULL)
			{
				GetViewModel()->RemoveEffects(EF_NODRAW);
				if (GetActiveWeapon())
				{
					GetActiveWeapon()->SetViewModel();
				}
			}
			if (!rezsound)
			{
				EmitSound( "Resurrect.Finish" );
			}
		}
		
		KO = false;
		pl.deadflag = false;
		timeofdeath = -1.0f;
		rezsound = false;
	}
}

void CHL2MP_Player::VampireCheckRegen()
{
	if (coven_timer_damage > 0.0f || KO)
	{
		if (gpGlobals->curtime > coven_timer_damage)
		{
			coven_timer_damage = -1.0f;
		}
		return;
	}
	int midhealth = GetMaxHealth()*0.5f;
	if (IsAlive() && GetHealth() < midhealth && gpGlobals->curtime > coven_timer_regen)
	{
		coven_timer_regen = gpGlobals->curtime + 1.5f;
		int hp = 3;
		TakeHealth( hp, DMG_GENERIC );
		EmitSound("Vampire.Regen");
		if (GetHealth() > midhealth)
			SetHealth(midhealth);
	}
}

void CHL2MP_Player::PostThink( void )
{
	BaseClass::PostThink();

	if (GetTeamNumber() == COVEN_TEAMID_VAMPIRES)
	{
		DoVampirePostThink();
	}
	else if (GetTeamNumber() == COVEN_TEAMID_SLAYERS)
	{
		DoSlayerPostThink();
	}
	
	if ( GetFlags() & FL_DUCKING )
	{
		SetCollisionBounds( VEC_CROUCH_TRACE_MIN, VEC_CROUCH_TRACE_MAX );
	}

	m_PlayerAnimState.Update();

	// Store the eye angles pitch so the client can compute its animation state correctly.
	m_angEyeAngles = EyeAngles();

	QAngle angles = GetLocalAngles();
	angles[PITCH] = 0;
	SetLocalAngles( angles );
}

void CHL2MP_Player::PlayerDeathThink()
{
	//BB: TODO: FIX THIS!
	if( !IsObserver() )
	{
		BaseClass::PlayerDeathThink();
	}
}

void CHL2MP_Player::FireBullets ( const FireBulletsInfo_t &info )
{
	// Move other players back to history positions based on local player's lag
	lagcompensation->StartLagCompensation( this, this->GetCurrentCommand() );

	FireBulletsInfo_t modinfo = info;

	CWeaponHL2MPBase *pWeapon = dynamic_cast<CWeaponHL2MPBase *>( GetActiveWeapon() );

	if ( pWeapon )
	{
		int val = pWeapon->GetHL2MPWpnData().m_iPlayerDamage;
		int add = 0;
		if (Q_strcmp(pWeapon->GetHL2MPWpnData().szAmmo1,"Buckshot") == 0)
		{
			if (covenClassID == COVEN_CLASSID_REAVER)
				add = myStrength() - COVEN_BASESTR_REAVER;
			else
				add = myStrength() - COVEN_BASESTR_AVENGER;
		}
		else if (Q_strcmp(pWeapon->GetHL2MPWpnData().szAmmo1,"357") == 0)
		{
			add = myStrength() - COVEN_BASESTR_HELLION;
		}
		else if (Q_strcmp(pWeapon->GetHL2MPWpnData().szAmmo1,"Pistol") == 0)
		{
			add = myStrength() - 10;
		}
		else if (Q_strcmp(pWeapon->GetHL2MPWpnData().szAmmo1,"SMG1") == 0)
		{
			add = myStrength() - 10;
		}
		modinfo.m_iPlayerDamage = modinfo.m_flDamage = val + add;
		//Msg("Damage: %d\n", val+add);
	}

	NoteWeaponFired();

	BaseClass::FireBullets( modinfo );

	// Move other players back to history positions based on local player's lag
	lagcompensation->FinishLagCompensation( this );
}

void CHL2MP_Player::NoteWeaponFired( void )
{
	Assert( m_pCurrentCommand );
	if( m_pCurrentCommand )
	{
		m_iLastWeaponFireUsercmd = m_pCurrentCommand->command_number;
	}
}

extern ConVar sv_maxunlag;

bool CHL2MP_Player::WantsLagCompensationOnEntity( const CBasePlayer *pPlayer, const CUserCmd *pCmd, const CBitVec<MAX_EDICTS> *pEntityTransmitBits ) const
{
	// No need to lag compensate at all if we're not attacking in this command and
	// we haven't attacked recently.
	if ( !( pCmd->buttons & IN_ATTACK ) && (pCmd->command_number - m_iLastWeaponFireUsercmd > 5) )
		return false;

	// If this entity hasn't been transmitted to us and acked, then don't bother lag compensating it.
	if ( pEntityTransmitBits && !pEntityTransmitBits->Get( pPlayer->entindex() ) )
		return false;

	const Vector &vMyOrigin = GetAbsOrigin();
	const Vector &vHisOrigin = pPlayer->GetAbsOrigin();

	// get max distance player could have moved within max lag compensation time, 
	// multiply by 1.5 to to avoid "dead zones"  (sqrt(2) would be the exact value)
	float maxDistance = 1.5 * pPlayer->MaxSpeed() * sv_maxunlag.GetFloat();

	// If the player is within this distance, lag compensate them in case they're running past us.
	if ( vHisOrigin.DistTo( vMyOrigin ) < maxDistance )
		return true;

	// If their origin is not within a 45 degree cone in front of us, no need to lag compensate.
	Vector vForward;
	AngleVectors( pCmd->viewangles, &vForward );
	
	Vector vDiff = vHisOrigin - vMyOrigin;
	VectorNormalize( vDiff );

	float flCosAngle = 0.707107f;	// 45 degree angle
	if ( vForward.Dot( vDiff ) < flCosAngle )
		return false;

	return true;
}

Activity CHL2MP_Player::TranslateTeamActivity( Activity ActToTranslate )
{
	if ( m_iModelType == TEAM_COMBINE )
		 return ActToTranslate;
	
	if ( ActToTranslate == ACT_RUN )
		 return ACT_RUN_AIM_AGITATED;

	if ( ActToTranslate == ACT_IDLE )
		 return ACT_IDLE_AIM_AGITATED;

	if ( ActToTranslate == ACT_WALK )
		 return ACT_WALK_AIM_AGITATED;

	return ActToTranslate;
}

extern ConVar hl2_normspeed;

// Set the activity based on an event or current state
void CHL2MP_Player::SetAnimation( PLAYER_ANIM playerAnim )
{
	int animDesired;

	float speed;

	speed = GetAbsVelocity().Length2D();

	
	// bool bRunning = true;

	//Revisit!
/*	if ( ( m_nButtons & ( IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT ) ) )
	{
		if ( speed > 1.0f && speed < hl2_normspeed.GetFloat() - 20.0f )
		{
			bRunning = false;
		}
	}*/

	if ( GetFlags() & ( FL_FROZEN | FL_ATCONTROLS ) )
	{
		speed = 0;
		playerAnim = PLAYER_IDLE;
	}

	Activity idealActivity = ACT_HL2MP_RUN;

	// This could stand to be redone. Why is playerAnim abstracted from activity? (sjb)
	if ( playerAnim == PLAYER_JUMP )
	{
		idealActivity = ACT_HL2MP_JUMP;
	}
	else if ( playerAnim == PLAYER_DIE )
	{
		if ( m_lifeState == LIFE_ALIVE )
		{
			return;
		}
	}
	else if ( playerAnim == PLAYER_ATTACK1 )
	{
		if ( GetActivity( ) == ACT_HOVER	|| 
			 GetActivity( ) == ACT_SWIM		||
			 GetActivity( ) == ACT_HOP		||
			 GetActivity( ) == ACT_LEAP		||
			 GetActivity( ) == ACT_DIESIMPLE )
		{
			idealActivity = GetActivity( );
		}
		else
		{
			idealActivity = ACT_HL2MP_GESTURE_RANGE_ATTACK;
		}
	}
	else if ( playerAnim == PLAYER_RELOAD )
	{
		idealActivity = ACT_HL2MP_GESTURE_RELOAD;
	}
	else if ( playerAnim == PLAYER_IDLE || playerAnim == PLAYER_WALK )
	{
		if ( !( GetFlags() & FL_ONGROUND ) && GetActivity( ) == ACT_HL2MP_JUMP )	// Still jumping
		{
			idealActivity = GetActivity( );
		}
		/*
		else if ( GetWaterLevel() > 1 )
		{
			if ( speed == 0 )
				idealActivity = ACT_HOVER;
			else
				idealActivity = ACT_SWIM;
		}
		*/
		else
		{
			if ( GetFlags() & FL_DUCKING )
			{
				if ( speed > 0 )
				{
					idealActivity = ACT_HL2MP_WALK_CROUCH;
				}
				else
				{
					idealActivity = ACT_HL2MP_IDLE_CROUCH;
				}
			}
			else
			{
				if ( speed > 0 )
				{
					/*
					if ( bRunning == false )
					{
						idealActivity = ACT_WALK;
					}
					else
					*/
					{
						idealActivity = ACT_HL2MP_RUN;
					}
				}
				else
				{
					idealActivity = ACT_HL2MP_IDLE;
				}
			}
		}

		idealActivity = TranslateTeamActivity( idealActivity );
	}
	
	if ( idealActivity == ACT_HL2MP_GESTURE_RANGE_ATTACK )
	{
		RestartGesture( Weapon_TranslateActivity( idealActivity ) );

		// FIXME: this seems a bit wacked
		Weapon_SetActivity( Weapon_TranslateActivity( ACT_RANGE_ATTACK1 ), 0 );

		return;
	}
	else if ( idealActivity == ACT_HL2MP_GESTURE_RELOAD )
	{
		RestartGesture( Weapon_TranslateActivity( idealActivity ) );
		return;
	}
	else
	{
		SetActivity( idealActivity );

		animDesired = SelectWeightedSequence( Weapon_TranslateActivity ( idealActivity ) );

		if (animDesired == -1)
		{
			animDesired = SelectWeightedSequence( idealActivity );

			if ( animDesired == -1 )
			{
				animDesired = 0;
			}
		}
	
		// Already using the desired animation?
		if ( GetSequence() == animDesired )
			return;

		m_flPlaybackRate = 1.0;
		ResetSequence( animDesired );
		SetCycle( 0 );
		return;
	}

	// Already using the desired animation?
	if ( GetSequence() == animDesired )
		return;

	//Msg( "Set animation to %d\n", animDesired );
	// Reset to first frame of desired animation
	ResetSequence( animDesired );
	SetCycle( 0 );
}


extern int	gEvilImpulse101;
//-----------------------------------------------------------------------------
// Purpose: Player reacts to bumping a weapon. 
// Input  : pWeapon - the weapon that the player bumped into.
// Output : Returns true if player picked up the weapon
//-----------------------------------------------------------------------------
bool CHL2MP_Player::BumpWeapon( CBaseCombatWeapon *pWeapon )
{
	CBaseCombatCharacter *pOwner = pWeapon->GetOwner();

	// Can I have this weapon type?
	if ( !IsAllowedToPickupWeapons() )
		return false;

	if ( pOwner || !Weapon_CanUse( pWeapon ) || !g_pGameRules->CanHavePlayerItem( this, pWeapon ) )
	{
		if ( gEvilImpulse101 )
		{
			UTIL_Remove( pWeapon );
		}
		return false;
	}

	// Don't let the player fetch weapons through walls (use MASK_SOLID so that you can't pickup through windows)
	if( !pWeapon->FVisible( this, MASK_SOLID ) && !(GetFlags() & FL_NOTARGET) )
	{
		return false;
	}

	bool bOwnsWeaponAlready = !!Weapon_OwnsThisType( pWeapon->GetClassname(), pWeapon->GetSubType());

	if ( bOwnsWeaponAlready == true ) 
	{
		//If we have room for the ammo, then "take" the weapon too.
		 if ( Weapon_EquipAmmoOnly( pWeapon ) )
		 {
			 pWeapon->CheckRespawn();

			 UTIL_Remove( pWeapon );
			 return true;
		 }
		 else
		 {
			 return false;
		 }
	}

	pWeapon->CheckRespawn();
	Weapon_Equip( pWeapon );

	return true;
}

void CHL2MP_Player::ChangeTeam( int iTeam )
{
/*	if ( GetNextTeamChangeTime() >= gpGlobals->curtime )
	{
		char szReturnString[128];
		Q_snprintf( szReturnString, sizeof( szReturnString ), "Please wait %d more seconds before trying to switch teams again.\n", (int)(GetNextTeamChangeTime() - gpGlobals->curtime) );

		ClientPrint( this, HUD_PRINTTALK, szReturnString );
		return;
	}*/

	bool bKill = false;

	if ( HL2MPRules()->IsTeamplay() != true && iTeam != TEAM_SPECTATOR )
	{
		//don't let them try to join combine or rebels during deathmatch.
		iTeam = TEAM_UNASSIGNED;
	}

	if ( HL2MPRules()->IsTeamplay() == true )
	{
		if ( iTeam != GetTeamNumber() && GetTeamNumber() != TEAM_UNASSIGNED )
		{
			bKill = true;
		}
	}

	BaseClass::ChangeTeam( iTeam );

	m_flNextTeamChangeTime = gpGlobals->curtime + TEAM_CHANGE_INTERVAL;

	if ( HL2MPRules()->IsTeamplay() == true )
	{
		SetPlayerTeamModel();
	}
	else
	{
		SetPlayerModel();
	}

	if ( iTeam == TEAM_SPECTATOR )
	{
		RemoveAllItems( true );

		State_Transition( STATE_OBSERVER_MODE );
	}

	if ( bKill == true )
	{
		CommitSuicide();
		covenClassID = 0;

		if (coven_ignore_respawns.GetInt() == 0 && covenLevelCounter > 0)
		{
			if (iTeam == COVEN_TEAMID_VAMPIRES)
				covenRespawnTimer = gpGlobals->curtime + COVEN_RESPAWNTIME_BASE + COVEN_RESPAWNTIME_VAMPIRES_MULT*covenLevelCounter;
			else if (iTeam == COVEN_TEAMID_SLAYERS)
				covenRespawnTimer = HL2MPRules()->GetSlayerRespawnTime();
		}
		else
			covenRespawnTimer = gpGlobals->curtime;
	}
}

void CHL2MP_Player::SlayerGutcheckThink()
{
	if (covenClassID == COVEN_CLASSID_REAVER && GetLoadout(2) > 0 && IsAlive())
	{
		if (gpGlobals->curtime > GetCooldown(2))
		{
			covenStatusEffects |= COVEN_FLAG_GCHECK;
		}
	}
}

void CHL2MP_Player::DoStatusThink()
{
	RefreshLoadout();
	RefreshCooldowns();
	if (PointsToSpend() > 0)
	{
		covenStatusEffects |= COVEN_FLAG_LEVEL;
	}
	else
	{
		covenStatusEffects &= covenStatusEffects & ~COVEN_FLAG_LEVEL;
	}

	//SPRINT (actual sprintspeed handled in ComputeSpeed in HL2Player)

	//STUN (handle it here)
	if (covenStatusEffects & COVEN_FLAG_STUN)
	{
		if (gpGlobals->curtime > GetStatusTime(COVEN_BUFF_STUN))
		{
			covenStatusEffects &= covenStatusEffects & ~COVEN_FLAG_STUN;
			SetStatusTime(COVEN_BUFF_STUN, 0.0f);
			SetStatusMagnitude(COVEN_BUFF_STUN, 0);
			RemoveFlag(FL_FROZEN);
		}
		else
			AddFlag(FL_FROZEN);

	}

	//SLOW (actual slow handled in ComputeSpeed in HL2Player)
	if (covenStatusEffects & COVEN_FLAG_SLOW)
	{
		if (gpGlobals->curtime > GetStatusTime(COVEN_BUFF_SLOW))
		{
			covenStatusEffects &= covenStatusEffects & ~COVEN_FLAG_SLOW;
			SetStatusMagnitude(COVEN_BUFF_SLOW, 0);
			SetStatusTime(COVEN_BUFF_SLOW, 0.0f);
			ComputeSpeed();
		}
	}

	//BLOODLUST
	if (covenStatusEffects & COVEN_FLAG_BLUST)
	{
		if (gpGlobals->curtime > GetStatusTime(COVEN_BUFF_BLUST))
		{
			covenStatusEffects &= covenStatusEffects & ~COVEN_FLAG_BLUST;
			SetStatusTime(COVEN_BUFF_BLUST, 0.0f);
			SetStatusMagnitude(COVEN_BUFF_BLUST, 0);
		}
	}

	//MASOCHISM
	if (covenStatusEffects & COVEN_FLAG_MASOCHIST)
	{
		if (gpGlobals->curtime > GetStatusTime(COVEN_BUFF_MASOCHIST))
		{
			covenStatusEffects &= covenStatusEffects & ~COVEN_FLAG_MASOCHIST;
			SetStatusTime(COVEN_BUFF_MASOCHIST, 0.0f);
			SetStatusMagnitude(COVEN_BUFF_MASOCHIST, 0);
			ComputeSpeed();
		}
	}

	//NEW PHASE
	if (covenStatusEffects & COVEN_FLAG_PHASE)
	{
		if (gpGlobals->curtime > GetStatusTime(COVEN_BUFF_PHASE))
		{
			covenStatusEffects &= covenStatusEffects & ~COVEN_FLAG_PHASE;
			SetStatusTime(COVEN_BUFF_PHASE, 0.0f);
			SetStatusMagnitude(COVEN_BUFF_PHASE, 0);
			DoGorePhase();
		}
	}

	//BYELL (actual battleyell will be handled in takedamage)
	if (covenStatusEffects & COVEN_FLAG_BYELL)
	{
		if (gpGlobals->curtime > GetStatusTime(COVEN_BUFF_BYELL))
		{
			covenStatusEffects &= covenStatusEffects & ~COVEN_FLAG_BYELL;
			SetStatusTime(COVEN_BUFF_BYELL, 0.0f);
			SetStatusMagnitude(COVEN_BUFF_BYELL, 0);
		}
	}

	//SHEERWILL
	if (covenStatusEffects & COVEN_FLAG_STATS)
	{
		if (gpGlobals->curtime > GetStatusTime(COVEN_BUFF_STATS))
		{
			covenStatusEffects &= covenStatusEffects & ~COVEN_FLAG_STATS;
			SetStatusTime(COVEN_BUFF_STATS, 0.0f);
			ResetStats();
			//dont kill us!!!
			//reaver
			int conmult = 3;
			if (covenClassID == COVEN_CLASSID_AVENGER)
				conmult = 2;
			int hp = max(GetHealth()-conmult*GetStatusMagnitude(COVEN_BUFF_STATS)*COVEN_HP_PER_CON,1);
			SetHealth(hp);
			ResetVitals();
			SetStatusMagnitude(COVEN_BUFF_STATS, 0);
		}
	}

	//BERSERK
	if (covenStatusEffects & COVEN_FLAG_BERSERK)
	{
		if (gpGlobals->curtime > GetStatusTime(COVEN_BUFF_BERSERK))
		{
			covenStatusEffects &= covenStatusEffects & ~COVEN_FLAG_BERSERK;
			SetStatusTime(COVEN_BUFF_BERSERK, 0.0f);
			int oldmax = GetMaxHealth();
			ResetVitals();
			//dont kill us!!!
			int hp = max(GetHealth()-(oldmax-GetMaxHealth()),1);
			SetHealth(hp);
			SetStatusMagnitude(COVEN_BUFF_BERSERK, 0);
		}
	}
}

void CHL2MP_Player::SlayerHolywaterThink()
{
	//HOLYWATER
	if (IsAlive() && covenStatusEffects & COVEN_FLAG_HOLYWATER && gpGlobals->curtime > coven_timer_holywater)
	{
		int mag = GetStatusMagnitude(COVEN_BUFF_HOLYWATER);
		float temp = ceil(mag/50.0f);
		int newtot = mag-temp;
		if (newtot <= 0)
		{
			newtot = 0;
			coven_timer_holywater = -1.0f;
			covenStatusEffects &= covenStatusEffects & ~COVEN_FLAG_HOLYWATER;
		}
		else
			coven_timer_holywater = gpGlobals->curtime + 1.0f;
		SetStatusMagnitude(COVEN_BUFF_HOLYWATER, newtot);
		TakeHealth(temp, DMG_GENERIC);
	}
}

bool CHL2MP_Player::HandleCommand_SelectClass( int select )
{
	if (select < 0)
	{
		return false;
	}
	if (select == 0)
	{
		if (GetTeamNumber() == COVEN_TEAMID_SLAYERS)
		{
			//int n = random->RandomInt(0,3);
			int n = random->RandomInt(0,2);
			if (n == 0)
				select = COVEN_CLASSID_AVENGER;
			else if (n == 1)
				select = COVEN_CLASSID_REAVER;
			else if (n == 2)
				select = COVEN_CLASSID_HELLION;
			/*else if (n == 3)
				select = DEADEYE;*/
		}
		else if (GetTeamNumber() == COVEN_TEAMID_VAMPIRES)
		{
			//int n = random->RandomInt(0,3);
			int n = random->RandomInt(0,2);
			if (n == 0)
				select = COVEN_CLASSID_FIEND;
			else if (n == 1)
				select = COVEN_CLASSID_GORE;
			else if (n == 2)
				select = COVEN_CLASSID_DEGEN;
			/*else if (n == 3)
				select = BLOODBANK;*/
		}
	}

	covenClassID = select;

	if (!IsDead())
	{
		RemoveAllItems( true );
		CommitSuicide();
		// add 1 to frags to balance out the 1 subtracted for killing yourself
		IncrementFragCount( 1 );
		IncrementDeathCount(-1);
	}
	else
	{
		StopObserverMode();
		State_Transition(STATE_ACTIVE);
	}

	Spawn();

	return true;
}

bool CHL2MP_Player::HandleCommand_JoinTeam( int team )
{
	if ( !GetGlobalTeam( team ) || team == 0 )
	{
		Warning( "HandleCommand_JoinTeam( %d ) - invalid team index.\n", team );
		return false;
	}

	if ( team == TEAM_SPECTATOR )
	{
		// Prevent this is the cvar is set
		if ( !mp_allowspectators.GetInt() )
		{
			ClientPrint( this, HUD_PRINTCENTER, "#Cannot_Be_Spectator" );
			return false;
		}

		if ( GetTeamNumber() != TEAM_UNASSIGNED && !IsDead() )
		{
			m_fNextSuicideTime = gpGlobals->curtime;	// allow the suicide to work

			CommitSuicide();
			covenRespawnTimer = -1.0f;

			// add 1 to frags to balance out the 1 subtracted for killing yourself
			IncrementFragCount( 1 );
		}

		ChangeTeam( TEAM_SPECTATOR );

		return true;
	}
	/*else
	{
		StopObserverMode();
		State_Transition(STATE_ACTIVE);
	}*/

	// Switch their actual team...
	ChangeTeam( team );

	//Show the class menu...
	if (team == TEAM_COMBINE)
	{
		ShowViewPortPanel( PANEL_CLASS, true, NULL );
	}
	else if (team == TEAM_REBELS)
	{
		ShowViewPortPanel( PANEL_CLASS2, true, NULL );
	}

	return true;
}

int CHL2MP_Player::PointsToSpend()
{
	if (covenLevelCounter > COVEN_MAX_LEVEL)
		return COVEN_MAX_LEVEL - GetLevelsSpent();
	int ret = covenLevelCounter - GetLevelsSpent();
	return ret;
}

bool CHL2MP_Player::ClientCommand( const CCommand &args )
{
	if (FStrEq( args[0], "levelskill" ))
	{
		if ( args.ArgC() < 2 )
		{
			Warning( "Player sent bad levelskill syntax\n" );
		}
		else
		{
			if ( ShouldRunRateLimitedCommand( args ) )
			{
				int iSkill = atoi( args[1] )-1;
				if (iSkill < 0 || iSkill > 3)
					return true;
				if (iSkill == 3)
				{
					if (covenLevelCounter < 6)
						return true;
					if (covenLevelCounter < 8 && GetLoadout(iSkill) > 0)
						return true;
					if (covenLevelCounter < 10 && GetLoadout(iSkill) > 1)
						return true;
				}
				if ((covenLevelCounter < 3 && GetLoadout(iSkill) > 0) || (covenLevelCounter < 5 && GetLoadout(iSkill) > 1))
					return true;
				if (PointsToSpend() > 0 && GetLoadout(iSkill) < 3)
				{
					SpendPoint(iSkill);
					SetGlobalCooldown(iSkill, gpGlobals->curtime);
				}
			}
			return true;
		}
	}
	else if ( FStrEq( args[0], "spectate" ) )
	{
		if ( ShouldRunRateLimitedCommand( args ) )
		{
			// instantly join spectators
			HandleCommand_JoinTeam( TEAM_SPECTATOR );	
		}
		return true;
	}
	else if ( FStrEq( args[0], "jointeam" ) ) 
	{
		if ( args.ArgC() < 2 )
		{
			Warning( "Player sent bad jointeam syntax\n" );
		}

		if ( ShouldRunRateLimitedCommand( args ) )
		{
			int iTeam = atoi( args[1] );
			HandleCommand_JoinTeam( iTeam );
		}
		return true;
	}
	else if ( FStrEq( args[0], "class") )
	{
		if (args.ArgC() < 2)
		{
			Warning( "Player sent bad class syntax\n" );
		}
		if ( ShouldRunRateLimitedCommand( args ) )
		{
			int iClass = atoi( args[1] );
			HandleCommand_SelectClass( iClass );
		}
		return true;
	}
	else if ( FStrEq( args[0], "joingame" ) )
	{
		return true;
	}
	else if ( FStrEq( args[0], "drop_supplies" ) )
	{
		if (HL2MPRules()->cts_inplay && (covenStatusEffects & COVEN_FLAG_CTS))
		{
			Vector forward, forwardvel, pos;
			forwardvel = GetAbsVelocity();
			VectorNormalize(forwardvel);
			forwardvel.z = 0;
			AngleVectors(GetAbsAngles(), &forward);
			VectorNormalize(forward);
			forward.z = 0;
			trace_t tr;
			if (forwardvel.Length() > 0)
			{
				pos = GetAbsOrigin()-forwardvel*64+Vector(0,0,32);
			}
			else
			{
				pos = GetAbsOrigin()+forward*64+Vector(0,0,32);
			}
			UTIL_TraceLine(GetAbsOrigin(), pos, MASK_SOLID, this, COLLISION_GROUP_PLAYER, &tr);
			//drop it... if theres room
			if (!tr.DidHit())
			{
				covenStatusEffects &= ~COVEN_FLAG_CTS;
				CBaseEntity *mysupplies = CreateEntityByName( "item_cts" );
				mysupplies->SetAbsOrigin(pos);
				mysupplies->SetLocalAngles(QAngle(random->RandomInt(0,180), random->RandomInt(0,90), random->RandomInt(0,180)));
				mysupplies->AddSpawnFlags( SF_NORESPAWN );
				mysupplies->Spawn();
				HL2MPRules()->thects = mysupplies;
				HL2MPRules()->cts_return_timer = gpGlobals->curtime + sv_coven_cts_returntime.GetFloat();
			}
		}
		return true;
	}

	return BaseClass::ClientCommand( args );
}

void CHL2MP_Player::CheatImpulseCommands( int iImpulse )
{
	switch ( iImpulse )
	{
		case 101:
			{
				if( sv_cheats->GetBool() )
				{
					GiveAllItems();
				}
			}
			break;

		default:
			BaseClass::CheatImpulseCommands( iImpulse );
	}
}

bool CHL2MP_Player::ShouldRunRateLimitedCommand( const CCommand &args )
{
	int i = m_RateLimitLastCommandTimes.Find( args[0] );
	if ( i == m_RateLimitLastCommandTimes.InvalidIndex() )
	{
		m_RateLimitLastCommandTimes.Insert( args[0], gpGlobals->curtime );
		return true;
	}
	else if ( (gpGlobals->curtime - m_RateLimitLastCommandTimes[i]) < HL2MP_COMMAND_MAX_RATE )
	{
		// Too fast.
		return false;
	}
	else
	{
		m_RateLimitLastCommandTimes[i] = gpGlobals->curtime;
		return true;
	}
}

void CHL2MP_Player::CreateViewModel( int index /*=0*/ )
{
	Assert( index >= 0 && index < MAX_VIEWMODELS );

	if ( GetViewModel( index ) )
		return;

	CPredictedViewModel *vm = ( CPredictedViewModel * )CreateEntityByName( "predicted_viewmodel" );
	if ( vm )
	{
		vm->SetAbsOrigin( GetAbsOrigin() );
		vm->SetOwner( this );
		vm->SetIndex( index );
		DispatchSpawn( vm );
		vm->FollowEntity( this, false );
		m_hViewModel.Set( index, vm );
	}
}

bool CHL2MP_Player::BecomeRagdollOnClient( const Vector &force )
{
	return true;
}

// -------------------------------------------------------------------------------- //
// Ragdoll entities.
// -------------------------------------------------------------------------------- //

class CHL2MPRagdoll : public CBaseAnimatingOverlay
{
public:
	DECLARE_CLASS( CHL2MPRagdoll, CBaseAnimatingOverlay );
	DECLARE_SERVERCLASS();

	// Transmit ragdolls to everyone.
	virtual int UpdateTransmitState()
	{
		return SetTransmitState( FL_EDICT_ALWAYS );
	}

public:
	// In case the client has the player entity, we transmit the player index.
	// In case the client doesn't have it, we transmit the player's model index, origin, and angles
	// so they can create a ragdoll in the right place.
	CNetworkHandle( CBaseEntity, m_hPlayer );	// networked entity handle 
	CNetworkVector( m_vecRagdollVelocity );
	CNetworkVector( m_vecRagdollOrigin );
};

LINK_ENTITY_TO_CLASS( hl2mp_ragdoll, CHL2MPRagdoll );

IMPLEMENT_SERVERCLASS_ST_NOBASE( CHL2MPRagdoll, DT_HL2MPRagdoll )
	SendPropVector( SENDINFO(m_vecRagdollOrigin), -1,  SPROP_COORD ),
	SendPropEHandle( SENDINFO( m_hPlayer ) ),
	SendPropModelIndex( SENDINFO( m_nModelIndex ) ),
	SendPropInt		( SENDINFO(m_nForceBone), 8, 0 ),
	SendPropVector	( SENDINFO(m_vecForce), -1, SPROP_NOSCALE ),
	SendPropVector( SENDINFO( m_vecRagdollVelocity ) )
END_SEND_TABLE()


void CHL2MP_Player::CreateRagdollEntity( int damagetype )
{
	if ( m_hRagdoll && m_hRagdoll.Get() != myServerRagdoll)
	{
		UTIL_RemoveImmediate( m_hRagdoll );
		m_hRagdoll = NULL;
	}

	if (myServerRagdoll != NULL && GetTeamNumber() == COVEN_TEAMID_VAMPIRES)
	{
		if (GetTeamNumber() == TEAM_REBELS && rezsound)
		{
			StopSound("Resurrect");
		}
		rezsound = true;
		if (!(damagetype & DMG_DISSOLVE))
		{
			CEntityFlame *pFlame = CEntityFlame::Create( myServerRagdoll );
			if (pFlame)
			{
				pFlame->SetLifetime( 4.5f );
				myServerRagdoll->AddFlag( FL_ONFIRE );

				myServerRagdoll->SetEffectEntity( pFlame );
			
				pFlame->SetSize( 5.0f );
			}
		}
		((CRagdollProp *)myServerRagdoll)->flClearTime = gpGlobals->curtime + 4.5f;
		RemoveFlag( FL_FROZEN );
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CHL2MP_Player::FlashlightIsOn( void )
{
	return IsEffectActive( EF_DIMLIGHT );
}

extern ConVar flashlight;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CHL2MP_Player::FlashlightTurnOn( void )
{
	if( flashlight.GetInt() > 0 && IsAlive() )
	{
		AddEffects( EF_DIMLIGHT );
		EmitSound( "HL2Player.FlashlightOn" );
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CHL2MP_Player::FlashlightTurnOff( void )
{
	RemoveEffects( EF_DIMLIGHT );
	
	if( IsAlive() )
	{
		EmitSound( "HL2Player.FlashlightOff" );
	}
}

void CHL2MP_Player::Weapon_Drop( CBaseCombatWeapon *pWeapon, const Vector *pvecTarget, const Vector *pVelocity )
{
	//Drop a grenade if it's primed.
	if ( GetActiveWeapon() )
	{
		CBaseCombatWeapon *pGrenade = Weapon_OwnsThisType("weapon_frag");

		if ( GetActiveWeapon() == pGrenade )
		{
			if ( ( m_nButtons & IN_ATTACK ) || (m_nButtons & IN_ATTACK2) )
			{
				DropPrimedFragGrenade( this, pGrenade );
				return;
			}
		}
	}

	BaseClass::Weapon_Drop( pWeapon, pvecTarget, pVelocity );
}


void CHL2MP_Player::DetonateTripmines( void )
{
	CBaseEntity *pEntity = NULL;

	while ((pEntity = gEntList.FindEntityByClassname( pEntity, "npc_satchel" )) != NULL)
	{
		CSatchelCharge *pSatchel = dynamic_cast<CSatchelCharge *>(pEntity);
		if (pSatchel->m_bIsLive && pSatchel->GetThrower() == this )
		{
			g_EventQueue.AddEvent( pSatchel, "Explode", 0.20, this, this );
		}
	}

	// Play sound for pressing the detonator
	EmitSound( "Weapon_SLAM.SatchelDetonate" );
}

int CHL2MP_Player::XPForKill(CHL2MP_Player *pAttacker)
{
	//BB: Bots always return a fixed value that is small (nixed... bots are decent enough)
	//if (IsBot())
	//	return 5;

	//BB: TODO: make this more ellaborate... based on player lvl difference
	int retval = sv_coven_xp_basekill.GetInt();

	retval += sv_coven_xp_inckill.GetInt()*(covenLevelCounter-1);

	retval += sv_coven_xp_diffkill.GetInt()*(covenLevelCounter-pAttacker->covenLevelCounter);
	//Msg("XPForKill: %s %d\n",pAttacker->GetPlayerName(), retval);

	retval = max(1,retval);

	return retval;
}

void CHL2MP_Player::GiveTeamXPCentered(int team, int xp, CBasePlayer *ignore)
{
	CTeam *theteam = GetGlobalTeam( team );
	int num = theteam->GetNumPlayers();
	//CUtlVector<CBasePlayer *> nearby;
	for (int i = 0; i < num; i++)
	{
		CBasePlayer *pTemp = theteam->GetPlayer(i);
		if (pTemp && (pTemp->GetLocalOrigin()-GetLocalOrigin()).Length() <= COVEN_XP_ASSIST_RADIUS && pTemp != ignore)
		{
			//nearby.AddToTail(pTemp);
			((CHL2_Player*)pTemp)->GiveXP(xp);
		}
		if (pTemp && ((CHL2MP_Player *)pTemp)->covenStatusEffects & COVEN_FLAG_CAPPOINT)
		{
			((CHL2_Player*)pTemp)->GiveXP(xp);
			//nearby.AddToTail(pTemp);
		}
	}
	/*int divider = nearby.Size();
	for (int j = 0; j < divider; j++)
	{
		CBasePlayer *pTemp = nearby[j];
		if (pTemp && pTemp != ignore)
			((CHL2_Player*)pTemp)->GiveXP(xp);
	}*/
}

void CHL2MP_Player::Event_Killed( const CTakeDamageInfo &info )
{
	if (coven_ignore_respawns.GetInt() == 0)
	{
		if (GetTeamNumber() == COVEN_TEAMID_VAMPIRES)
			covenRespawnTimer = gpGlobals->curtime + COVEN_RESPAWNTIME_BASE + COVEN_RESPAWNTIME_VAMPIRES_MULT*covenLevelCounter;
		else if (GetTeamNumber() == COVEN_TEAMID_SLAYERS)
			covenRespawnTimer = HL2MPRules()->GetSlayerRespawnTime();
	}
	else
		covenRespawnTimer = gpGlobals->curtime;

	if (GetTeamNumber() == COVEN_TEAMID_SLAYERS)
	{
		if (covenClassID == COVEN_CLASSID_AVENGER)
		{
			coven_timer_soul = -1.0f;
			coven_soul_power = 0;
		}
		RevengeCheck();
	}

	if (GetTeamNumber() == COVEN_TEAMID_VAMPIRES)
	{
		if (covenClassID == COVEN_CLASSID_DEGEN)
		{
			coven_timer_detonate = -1.0f;
			coven_detonate_power = 0;
		}
	}

	if (covenStatusEffects & COVEN_FLAG_CTS && HL2MPRules()->cts_inplay)
	{
		CBaseEntity *ent = CreateEntityByName( "item_cts" );
		ent->SetLocalOrigin(EyePosition());
		ent->SetLocalAngles(QAngle(random->RandomInt(0,180), random->RandomInt(0,90), random->RandomInt(0,180)));
		ent->Spawn();
		ent->AddSpawnFlags(SF_NORESPAWN);
		HL2MPRules()->thects = ent;
		HL2MPRules()->cts_return_timer = gpGlobals->curtime + sv_coven_cts_returntime.GetFloat();
	}

	SetRenderColorA(255.0f);
	if (GetActiveWeapon() != NULL)
	{
		GetActiveWeapon()->SetRenderColorA(255.0f);
	}
	if (GetViewModel())
	{
		GetViewModel()->SetRenderColorA(255.0f);
	}

	//update damage info with our accumulated physics force
	CTakeDamageInfo subinfo = info;
	subinfo.SetDamageForce( m_vecTotalBulletForce );

	SetNumAnimOverlays( 0 );

	// Note: since we're dead, it won't draw us on the client, but we don't set EF_NODRAW
	// because we still want to transmit to the clients in our PVS.
	CreateRagdollEntity(info.GetDamageType());

	DetonateTripmines();

	BaseClass::Event_Killed( subinfo );

	if ( info.GetDamageType() & DMG_DISSOLVE )
	{
		if ( m_hRagdoll )
		{
			m_hRagdoll->GetBaseAnimating()->Dissolve( NULL, gpGlobals->curtime, false, ENTITY_DISSOLVE_NORMAL );
			m_hRagdoll = NULL;
			myServerRagdoll = NULL;
		}
	}

	CBaseEntity *pAttacker = info.GetAttacker();

	if ( pAttacker )
	{
		//CTeam *team = GetGlobalTeam( pAttacker->GetTeamNumber() );
		int iScoreToAdd = 1;

		if ( pAttacker == this )
		{
			iScoreToAdd = -1;
		}
		else
		{
			//team->AddScore( iScoreToAdd );
		}

		//BB: this generates the taunt for slayers staking vampires...
		if (pAttacker->GetTeamNumber() == COVEN_TEAMID_SLAYERS && pAttacker != this)
		{
			ToHL2MPPlayer(pAttacker)->Taunt();
		}

		bool worldspawn = !strcmp( pAttacker->GetClassname(), "worldspawn" );

		if (pAttacker->IsPlayer() || worldspawn)
		{
			int t = pAttacker->GetTeamNumber();
			if (worldspawn)
			{
				t = GetTeamNumber();
			}
			if (pAttacker == this || worldspawn)
			{
				if (t == COVEN_TEAMID_SLAYERS)
					t = COVEN_TEAMID_VAMPIRES;
				else if (t == COVEN_TEAMID_VAMPIRES)
					t = COVEN_TEAMID_SLAYERS;
			}
			int xp = 0;
			if (worldspawn)
			{
				xp = XPForKill(this);
				GiveTeamXPCentered(t, xp, NULL);
			}
			else
			{
				xp = XPForKill((CHL2MP_Player *)pAttacker);
				GiveTeamXPCentered(t, xp, (CBasePlayer *)pAttacker);
			}
			/*if (divider <= 0)
				divider = 1;*/
			//BB: attacker ALWAYS gets part of the XP
			if (pAttacker != this && !worldspawn)
				((CHL2_Player*)pAttacker)->GiveXP(xp);
		}
	}

	FlashlightTurnOff();

	//BB: fade to black
	color32 black = {0, 0, 0, 255};
	UTIL_ScreenFade( this, black, 2, 60, FFADE_OUT | FFADE_STAYOUT);

	m_lifeState = LIFE_DEAD;

	RemoveEffects( EF_NODRAW );	// still draw player body
	StopZooming();
}

#if defined(COVEN_DEVELOPER_MODE)
CON_COMMAND(testprobe, "test probe")
{
	CHL2MP_Player *pPlayer = ToHL2MPPlayer( UTIL_GetCommandClient() );
	if (!pPlayer)
		return;
	trace_t	tr;
	QAngle angle = pPlayer->GetAbsAngles();
	Vector forward, right, up;
	AngleVectors( angle, &forward, &right, &up );
	UTIL_TraceLine(pPlayer->GetAbsOrigin()+right*16, pPlayer->GetAbsOrigin()-up*25, MASK_SHOT_HULL, pPlayer, COLLISION_GROUP_WEAPON, &tr);
	if (tr.fraction < 1.0)
	{
		Msg("Ground Right\n");
	}
	else
	{
		Msg("Air Right\n");
	}
	UTIL_TraceLine(pPlayer->GetAbsOrigin()-right*16, pPlayer->GetAbsOrigin()-up*25, MASK_SHOT_HULL, pPlayer, COLLISION_GROUP_WEAPON, &tr);
	if (tr.fraction < 1.0)
	{
		Msg("Ground Left\n");
	}
	else
	{
		Msg("Air Left\n");
	}
}
//BB: BOT PATH DEBUGGING
CON_COMMAND(next_node, "Move to next node")
{
	CHL2MP_Player *pPlayer = ToHL2MPPlayer( UTIL_GetCommandClient() );
	if (!pPlayer)
		return;
	if (pPlayer->coven_debug_nodeloc < 0)
		return;
	if (pPlayer->coven_debug_prevnode < 0)
		pPlayer->coven_debug_prevnode = 0;
	
	int con_count = HL2MPRules()->botnet[pPlayer->coven_debug_nodeloc]->connectors.Count();
	int redflag = HL2MPRules()->botnet[pPlayer->coven_debug_prevnode]->ID;

	if (con_count == 2)
	{
		int i = 0;
		if (redflag == HL2MPRules()->botnet[pPlayer->coven_debug_nodeloc]->connectors[i])
			i++;
		int id = HL2MPRules()->botnet[pPlayer->coven_debug_nodeloc]->connectors[i];
		int sel = 0;
		for (int j = 0; j < HL2MPRules()->bot_node_count; j++)
		{
			if (HL2MPRules()->botnet[j]->ID == id)
			{
				sel = j;
				break;
			}
		}
		pPlayer->SetLocalOrigin(HL2MPRules()->botnet[sel]->location);
		pPlayer->coven_debug_prevnode = pPlayer->coven_debug_nodeloc;
		pPlayer->coven_debug_nodeloc = sel;
		int c = HL2MPRules()->botnet[pPlayer->coven_debug_nodeloc]->connectors.Count();
		Msg("At node %d. %d connectors: ", HL2MPRules()->botnet[pPlayer->coven_debug_nodeloc]->ID, c);
		for (int j = 0; j < c; j++)
		{
			Msg("%d,", HL2MPRules()->botnet[pPlayer->coven_debug_nodeloc]->connectors[j]);
		}
		Msg("\n");
	}
}

CON_COMMAND(prev_node, "Go to previous node")
{
	CHL2MP_Player *pPlayer = ToHL2MPPlayer( UTIL_GetCommandClient() );
	if (!pPlayer)
		return;
	if (pPlayer->coven_debug_prevnode < 0)
		return;
	int temp = pPlayer->coven_debug_prevnode;
	pPlayer->coven_debug_prevnode = pPlayer->coven_debug_nodeloc;
	pPlayer->coven_debug_nodeloc = temp;
	pPlayer->SetLocalOrigin(HL2MPRules()->botnet[temp]->location);
	int c = HL2MPRules()->botnet[pPlayer->coven_debug_nodeloc]->connectors.Count();
	Msg("At node %d. %d connectors: ", HL2MPRules()->botnet[pPlayer->coven_debug_nodeloc]->ID, c);
	for (int j = 0; j < c; j++)
	{
		Msg("%d,", HL2MPRules()->botnet[pPlayer->coven_debug_nodeloc]->connectors[j]);
	}
	Msg("\n");
}

CON_COMMAND(go_to_node, "Go to a node <id>")
{
	CHL2MP_Player *pPlayer = ToHL2MPPlayer( UTIL_GetCommandClient() );
	if (!pPlayer)
		return;
	if ( args.ArgC() != 2 )
		return;
	int temp = atoi(args[ 1 ]);
	//if (temp > HL2MPRules()->botnet.Count())
	//	return;
	int sel = 0;
	/*for (int i = 0; i < HL2MPRules()->botnet.Count(); i++)
	{
		if (HL2MPRules()->botnet[i]->ID == temp)
		{
			sel = i;
			break;
		}
	}
	pPlayer->coven_debug_prevnode = pPlayer->coven_debug_nodeloc;
	pPlayer->coven_debug_nodeloc = sel;
	pPlayer->SetLocalOrigin(HL2MPRules()->botnet[sel]->location);
	//Msg("At node %d.\n", HL2MPRules()->botnet[sel]->ID);*/
	int c = HL2MPRules()->botnet[temp]->connectors.Count();
	pPlayer->SetLocalOrigin(HL2MPRules()->botnet[temp]->location);
	Msg("At node %d. %d connectors: ", HL2MPRules()->botnet[temp]->ID, c);
	for (int j = 0; j < c; j++)
	{
		Msg("%d,", HL2MPRules()->botnet[temp]->connectors[j]);
	}
	Msg("\n");
}

CON_COMMAND(level, "give me some XP")
{
	CHL2MP_Player *pPlayer = ToHL2MPPlayer( UTIL_GetCommandClient() );
	if (!pPlayer)
		return;
	pPlayer->LevelUp(1);
}

CON_COMMAND(location, "print current location")
{
	CHL2MP_Player *pPlayer = ToHL2MPPlayer( UTIL_GetCommandClient() );
	if (!pPlayer)
		return;
	char szReturnString[512];
	Vector temp = pPlayer->GetAbsOrigin();
	Q_snprintf( szReturnString, sizeof( szReturnString ), "\"%f %f %f\"\n", temp.x, temp.y, temp.z);
	ClientPrint( pPlayer, HUD_PRINTCONSOLE, szReturnString );
}

CON_COMMAND(store_loc, "store location for distance")
{
	CHL2MP_Player *pPlayer = ToHL2MPPlayer( UTIL_GetCommandClient() );
	if (!pPlayer)
		return;
	char szReturnString[512];
	Vector temp = pPlayer->GetLocalOrigin();
	pPlayer->store_loc = temp;
	Q_snprintf( szReturnString, sizeof( szReturnString ), "Location Stored: \"%f %f %f\"\n", temp.x, temp.y, temp.z);
	ClientPrint( pPlayer, HUD_PRINTCONSOLE, szReturnString );
}

CON_COMMAND(distance, "distance from store_loc")
{
	CHL2MP_Player *pPlayer = ToHL2MPPlayer( UTIL_GetCommandClient() );
	if (!pPlayer)
		return;
	char szReturnString[512];
	Vector temp = pPlayer->GetLocalOrigin();
	Q_snprintf( szReturnString, sizeof( szReturnString ), "Distance from: \"%f %f %f\" %f\n", temp.x, temp.y, temp.z, (temp-pPlayer->store_loc).Length());
	ClientPrint( pPlayer, HUD_PRINTCONSOLE, szReturnString );
}
#endif

void CHL2MP_Player::VampireDodgeHandler()
{
	if (covenClassID == COVEN_CLASSID_FIEND && covenStatusEffects & COVEN_FLAG_DODGE && (SuitPower_GetCurrentPercentage() <= 0.0f))
	{
		VampireUnDodge();
	}
}

void CHL2MP_Player::SlayerLightHandler()
{
	if (FlashlightIsOn() && IsAlive())
	{
		CTeam *pRebels = GetGlobalTeam( TEAM_REBELS );
		for (int i=0; i < pRebels->GetNumPlayers(); i++)
		{
			if (pRebels->GetPlayer(i))
			{
				CHL2MP_Player *temp = ((CHL2MP_Player *)pRebels->GetPlayer(i));
				Vector direction = temp->EyePosition() - EyePosition();
				float distance = direction.Length();
				if (distance <= 750.0f)
				{
					//close enough to be in range of flashlight, continue...
					Vector forward;
					AngleVectors(EyeAngles(), &forward);
					VectorNormalize(forward);
					VectorNormalize(direction);
					float dot = DotProduct(direction, forward);
					if (dot >= 0.9397f) //.9703f good number... version 2.5
					{
						//within correct FOV, check to see line of sight...
						if (distance < 128.0f)
						{
							//too close... problems happen
							distance = 128.0f;
						}

						trace_t tr;
						Vector dt = EyePosition();
						UTIL_TraceLine(dt, dt + direction*750.0f, MASK_SOLID, this, COLLISION_GROUP_PLAYER, &tr);
						if (tr.DidHitNonWorldEntity() && tr.m_pEnt->IsPlayer() && temp == tr.m_pEnt)
						{
							//WE GOT ONE!!!!!
							/*float adjustedmaxalpha = (750.0f - distance)/450.0f;
							if (adjustedmaxalpha > 1.0f)
								adjustedmaxalpha = 1.0f;
							if (adjustedmaxalpha < 0.0f)
								adjustedmaxalpha = 0.0f;
							//do alpha stuff*/
							temp->m_floatCloakFactor = 0.0f;
							temp->coven_timer_vstealth = 0.0f;

							//do pushback stuff
							if (distance < 250.0f)
							{
								float prop = 100.0f/distance * 100.0f;
								if (prop < 0.0f)
									prop = 0.0f;
								if (prop > 100.0f)
									prop = 100.0f;
								Vector vel = temp->GetAbsVelocity();
								VectorAdd(vel, prop*direction, vel);
								temp->SetAbsVelocity(vel);
							}
							//do damage stuff
							if (distance < 225.0f && (gpGlobals->curtime - temp->coven_timer_light) >= 0.2f )//0.1f
							{
								//BB: boost UV light damage
								float dmg = 3.0f;
								//more damage from light to vamps on fire
								if (temp->GetFlags() & FL_ONFIRE)
									dmg = 5.5f;

								CTakeDamageInfo burn(this, this, dmg, DMG_BURN | DMG_DIRECT);
								temp->OnTakeDamage(burn);
								temp->coven_timer_light = gpGlobals->curtime;
							}
						}
					}
				}
			}
		}
	}
}

void CHL2MP_Player::SlayerEnergyHandler()
{
	if (FlashlightIsOn() && IsAlive() && SuitPower_GetCurrentPercentage() <= 0.0f)
	{
		RemoveEffects( EF_DIMLIGHT );
		SuitPower_ResetDrain();
		SetGlobalCooldown(3, gpGlobals->curtime + 3.0f);
	
		if( IsAlive() )
		{
			EmitSound( "HL2Player.FlashlightOff" );
		}
	}

	if (covenClassID == COVEN_CLASSID_REAVER && SuitPower_GetCurrentPercentage() <= 0.0f)
	{
		covenStatusEffects &= covenStatusEffects & ~COVEN_FLAG_SPRINT;
		ComputeSpeed();
		SetGlobalCooldown(0, gpGlobals->curtime + 3.0f);
		SuitPower_ResetDrain();
	}
}

int CHL2MP_Player::OnTakeDamage_Alive( const CTakeDamageInfo &inputInfo )
{
	int ret = BaseClass::OnTakeDamage_Alive(inputInfo);
	
	if (m_iHealth <= 0 && GetTeamNumber() == COVEN_TEAMID_VAMPIRES && covenClassID == COVEN_CLASSID_GORE && gorephased)
		DoGorePhase();

	if (m_iHealth <= 0 && GetTeamNumber() == COVEN_TEAMID_VAMPIRES && covenClassID == COVEN_CLASSID_FIEND && covenStatusEffects & COVEN_FLAG_BERSERK)
	{
		covenStatusEffects &= covenStatusEffects & ~COVEN_FLAG_BERSERK;
		SetStatusTime(COVEN_BUFF_BERSERK, 0.0f);
		SetStatusMagnitude(COVEN_BUFF_BERSERK, 0);
	}

	if (m_iHealth <= 0 && GetTeamNumber() == COVEN_TEAMID_VAMPIRES && covenClassID == COVEN_CLASSID_FIEND && GetRenderColor().a < 255)
	{
		SetRenderColorA(255);
		SuitPower_ResetDrain();
	}

	//BLOODLUST
	if (inputInfo.GetAttacker() && inputInfo.GetAttacker()->IsPlayer() && inputInfo.GetAttacker() != this)
	{
		CHL2MP_Player *temp = (CHL2MP_Player *)inputInfo.GetAttacker();
		if (temp->covenStatusEffects & COVEN_FLAG_BLUST)
		{
			temp->TakeHealth(inputInfo.GetDamage()*0.1f*temp->GetStatusMagnitude(COVEN_BUFF_BLUST), DMG_GENERIC);
		}
	}

	return ret;
}

void CHL2MP_Player::Extinguish()
{
	m_pFlame = NULL;
	covenStatusEffects &= covenStatusEffects & ~COVEN_FLAG_HOLYWATER;
	SetStatusTime(COVEN_BUFF_HOLYWATER, 0.0f);
	BaseClass::Extinguish(); //FL_ONFIRE ALREADY REMOVED...
}

int CHL2MP_Player::OnTakeDamage( const CTakeDamageInfo &inputInfo )
{
	if (IsBot())
		Bot_Combat_Check(this, inputInfo.GetAttacker());

	//return here if the player is in the respawn grace period vs. slams.
	if ( gpGlobals->curtime < m_flSlamProtectTime &&  (inputInfo.GetDamageType() == DMG_BLAST ) )
		return 0;

	m_vecTotalBulletForce += inputInfo.GetDamageForce();

	CTakeDamageInfo inputInfoAdjust = inputInfo;

	//DODGE
	if (covenClassID == COVEN_CLASSID_FIEND && covenStatusEffects & COVEN_FLAG_DODGE)
	{
		inputInfoAdjust.SetDamage(inputInfoAdjust.GetDamage() * (0.9f - 0.2f*GetLoadout(1)));
		int r = random->RandomInt(0,5);
		if (r < GetLoadout(1))
			inputInfoAdjust.SetDamage(0.0f);
	}

	//BLOOD EXPLODE
	if (inputInfoAdjust.GetDamageType() & DMG_PARALYZE)
	{
		int bits = inputInfoAdjust.GetDamageType() & ~DMG_PARALYZE;
		covenStatusEffects |= COVEN_FLAG_STUN;
		SetStatusTime(COVEN_BUFF_STUN, gpGlobals->curtime + 1.5f);
		inputInfoAdjust.SetDamageType(bits);
	}

	if (GetTeamNumber() == COVEN_TEAMID_SLAYERS && covenStatusEffects & COVEN_FLAG_HOLYWATER)
	{
		float temp = floor(0.2f * inputInfoAdjust.GetDamage());
		int mag = GetStatusMagnitude(COVEN_BUFF_HOLYWATER);
		if (temp > mag)
		{
			temp = mag;
		}
		//moved this only fall damage
		/*
		flIntegerDamage -= temp;
		holyhealtotal -= temp;
		*/
		if (inputInfoAdjust.GetDamageType() == DMG_FALL)
		{
			//holyhealtotal -= 30;
			inputInfoAdjust.SetDamage(inputInfoAdjust.GetDamage() - temp);
			int newtot = mag-temp;
			if (newtot <= 0)
			{
				newtot = 0;
				covenStatusEffects &= covenStatusEffects & ~COVEN_FLAG_HOLYWATER;
				coven_timer_holywater = -1.0f;
			}
			SetStatusMagnitude(COVEN_BUFF_HOLYWATER, newtot);
		}
	}

	//BB: level diff damage adjust
	if (inputInfo.GetAttacker() && inputInfo.GetAttacker()->IsPlayer())
	{
		int diff = ((CHL2MP_Player *)inputInfo.GetAttacker())->covenLevelCounter - covenLevelCounter;
		diff = min(diff,6);
		diff = max(diff,-6);
		inputInfoAdjust.SetDamage(inputInfoAdjust.GetDamage()*(1.0f+diff*COVEN_DAMAGE_PER_LEVEL_ADJUST));
	}

	//BB: handle damage boost
	if (inputInfo.GetAttacker() && inputInfo.GetAttacker()->IsPlayer() && ((CHL2MP_Player *)inputInfo.GetAttacker())->covenStatusEffects & COVEN_FLAG_BYELL)
	{
		inputInfoAdjust.SetDamage(inputInfoAdjust.GetDamage()*(1.0f+0.1f*((CHL2MP_Player *)inputInfo.GetAttacker())->GetStatusMagnitude(COVEN_BUFF_BYELL)));
	}
	
	gamestats->Event_PlayerDamage( this, inputInfoAdjust );

	if (GetTeamNumber() == COVEN_TEAMID_SLAYERS)
	{
		if (covenClassID == COVEN_CLASSID_REAVER && covenStatusEffects & COVEN_FLAG_GCHECK && !(inputInfoAdjust.GetDamageType() & DMG_NO) && !(inputInfoAdjust.GetDamageType() & DMG_HOLY))
		{
			covenStatusEffects &= covenStatusEffects & ~COVEN_FLAG_GCHECK;
			//coven_timer_gcheck = gpGlobals->curtime + 20.0f - 5.0f*GetLoadout(3);
			SetGlobalCooldown(2,gpGlobals->curtime + 20.0f - 5.0f*GetLoadout(2));
			EmitSound("Weapon_Crowbar.Melee_HitWorld");
			inputInfoAdjust.SetDamage(0.0f);
		}
	}
	else if (GetTeamNumber() == COVEN_TEAMID_VAMPIRES)
	{
		if (inputInfo.GetDamageType() & DMG_DIRECT)
		{
			coven_timer_damage = gpGlobals->curtime + 0.5f;
		}
		else
		{
			coven_timer_damage = gpGlobals->curtime + 2.0f;
		}

		if (covenClassID == COVEN_CLASSID_DEGEN && GetLoadout(2) > 0 && !(inputInfoAdjust.GetDamageType() & DMG_NO))
		{
			covenStatusEffects |= COVEN_FLAG_MASOCHIST;
			int stat = GetStatusMagnitude(COVEN_BUFF_MASOCHIST);
			stat += (0.5f*GetLoadout(2))*inputInfoAdjust.GetDamage();
			if (stat > 100)
				stat = 100;
			SetStatusMagnitude(COVEN_BUFF_MASOCHIST, stat);
			SetStatusTime(COVEN_BUFF_MASOCHIST, gpGlobals->curtime + 5.0f + 1.0f*GetLoadout(2));
			ComputeSpeed();
		}
	}

	//BB: DO HOLY WATER DAMAGE DETECT
	if ((inputInfo.GetDamageType() & DMG_HOLY) && IsAlive() && !KO)
	{
		//do holy stuff...
		inputInfoAdjust.SetDamage(0.0f);
		if (GetTeamNumber() == COVEN_TEAMID_SLAYERS)
		{
			//startup the healing aura...
			int mag = GetStatusMagnitude(COVEN_BUFF_HOLYWATER)+inputInfo.GetDamage()/5.0f*40.0f;//50.0f
			if (mag > 150)
				mag = 150;
			SetStatusMagnitude(COVEN_BUFF_HOLYWATER, mag);
			covenStatusEffects |= COVEN_FLAG_HOLYWATER;
			coven_timer_holywater = gpGlobals->curtime + 1.0f;
			//insta heal component
			//TakeHealth(inputInfo.GetDamage()/5.0f*20.0f, DMG_GENERIC);
			//BB: no insta heal anymore?
		}
		else
		{
			//set me on fire or extend fire appropriately...
			if (m_pFlame == NULL)
			{
				m_pFlame = CEntityFlame::Create( this );
				if (m_pFlame)
				{
					m_pFlame->creator = (CBasePlayer *)inputInfo.GetInflictor();
					m_pFlame->SetLifetime( max(inputInfo.GetDamage()/5.0f*10.0f,1.0f) );//15.0f
					AddFlag( FL_ONFIRE );

					SetEffectEntity( m_pFlame );
				}
			}
			else
			{
				m_pFlame->SupplementDamage(inputInfo.GetDamage());
			}
			SetStatusTime(COVEN_BUFF_HOLYWATER, gpGlobals->curtime+m_pFlame->GetRemainingLife());
			covenStatusEffects |= COVEN_FLAG_HOLYWATER;
		}
	}

	return BaseClass::OnTakeDamage( inputInfoAdjust );
}

void CHL2MP_Player::DeathSound( const CTakeDamageInfo &info )
{
	if ( m_hRagdoll && m_hRagdoll->GetBaseAnimating()->IsDissolving() )
		 return;

	char szStepSound[128];

	Q_snprintf( szStepSound, sizeof( szStepSound ), "%s.Die", GetPlayerModelSoundPrefix() );

	const char *pModelName = STRING( GetModelName() );

	//BB: BS fix... MUST do this or gman is a chick
	if ( Q_stristr( pModelName, "models/gman") )
	{
		pModelName = "models/humans/group03/male_08.mdl";
	}

	//BB: manufactured breen/degen also sounds like chick
	if ( Q_stristr( pModelName, "models/brdeg") )
	{
		pModelName = "models/humans/group03/male_08.mdl";
	}

	CSoundParameters params;
	if ( GetParametersForSound( szStepSound, params, pModelName ) == false )
		return;

	Vector vecOrigin = GetAbsOrigin();
	
	CRecipientFilter filter;
	filter.AddRecipientsByPAS( vecOrigin );

	EmitSound_t ep;
	ep.m_nChannel = params.channel;
	ep.m_pSoundName = params.soundname;
	ep.m_flVolume = params.volume;
	ep.m_SoundLevel = params.soundlevel;
	ep.m_nFlags = 0;
	ep.m_nPitch = params.pitch;
	ep.m_pOrigin = &vecOrigin;

	EmitSound( filter, entindex(), ep );
}

CBaseEntity* CHL2MP_Player::EntSelectSpawnPoint( void )
{
	CBaseEntity *pSpot = NULL;
	CBaseEntity *pLastSpawnPoint = g_pLastSpawn;
	edict_t		*player = edict();
	const char *pSpawnpointName = "info_player_deathmatch";

	if ( HL2MPRules()->IsTeamplay() == true )
	{
		if ( GetTeamNumber() == TEAM_COMBINE )
		{
			pSpawnpointName = "info_player_combine";
			pLastSpawnPoint = g_pLastCombineSpawn;
		}
		else if ( GetTeamNumber() == TEAM_REBELS )
		{
			pSpawnpointName = "info_player_rebel";
			pLastSpawnPoint = g_pLastRebelSpawn;
		}

		if ( gEntList.FindEntityByClassname( NULL, pSpawnpointName ) == NULL )
		{
			pSpawnpointName = "info_player_deathmatch";
			pLastSpawnPoint = g_pLastSpawn;
		}
	}

	pSpot = pLastSpawnPoint;
	// Randomize the start spot
	for ( int i = random->RandomInt(1,5); i > 0; i-- )
		pSpot = gEntList.FindEntityByClassname( pSpot, pSpawnpointName );
	if ( !pSpot )  // skip over the null point
		pSpot = gEntList.FindEntityByClassname( pSpot, pSpawnpointName );

	CBaseEntity *pFirstSpot = pSpot;

	do 
	{
		if ( pSpot )
		{
			// check if pSpot is valid
			if ( g_pGameRules->IsSpawnPointValid( pSpot, this ) )
			{
				if ( pSpot->GetLocalOrigin() == vec3_origin )
				{
					pSpot = gEntList.FindEntityByClassname( pSpot, pSpawnpointName );
					continue;
				}

				// if so, go to pSpot
				goto ReturnSpot;
			}
		}
		// increment pSpot
		pSpot = gEntList.FindEntityByClassname( pSpot, pSpawnpointName );
	} while ( pSpot != pFirstSpot ); // loop if we're not back to the start

	// we haven't found a place to spawn yet,  so kill any guy at the first spawn point and spawn there
	if ( pSpot )
	{
		CBaseEntity *ent = NULL;
		for ( CEntitySphereQuery sphere( pSpot->GetAbsOrigin(), 128 ); (ent = sphere.GetCurrentEntity()) != NULL; sphere.NextEntity() )
		{
			// if ent is a client, kill em (unless they are ourselves)
			if ( ent->IsPlayer() && !(ent->edict() == player) )
				ent->TakeDamage( CTakeDamageInfo( GetContainingEntity(INDEXENT(0)), GetContainingEntity(INDEXENT(0)), 300, DMG_GENERIC ) );
		}
		goto ReturnSpot;
	}

	if ( !pSpot  )
	{
		pSpot = gEntList.FindEntityByClassname( pSpot, "info_player_start" );

		if ( pSpot )
			goto ReturnSpot;
	}

ReturnSpot:

	if ( HL2MPRules()->IsTeamplay() == true )
	{
		if ( GetTeamNumber() == TEAM_COMBINE )
		{
			g_pLastCombineSpawn = pSpot;
		}
		else if ( GetTeamNumber() == TEAM_REBELS ) 
		{
			g_pLastRebelSpawn = pSpot;
		}
	}

	g_pLastSpawn = pSpot;

	m_flSlamProtectTime = gpGlobals->curtime + 0.5;

	return pSpot;
} 


CON_COMMAND( timeleft, "prints the time remaining in the match" )
{
	CHL2MP_Player *pPlayer = ToHL2MPPlayer( UTIL_GetCommandClient() );

	int iTimeRemaining = (int)HL2MPRules()->GetMapRemainingTime();
    
	if ( iTimeRemaining == 0 )
	{
		if ( pPlayer )
		{
			ClientPrint( pPlayer, HUD_PRINTTALK, "This game has no timelimit." );
		}
		else
		{
			Msg( "* No Time Limit *\n" );
		}
	}
	else
	{
		int iMinutes, iSeconds;
		iMinutes = iTimeRemaining / 60;
		iSeconds = iTimeRemaining % 60;

		char minutes[8];
		char seconds[8];

		Q_snprintf( minutes, sizeof(minutes), "%d", iMinutes );
		Q_snprintf( seconds, sizeof(seconds), "%2.2d", iSeconds );

		if ( pPlayer )
		{
			ClientPrint( pPlayer, HUD_PRINTTALK, "Time left in map: %s1:%s2", minutes, seconds );
		}
		else
		{
			Msg( "Time Remaining:  %s:%s\n", minutes, seconds );
		}
	}	
}


void CHL2MP_Player::Reset()
{	
	ResetDeathCount();
	ResetFragCount();
}

bool CHL2MP_Player::IsReady()
{
	return m_bReady;
}

void CHL2MP_Player::SetReady( bool bReady )
{
	m_bReady = bReady;
}

void CHL2MP_Player::CheckChatText( char *p, int bufsize )
{
	//Look for escape sequences and replace

	char *buf = new char[bufsize];
	int pos = 0;

	// Parse say text for escape sequences
	for ( char *pSrc = p; pSrc != NULL && *pSrc != 0 && pos < bufsize-1; pSrc++ )
	{
		// copy each char across
		buf[pos] = *pSrc;
		pos++;
	}

	buf[pos] = '\0';

	// copy buf back into p
	Q_strncpy( p, buf, bufsize );

	delete[] buf;	

	const char *pReadyCheck = p;

	HL2MPRules()->CheckChatForReadySignal( this, pReadyCheck );
}

void CHL2MP_Player::State_Transition( HL2MPPlayerState newState )
{
	State_Leave();
	State_Enter( newState );
}


void CHL2MP_Player::State_Enter( HL2MPPlayerState newState )
{
	m_iPlayerState = newState;
	m_pCurStateInfo = State_LookupInfo( newState );

	// Initialize the new state.
	if ( m_pCurStateInfo && m_pCurStateInfo->pfnEnterState )
		(this->*m_pCurStateInfo->pfnEnterState)();
}


void CHL2MP_Player::State_Leave()
{
	if ( m_pCurStateInfo && m_pCurStateInfo->pfnLeaveState )
	{
		(this->*m_pCurStateInfo->pfnLeaveState)();
	}
}


void CHL2MP_Player::State_PreThink()
{
	if ( m_pCurStateInfo && m_pCurStateInfo->pfnPreThink )
	{
		(this->*m_pCurStateInfo->pfnPreThink)();
	}
}


CHL2MPPlayerStateInfo *CHL2MP_Player::State_LookupInfo( HL2MPPlayerState state )
{
	// This table MUST match the 
	static CHL2MPPlayerStateInfo playerStateInfos[] =
	{
		{ STATE_ACTIVE,			"STATE_ACTIVE",			&CHL2MP_Player::State_Enter_ACTIVE, NULL, &CHL2MP_Player::State_PreThink_ACTIVE },
		{ STATE_OBSERVER_MODE,	"STATE_OBSERVER_MODE",	&CHL2MP_Player::State_Enter_OBSERVER_MODE,	NULL, &CHL2MP_Player::State_PreThink_OBSERVER_MODE }
	};

	for ( int i=0; i < ARRAYSIZE( playerStateInfos ); i++ )
	{
		if ( playerStateInfos[i].m_iPlayerState == state )
			return &playerStateInfos[i];
	}

	return NULL;
}

bool CHL2MP_Player::StartObserverMode(int mode)
{
	//BB: TODO: FIX THIS!
	//we only want to go into observer mode if the player asked to, not on a death timeout
	//if ( m_bEnterObserver == true )
	{
		color32 nothing = {0,0,0,255};
		UTIL_ScreenFade( this, nothing, 0, 0, FFADE_IN | FFADE_PURGE );
		VPhysicsDestroyObject();
		return BaseClass::StartObserverMode( mode );
	}
	return false;
}

void CHL2MP_Player::StopObserverMode()
{
	m_bEnterObserver = false;
	BaseClass::StopObserverMode();
}

void CHL2MP_Player::State_Enter_OBSERVER_MODE()
{
	int observerMode = m_iObserverLastMode;
	if ( IsNetClient() )
	{
		const char *pIdealMode = engine->GetClientConVarValue( engine->IndexOfEdict( edict() ), "cl_spec_mode" );
		if ( pIdealMode )
		{
			observerMode = atoi( pIdealMode );
			if ( observerMode <= OBS_MODE_FIXED || observerMode > OBS_MODE_ROAMING )
			{
				observerMode = m_iObserverLastMode;
			}
		}
	}
	m_bEnterObserver = true;
	StartObserverMode( observerMode );
}

void CHL2MP_Player::State_PreThink_OBSERVER_MODE()
{
	// Make sure nobody has changed any of our state.
	//	Assert( GetMoveType() == MOVETYPE_FLY );
	Assert( m_takedamage == DAMAGE_NO );
	Assert( IsSolidFlagSet( FSOLID_NOT_SOLID ) );
	//	Assert( IsEffectActive( EF_NODRAW ) );

	// Must be dead.
	Assert( m_lifeState == LIFE_DEAD );
	Assert( pl.deadflag );
}


void CHL2MP_Player::State_Enter_ACTIVE()
{
	SetMoveType( MOVETYPE_WALK );
	
	// md 8/15/07 - They'll get set back to solid when they actually respawn. If we set them solid now and mp_forcerespawn
	// is false, then they'll be spectating but blocking live players from moving.
	// RemoveSolidFlags( FSOLID_NOT_SOLID );
	
	m_Local.m_iHideHUD = 0;
}


void CHL2MP_Player::State_PreThink_ACTIVE()
{
	//we don't really need to do anything here. 
	//This state_prethink structure came over from CS:S and was doing an assert check that fails the way hl2dm handles death
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHL2MP_Player::CanHearAndReadChatFrom( CBasePlayer *pPlayer )
{
	// can always hear the console unless we're ignoring all chat
	if ( !pPlayer )
		return false;

	return true;
}
