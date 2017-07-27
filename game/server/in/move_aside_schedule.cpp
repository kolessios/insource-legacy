//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "bot.h"

#include "in_utils.h"
#include "in_buttons.h"

#include "weapon_base.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
//================================================================================
BEGIN_SETUP_SCHEDULE( CMoveAsideSchedule )
    ADD_TASK( BTASK_GET_SPOT_ASIDE,   NULL )
	ADD_TASK( BTASK_MOVE_DESTINATION, NULL )

    ADD_INTERRUPT( BCOND_DEJECTED )
    ADD_INTERRUPT( BCOND_HEAVY_DAMAGE )
END_SCHEDULE()

//================================================================================
//================================================================================
float CurrentSchedule::GetDesire()
{
    if ( !CanMove() )
        return 0.0f;

    if ( GetBot()->HasDestination() )
        return 0.0f;

    if ( GetSkill()->GetLevel() <= SKILL_MEDIUM )
        return 0.0f;

    if ( IsCombating() || IsAlerted() ) {
        if ( HasCondition( BCOND_LIGHT_DAMAGE ) )
            return 0.70f;
    }

    if ( HasCondition( BCOND_BLOCKED_BY_FRIEND ) )
        return 0.91f;

    if ( GetBot()->GetTacticalMode() == TACTICAL_MODE_DEFENSIVE && !IsCombating() ) {
        if ( HasCondition( BCOND_ENEMY_OCCLUDED ) || HasCondition( BCOND_HEAR_COMBAT ) )
            return 0.14f;
    }

    if ( IsIdle() && m_nMoveAsideTimer.IsElapsed() )
        return 0.13f;

    return 0.0f;
}

//================================================================================
//================================================================================
void CurrentSchedule::OnStart()
{
	BaseClass::OnStart();
	m_nMoveAsideTimer.Start( RandomInt(5, 20) );
}