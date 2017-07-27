//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "bot.h"

#include "in_utils.h"
#include "in_buttons.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
//================================================================================
BEGIN_SETUP_SCHEDULE( CHuntEnemySchedule )
    // Task
    bool carefulApproach = GetBot()->ShouldCarefulApproach();

    // Corremos al objetivo hasta llegar a una distancia entre 700 y 900 unidades
    ADD_TASK( BTASK_RUN, NULL )
    ADD_TASK( BTASK_HUNT_ENEMY, RandomFloat(700.0, 900.0f) )

    // Debemos acercarnos cuidadosamente...
    if ( carefulApproach )
    {
        // Caminamos lentalmente hacia una distancia segura...
        ADD_TASK( BTASK_WALK, NULL )
        ADD_TASK( BTASK_HUNT_ENEMY, RandomFloat(400.0f, 550.0f) )
        ADD_TASK( BTASK_WAIT, RandomFloat( 0.5f, 3.5f ) )
    }
    
    ADD_TASK( BTASK_HUNT_ENEMY, NULL )        

    // Interrupts
    ADD_INTERRUPT( BCOND_EMPTY_CLIP1_AMMO )
    ADD_INTERRUPT( BCOND_ENEMY_DEAD )
    ADD_INTERRUPT( BCOND_ENEMY_UNREACHABLE )

    ADD_INTERRUPT( BCOND_HEAVY_DAMAGE )
    ADD_INTERRUPT( BCOND_REPEATED_DAMAGE )
    ADD_INTERRUPT( BCOND_LOW_HEALTH )
    ADD_INTERRUPT( BCOND_DEJECTED )
	ADD_INTERRUPT( BCOND_INDEFENSE )

    ADD_INTERRUPT( BCOND_TOO_CLOSE_TO_ATTACK )
    ADD_INTERRUPT( BCOND_NEW_ENEMY )
    ADD_INTERRUPT( BCOND_BETTER_WEAPON_AVAILABLE )
END_SCHEDULE()

//================================================================================
//================================================================================
float CurrentSchedule::GetDesire()
{
    if ( !GetBot()->GetEnemy() )
        return BOT_DESIRE_NONE;

    if ( !GetBot()->ShouldHuntEnemy() )
        return BOT_DESIRE_NONE;

    CEnemyMemory *memory = GetBot()->GetEnemyMemory();

    if ( !memory )
        return BOT_DESIRE_NONE;

    // Distancia que nos queda
    float distance = GetBot()->GetEnemyDistance();
    float tolerance = GetBot()->GetDestinationDistanceTolerance();

    if ( distance <= tolerance )
        return BOT_DESIRE_NONE;

    if ( HasCondition( BCOND_ENEMY_OCCLUDED ) )
        return 0.65;

    if ( HasCondition( BCOND_TOO_FAR_TO_ATTACK ) )
        return 0.38f;

    return BOT_DESIRE_NONE;
}