//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "bot.h"

#include "in_utils.h"
#include "in_buttons.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
//================================================================================
BEGIN_SCHEDULE( CCoverSchedule )
    ADD_TASK( BTASK_SAVE_LOCATION,	  NULL )
	ADD_TASK( BTASK_RUN,	          NULL )
	ADD_TASK( BTASK_GET_COVER,        NULL )
	ADD_TASK( BTASK_MOVE_DESTINATION, NULL )
	ADD_TASK( BTASK_CROUCH,			  NULL )
	ADD_TASK( BTASK_RELOAD_SAFE,      NULL )
    ADD_TASK( BTASK_WAIT,			  RandomFloat(1.0f, 3.0f) )
	ADD_TASK( BTASK_RESTORE_LOCATION, NULL )

    ADD_INTERRUPT( BCOND_DEJECTED )
END_SCHEDULE()

//================================================================================
//================================================================================
float CCoverSchedule::GetDesire()
{
    if ( !CanMove() )
        return 0.0f;

    if ( !GetBot()->ShouldHide() )
        return 0.0f;

    if ( IsCombating() || IsAlerted() ) {
        if ( GetSkill()->GetLevel() > SKILL_MEDIUM && GetBot()->IsDangerousEnemy() ) {
            if ( HasCondition( BCOND_LIGHT_DAMAGE ) )
                return 0.82f;
        }

        if ( HasCondition( BCOND_REPEATED_DAMAGE ) )
            return 0.83f;

        if ( HasCondition( BCOND_HEAVY_DAMAGE ) )
            return 0.9f;

        // Disparo de un sniper
        //if ( HasCondition(BCOND_HEAR_BULLET_IMPACT_SNIPER) )
            //return 0.93f;
    }

    return 0.0f;
}

//================================================================================
//================================================================================
BEGIN_SCHEDULE( CHideSchedule )
	ADD_TASK( BTASK_RUN,	          NULL )
    ADD_TASK( BTASK_GET_FAR_COVER,    NULL )
	ADD_TASK( BTASK_MOVE_DESTINATION, NULL )
	ADD_TASK( BTASK_RELOAD,			  NULL )
    ADD_TASK( BTASK_WAIT,			  RandomFloat(1.0f, 3.0f) )

    ADD_INTERRUPT( BCOND_NEW_ENEMY )
    ADD_INTERRUPT( BCOND_DEJECTED )

    if ( HasCondition(BCOND_INDEFENSE) )
        ADD_INTERRUPT( BCOND_BETTER_WEAPON_AVAILABLE )
END_SCHEDULE()

//================================================================================
//================================================================================
float CHideSchedule::GetDesire()
{
	if ( !CanMove() )
		return 0.0f;

    if ( IsCombating() || IsAlerted() )
    {
	    // Estamos indefensos
	    if ( HasCondition(BCOND_INDEFENSE) )
	        return 0.94f;
    }

	return 0.0f;
}