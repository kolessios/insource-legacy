//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef IN_PLAYERANIMSTATE_PROXY_H
#define IN_PLAYERANIMSTATE_PROXY_H

#ifdef _WIN32
#pragma once
#endif

#ifdef CLIENT_DLL
    #include "c_basetempentity.h"

    #define CBaseTempEntity C_BaseTempEntity
    #define CPlayerAnimationSystemProxy C_PlayerAnimationSystemProxy
#else
    enum PlayerAnimEvent_t;
#endif

//================================================================================
// Crea una entidad temporal que le permite al servidor envíar una animación
// al cliente.
//================================================================================
class CPlayerAnimationSystemProxy : public CBaseTempEntity
{
    DECLARE_CLASS( CPlayerAnimationSystemProxy, CBaseTempEntity );
    DECLARE_NETWORKCLASS();

public:
#ifndef CLIENT_DLL
    CPlayerAnimationSystemProxy( const char *name ) : CBaseTempEntity( name )
    { }

    virtual void Init( CBasePlayer *, PlayerAnimEvent_t , int = 0 );
    virtual void SetPlayer( CBasePlayer *pPlayer ) { m_nPlayer = pPlayer; }
#else
    CPlayerAnimationSystemProxy();
    virtual void PostDataUpdate( DataUpdateType_t );
#endif

protected:
    CNetworkHandle( CBasePlayer, m_nPlayer );
    CNetworkVar( int, m_iEvent );
    CNetworkVar( int, m_nData );
};

#ifndef CLIENT_DLL
extern void SendPlayerAnimation( CBasePlayer *pPlayer, PlayerAnimEvent_t pEvent, int nData, bool bPredicted );
#endif

#endif // IN_PLAYERANIMSTATE_PROXY_H
