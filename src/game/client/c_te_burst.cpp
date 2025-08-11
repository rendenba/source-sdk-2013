#include "cbase.h"
#include "c_basetempentity.h"
#include "IEffects.h"
#include "tier0/vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Energy Splash TE
//-----------------------------------------------------------------------------
class C_TEBurst : public C_BaseTempEntity
{
public:
	DECLARE_CLIENTCLASS();

	C_TEBurst(void);
	virtual			~C_TEBurst(void);

	virtual void	PostDataUpdate(DataUpdateType_t updateType);

	virtual void	Precache(void);

public:
	Vector				m_vecPos;
	color32				m_iClr;
	CovenBurstType_t	m_type;
	EHANDLE				m_pFollowEnt;

	const struct model_t *m_pModel;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TEBurst::C_TEBurst(void)
{
	m_vecPos.Init();
	m_iClr = { 0, 0, 0, 0 };
	m_type = COVEN_BURST_TYPE_DEFAULT;
	m_pFollowEnt = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TEBurst::~C_TEBurst(void)
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TEBurst::Precache(void)
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bool - 
//-----------------------------------------------------------------------------
void C_TEBurst::PostDataUpdate(DataUpdateType_t updateType)
{
	VPROF("C_TEBurst::PostDataUpdate");

	g_pEffects->Burst(m_vecPos, m_iClr, m_type, m_pFollowEnt);
}

void TE_Burst(IRecipientFilter& filter, float delay, const Vector* pos, color32 clr, CovenBurstType_t type, CBaseEntity *pFollowEnt)
{
	g_pEffects->Burst(*pos, clr, type, pFollowEnt);
}

// Expose the TE to the engine.
IMPLEMENT_CLIENTCLASS_EVENT(C_TEBurst, DT_TEBurst, CTEBurst);

BEGIN_RECV_TABLE_NOBASE(C_TEBurst, DT_TEBurst)
	RecvPropVector(RECVINFO(m_vecPos)),
	RecvPropInt(RECVINFO(m_iClr)),
	RecvPropInt(RECVINFO(m_type)),
	RecvPropEHandle(RECVINFO(m_pFollowEnt)),
END_RECV_TABLE()

