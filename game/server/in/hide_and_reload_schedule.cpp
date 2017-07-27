//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "bot.h"

#include "in_utils.h"
#include "in_buttons.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
//================================================================================
BEGIN_SETUP_SCHEDULE( CHideAndReloadSchedule )
    ADD_TASK( BTASK_SAVE_LOCATION,	  NULL )
	ADD_TASK( BTASK_RUN,	          NULL )
	ADD_TASK( BTASK_RELOAD_ASYNC,	  NULL )
    ADD_TASK( BTASK_GET_COVER,		  NULL )
	ADD_TASK( BTASK_MOVE_DESTINATION, NULL )
	ADD_TASK( BTASK_CROUCH,			  NULL )
    ADD_TASK( BTASK_WAIT,			  RandomFloat(0.5f, 2.5f) )
	ADD_TASK( BTASK_RESTORE_LOCATION, NULL )

    ADD_INTERRUPT( BCOND_EMPTY_PRIMARY_AMMO )
    ADD_INTERRUPT( BCOND_ENEMY_DEAD )
    ADD_INTERRUPT( BCOND_LOW_HEALTH )
    ADD_INTERRUPT( BCOND_DEJECTED )
END_SCHEDULE()

//================================================================================
//================================================================================
float CurrentSchedule::GetDesire()
{
    if ( !CanMove() )
        return 0.0f;

    if ( !GetBot()->ShouldHide() )
        return 0.0f;

    if ( HasCondition( BCOND_EMPTY_CLIP1_AMMO ) && !HasCondition( BCOND_EMPTY_PRIMARY_AMMO ) ) {
        if ( IsCombating() || IsAlerted() )
            return 0.92f;
    }

    return 0.0f;
}

//================================================================================
//================================================================================
void CurrentSchedule::TaskRun()
{
    BotTaskInfo_t *pTask = GetActiveTask();

    switch ( pTask->task ) {
        case BTASK_MOVE_DESTINATION:
        case BTASK_CROUCH:
        case BTASK_WAIT:
        {
            // Si no somos noob, entonces cancelamos estas tareas
            // en cuanto tengamos el arma recargada.
            if ( !GetSkill()->IsEasy() ) {
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
            }

            BaseClass::TaskRun();
            break;
        }

        default:
        {
            BaseClass::TaskRun();
            break;
        }
    }
}