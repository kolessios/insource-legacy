//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#include "cbase.h"
#include "bot.h"

#include "in_utils.h"
#include "in_player.h"
#include "bot_manager.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
//================================================================================
void CBotMemory::Update()
{
    VPROF_BUDGET( "CBotMemory::Update", VPROF_BUDGETGROUP_BOTS );

    if ( !IsEnabled() )
        return;

    UpdateMemory();
    UpdateIdealThreat();
    UpdateThreat();
}

//================================================================================
// Update memory, get information about enemies and friends.
//================================================================================
void CBotMemory::UpdateMemory()
{
    //m_iNearbyThreats = 0;
    //m_iNearbyFriends = 0;
    //m_iNearbyDangerousThreats = 0;

    int nearbyThreats = 0;
    int nearbyFriends = 0;
    int nearbyDangerousThreats = 0;

    UpdateDataMemory( "NearbyThreats", 0 );
    UpdateDataMemory( "NearbyFriends", 0 );
    UpdateDataMemory( "NearbyDangerousThreats", 0 );

    m_Threats.Purge();
    m_Friends.Purge();

    FOR_EACH_MAP_FAST( m_DataMemory, it )
    {
        CDataMemory *memory = m_DataMemory[it];
        Assert( memory );

        if ( !memory )
            continue;

        if ( memory->IsExpired() ) {
            m_DataMemory.RemoveAt( it );
            --it;
            continue;
        }
    }

    FOR_EACH_MAP_FAST( m_Memory, it )
    {
        CEntityMemory *memory = m_Memory[it];
        Assert( memory );

        if ( !memory )
            continue;

        // The entity has been deleted.
        if ( !memory->GetEntity() ) {
            m_Memory.RemoveAt( it );
            --it;
            continue;
        }

        // Reset visibility status
        memory->UpdateVisibility( false );

        CBaseEntity *pEntity = memory->GetEntity();
        int relationship = memory->GetRelationship();
        bool isThreat = relationship == GR_ENEMY && GetDecision()->CanBeEnemy( pEntity );

        if ( isThreat ) {
            m_Threats.AddToTail( memory );
        }
        else if ( relationship == GR_ALLY ) {
            m_Friends.AddToTail( memory );
        }

        // The last known position of this entity is close to us.
        // We mark how many allied/enemy entities are close to us to make better decisions.
        if ( memory->IsInRange( m_flNearbyDistance ) ) {
            if ( isThreat ) {
                if ( GetDecision()->IsDangerousEnemy( pEntity ) ) {
                    ++nearbyDangerousThreats;
                }
             
                ++nearbyThreats;
            }
            else if ( relationship == GR_ALLY ) {
                ++nearbyFriends;
            }
        }
    }

    // We see, we smell, we feel
    if ( GetHost()->GetSenses() ) {
        GetHost()->GetSenses()->PerformSensing();
    }

    UpdateDataMemory( "NearbyThreats", nearbyThreats );
    UpdateDataMemory( "NearbyFriends", nearbyFriends );
    UpdateDataMemory( "NearbyDangerousThreats", nearbyDangerousThreats );
}

//================================================================================
// Update who should be our main threat.
//================================================================================
void CBotMemory::UpdateIdealThreat()
{
    CEntityMemory *pIdeal = NULL;

    FOR_EACH_VEC( m_Threats, it )
    {
        CEntityMemory *memory = m_Memory[it];
        Assert( memory );

        if ( !memory->IsVisible() )
            continue;

        CBaseEntity *pEnt = memory->GetEntity();

        if ( !pIdeal || GetDecision()->IsBetterEnemy( pEnt, pIdeal->GetEntity() ) ) {
            pIdeal = memory;
        }
    }

    m_pIdealThreat = pIdeal;
}

