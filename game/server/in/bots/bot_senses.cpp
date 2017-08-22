//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#include "cbase.h"

#include "bots\bot.h"
#include "in_utils.h"
#include "in_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar bot_optimize;
extern ConVar bot_far_distance;

//================================================================================
// Procesa lo que esta mirando el Bot ahora mismo
//================================================================================
void CBot::OnLooked( int iDistance )
{
    VPROF_BUDGET( "OnLooked", VPROF_BUDGETGROUP_BOTS );

    // Limpiamos las condiciones
    ClearCondition( BCOND_SEE_HATE );
    ClearCondition( BCOND_SEE_FEAR );
    ClearCondition( BCOND_SEE_DISLIKE );
    ClearCondition( BCOND_BETTER_WEAPON_AVAILABLE );
    ClearCondition( BCOND_SEE_DEJECTED_FRIEND );

    AISightIter_t iter;
    CBaseEntity *pSightEnt = GetSenses()->GetFirstSeenEntity( &iter );

	if ( !pSightEnt )
		return;

	int limit = 25;

    if ( ShouldOptimize() ) {
        limit = 5;
    }

    while ( pSightEnt ) {
        OnLooked( pSightEnt );

        pSightEnt = GetSenses()->GetNextSeenEntity( &iter );
        --limit;

        if ( limit <= 0 )
            break;
    }
}

//================================================================================
// El Bot tiene visión de la entidad
//================================================================================
void CBot::OnLooked( CBaseEntity *pSightEnt )
{
    if ( !GetMemory() )
        return;

    BCOND condition = BCOND_NONE;
    int relation = TheGameRules->PlayerRelationship( GetHost(), pSightEnt );

    CEntityMemory *memory = GetMemory()->UpdateEntityMemory( pSightEnt, pSightEnt->GetAbsOrigin() );
    memory->UpdateVisibility( true );

    switch ( relation ) {
        case GR_ENEMY:
        {
            int priority = GetHost()->IRelationPriority( pSightEnt );

            if ( priority < 0 ) {
                condition = BCOND_SEE_DISLIKE;
            }
            else {
                condition = BCOND_SEE_HATE;
            }

            break;
        }

        case GR_NEUTRAL:
        {
            if ( pSightEnt->IsBaseCombatWeapon() ) {
                CBaseWeapon *pWeapon = ToBaseWeapon( pSightEnt );
                Assert( pWeapon );

                if ( GetDecision()->ShouldGrabWeapon( pWeapon ) ) {
                    GetMemory()->UpdateDataMemory( "BestWeapon", pSightEnt, 30.0f );
                    SetCondition( BCOND_BETTER_WEAPON_AVAILABLE );
                }
            }

            break;
        }

        case GR_ALLY:
        {
            if ( pSightEnt->IsPlayer() ) {
                CPlayer *pSightPlayer = ToInPlayer( pSightEnt );

                if ( GetDecision()->ShouldHelpDejectedFriend( pSightPlayer ) ) {
                    GetMemory()->UpdateDataMemory( "DejectedFriend", pSightEnt, 30.0f );
                    SetCondition( BCOND_SEE_DEJECTED_FRIEND );
                }
            }

            break;
        }
    }

    if ( condition != BCOND_NONE ) {
        SetCondition( condition );
    }
}

