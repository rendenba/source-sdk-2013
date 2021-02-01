#ifndef C_SERVERRAGDOLL_H
#define C_SERVERRAGDOLL_H

#include "cbase.h"
#include "c_baseanimating.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class C_ServerRagdoll : public C_BaseAnimating
{
public:
	DECLARE_CLASS(C_ServerRagdoll, C_BaseAnimating);
	DECLARE_CLIENTCLASS();
	DECLARE_INTERPOLATION();

	C_ServerRagdoll(void);

	virtual void OnDataChanged(DataUpdateType_t updateType);
	virtual void PostDataUpdate(DataUpdateType_t updateType);

	virtual int InternalDrawModel(int flags);
	virtual CStudioHdr *OnNewModel(void);
	virtual unsigned char GetClientSideFade();
	virtual void	SetupWeights(const matrix3x4_t *pBoneToWorld, int nFlexWeightCount, float *pFlexWeights, float *pFlexDelayedWeights);

	void GetRenderBounds(Vector& theMins, Vector& theMaxs);
	virtual void AddEntity(void);
	virtual void AccumulateLayers(IBoneSetup &boneSetup, Vector pos[], Quaternion q[], float currentTime);
	virtual void BuildTransformations(CStudioHdr *pStudioHdr, Vector *pos, Quaternion q[], const matrix3x4_t &cameraTransform, int boneMask, CBoneBitList &boneComputed);
	IPhysicsObject *GetElement(int elementNum);
	virtual void UpdateOnRemove();
	virtual float LastBoneChangedTime();
	virtual bool IsServerdoll() const { return true; }

	// Incoming from network
	Vector		m_ragPos[RAGDOLL_MAX_ELEMENTS];
	QAngle		m_ragAngles[RAGDOLL_MAX_ELEMENTS];

	CInterpolatedVarArray< Vector, RAGDOLL_MAX_ELEMENTS >	m_iv_ragPos;
	CInterpolatedVarArray< QAngle, RAGDOLL_MAX_ELEMENTS >	m_iv_ragAngles;

	Vector		m_ragPosNet[RAGDOLL_MAX_ELEMENTS];

	int			m_elementCount;
	int			m_boneIndex[RAGDOLL_MAX_ELEMENTS];

	CNetworkVar(int, iSlot);

#ifdef GLOWS_ENABLE
	void		GetGlowEffectColor(float *r, float *g, float *b, float *a);
#endif

protected:
#ifdef GLOWS_ENABLE
	void		UpdateGlowEffect(void);
#endif

private:
	C_ServerRagdoll(const C_ServerRagdoll &src);

	typedef CHandle<C_BaseAnimating> CBaseAnimatingHandle;
	CNetworkVar(CBaseAnimatingHandle, m_hUnragdoll);
	CNetworkVar(float, m_flBlendWeight);
	float m_flBlendWeightCurrent;
	CNetworkVar(int, m_nOverlaySequence);
	float m_flLastBoneChangeTime;
};

#endif