//================================================================================
// Update who will be our primary threat.
//================================================================================
void CBotMemory::UpdateThreat()
{
    CEntityMemory *threat = GetPrimaryThreat();
    CEntityMemory *ideal = GetIdealThreat();

    // We do not have a primary threat, 
    // if we have an ideal threat then we use it as the primary,
    // otherwise there is no enemy.
    if ( !threat ) {
        if ( ideal ) {
            threat = m_pPrimaryThreat = ideal;
        }
        else {
            return;
        }
    }
    else {
        // We already have a primary threat but it is low priority, 
        // the ideal threat is better.
        if ( ideal && threat != ideal ) {
            if ( GetDecision()->IsEnemyLowPriority() ) {
                threat = m_pPrimaryThreat = ideal;
            }
        }
    }

    // We update the location of hitboxes
    threat->UpdateHitboxAndVisibility();
}

//================================================================================
// It maintains the current primary threat, preventing it from being forgotten.
//================================================================================
void CBotMemory::MaintainThreat()
{
    CEntityMemory *memory = GetPrimaryThreat();

    if ( !memory ) {
        memory->Maintain();
    }
}

//================================================================================
//================================================================================
float CBotMemory::GetPrimaryThreatDistance() const
{
    CEntityMemory *memory = GetPrimaryThreat();

    if ( !memory )
        return -1.0f;

    return memory->GetDistance();
}

//================================================================================
//================================================================================
void CBotMemory::SetEnemy( CBaseEntity * pEnt, bool bUpdate )
{
    if ( !pEnt ) {
        m_pPrimaryThreat = NULL;
        return;
    }

    // What?
    if ( pEnt == GetHost() )
        return;

    if ( !GetDecision()->CanBeEnemy( pEnt ) )
        return;

    CEntityMemory *memory = GetEntityMemory( pEnt );

    if ( bUpdate || !memory ) {
        memory = UpdateEntityMemory( pEnt, pEnt->GetAbsOrigin() );
    }

    if ( m_pPrimaryThreat == memory )
        return;

    m_pPrimaryThreat = memory;
    SetCondition( BCOND_NEW_ENEMY );
}

//================================================================================
//================================================================================
CEntityMemory * CBotMemory::UpdateEntityMemory( CBaseEntity * pEnt, const Vector & vecPosition, CBaseEntity * pInformer )
{
    VPROF_BUDGET( "CBotMemory::UpdateEntityMemory", VPROF_BUDGETGROUP_BOTS );

    if ( !pEnt )
        return NULL;

    if ( !IsEnabled() )
        return NULL;

    CEntityMemory *memory = GetEntityMemory( pEnt );

    if ( memory ) {
        // We avoid updating the memory several times in the same frame.
        if ( memory->GetFrameLastUpdate() == gpGlobals->absoluteframetime ) {
            return memory;
        }

        // Someone else is informing us about this entity.
        if ( pInformer ) {
            // We have vision about it, it does not take that someone else to inform me.
            if ( memory->IsVisible() ) {
                return memory;
            }

            // Optimization: If you are informing us, 
            // we will avoid updating until 2 seconds after the last report.
            if ( memory->IsUpdatedRecently( 2.0f ) ) {
                return memory;
            }
        }
    }

    // I have seen this entity with my own eyes, 
    // we must communicate it to our squad.
    if ( !pInformer && GetBot()->GetSquad() ) {
        GetBot()->GetSquad()->ReportEnemy( GetHost(), pEnt );
    }

    if ( !memory ) {
        memory = new CEntityMemory( GetBot(), pEnt, pInformer );
        m_Memory.Insert( pEnt->entindex(), memory );
    }

    memory->UpdatePosition( vecPosition );
    memory->SetInformer( pInformer );
    memory->MarkLastFrame();

    return memory;
}

//================================================================================
//================================================================================
void CBotMemory::ForgetEntity( CBaseEntity * pEnt )
{
    CEntityMemory *memory = GetEntityMemory( pEnt );

    if ( !memory )
        return;

    m_Memory.Remove( pEnt->entindex() );
}

//================================================================================
//================================================================================
void CBotMemory::ForgetAllEntities()
{
    Reset();
}

