//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "bot.h"

#include "in_utils.h"
#include "in_buttons.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
//================================================================================
BEGIN_SETUP_SCHEDULE( CReloadSchedule )
    ADD_TASK( BTASK_RELOAD, NULL )

    ADD_INTERRUPT( BCOND_HEAVY_DAMAGE )
    ADD_INTERRUPT( BCOND_EMPTY_PRIMARY_AMMO )
END_SCHEDULE()

//================================================================================
//================================================================================
float CurrentSchedule::GetDesire()
{
	if ( GetBot()->ShouldHide() )
		return 0.0f;

    if ( HasCondition(BCOND_EMPTY_PRIMARY_AMMO) )
        return 0.0f;

    if ( HasCondition(BCOND_EMPTY_CLIP1_AMMO) )
		return 0.81f;

    if ( HasCondition(BCOND_LOW_CLIP1_AMMO) && !HasCondition(BCOND_LOW_PRIMARY_AMMO) && IsIdle() )
        return 0.43f;

	return 0.0f;
}