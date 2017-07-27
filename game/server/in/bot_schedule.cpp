//==== Woots 2017. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "bot.h"

#include "in_utils.h"
#include "in_buttons.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
// Hemos fallado al realizar este conjunto de tareas
//================================================================================
void CBotSchedule::Fail( const char *pWhy )
{
	m_bFailed = true;
    GetBot()->DebugAddMessage("Scheduled %s Failed: %s", g_BotSchedules[GetID()], pWhy);
}

//================================================================================
// Devuelve el deseo real según el motor
//================================================================================
float CBotSchedule::GetRealDesire() 
{
    if ( HasStarted() ) {
        if ( ShouldInterrupted() )
            return BOT_DESIRE_NONE;

        if ( ShouldForceFinish() )
            return m_flLastDesire;
    }

	m_flLastDesire = GetDesire();
	return m_flLastDesire;
}

//================================================================================
// Comienza el conjunto de tareas
//================================================================================
void CBotSchedule::OnStart()
{
    Assert( m_Tasks.Count() == 0 );

    // Tarea activa
    m_bFinished = false;
    m_bStarted = true;

    m_bFailed = false;
    m_nActiveTask = NULL;
    //m_flLastDesire = BOT_DESIRE_NONE;

    m_vecSavedLocation.Invalidate();
    m_vecLocation.Invalidate();
    m_flDistanceTolerance = 0.0f;

    // Hemos empezado
    m_StartTimer.Start();

    GetBot()->NormalWalk();
    GetBot()->StandUp();
    GetBot()->PauseFollow();

    Setup();
}

//================================================================================
// Termina el conjunto de tareas
//================================================================================
void CBotSchedule::OnEnd()
{
    // Marcamos como terminado
    m_bFinished = true;
    m_bStarted = false;

    m_bFailed = false;
    m_nActiveTask = NULL;
    m_flLastDesire = BOT_DESIRE_NONE;
    m_StartTimer.Invalidate();

    // Limpiamos todo
    m_Tasks.Purge();
    m_Interrupts.Purge();

    GetBot()->DebugAddMessage("Scheduled %s Finished", g_BotSchedules[GetID()]);

    if ( GetBot()->HasDestination() )
        GetBot()->StopNavigation();

    GetBot()->NormalWalk();
    GetBot()->StandUp();   
    GetBot()->ResumeFollow();
}

//================================================================================
// Devuelve si el conjunto de tareas debe ser interrumpido, sin importar el
// nivel de deseo.
//================================================================================
bool CBotSchedule::ShouldInterrupted()
{
    if ( HasFinished() )
        return true;

    if ( m_bFailed )
        return true;

    if ( !GetHost()->IsAlive() )
        return true;

    // Verificamos si tenemos una condición que interrumpe
    // este conjunto de tareas
    FOR_EACH_VEC( m_Interrupts, it )
    {
        BCOND condition = m_Interrupts.Element( it );

        if ( HasCondition( condition ) ) {
            if ( GetBot()->GetActiveSchedule() == this )
                Fail( UTIL_VarArgs( "Interrupted by Condition (%s)", g_Conditions[condition] ) );

            return true;
        }
    }

    return false;
}

//================================================================================
// Ejecuta el conjunto de tareas
//================================================================================
void CBotSchedule::Think()
{
    VPROF_BUDGET( UTIL_VarArgs( "CBotSchedule::%s::Think", g_BotSchedules[GetID()] ), VPROF_BUDGETGROUP_BOTS );
    Assert( m_Tasks.Count() > 0 );

    BotTaskInfo_t *idealTask = m_Tasks.Element( 0 );

    if ( idealTask != m_nActiveTask ) {
        m_nActiveTask = idealTask;
        TaskStart();
        return;
    }

    TaskRun();
}

//================================================================================
// Marca que la tarea actual debe esperar la cantidad de segundos
//================================================================================
void CBotSchedule::Wait( float seconds )
{
    m_WaitTimer.Start( seconds );
    GetBot()->DebugAddMessage("Task Wait for %.2fs", seconds);
}

