//==== Woots 2017. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "bot.h"

#include "in_utils.h"
#include "in_buttons.h"

#include "weapon_base.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
//================================================================================
BEGIN_SETUP_SCHEDULE( CDefendSpawnSchedule )
    ADD_TASK( BTASK_MOVE_SPAWN, NULL )

    ADD_INTERRUPT( BCOND_NEW_ENEMY )
    ADD_INTERRUPT( BCOND_SEE_ENEMY )
    ADD_INTERRUPT( BCOND_SEE_FEAR )
    ADD_INTERRUPT( BCOND_LIGHT_DAMAGE )
    ADD_INTERRUPT( BCOND_HEAVY_DAMAGE )
    ADD_INTERRUPT( BCOND_LOW_HEALTH )
    ADD_INTERRUPT( BCOND_DEJECTED )
END_SCHEDULE()

//================================================================================
//================================================================================
float CurrentSchedule::GetDesire()
{
    if ( !CanMove() )
        return BOT_DESIRE_NONE;

    if ( !IsIdle() )
        return BOT_DESIRE_NONE;

    if ( GetBot()->GetTacticalMode() != TACTICAL_MODE_DEFENSIVE )
        return BOT_DESIRE_NONE;

    if ( GetBot()->IsFollowingSomeone() )
        return BOT_DESIRE_NONE;

    float distance = GetAbsOrigin().DistTo( GetBot()->m_vecSpawnSpot );

    if ( distance < 200.0f )
        return BOT_DESIRE_NONE;

    return 0.05f;
}