#include "cbase.h"
#include "te_particlesystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Dispatches energy splashes
//-----------------------------------------------------------------------------
class CTEBurst : public CBaseTempEntity
{
	DECLARE_CLASS(CTEBurst, CBaseTempEntity);

public:
	CTEBurst(const char *name);
	virtual			~CTEBurst(void);

	virtual void	Test(const Vector& current_origin, const QAngle& current_angles);

	DECLARE_SERVERCLASS();

public:
	CNetworkVector(m_vecPos);
	CNetworkColor32(m_iClr);
	CNetworkHandle(CBaseEntity, m_pFollowEnt);
	CNetworkVar(CovenBurstType_t, m_type);
};

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
CTEBurst::CTEBurst(const char *name) :
CBaseTempEntity(name)
{
	m_vecPos.Init();
	m_iClr.Init(0, 0, 0);
	m_type = 0;
	m_pFollowEnt = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTEBurst::~CTEBurst(void)
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *current_origin - 
//			*current_angles - 
//-----------------------------------------------------------------------------
void CTEBurst::Test(const Vector& current_origin, const QAngle& current_angles)
{
	// Fill in data
	m_vecPos = current_origin;

	m_vecPos.GetForModify()[2] += 24;

	CBroadcastRecipientFilter filter;
	Create(filter, 0.0);
}

IMPLEMENT_SERVERCLASS_ST_NOBASE(CTEBurst, DT_TEBurst)
	SendPropVector(SENDINFO(m_vecPos), -1, SPROP_COORD),
	SendPropInt(SENDINFO(m_iClr), -1, SPROP_UNSIGNED),
	SendPropInt(SENDINFO(m_type), 4, SPROP_UNSIGNED),
	SendPropEHandle(SENDINFO(m_pFollowEnt)),
END_SEND_TABLE()

// Singleton to fire TEEnergySplash objects
static CTEBurst g_TEBurst("Energy Burst");

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : msg_dest - 
//			delay - 
//			*origin - 
//			*recipient - 
//			*pos - 
//			scale - 
//-----------------------------------------------------------------------------
void TE_Burst(IRecipientFilter& filter, float delay, const Vector* pos, color32 color, CovenBurstType_t type, CBaseEntity *pFollowEnt)
{
	g_TEBurst.m_vecPos = *pos;
	g_TEBurst.m_iClr = color;
	g_TEBurst.m_type = type;
	g_TEBurst.m_pFollowEnt = pFollowEnt;

	// Send it over the wire
	g_TEBurst.Create(filter, delay);
}