//================================================================================
// Devuelve el nombre de la tarea activa
//================================================================================
const char *CBotSchedule::GetActiveTaskName()
{
    BotTaskInfo_t *info = GetActiveTask();

    if ( !info )
        return "UNKNOWN";

    if ( info->task >= BLAST_TASK )
        return UTIL_VarArgs("CUSTOM: %i", info->task);

    return g_BotTasks[ info->task ];
}

//================================================================================
// Comienza la ejecución de la tarea activa
//================================================================================
void CBotSchedule::TaskStart()
{
    // Ha sido manejado por el Bot
    if ( GetBot()->TaskStart( GetActiveTask() ) )
        return;

    BotTaskInfo_t *pTask = GetActiveTask();

    switch ( pTask->task ) {
        // Esperar una determinada cantidad de segundos
        case BTASK_WAIT:
        {
            Wait( pTask->flValue );
            break;
        }

        case BTASK_SET_TOLERANCE:
        {
            m_flDistanceTolerance = pTask->flValue;
            TaskComplete();
            break;
        }

        case BTASK_PLAY_ANIMATION:
        {
            Activity activity = (Activity)pTask->iValue;
            GetHost()->DoAnimationEvent( PLAYERANIMEVENT_CUSTOM, activity );
            break;
        }

        case BTASK_PLAY_GESTURE:
        {
            Activity activity = (Activity)pTask->iValue;
            GetHost()->DoAnimationEvent( PLAYERANIMEVENT_CUSTOM_GESTURE, activity );
            TaskComplete();
            break;
        }

        case BTASK_PLAY_SEQUENCE:
        {
            GetHost()->DoAnimationEvent( PLAYERANIMEVENT_CUSTOM_SEQUENCE, pTask->iValue );
            break;
        }

        // Guardar nuestra posición actual
        case BTASK_SAVE_LOCATION:
        {
            m_vecSavedLocation = GetAbsOrigin();

            TaskComplete();
            break;
        }

        // Restaurar la posición guardada
        case BTASK_RESTORE_LOCATION:
        {
            Assert( m_vecSavedLocation.IsValid() );

            // No hace falta volver a donde estabamos, solamente seguimos a nuestro líder
            if ( GetBot()->IsFollowingSomeone() ) {
                TaskComplete();
                return;
            }

            break;
        }

        // Movernos al destino especificado
        case BTASK_MOVE_DESTINATION:
        {
            break;
        }

        // Movernos al lugar donde hemos aparecido
        case BTASK_MOVE_SPAWN:
        {
            m_vecLocation = GetBot()->m_vecSpawnSpot;
            break;
        }

        // Seguir un destino, aunque se mueva
        case BTASK_MOVE_LIVE_DESTINATION:
        {
            // TODO
            break;
        }

        // Perseguir al enemigo
        case BTASK_HUNT_ENEMY:
        {
            // No tenemos enemigo
            if ( !GetBot()->GetEnemy() ) {
                Fail( "Without Enemy" );
                return;
            }

            // Accedemos a la memoria del enemigo actual
            CEnemyMemory *memory = GetBot()->GetEnemyMemory();

            if ( !memory ) {
                Fail( "Without Enemy Memory" );
                return;
            }

            // Los Pros aprovechan para recargar si el enemigo esta a una distancia
            if ( !GetSkill()->IsEasy() ) {
                float distance = GetAbsOrigin().DistTo( memory->GetLastPosition() );

                // Aprovechamos para recargar
                if ( HasCondition( BCOND_LOW_CLIP1_AMMO ) || HasCondition( BCOND_EMPTY_CLIP1_AMMO ) ) {
                    if ( distance > 1000.0f )
                        InjectButton( IN_RELOAD );
                }
            }

            // Agresivos, vamos a por el!!
            if ( GetBot()->GetTacticalMode() == TACTICAL_MODE_AGGRESSIVE )
                GetBot()->Run();

            break;
        }

        // Encontrar y guardar un lugar cerca donde movernos
        case BTASK_GET_SPOT_ASIDE:
        {
            try {
                int attempts = 0;
                m_vecLocation.Invalidate();

                while ( attempts < 60 ) {
                    ++attempts;
                    m_vecLocation = GetHost()->GetLastKnownArea()->GetRandomPoint();

                    if ( GetAbsOrigin().DistTo( m_vecLocation ) < 160.0f )
                        break;
                }

                if ( !m_vecLocation.IsValid() )
                    Fail( "CMoveAsideSchedule Invalid GetRandomPoint. \n" );
            }
            catch ( ... ) {
                Fail( "CMoveAsideSchedule Invalid Exception GetRandomPoint. \n" );
            }

            TaskComplete();
            break;
        }

        // Encontrar y guardar un lugar de cobertura
        case BTASK_GET_COVER:
        {
            // Ya estamos en un lugar seguro
            /*if ( GetBot()->IsInCoverPosition() )
            {
                TaskComplete();
                TaskComplete(); // FIXME:
                return;
            }*/

            float maxRange = pTask->flValue;

            if ( maxRange <= 0 )
                maxRange = 1500.0f;

            // Buscamos un lugar seguro para ocultarnos
            CSpotCriteria criteria;
            criteria.SetMaxRange( maxRange );
            criteria.UseNearest( true );
            criteria.OutOfVisibility( true );
            criteria.AvoidTeam( GetBot()->GetEnemy() );

            // No se ha encontrado ningún lugar seguro
            if ( !Utils::FindCoverPosition( &m_vecLocation, GetHost(), criteria ) ) {
                Fail( "No safe place found" );
                return;
            }

            TaskComplete();
            break;
        }

        // Encontrar y guardar un lugar lejano de cobertura
        case BTASK_GET_FAR_COVER:
        {
            float minRange = pTask->flValue;

            if ( minRange <= 0 )
                minRange = 1000.0f;

            float maxRange = (minRange * 10);

            // Buscamos un lugar seguro para ocultarnos
            CSpotCriteria criteria;
            criteria.SetMaxRange( maxRange );
            criteria.SetMinDistanceAvoid( minRange );
            criteria.UseNearest( false );
            criteria.UseRandom( true );
            criteria.OutOfVisibility( true );
            criteria.AvoidTeam( GetBot()->GetEnemy() );

            // No se ha encontrado ningún lugar seguro
            if ( !Utils::FindCoverPosition( &m_vecLocation, GetHost(), criteria ) ) {
                Fail( "No far safe place found" );
                return;
            }

            // Siempre debemos correr
            TaskComplete();
            break;
        }

        // Apuntar a un destino especifico
        case BTASK_AIM:
        {
            break;
        }

        // Usar
        case BTASK_USE:
        {
            InjectButton( IN_USE );
            TaskComplete();
            break;
        }

        // Saltar
        case BTASK_JUMP:
        {
            InjectButton( IN_JUMP );
            TaskComplete();
            break;
        }

        // Agacharse
        case BTASK_CROUCH:
        {
            GetBot()->Crouch();
            TaskComplete();
            break;
        }

        // Levantarse
        case BTASK_STANDUP:
        {
            GetBot()->StandUp();
            TaskComplete();
            break;
        }

        // Correr
        case BTASK_RUN:
        {
            GetBot()->Run();
            TaskComplete();
            break;
        }

        // Caminar (lento) No funciona si esta corriendo
        case BTASK_WALK:
        {
            GetBot()->Walk();
            TaskComplete();
            break;
        }

        // Caminar
        case BTASK_WALK_NORMAL:
        {
            GetBot()->NormalWalk();
            TaskComplete();
            break;
        }

        // Recargar y esperar a que termine
        case BTASK_RELOAD:
        {
            InjectButton( IN_RELOAD );
            break;
        }

        // Recargar si el enemigo esta lejos
        case BTASK_RELOAD_SAFE:
        {
            bool reload = true;

            // Los noobs siempre intentan recargar
            if ( !GetSkill()->IsEasy() ) {
                if ( GetBot()->GetEnemy() ) {
                    // Un enemigo esta cerca, no recarguemos
                    if ( GetBot()->GetEnemyDistance() < 1000.0f )
                        reload = false;
                }
            }

            if ( reload )
                InjectButton( IN_RELOAD );
            else
                TaskComplete();

            break;
        }

        // Recargar sin esperar
        case BTASK_RELOAD_ASYNC:
        {
            InjectButton( IN_RELOAD );
            TaskComplete();
            break;
        }

        // Curarse
        // TODO
        case BTASK_HEAL:
        {
            GetHost()->TakeHealth( 30.0f, DMG_GENERIC );
            TaskComplete();
            break;
        }

        // Llamar refuerzos
        case BTASK_CALL_FOR_BACKUP:
        {
            break;
        }

        default:
        {
            Assert( !"Task Start not handled!" );
            break;
        }
    }
}

