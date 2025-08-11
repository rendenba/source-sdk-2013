#ifndef COVEN_APC_H
#define COVEN_APC_H
#ifdef _WIN32
#pragma once
#endif

#include "props.h"
#include "basecombatcharacter.h"
#include "Sprite.h"

#define COVEN_APC_MAX_TURNING_VALUE	25.0f
#define COVEN_APC_MAX_TURN_ACCEL	3.0f
#define COVEN_APC_GLOW_SPRITE		"sprites/glow1.vmt"

typedef enum
{
	FRONT_PASSENGER,
	REAR_PASSENGER,
	FRONT_DRIVER,
	REAR_DRIVER
	
} WheelLocation_t;

class CCoven_APC;
class CCoven_APCPart;

class CCoven_APCProp : public CPhysicsProp
{
	DECLARE_CLASS(CCoven_APCProp, CPhysicsProp);
public:
	CCoven_APCProp();

	CCoven_APCPart		*m_pParent;
	virtual int			ObjectCaps(void);
	virtual void		Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
};

class CCoven_APCPart : public CBaseAnimating
{
	DECLARE_CLASS(CCoven_APCPart, CBaseAnimating);
public:
	CCoven_APCPart();
	~CCoven_APCPart();

	virtual void			Spawn(void);
	virtual void			Precache(void);
	void					AttachPhysics(void);
	void					MakeSolid(void);
	void					MoveToCorrectLocation(void);
	void					DropToFloor(void);
	void					MoveTo(const Vector *vec, const QAngle *ang);
	void					UpdatePosition(void);
	Vector					GetOffsetPosition(void);
	QAngle					GetOffsetAngle(void);
	float					RestHeight(void);
	inline CCoven_APCProp	*GetProp(void) { return m_pPart; };

	static CCoven_APCPart *CreateAPCPart(CCoven_APC *pParentAPC, char *szModelName, const Vector &vecOffset, const QAngle &angOffset, float flRestHeight, Collision_Group_t collisionGroup, bool bMakeUsable = false);

private:
	CCoven_APCProp			*m_pPart;
	CHandle<CCoven_APC>		m_hAPC;
	IPhysicsConstraint		*m_pConstraint;
	Vector					m_vecOffset;
	QAngle					m_angOffset;
	float					m_flRestHeight;
};

class CCoven_APC : public CBaseCombatCharacter
{
	DECLARE_CLASS(CCoven_APC, CBaseCombatCharacter);
public:
	CCoven_APC();

	void			Spawn(void);
	void			Precache(void);
	void			MakeSolid(void);
	void			StartUp(void);
	void			MoveForward(void);
	void			TurnRight(float flMagnitude);
	void			TurnLeft(float flMagnitude);
	void			ResetLocation(void);
	void			SetGoal(int iGoalNode);
	void			SetCheckpoint(int iGoalCheckpoint);
	void			SetMaxSpeed(float flMaxSpeed);
	void			SetFuelUp(int iFuelUp);
	bool			HasReachedCurrentGoal(void);
	bool			HasValidGoal(void);
	const Vector	&GetFrontAxelPosition(void);
	virtual void	UpdateOnRemove(void);
	void			ToggleLights(bool bToggle);
	int				CurrentGoal(void);
	int				CurrentCheckpoint(void);
	bool			IsRunning(void);
	void			StartSiren(void);
	void			StopSiren(void);

	CBaseEntity		*GetBody(void) { if (m_pBody) return m_pBody->GetProp(); return NULL; }

	static Vector	wheelOffset[4];
	static QAngle	wheelOrientation[4];
	static int		wheelDirection[4];

private:
	void			APCThink(void);
	void			UpdateLightPosition(void);

	static float			m_flWheelDiameter;
	static float			m_flWheelRadius;
	static float			m_flWheelBase;
	static float			m_flWheelTrack;
	inline float			WheelDistanceTraveled(float degrees) { return degrees / 360.0f * M_PI * CCoven_APC::m_flWheelDiameter; };
	inline float			WheelDegreesTraveled(float distance) { return distance / (M_PI * CCoven_APC::m_flWheelDiameter) * 360.0f; };
	static WheelLocation_t	OppositeWheel(WheelLocation_t wheel);
	static WheelLocation_t	AdjacentWheel(WheelLocation_t wheel);

	int				m_iGoalNode;
	int				m_iGoalCheckpoint;
	QAngle			m_angTurningAngle;
	float			m_flTurningAngle;
	int				m_iFuel;
	int				m_iFuelUpAmount;
	bool			m_bPlayingSound;
	bool			m_bPlayingSiren;
	bool			m_bRunning;
	CCoven_APCPart	*m_pWheels[4];
	CCoven_APCPart	*m_pBody;
	float			m_flMaxSpeed;

	Vector			m_vecFrontAxelPosition;
	int				m_vecTick;
	float			m_flSoundSwitch;
	float			m_flSoundSirenSwitch;
	CHandle<CSprite>		m_hRightLampGlow;
	CHandle<CSprite>		m_hLeftLampGlow;
};

#endif