//================================================================================
// Procesa lo que esta escuchando el Bot ahora mismo
//================================================================================
void CBot::OnListened()
{
    VPROF_BUDGET( "OnListened", VPROF_BUDGETGROUP_BOTS );

    // Limpiamos las condiciones
    ClearCondition( BCOND_HEAR_COMBAT );
    ClearCondition( BCOND_HEAR_WORLD );
    ClearCondition( BCOND_HEAR_ENEMY );
    ClearCondition( BCOND_HEAR_ENEMY_FOOTSTEP );
    ClearCondition( BCOND_HEAR_BULLET_IMPACT );
    ClearCondition( BCOND_HEAR_BULLET_IMPACT_SNIPER );
    ClearCondition( BCOND_HEAR_DANGER );
    ClearCondition( BCOND_HEAR_MOVE_AWAY );
    ClearCondition( BCOND_HEAR_SPOOKY );
    ClearCondition( BCOND_SMELL_MEAT );
    ClearCondition( BCOND_SMELL_CARCASS );
    ClearCondition( BCOND_SMELL_GARBAGE );

    AISoundIter_t iter;
    CSound *pCurrentSound = GetSenses()->GetFirstHeardSound( &iter );

    while ( pCurrentSound ) {
        if ( pCurrentSound->IsSoundType( SOUND_DANGER ) ) {
            if ( pCurrentSound->IsSoundType( SOUND_CONTEXT_BULLET_IMPACT ) ) {
                SetCondition( BCOND_HEAR_MOVE_AWAY );

                if ( pCurrentSound->IsSoundType( SOUND_CONTEXT_FROM_SNIPER ) ) {
                    SetCondition( BCOND_HEAR_BULLET_IMPACT_SNIPER );
                }
                else {
                    SetCondition( BCOND_HEAR_BULLET_IMPACT );
                }
            }

            if ( pCurrentSound->IsSoundType( SOUND_CONTEXT_EXPLOSION ) ) {
                SetCondition( BCOND_HEAR_SPOOKY );
                SetCondition( BCOND_HEAR_MOVE_AWAY );
            }

            SetCondition( BCOND_HEAR_DANGER );
        }

        if ( pCurrentSound->IsSoundType( SOUND_COMBAT ) ) {
            if ( pCurrentSound->IsSoundType( SOUND_CONTEXT_GUNFIRE ) ) {
                SetCondition( BCOND_HEAR_SPOOKY );
            }

            SetCondition( BCOND_HEAR_COMBAT );
        }

        if ( pCurrentSound->IsSoundType( SOUND_WORLD ) ) {
            SetCondition( BCOND_HEAR_WORLD );
        }

        if ( pCurrentSound->IsSoundType( SOUND_PLAYER ) ) {
            if ( pCurrentSound->IsSoundType( SOUND_CONTEXT_FOOTSTEP ) ) {
                SetCondition( BCOND_HEAR_ENEMY_FOOTSTEP );
            }

            SetCondition( BCOND_HEAR_ENEMY );
        }

        if ( pCurrentSound->IsSoundType( SOUND_CARCASS ) ) {
            SetCondition( BCOND_SMELL_CARCASS );
        }

        if ( pCurrentSound->IsSoundType( SOUND_MEAT ) ) {
            SetCondition( BCOND_SMELL_MEAT );
        }

        if ( pCurrentSound->IsSoundType( SOUND_GARBAGE ) ) {
            SetCondition( BCOND_SMELL_GARBAGE );
        }

        pCurrentSound = GetSenses()->GetNextHeardSound( &iter );
    }
}

//================================================================================
// Hemos recibido daño
//================================================================================
void CBot::OnTakeDamage( const CTakeDamageInfo &info )
{
    CBaseEntity *pAttacker = info.GetAttacker();
    float farDistance = bot_far_distance.GetFloat();

    // El atacante es un personaje enemigo
    if ( pAttacker && pAttacker->MyCombatCharacterPointer() && TheGameRules->PlayerRelationship( GetHost(), pAttacker ) != GR_ALLY ) {
        float distance = GetAbsOrigin().DistTo( pAttacker->GetAbsOrigin() );

        // Posición estimada del atacante
        Vector vecEstimatedPosition = pAttacker->WorldSpaceCenter();

        // ¡Esta muy lejos! No podemos saber el punto exacto
        if ( distance >= farDistance ) {
            float errorDistance = 200.0f;

            // Somos novatos, calculamos mal
            if ( GetSkill()->GetLevel() <= SKILL_MEDIUM )
                errorDistance = 500.0f;

            vecEstimatedPosition.x += RandomFloat( -errorDistance, errorDistance );
            vecEstimatedPosition.y += RandomFloat( -errorDistance, errorDistance );
        }

        // Estamos calmados...
        if ( GetState() == STATE_IDLE ) {
            // No podemos verlo, entramos en pánico!!
            if ( !GetHost()->IsAbleToSee( pAttacker ) ) {
                Panic();
            }

            // Miramos al lugar de ataque
            GetVision()->LookAt( "Unknown Enemy Spot", vecEstimatedPosition, PRIORITY_HIGH, GetSkill()->GetAlertDuration() );
        }
        else {
            bool setAsEnemy = true;

            // Si esta muy lejos (como un francotirador oculto)
            // entonces solo actualizamos su posición apróximada
            if ( distance >= farDistance ) {
                setAsEnemy = false;
            }

            // Nuestro nuevo enemigo
            if ( setAsEnemy && GetEnemy() != pAttacker ) {
                // Noob: ¡Otro enemigo! ¡Panico total!
                if ( GetSkill()->IsEasy() ) {
                    Panic( RandomFloat( 0.2f, 0.6f ) );
                }

                SetEnemy( pAttacker, true );
            }
            else {
                GetMemory()->UpdateEntityMemory( pAttacker, vecEstimatedPosition );
            }
        }
    }
    
    // El último daño recibido fue hace menos de 2s
    // Al parecer estamos recibiendo daño continuo
    if ( GetHost()->GetLastDamageTimer().IsLessThen( 2.0f ) ) {
        ++m_iRepeatedDamageTimes;
        m_flDamageAccumulated += info.GetDamage();
    }
}

//================================================================================
// Hemos muerto
//================================================================================
void CBot::OnDeath( const CTakeDamageInfo &info ) 
{
    if ( GetActiveSchedule() ) {
        GetActiveSchedule()->Fail("Player Death");
        GetActiveSchedule()->Finish();
        m_nActiveSchedule = NULL;
    }
}