//================================================================================
// Ejecución de una tarea hasta terminarla
//================================================================================
void CBotSchedule::TaskRun()
{
    // Ha sido manejado por el Bot
    if ( GetBot()->TaskRun( GetActiveTask() ) )
        return;

    BotTaskInfo_t *pTask = GetActiveTask();

    switch ( pTask->task ) {
        // Esperar un determinado número de segundos
        case BTASK_WAIT:
        {
            GetBot()->MaintainEnemy();

            if ( IsWaitFinished() )
                TaskComplete();

            break;
        }

        case BTASK_PLAY_ANIMATION:
        case BTASK_PLAY_SEQUENCE:
        {
            if ( GetHost()->IsActivityFinished() )
                TaskComplete();

            break;
        }

        // 
        case BTASK_SAVE_LOCATION:
        {
            break;
        }

        // Restaura la ubicación del Bot
        case BTASK_RESTORE_LOCATION:
        {
            GetBot()->MaintainEnemy();

            // Paramos en cuanto veamos un enemigo
            if ( HasCondition( BCOND_SEE_HATE ) || HasCondition( BCOND_SEE_ENEMY ) ) {
                TaskComplete();
                return;
            }

            // Distancia que nos queda
            float distance = GetAbsOrigin().DistTo( m_vecSavedLocation );
            const float tolerance = GetBot()->GetDestinationDistanceTolerance();

            // Hemos llegado
            if ( distance <= tolerance ) {
                TaskComplete();
                return;
            }

            GetBot()->SetDestination( m_vecSavedLocation, PRIORITY_NORMAL );
            break;
        }

        // Restaura la ubicación del Bot
        // Movernos al lugar donde hemos aparecido
        case BTASK_MOVE_DESTINATION:
        case BTASK_MOVE_SPAWN:
        {
            // Entidad
            if ( pTask->pszValue.Get() ) {
                pTask->vecValue = pTask->pszValue->GetAbsOrigin();
            }

            // Posición en la tarea
            if ( pTask->vecValue.IsValid() ) {
                m_vecLocation = pTask->vecValue;
            }

            // Destino inválido
            if ( !m_vecLocation.IsValid() ) {
                Fail( "Destination is invalid" );
                return;
            }

            // Distancia que nos queda
            float distance = GetAbsOrigin().DistTo( m_vecLocation );
            float tolerance = GetBot()->GetDestinationDistanceTolerance();

            if ( m_flDistanceTolerance > 0.0f ) {
                tolerance = m_flDistanceTolerance;
            }

            // Hemos llegado
            if ( distance <= tolerance ) {
                m_flDistanceTolerance = 0.0f;
                TaskComplete();
                return;
            }

            GetBot()->SetDestination( m_vecLocation, PRIORITY_HIGH );
            break;
        }

        // Perseguir a nuestro enemigo
        case BTASK_HUNT_ENEMY:
        {
            // Accedemos a la memoria del enemigo actual
            CEnemyMemory *memory = GetBot()->GetEnemyMemory();

            if ( !memory ) {
                Fail( "Without Enemy Memory" );
                return;
            }

            // No olvidemos que tenemos un enemigo
            GetBot()->MaintainEnemy();

            // Distancia que nos queda
            float distance = GetAbsOrigin().DistTo( memory->GetLastPosition() );
            float tolerance = pTask->flValue;

            // Tolerancia predeterminada
            if ( tolerance < 1.0f )
                tolerance = GetBot()->GetDestinationDistanceTolerance();

            // Queremos cazar a nuestro enemigo porque nuestra arma actual
            // no tiene rango para poder atacarlo...
            if ( HasCondition( BCOND_TOO_FAR_TO_ATTACK ) ) {
                CBaseWeapon *pWeapon = GetHost()->GetBaseWeapon();

                // Tenemos un arma
                if ( pWeapon ) {
                    float maxWeaponDistance = pWeapon->GetWeaponInfo().m_flIdealDistance;

                    // Nos detendremos en cuanto tengamos rango para disparar
                    if ( pWeapon->IsMeleeWeapon() ) {
                        tolerance = maxWeaponDistance;
                    }
                    else {
                        tolerance = (maxWeaponDistance - 100.0f);
                    }
                }
            }

            bool completed = (distance <= tolerance);

            // Nuestra arma ya puede atacar a nuestro enemigo
            // nos detenemos en cuanto lo veamos!
            if ( !HasCondition( BCOND_TOO_FAR_TO_ATTACK ) && !completed )
                completed = HasCondition( BCOND_SEE_ENEMY );

            // Hemos llegado
            if ( completed ) {
                TaskComplete();
                return;
            }

            // Estamos en modo defensivo y el enemigo esta lejos
            if ( GetBot()->GetTacticalMode() == TACTICAL_MODE_DEFENSIVE && distance >= 450.0f && memory->ReportedBy() ) {
                GetBot()->SetDestination( memory->ReportedBy()->GetAbsOrigin(), PRIORITY_HIGH );
            }
            else {
                GetBot()->SetDestination( memory->GetLastPosition(), PRIORITY_HIGH );
            }


            break;
        }

        // Nos cubrimos
        case BTASK_GET_COVER:
        case BTASK_GET_FAR_COVER:
        {
            break;
        }

        // Apuntar a un lugar
        case BTASK_AIM:
        {
            if ( pTask->pszValue.Get() ) {
                GetBot()->LookAt( "TASK_AIM", pTask->pszValue, PRIORITY_CRITICAL, 1.0f );
            }
            else {
                // Lugar inválido
                if ( !pTask->vecValue.IsValid() ) {
                    Fail( "Aim Destination is invalid" );
                    return;
                }

                GetBot()->LookAt( "TASK_AIM", pTask->vecValue, PRIORITY_CRITICAL, 1.0f );
            }

            if ( GetBot()->IsAimingReady() )
                TaskComplete();

            break;
        }

        // Recargar el arma
        case BTASK_RELOAD:
        case BTASK_RELOAD_SAFE:
        {
            GetBot()->MaintainEnemy();

            CBaseWeapon *pWeapon = GetHost()->GetBaseWeapon();

            // Sin arma
            if ( !pWeapon ) {
                TaskComplete();
                return;
            }

            // Ya hemos terminado de recargar
            if ( !pWeapon->IsReloading() || !HasCondition( BCOND_EMPTY_CLIP1_AMMO ) ) {
                TaskComplete();
                return;
            }

            break;
        }

        // Recargar el arma
        // Pero sin esperar a que se termine de recargar
        case BTASK_RELOAD_ASYNC:
        {
            break;
        }

        case BTASK_CALL_FOR_BACKUP:
        {
            break;
        }

        default:
        {
            Assert( !"Task Run not handled!" );
            break;
        }
    }
}

//================================================================================
// Marca una tarea como completada
//================================================================================
void CBotSchedule::TaskComplete()
{
    if ( GetBot()->HasDestination() ) {
        GetBot()->StopNavigation();
    }

    if ( m_Tasks.Count() == 0 ) {
        Assert( !"m_Tasks.Count() == 0" );
        return;
    }

    m_Tasks.Remove( 0 );

    if ( m_Tasks.Count() == 0 ) {
        m_bFinished = true;
    }
}

//================================================================================
// Devuelve si el Bot puede moverse
//================================================================================
bool CBotSchedule::CanMove() 
{
    return GetBot()->CanMove();
}