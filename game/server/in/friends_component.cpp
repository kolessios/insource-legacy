//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "bot.h"

#include "in_utils.h"
#include "in_player.h"
#include "bot_manager.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
// Constructor
//================================================================================
CFriendsComponent::CFriendsComponent()
{
    SetDefLessFunc( m_nEnemyMemory );
}

//================================================================================
//================================================================================
void CFriendsComponent::OnSpawn()
{
    m_nEnemy = NULL;
    m_nEnemyMemory.Purge();

    m_iNearbyEnemies = 0;
    m_iNearbyFriends = 0;
    m_bEnabled = true;
}

//================================================================================
//================================================================================
void CFriendsComponent::OnUpdate( CBotCmd* &cmd )
{
    VPROF_BUDGET( "CFriendsComponent::OnUpdate", VPROF_BUDGETGROUP_BOTS );

    if ( !m_bEnabled )
        return;

    if ( GetBot()->OptimizeThisFrame() )
        return;

    UpdateMemory();

    m_nIdealEnemy = SelectIdealEnemy();

    UpdateEnemy();
    UpdateNewEnemy();
}

//================================================================================
// Actualiza la memoria del bot
// Elimina los enemigos que son inválidos
//================================================================================
void CFriendsComponent::UpdateMemory()
{
    int enemies = 0;
    int nearby = 0;

    FOR_EACH_MAP_FAST( m_nEnemyMemory, it )
    {
        CEnemyMemory *memory = m_nEnemyMemory.Element( it );

        if ( !memory )
            break;

        if ( !memory->GetEnemy() ) {
            m_nEnemyMemory.RemoveAt( it );
            continue;
        }

        if ( memory->HasExpired() ) {
            m_nEnemyMemory.RemoveAt( it );
            continue;
        }

        CBaseEntity *pEnemy = memory->GetEnemy();

        // Es un jugador en modo espectador, debemos olvidarlo
        if ( pEnemy->IsPlayer() && ToInPlayer( pEnemy )->IsObserver() ) {
            m_nEnemyMemory.RemoveAt( it );
            continue;
        }

        // Ha muerto
        if ( !pEnemy->IsAlive() ) {
            if ( GetSkill()->IsEasy() ) {
                // Esperamos a que este completamente muerto para marcarlo como muerto
                // Es decir, que seguimos mirandolo/atacandolo mientras hace su animación
                // de muerte...
                if ( pEnemy->m_lifeState == LIFE_DEAD )
                    m_nEnemyMemory.RemoveAt( it );
            }
            else {
                m_nEnemyMemory.RemoveAt( it );
            }

            continue;
        }

        // Si este enemigo es considerado peligroso y estamos a una distancia considerable de el
        // lo marcamos como "enemigo cercano", esto nos servirá para tomar una mejor decisión al
        // perseguir a alguién.
        if ( GetBot()->IsDangerousEnemy( pEnemy ) ) {
            if ( memory->GetLastPosition().DistTo( GetAbsOrigin() ) <= 1000.0f )
                ++nearby;
        }

        ++enemies;
    }

    m_iEnemies = enemies;
    m_iNearbyEnemies = nearby;
}

//================================================================================
//================================================================================
void CFriendsComponent::Stop()
{
    m_bEnabled = false;
    m_nEnemyMemory.Purge();
    SetEnemy( NULL );
}

//================================================================================
//================================================================================
void CFriendsComponent::Resume()
{
    m_bEnabled = true;
}

//================================================================================
// Actualiza nuestro enemigo actual
//================================================================================
void CFriendsComponent::UpdateEnemy()
{
    if ( !GetEnemy() )
        return;

    CEnemyMemory *memory = GetEnemyMemory();

    if ( !memory ) {
        SetEnemy( NULL );
        return;
    }

    if ( GetIdealEnemy() ) {
        if ( GetIdealEnemy() != GetEnemy() ) {
            if ( GetBot()->IsEnemyLowPriority() ) {
                SetEnemy( NULL );
                return;
            }
        }
    }

    // Actualizamos la memoria de las partes visibles del cuerpo
    HitboxPositions positions;
    Utils::GetHitboxPositions( GetEnemy(), positions );
    memory->SetBodyPositions( positions );

    LookAtEnemy();

    if ( !IsCombating() )
        Alert();
}

//================================================================================
// Actualiza el Bot con un nuevo enemigo
//================================================================================
void CFriendsComponent::UpdateNewEnemy()
{
    // Ya tenemos un enemigo
    if ( GetEnemy() )
        return;

    // Ningún enemigo ideal por ahora...
    if ( !GetIdealEnemy() )
        return;

    SetEnemy( GetIdealEnemy() );
}

