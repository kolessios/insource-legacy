//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "bot.h"

#include "in_utils.h"
#include "in_buttons.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
//================================================================================
BEGIN_SETUP_SCHEDULE( CHideAndHealSchedule )
    ADD_TASK( BTASK_SAVE_LOCATION,	  NULL )
	ADD_TASK( BTASK_RUN,	          NULL )
    ADD_TASK( BTASK_GET_COVER,	      1500.0f )
    ADD_TASK( BTASK_MOVE_DESTINATION, NULL )
	ADD_TASK( BTASK_CROUCH,			  NULL )
	ADD_TASK( BTASK_HEAL,			  NULL )
	ADD_TASK( BTASK_RELOAD_SAFE,	  NULL )
    ADD_TASK( BTASK_WAIT,			  RandomFloat(0.5f, 3.5f) )
	ADD_TASK( BTASK_RESTORE_LOCATION, NULL )

    ADD_INTERRUPT( BCOND_DEJECTED )
END_SCHEDULE()

//================================================================================
//================================================================================
float CurrentSchedule::GetDesire()
{
    if ( !CanMove() )
        return 0.0f;

    if ( !HasCondition( BCOND_LOW_HEALTH ) )
        return 0.0f;

    if ( !GetBot()->ShouldHide() )
        return 0.0f;

    // Estamos en una batalla
    if ( IsCombating() || IsAlerted() ) {
        return 0.95f;
    }

    return 0.0f;
}