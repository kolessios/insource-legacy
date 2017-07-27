//==== InfoSmart 2015. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef IN_RAGDOLL_H
#define IN_RAGDOLL_H

#pragma once

#ifdef CLIENT_DLL
#define CInRagdoll C_InRagdoll
#endif

class CPlayer;

class CInRagdoll : public CBaseAnimatingOverlay
{
    DECLARE_CLASS( CInRagdoll, CBaseAnimatingOverlay );
    DECLARE_NETWORKCLASS();
public:

#ifndef CLIENT_DLL
    virtual void Init( CPlayer *pPlayer );
    virtual int UpdateTransmitState();
#else
#endif

public:
    CNetworkHandle( CBaseEntity, m_nPlayer );
    CNetworkVector( m_vecRagdollVelocity );
    CNetworkVector( m_vecRagdollOrigin );
};

#endif // IN_RAGDOLL_H
