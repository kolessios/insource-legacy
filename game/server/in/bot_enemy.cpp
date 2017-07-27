//==== Woots 2017. http://creativecommons.org/licenses/by/2.5/mx/ ===========//
//
// Funciones de acceso rápido y personalización sobre el sistema de relaciones
//
//=============================================================================//
#include "cbase.h"

#include "bot.h"
#include "in_utils.h"

#include "in_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar bot_primary_attack;

//================================================================================
//================================================================================
void CBot::SetPeaceful( bool enabled )
{
    if ( !Friends() )
        return;

    if ( enabled )
        Friends()->Stop();
    else
        Friends()->Resume();
}

//================================================================================
//================================================================================
void CBot::MaintainEnemy()
{
	if ( !Friends() )
		return;

	Friends()->MaintainEnemy();
}

//================================================================================
//================================================================================
float CBot::GetEnemyDistance() 
{
	if ( !Friends() )
		return -1.0f;

	return Friends()->GetEnemyDistance();
}

//================================================================================
// Devuelve si el enemigo actual se ha perdido totalmente de la vista
//================================================================================
bool CBot::IsEnemyLost() 
{
    return (HasCondition( BCOND_ENEMY_OCCLUDED ));
}

//================================================================================
//================================================================================
bool CBot::IsEnemyLowPriority() 
{
    CBaseEntity *pEnemy = GetEnemy();

    if ( !pEnemy )
        return true;

    if ( HasCondition( BCOND_ENEMY_DEAD ) )
        return true;

    // Nuestro enemigo es un jugador
    if ( pEnemy->IsPlayer() ) {
        CPlayer *pPlayer = ToInPlayer( pEnemy );
        Assert( pPlayer );

        // Esta incapacitado
        if ( pPlayer->IsDejected() )
            return true;
    }

    if ( GetIdealEnemy() ) {
        // El enemigo ideal esta muy cerca!
        if ( GetIdealEnemy()->GetAbsOrigin().DistTo( GetAbsOrigin() ) <= 200.0f )
            return true;

        if ( pEnemy->IsPlayer() && !GetIdealEnemy()->IsPlayer() )
            return false;
    }

    if ( IsEnemyLost() )
        return true;

    return false;
}

//================================================================================
//================================================================================
bool CBot::IsBetterEnemy( CBaseEntity * pIdeal, CBaseEntity * pPrevious )
{
    if ( !pPrevious )
        return true;

    // No somos noobs
    if ( !GetSkill()->IsEasy() ) {
        // Puedo ver al ideal pero no al anterior, entonces si es mejor
        if ( GetHost()->IsAbleToSee( pIdeal ) && !GetHost()->IsAbleToSee( pPrevious ) )
            return true;
    }

    int previousPriority = GetHost()->IRelationPriority( pPrevious );
    int idealPriority = GetHost()->IRelationPriority( pIdeal );

    float flPreviousDistance = pPrevious->GetAbsOrigin().DistTo( GetAbsOrigin() );
    float flIdealDistance = pIdeal->GetAbsOrigin().DistTo( GetAbsOrigin() );

    // El enemigo a comparar esta más lejos
    if ( flIdealDistance > flPreviousDistance ) {
        // Pero tiene más prioridad
        if ( idealPriority > previousPriority )
            return true;

        return false;
    }

    // Tiene más prioridad
    if ( idealPriority > previousPriority )
        return true;

    // Sin duda le damos prioridad a enemigos MUY cerca
    if ( flIdealDistance <= 200.0f )
        return true;

    // Son jugadores
    if ( pIdeal->IsPlayer() && pPrevious->IsPlayer() ) {
        CPlayer *pIdealPlayer = ToInPlayer( pIdeal );
        CPlayer *pPreviousPlayer = ToInPlayer( pPrevious );

        // El enemigo ideal no esta incapacitado, pero el anterior si!
        if ( pPreviousPlayer->IsDejected() && !pIdealPlayer->IsDejected() )
            return true;
    }

    // Es el enemigo de algún miembro del escuadron
    if ( GetSquad() && GetSquad()->IsSquadEnemy( pIdeal, GetHost() ) )
        return true;

    return false;
}