//================================================================================
// Mantiene el enemigo actual, recordando su última posición
//================================================================================
void CFriendsComponent::MaintainEnemy()
{
    // No tenemos enemigo
    if ( !GetEnemy() )
        return;

    CEnemyMemory *memory = GetEnemyMemory();
    Assert( memory );

    memory->SetLastPosition( memory->GetLastPosition() );

    // Si queda 1s, reiniciamos
    if ( memory->ExpireTimer().GetRemainingTime() <= 1.0f )
        memory->SetDuration( GetSkill()->GetMemoryDuration() );
}

//================================================================================
// Apunta hacia una posición del cuerpo del enemigo o su última posición conocida
//================================================================================
void CFriendsComponent::LookAtEnemy()
{
    if ( !GetBot()->Aim() )
        return;

    if ( !GetBot()->ShouldLookEnemy() ) {
        if ( GetBot()->GetLookTarget() == GetEnemy() )
            GetBot()->ClearLook();

        if ( GetBot()->ShouldLookInterestingSpot() )
            GetBot()->Aim()->LookInterestingSpot();

        return;
    }

    CEnemyMemory *memory = GetEnemyMemory();
    Assert( memory );

    if ( GetBot()->IsEnemyLost() ) {
        // Apuntamos a la última posición conocida
        GetBot()->LookAt( "Last Enemy Position", GetEnemy(), memory->GetLastPosition(), PRIORITY_HIGH, 0.5f );
    }
    else {
        // Obtenemos la posición del hitbox de preferencia o cualquier otro visible
        // @TODO: Que pasa con los enemigos sin Hitbox
        Vector vecLookAt;
        bool result = memory->GetVisibleHitboxPosition( GetHost(), vecLookAt, GetSkill()->GetFavoriteHitbox() );

        // No podemos ninguna parte del cuerpo de nuestro enemigo.
        if ( !result ) {
            GetBot()->LookAt( "NO VISIBLE HITBOX!!", GetEnemy(), memory->GetLastPosition(), PRIORITY_HIGH, 0.5f );
            return;
        }

        // TODO: Según el arma
        //vecLookAt.z -= 5.0f;

        // Establecemos la posición de la parte del cuerpo
        // como la última posición conocida del enemigo
        memory->SetLastPosition( vecLookAt );

        // Si no somos pro, agregamos un margen de error
        if ( GetSkill()->GetLevel() < SKILL_HARDEST ) {
            float errorRange = 0.0f;

            if ( GetSkill()->GetLevel() >= SKILL_HARD )
                errorRange = RandomFloat( 0.0f, 5.0f );
            if ( GetSkill()->IsMedium() )
                errorRange = RandomFloat( 1.0f, 10.0f );
            else
                errorRange = RandomFloat( 5.0f, 15.0f );

            vecLookAt.x += RandomFloat( -errorRange, errorRange );
            vecLookAt.y += RandomFloat( -errorRange, errorRange );
            vecLookAt.z += RandomFloat( -errorRange, errorRange );
        }
        else {
            vecLookAt.z += RandomFloat( -6.0f, 6.0f );
        }

        // Apuntamos
        GetBot()->LookAt( "Enemy Body Part", GetEnemy(), vecLookAt, PRIORITY_HIGH, 0.5f );
    }
}

//================================================================================
// Devuelve la distancia al enemigo actual
//================================================================================
float CFriendsComponent::GetEnemyDistance()
{
    if ( !GetEnemy() )
        return -1.0f;

    CEnemyMemory *memory = GetEnemyMemory();

    if ( memory )
        return GetAbsOrigin().DistTo( memory->GetLastPosition() );

    return GetAbsOrigin().DistTo( GetEnemy()->GetAbsOrigin() );
}

//================================================================================
// Devuelve el enemigo ideal ahora mismo
//================================================================================
CBaseEntity *CFriendsComponent::SelectIdealEnemy()
{
    CBaseEntity *pIdealEnemy = NULL;

    FOR_EACH_MAP_FAST( m_nEnemyMemory, it )
    {
        CEnemyMemory *memory = GetEnemyMemory( it );
        Assert( memory );

        // El enemigo
        CBaseEntity *pSightEnt = memory->GetEnemy();

        // Relación que tenemos con el
        Disposition_t relation = GetHost()->IRelationType( pSightEnt );

        // Solo enemigos
        if ( relation != D_HT && relation != D_FR || !GetBot()->CanBeEnemy( pSightEnt ) )
            continue;

        // Este es mejor enemigo
        if ( GetBot()->IsBetterEnemy( pSightEnt, pIdealEnemy ) )
            pIdealEnemy = pSightEnt;
    }

    return pIdealEnemy;
}