//================================================================================
//================================================================================
CEntityMemory * CBotMemory::GetEntityMemory( CBaseEntity * pEnt ) const
{
    if ( !pEnt ) {
        if ( !GetPrimaryThreat() )
            return NULL;

        pEnt = GetPrimaryThreat()->GetEntity();
    }

    Assert( pEnt );

    if ( !pEnt )
        return NULL;

    return GetEntityMemory( pEnt->entindex() );
}

//================================================================================
//================================================================================
CEntityMemory * CBotMemory::GetEntityMemory( int index ) const
{
    if ( !m_Memory.IsValidIndex( index ) )
        return NULL;

    return m_Memory.Element( index );
}

//================================================================================
//================================================================================
float CBotMemory::GetMemoryDuration() const
{
    return GetSkill()->GetMemoryDuration();
}

//================================================================================
//================================================================================
CEntityMemory * CBotMemory::GetClosestThreat( float *distance ) const
{
    float closest = MAX_TRACE_LENGTH;
    CEntityMemory *closestMemory = NULL;

    FOR_EACH_VEC( m_Threats, it )
    {
        CEntityMemory *memory = m_Threats[it];
        Assert( memory );

        float distance = memory->GetDistance();

        if ( distance < closest ) {
            closest = distance;
            closestMemory = memory;
        }
    }

    if ( distance ) {
        distance = &closest;
    }

    return closestMemory;
}

//================================================================================
//================================================================================
int CBotMemory::GetThreatCount( float range ) const
{
    int count = 0;
    
    FOR_EACH_VEC( m_Threats, it )
    {
        CEntityMemory *memory = m_Threats[it];
        Assert( memory );

        float distance = memory->GetDistance();

        if ( distance > range )
            continue;

        ++count;
    }

    return count;
}

//================================================================================
//================================================================================
CEntityMemory * CBotMemory::GetClosestFriend( float *distance ) const
{
    float closest = MAX_TRACE_LENGTH;
    CEntityMemory *closestMemory = NULL;

    FOR_EACH_VEC( m_Friends, it )
    {
        CEntityMemory *memory = m_Friends[it];
        Assert( memory );

        float distance = memory->GetDistance();

        if ( distance < closest ) {
            closest = distance;
            closestMemory = memory;
        }
    }

    if ( distance ) {
        distance = &closest;
    }

    return closestMemory;
}

//================================================================================
//================================================================================
int CBotMemory::GetFriendCount( float range ) const
{
    int count = 0;

    FOR_EACH_VEC( m_Friends, it )
    {
        CEntityMemory *memory = m_Friends[it];
        Assert( memory );

        float distance = memory->GetDistance();

        if ( distance > range )
            continue;

        ++count;
    }

    return count;
}

//================================================================================
//================================================================================
CEntityMemory * CBotMemory::GetClosestKnown( int teamnum, float *distance ) const
{
    float closest = MAX_TRACE_LENGTH;
    CEntityMemory *closestMemory = NULL;

    FOR_EACH_MAP_FAST( m_Memory, it )
    {
        CEntityMemory *memory = m_Memory[it];
        Assert( memory );

        if ( memory->GetEntity()->GetTeamNumber() != teamnum )
            continue;

        float distance = memory->GetDistance();

        if ( distance < closest ) {
            closest = distance;
            closestMemory = memory;
        }
    }

    if ( distance ) {
        distance = &closest;
    }

    return closestMemory;
}

//================================================================================
//================================================================================
int CBotMemory::GetKnownCount( int teamnum, float range ) const
{
    int count = 0;

    FOR_EACH_MAP_FAST( m_Memory, it )
    {
        CEntityMemory *memory = m_Memory[it];
        Assert( memory );

        if ( memory->GetEntity()->GetTeamNumber() != teamnum )
            continue;

        float distance = memory->GetDistance();

        if ( distance > range )
            continue;

        ++count;
    }

    return count;
}

//================================================================================
//================================================================================
float CBotMemory::GetTimeSinceVisible( int teamnum ) const
{
    float closest = -1.0f;

    FOR_EACH_MAP_FAST( m_Memory, it )
    {
        CEntityMemory *memory = m_Memory[it];
        Assert( memory );

        if ( memory->GetEntity()->GetTeamNumber() != teamnum )
            continue;

        float time = memory->GetTimeLastVisible();

        if ( time > closest ) {
            closest = time;
        }
    }

    return closest;
}