//================================================================================
//================================================================================
int CBot::GetEnemiesCount()
{
    if ( !Friends() )
        return 0;

    return Friends()->GetEnemiesCount();
}

//================================================================================
// Devuelve la cantidad de enemigos peligrosos que estan cerca
//================================================================================
int CBot::GetNearbyEnemiesCount()
{
    if ( !Friends() )
        return 0;

    return Friends()->GetNearbyEnemiesCount();
}

//================================================================================
// Devuelve el mejor candidato a enemigo
//================================================================================
CBaseEntity * CBot::GetIdealEnemy()
{
    if ( !Friends() )
        return NULL;

    return Friends()->GetIdealEnemy();
}

//================================================================================
// Devuelve el enemigo actual
//================================================================================
CBaseEntity *CBot::GetEnemy()
{
    if ( !Friends() )
        return NULL;

    return Friends()->GetEnemy();
}

//================================================================================
// Establece un nuevo enemigo
//================================================================================
void CBot::SetEnemy( CBaseEntity *pEnemy, bool bUpdate )
{
    if ( !Friends() )
        return;

    return Friends()->SetEnemy( pEnemy, bUpdate );
}

//================================================================================
// Devuelve si el enemigo especificado es peligroso
//================================================================================
bool CBot::IsDangerousEnemy( CBaseEntity *pEnemy ) 
{
	if ( !pEnemy )
		pEnemy = GetEnemy();

	if ( !pEnemy )
		return false;
		
    if ( pEnemy->IsPlayer() ) {
        CPlayer *pPlayer = ToInPlayer( pEnemy );

        if ( pPlayer->IsDejected() )
            return false;

        if ( pPlayer->IsMovementDisabled() )
            return false;

        if ( pPlayer->IsBot() ) {
            if ( pPlayer->GetAI()->HasCondition( BCOND_INDEFENSE ) )
                return false;

            if ( !pPlayer->GetAI()->CanMove() )
                return false;
        }

        return true;
    }

    // Cada bot debe implementar su criterio propio
	return false;
}

//================================================================================
// Devuelve si el debemos perseguir a nuestro enemigo cuidadosamente
//================================================================================
bool CBot::ShouldCarefulApproach()
{
    if ( GetSkill()->GetLevel() <= SKILL_MEDIUM )
        return false;

    if ( GetNearbyEnemiesCount() >= 2 )
        return true;

    if ( !IsDangerousEnemy() )
        return false;

    return true;
}

//================================================================================
//================================================================================
CEnemyMemory *CBot::GetEnemyMemory( int index ) 
{
	if ( !Friends() )
		return NULL;

	return Friends()->GetEnemyMemory( index );
}

//================================================================================
//================================================================================
CEnemyMemory *CBot::GetEnemyMemory( CBaseEntity *pEnemy ) 
{
	if ( !Friends() )
		return NULL;

	return Friends()->GetEnemyMemory( pEnemy );
}

//================================================================================
// Actualiza la memoría del enemigo actual
//================================================================================
CEnemyMemory *CBot::UpdateEnemyMemory( CBaseEntity *pEnemy, const Vector vecLocation, float duration, CBaseEntity *reported )
{
    if ( !Friends() )
        return NULL;

    return Friends()->UpdateEnemyMemory( pEnemy, vecLocation, duration, reported );
}

