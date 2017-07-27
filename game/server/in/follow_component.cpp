//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "bot.h"

#include "in_utils.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
// Pensamiento
//================================================================================
void CFollowComponent::OnUpdate( CBotCmd* &cmd )
{
    VPROF_BUDGET( "CFollowComponent::OnUpdate", VPROF_BUDGETGROUP_BOTS );

    if ( GetBot()->OptimizeThisFrame() )
        return;

    if ( !GetBot()->ShouldFollow() )
        return;

    CBaseEntity *pEntity = GetFollowing();

    if ( !pEntity || !pEntity->IsAlive() ) {
        Stop();
        return;
    }

    GetBot()->Navigation()->SetDestination( pEntity, PRIORITY_FOLLOWING );
}

//================================================================================
// Devuelve si estamos siguiendo a un Jugador
//================================================================================
bool CFollowComponent::IsFollowingPlayer()
{
    if ( !IsFollowingSomeone() )
        return false;

    return GetFollowing()->IsPlayer();
}

//================================================================================
// Establece el líder a quien seguir
//================================================================================
void CFollowComponent::Start( CBaseEntity *pEntity, bool bFollow )
{
    if ( pEntity ) {
        if ( !pEntity->IsAlive() )
            return;

        if ( pEntity == GetHost() ) {
            Stop();
            return;
        }
    }

    m_nFollowingEntity = pEntity;
    m_bFollow = bFollow;
}

//================================================================================
//================================================================================
void CFollowComponent::Start( const char *pEntityName, bool bFollow )
{
    CBaseEntity *pEntity = gEntList.FindEntityByName( NULL, pEntityName );

    if ( !pEntity )
        return;

    Start( pEntity, bFollow );
}

//================================================================================
//================================================================================
void CFollowComponent::Stop()
{
    m_nFollowingEntity = NULL;
    m_bFollow = false;
}