//================================================================================
//================================================================================
CDataMemory * CBotMemory::UpdateDataMemory( const char * name, const Vector & value, float forgetTime )
{
    CDataMemory *memory = GetDataMemory( name );

    if ( memory ) {
        memory->SetVector( value );
    }
    else {
        memory = new CDataMemory( value );
        m_DataMemory.Insert( AllocPooledString(name), memory );
    }

    memory->ForgetIn( forgetTime );
    return memory;
}

//================================================================================
//================================================================================
CDataMemory * CBotMemory::UpdateDataMemory( const char * name, float value, float forgetTime )
{
    CDataMemory *memory = GetDataMemory( name );

    if ( memory ) {
        memory->SetFloat( value );
    }
    else {
        memory = new CDataMemory( value );
        m_DataMemory.Insert( AllocPooledString( name ), memory );
    }

    memory->ForgetIn( forgetTime );
    return memory;
}

//================================================================================
//================================================================================
CDataMemory * CBotMemory::UpdateDataMemory( const char * name, int value, float forgetTime )
{
    CDataMemory *memory = GetDataMemory( name );

    if ( memory ) {
        memory->SetInt( value );
    }
    else {
        memory = new CDataMemory( value );
        m_DataMemory.Insert( AllocPooledString( name ), memory );
    }

    memory->ForgetIn( forgetTime );
    return memory;
}

//================================================================================
//================================================================================
CDataMemory * CBotMemory::UpdateDataMemory( const char * name, const char * value, float forgetTime )
{
    CDataMemory *memory = GetDataMemory( name );

    if ( memory ) {
        memory->SetString( value );
    }
    else {
        memory = new CDataMemory( value );
        m_DataMemory.Insert( AllocPooledString( name ), memory );
    }

    memory->ForgetIn( forgetTime );
    return memory;
}

//================================================================================
//================================================================================
CDataMemory * CBotMemory::UpdateDataMemory( const char * name, CBaseEntity * value, float forgetTime )
{
    CDataMemory *memory = GetDataMemory( name );

    if ( memory ) {
        memory->SetEntity( value );
    }
    else {
        memory = new CDataMemory( value );
        m_DataMemory.Insert( AllocPooledString( name ), memory );
    }

    memory->ForgetIn( forgetTime );
    return memory;
}

//================================================================================
//================================================================================
CDataMemory * CBotMemory::AddDataMemoryList( const char * name, CDataMemory * value, float forgetTime )
{
    CDataMemory *memory = GetDataMemory( name );

    if ( !memory ) {
        memory = new CDataMemory();
        memory->ForgetIn( forgetTime );

        m_DataMemory.Insert( AllocPooledString( name ), memory );
    }

    memory->Add( value );
    return memory;
}

//================================================================================
//================================================================================
CDataMemory * CBotMemory::RemoveDataMemoryList( const char * name, CDataMemory * value, float forgetTime )
{
    CDataMemory *memory = GetDataMemory( name );

    if ( !memory ) {
        return NULL;
    }

    memory->Remove( value );
    return memory;
}

//================================================================================
//================================================================================
CDataMemory * CBotMemory::GetDataMemory( const char * name, bool forceIfNotExists ) const
{
    string_t szName = AllocPooledString( name );
    int index = m_DataMemory.Find( szName );

    if ( !m_DataMemory.IsValidIndex( index ) ) {
        if ( forceIfNotExists ) {
            return new CDataMemory();
        }
        else {
            return NULL;
        }
    }

    return m_DataMemory.Element( index );
}

//================================================================================
//================================================================================
void CBotMemory::ForgetData( const char * name )
{
    string_t szName = AllocPooledString( name );
    m_DataMemory.Remove( szName );
}

//================================================================================
//================================================================================ 
void CBotMemory::ForgetAllData()
{
    m_DataMemory.Purge();
}