//================================================================================
// Devuelve el enemigo actual
//================================================================================
CBaseEntity *CFriendsComponent::GetEnemy()
{
    return m_nEnemy.Get();
}

//================================================================================
// Establece el enemigo actual
//================================================================================
void CFriendsComponent::SetEnemy( CBaseEntity *pEnemy, bool bUpdate )
{
    // Ya no tenemos enemigo
    if ( !pEnemy ) {
        m_nEnemy = pEnemy;
        return;
    }

    // What?
    if ( pEnemy == GetHost() )
        return;

    // No puede ser mi enemigo
    if ( !GetBot()->CanBeEnemy( pEnemy ) )
        return;

    // Actualizamos la memoria
    if ( bUpdate )
        UpdateEnemyMemory( pEnemy, pEnemy->WorldSpaceCenter() );

    // Ya tengo un enemigo
    if ( GetEnemy() ) {
        // Ya es nuestro enemigo
        if ( GetEnemy() == pEnemy )
            return;
    }

    // Tenemos un nuevo enemigo
    m_nEnemy = pEnemy;
    SetCondition( BCOND_NEW_ENEMY );
}

//================================================================================
//================================================================================
CEnemyMemory *CFriendsComponent::GetEnemyMemory( int index )
{
    // No lo tenemos en nuestra memoria
    if ( !m_nEnemyMemory.IsValidIndex( index ) )
        return NULL;

    // Obtenemos la memoria
    return m_nEnemyMemory.Element( index );
}

//================================================================================
// Devuelve la memoria que se tiene sobre un enemigo
//================================================================================
CEnemyMemory *CFriendsComponent::GetEnemyMemory( CBaseEntity *pEnemy )
{
    if ( !pEnemy )
        pEnemy = GetEnemy();

    // Inválido
    if ( !pEnemy )
        return NULL;

    int index = m_nEnemyMemory.Find( pEnemy->entindex() );
    return GetEnemyMemory( index );
}

//================================================================================
// Actualiza la posición y partes visibles de una entidad durante un tiempo
//================================================================================
CEnemyMemory *CFriendsComponent::UpdateEnemyMemory( CBaseEntity *pEnemy, const Vector vecLocation, float duration, CBaseEntity *reported )
{
    VPROF_BUDGET( "CFriendsComponent::UpdateEnemyMemory", VPROF_BUDGETGROUP_BOTS );

    if ( !pEnemy )
        return NULL;

    if ( !m_bEnabled )
        return NULL;

    CEnemyMemory *memory = GetEnemyMemory( pEnemy );

    if ( memory ) {
        // Evitamos actualizar varias veces en un mismo frame.
        if ( memory->GetFrame() == gpGlobals->framecount )
            return memory;

        if ( reported ) {
            // Nosotros ya estamos viendo esa posición, no hace falta.
            if ( memory->IsLastPositionVisible( GetHost() ) )
                return memory;

            // Me lo dijiste hace menos de 2s, calma.
            if ( memory->ExpireTimer().GetElapsedTime() < 2.0f )
                return memory;
        }
    }

    // He visto este enemigo con mis propios ojos
    // Atención escuadron: Posición del enemigo...
    if ( !reported && GetBot()->GetSquad() )
        GetBot()->GetSquad()->ReportEnemy( GetHost(), pEnemy );

    // Duración por dificultad
    if ( duration < 0 ) {
        if ( IsPanicked() )
            duration = GetSkill()->GetMemoryDuration() + GetStateDuration();
        else
            duration = GetSkill()->GetMemoryDuration();
    }

    if ( memory ) {
        memory->SetLastPosition( vecLocation );
        memory->SetDuration( duration );
        memory->SetFrame( gpGlobals->absoluteframetime );
        memory->SetReportedBy( reported );
    }
    else {
        // Actualizamos la memoria de las partes visibles del cuerpo al menos una vez.
        HitboxPositions positions;
        Utils::GetHitboxPositions( pEnemy, positions );

        memory = new CEnemyMemory( pEnemy, vecLocation, duration, gpGlobals->framecount, positions );
        memory->SetReportedBy( reported );

        m_nEnemyMemory.InsertOrReplace( pEnemy->entindex(), memory );
    }


    return memory;
}
