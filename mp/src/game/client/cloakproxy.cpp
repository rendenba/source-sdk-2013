#include "cbase.h"
#include "materialsystem/imaterialvar.h"
#include "materialsystem/imaterialproxy.h"
#include "baseviewmodel_shared.h"
 
class C_CloakProxy : public IMaterialProxy
{
public:
    C_CloakProxy();
    virtual ~C_CloakProxy();
    virtual bool Init( IMaterial *pMaterial, KeyValues *pKeyValues );
    C_BaseEntity *BindArgToEntity( void *pArg );
    virtual void OnBind( void* C_BaseEntity );
    virtual void Release( void ) { delete this; }
    IMaterial *GetMaterial( void );
 
private:
    IMaterialVar* cloakFactorVar;
};
 
C_CloakProxy::C_CloakProxy()
{
    cloakFactorVar = NULL;
}
 
C_CloakProxy::~C_CloakProxy()
{
}
 
bool C_CloakProxy::Init( IMaterial *pMaterial, KeyValues *pKeyValues )
{
    bool found;
 
    pMaterial->FindVar( "$cloakpassenabled", &found, false );
    if ( !found )
        return false;
 
    cloakFactorVar = pMaterial->FindVar( "$cloakfactor", &found, false );
    if ( !found )
        return false;
 
    return true;
}
 
C_BaseEntity *C_CloakProxy::BindArgToEntity( void *pArg )
{
    IClientRenderable *pRend = (IClientRenderable *)pArg;
    return pRend ? pRend->GetIClientUnknown()->GetBaseEntity() : NULL;
}
 
void C_CloakProxy::OnBind( void* pC_BaseEntity )
{
    if ( !pC_BaseEntity )
        return;
 
    C_BaseEntity *pEntity = BindArgToEntity( pC_BaseEntity );
 
    //If this is a player's viewmodel...
    if ( C_BaseViewModel *pViewModel = dynamic_cast< C_BaseViewModel *>(pEntity) )
    {
        C_BasePlayer *pPlayer = ToBasePlayer( pViewModel->GetOwner() );
        cloakFactorVar->SetFloatValue( pPlayer->m_floatCloakFactor );
    }
 
    //If this is a non-player character...
    else if ( C_BaseCombatCharacter *pNPC = dynamic_cast< C_BaseCombatCharacter *>(pEntity) )
    {
        cloakFactorVar->SetFloatValue( pNPC->m_floatCloakFactor );
    }
 
    //If this is a weapon's worldmodel (under the assumption it's in something's possesion)...
    else if ( C_BaseCombatWeapon *pWeapon = dynamic_cast< C_BaseCombatWeapon *>(pEntity) )
    {
        C_BaseCombatCharacter *pOwner = ToBaseCombatCharacter( pWeapon->GetOwner() );
        if ( !pOwner )
            return;
 
        cloakFactorVar->SetFloatValue( pOwner->m_floatCloakFactor );
    }
    else
        return;
}
IMaterial *C_CloakProxy::GetMaterial()
{
    return cloakFactorVar->GetOwningMaterial();
}
EXPOSE_INTERFACE( C_CloakProxy, IMaterialProxy, "CovenCloak" IMATERIAL_PROXY_INTERFACE_VERSION );