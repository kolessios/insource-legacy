//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "bot.h"

#include "in_utils.h"
#include "bot_manager.h"

#include "in_buttons.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
//================================================================================
void CAttackComponent::OnUpdate( CBotCmd* &cmd )
{
    VPROF_BUDGET( "CAttackComponent::OnUpdate", VPROF_BUDGETGROUP_BOTS );

    // Podemos disparar o atacar con arma primaria
    if (
        HasCondition( BCOND_CAN_RANGE_ATTACK1 ) || HasCondition( BCOND_CAN_MELEE_ATTACK1 ) ||
        HasCondition( BCOND_CAN_RANGE_ATTACK2 ) || HasCondition( BCOND_CAN_MELEE_ATTACK2 )
        ) {
        // Estamos usando un arma de fuego
        if ( IsUsingFiregun() ) {
            FiregunAttack();
        }
        else {
            MeleeWeaponAttack();
        }
    }
}

//================================================================================
// Realiza un ataque con el arma de fuego
//================================================================================
void CAttackComponent::FiregunAttack()
{
    CBaseWeapon *pWeapon = GetHost()->GetBaseWeapon();
    Assert( pWeapon );

    float fireRate = pWeapon->GetFireRate();
    float minRate = fireRate + GetSkill()->GetMinAttackRate();
    float maxRate = fireRate + GetSkill()->GetMaxAttackRate();

    // Si tenemos un enemigo
    if ( GetBot()->GetEnemy() ) {
        float flDistance = GetBot()->Friends()->GetEnemyDistance();

        // Nos agachamos para obtener precisión
        if ( CanDuckShot() && flDistance >= 140.0f )
            GetBot()->Crouch();
        else
            GetBot()->StandUp();
    }

    // Podemos disparar
    if ( HasCondition( BCOND_CAN_RANGE_ATTACK1 ) && CanShot() ) {
        Combat();

        // Atacamos
        InjectButton( IN_ATTACK );
        m_ShotRateTimer.Start( RandomFloat( minRate, maxRate ) );
    }

    // Podemos usar el disparo secundario
    // TODO
    if ( HasCondition( BCOND_CAN_RANGE_ATTACK2 ) ) {
        Combat();
        InjectButton( IN_ATTACK2 );
    }
}

//================================================================================
// Realiza un ataque cuerpo a cuerpo
//================================================================================
void CAttackComponent::MeleeWeaponAttack()
{
    if ( HasCondition( BCOND_CAN_MELEE_ATTACK1 ) ) {
        // Combatiendo!
        Combat();
        InjectButton( IN_ATTACK );
    }

    if ( HasCondition( BCOND_CAN_MELEE_ATTACK2 ) ) {
        // Combatiendo!
        Combat();
        InjectButton( IN_ATTACK2 );
    }
}

//================================================================================
// Devuelve si estamos usando un arma de fuego
//================================================================================
bool CAttackComponent::IsUsingFiregun()
{
    CBaseWeapon *pWeapon = GetHost()->GetBaseWeapon();

    if ( !pWeapon )
        return false;

    return !pWeapon->IsMeleeWeapon();
}

//================================================================================
// Devuelve si el Bot puede disparar agachado
//================================================================================
bool CAttackComponent::CanDuckShot()
{
    if ( GetSkill()->IsEasy() )
        return false;

    if ( HasCondition( BCOND_DEJECTED ) )
        return false;

    CEnemyMemory *memory = GetBot()->GetEnemyMemory();

    if ( !memory )
        return false;

    // Altura de los ojos al agacharse
    Vector vecEyePosition = GetHost()->EyePosition();

    if ( !GetHost()->IsCrouching() )
        vecEyePosition.z -= VEC_DUCK_VIEW.z;

    // Trazamos una línea simulando ser las balas
    CBulletsTraceFilter traceFilter( COLLISION_GROUP_NONE );
    traceFilter.SetPassEntity( GetHost() );

    Vector vecForward;
    GetHost()->GetVectors( &vecForward, NULL, NULL );

    trace_t tr;
    UTIL_TraceLine( vecEyePosition, vecEyePosition + vecForward * 3000.0f, MASK_SHOT, &traceFilter, &tr );

    // Sí podemos
    if ( tr.m_pEnt == memory->GetEnemy() ) {
        //NDebugOverlay::Line( tr.startpos, tr.endpos, 0, 255, 0, true, 0.5f );
        return true;
    }

    //NDebugOverlay::Line( tr.startpos, tr.endpos, 255, 0, 0, true, 0.5f );
    return false;
}
