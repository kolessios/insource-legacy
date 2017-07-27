//==== InfoSmart 2015. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "in_ragdoll.h"

#ifndef CLIENT_DLL
#include "in_player.h"
#else
#include "c_in_player.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
// Información y Red
//================================================================================

LINK_ENTITY_TO_CLASS( in_ragdoll, CInRagdoll );

IMPLEMENT_NETWORKCLASS_ALIASED( InRagdoll, DT_InRagdoll );

BEGIN_NETWORK_TABLE_NOBASE( CInRagdoll, DT_InRagdoll )
#ifndef CLIENT_DLL
    SendPropVector( SENDINFO(m_vecRagdollOrigin), -1,  SPROP_COORD ),
    SendPropEHandle( SENDINFO( m_nPlayer ) ),
    SendPropModelIndex( SENDINFO( m_nModelIndex ) ),
    SendPropInt    ( SENDINFO(m_nForceBone), 8, 0 ),
    SendPropVector( SENDINFO(m_vecForce), -1, SPROP_NOSCALE ),
    SendPropVector( SENDINFO( m_vecRagdollVelocity ) )
#else
    RecvPropVector( RECVINFO(m_vecRagdollOrigin) ),
    RecvPropEHandle( RECVINFO( m_nPlayer ) ),
    RecvPropInt( RECVINFO( m_nModelIndex ) ),
    RecvPropInt    ( RECVINFO(m_nForceBone) ),
    RecvPropVector( RECVINFO(m_vecForce) ),
    RecvPropVector( RECVINFO( m_vecRagdollVelocity ) )
#endif
END_NETWORK_TABLE()

#ifndef CLIENT_DLL

//================================================================================
//================================================================================
void CInRagdoll::Init( CBaseInPlayer *pPlayer )
{
    m_nPlayer                = pPlayer;
    m_vecRagdollOrigin        = pPlayer->GetAbsOrigin();
    m_vecRagdollVelocity    = pPlayer->GetAbsVelocity();
    m_nModelIndex            = pPlayer->GetModelIndex();
    m_nForceBone            = pPlayer->m_nForceBone;
    m_vecForce                = Vector( 0, 0, 0 );

    SetAbsOrigin( pPlayer->GetAbsOrigin() );
}

//================================================================================
//================================================================================
int CInRagdoll::UpdateTransmitState()
{
    return SetTransmitState( FL_EDICT_ALWAYS );
}

#else

#endif