//================================================================================
// Devuelve la condición actual sobre el disparo primario de un arma de fuego
//================================================================================
BCOND CBot::ShouldRangeAttack1()
{
    if ( bot_primary_attack.GetBool() )
        return BCOND_CAN_RANGE_ATTACK1;

    CEnemyMemory *memory = Friends()->GetEnemyMemory();

    if ( !GetEnemy() || !memory )
        return BCOND_NONE;

    CBaseWeapon *pWeapon = GetHost()->GetBaseWeapon();

    if ( IsEnemyLost() ) {
        // Hemos perdido de vista al enemigo:
        // Si somos un bot dificil entonces seguimos disparando por
        // 2.5s a la última posición de nuestro enemigo.

        if ( !pWeapon || pWeapon->IsMeleeWeapon() )
            return BCOND_NONE;

        if ( GetSkill()->GetLevel() <= SKILL_MEDIUM )
            return BCOND_NONE;

        if ( GetActiveSchedule() != NULL )
            return BCOND_NONE;

        if ( GetLookTarget() != GetEnemy() || !IsAimingReady() )
            return BCOND_NONE;

        if ( HasCondition( BCOND_ENEMY_LOST ) )
            return BCOND_NONE;
    }

    float flDistance = GetEnemyDistance();

    if ( pWeapon && !pWeapon->IsMeleeWeapon() ) {
        if ( HasCondition( BCOND_EMPTY_CLIP1_AMMO ) )
            return BCOND_NONE;

        if ( !pWeapon->CanPrimaryAttack() )
            return BCOND_NONE;

        if ( !GetSkill()->IsEasy() && GetEnemy()->m_lifeState == LIFE_DYING )
            return BCOND_NONE;

        if ( flDistance > pWeapon->GetWeaponInfo().m_flIdealDistance )
            return BCOND_TOO_FAR_TO_ATTACK;
    }
    else {
        // TODO
        if ( flDistance > 120.0f )
            return BCOND_TOO_FAR_TO_ATTACK;
    }

    // Trazamos una línea simulando ser las balas
    CBulletsTraceFilter traceFilter( COLLISION_GROUP_NONE );
    traceFilter.SetPassEntity( GetHost() );

    Vector vecForward, vecOrigin( GetHost()->EyePosition() );
    GetHost()->GetVectors( &vecForward, NULL, NULL );

    trace_t tr;
    UTIL_TraceLine( vecOrigin, vecOrigin + vecForward * 3000.0f, MASK_SHOT, &traceFilter, &tr );

    if ( Aim() ) {
        // Si la línea trazada demuestra que dispararemos directo al enemigo entonces nos
        // evitamos esta verificación que no es confiable a cortas distancias.
        if ( tr.m_pEnt != GetEnemy() ) {
            if ( !IsAimingReady() || GetLookTarget() != GetEnemy() )
                return BCOND_NOT_FACING_ATTACK;
        }
    }

    // Un aliado nos esta bloqueando la línea de fuego
    if ( tr.m_pEnt && RELATIONSHIP( tr.m_pEnt ) == GR_ALLY )
        return BCOND_BLOCKED_BY_FRIEND;

    return BCOND_CAN_RANGE_ATTACK1;
}

//================================================================================
// Devuelve la condición actual sobre el disparo secundario de un arma de fuego
//================================================================================
BCOND CBot::ShouldRangeAttack2()
{
    CBaseWeapon *pWeapon = GetHost()->GetBaseWeapon();

    // Tenemos un francotirador
    if ( pWeapon && pWeapon->IsSniper() ) {
        // No somos noob
        if ( !GetSkill()->IsEasy() ) {
            // Hacemos zoom
            if ( IsCombating() && !pWeapon->IsWeaponZoomed() ) {
                return BCOND_CAN_RANGE_ATTACK2;
            }

            // Quitamos el zoom
            else if ( pWeapon->IsWeaponZoomed() ) {
                return BCOND_CAN_RANGE_ATTACK2;
            }
        }
    }

    return BCOND_NONE;
}

//================================================================================
// Establece vecPosition con la posicion del hitbox especificado
// del enemigo guardado en la memoria (Sin verificar visibilidad)
//================================================================================
void CEnemyMemory::GetHitboxPosition( Vector &vecPosition, HitboxType part )
{
    vecPosition.Invalidate();

    switch ( part )
    {
        // PRO
        case HITGROUP_HEAD:
            vecPosition = GetBodyPositions().head;
        break;

        // PRO/Normal
        case HITGROUP_CHEST:
        default:
            vecPosition = GetBodyPositions().gut;
        break;

        // Noobs
        case HITGROUP_LEFTLEG:
            vecPosition = GetBodyPositions().leftFoot;
        break;

        // Noobs
        case HITGROUP_RIGHTLEG:
            vecPosition = GetBodyPositions().rightFoot;
        break;
    }
}

//================================================================================
// Revisa la memoria del enemigo y establece vecPosition con la posición
// del Hitbox especificado siempre y cuando sea visible.
//================================================================================
bool CEnemyMemory::GetHitboxPosition(CBaseEntity *pViewer, Vector &vecPosition, HitboxType part)
{
    VPROF_BUDGET("CEnemyMemory::GetHitboxPosition", VPROF_BUDGETGROUP_BOTS);

    // Obtenemos la posición del hitbox
    GetHitboxPosition(vecPosition, part);

    // ¿Podemos ver esa parte de cuerpo?
    bool visible = false;

    if ( vecPosition.IsValid() ) {
        // Es un jugador
        if ( pViewer->IsPlayer() ) {
            CPlayer *pPlayer = ToInPlayer(pViewer);
            visible = pPlayer->IsAbleToSee(vecPosition, CBaseCombatCharacter::DISREGARD_FOV);
        }
        else {
            visible = pViewer->FVisible(vecPosition);
        }
    }

    return visible;
}

//================================================================================
// Revisa la memoria del enemigo y establece vecPosition con la posición
// del Hitbox si es visible, de otra forma, busca cualquier otro hitbox visible.
//================================================================================
bool CEnemyMemory::GetVisibleHitboxPosition( CBaseEntity *pViewer, Vector &vecPosition, HitboxType favorite )
{
    // El hitbox de preferencia esta visible.
    if ( GetHitboxPosition( pViewer, vecPosition, favorite ) )
        return true;

    // Pecho
    if ( favorite != HITGROUP_CHEST && GetHitboxPosition( pViewer, vecPosition, HITGROUP_CHEST ) )
        return true;

    // Cabeza
    if ( favorite != HITGROUP_HEAD && GetHitboxPosition( pViewer, vecPosition, HITGROUP_HEAD ) )
        return true;

    // Pie izquierdo
    if ( favorite != HITGROUP_LEFTLEG && GetHitboxPosition( pViewer, vecPosition, HITGROUP_LEFTLEG ) )
        return true;

    // Pie derecho
    if ( favorite != HITGROUP_RIGHTLEG && GetHitboxPosition( pViewer, vecPosition, HITGROUP_RIGHTLEG ) )
        return true;

    return false;
}

//================================================================================
// Devuelve si la última posición del enemigo es visible para pViewer
//================================================================================
bool CEnemyMemory::IsLastPositionVisible( CBaseEntity *pViewer )
{
    //if ( pViewer->IsPlayer() )
        //return ((CPlayer *)pViewer)->IsAbleToSee( GetLastPosition() );

    if ( pViewer->MyCombatCharacterPointer() )
        return pViewer->MyCombatCharacterPointer()->IsAbleToSee( GetLastPosition() );

    return pViewer->FVisible( GetLastPosition() );
}

//================================================================================
// Devuelve si la posición del hitbox es visible para pViewer
//================================================================================
bool CEnemyMemory::IsHitboxVisible( CBaseEntity *pViewer, HitboxType part )
{
	Vector vecDummy;
	return GetHitboxPosition( pViewer, vecDummy, part );
}

//================================================================================
// Devuelve si cualquier hitbox válido es visible para pViewer 
//================================================================================
bool CEnemyMemory::IsAnyHitboxVisible( CBaseEntity *pViewer )
{
    Vector vecDummy;
    return GetVisibleHitboxPosition( pViewer, vecDummy, HITGROUP_HEAD );
}

//================================================================================
// Devuelve si pViewer puede ver al enemigo
//================================================================================
bool CEnemyMemory::IsVisible( CBaseEntity * pViewer )
{
    if ( !IsAnyHitboxVisible( pViewer ) && !IsLastPositionVisible( pViewer ) )
        return false;

    return true;
}
