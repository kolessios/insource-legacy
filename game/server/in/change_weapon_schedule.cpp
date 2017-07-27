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
BEGIN_SETUP_SCHEDULE( CChangeWeaponSchedule )
    ADD_TASK( BTASK_SAVE_LOCATION, NULL )
    ADD_TASK( BTASK_RUN, NULL )
    ADD_TASK( BTASK_MOVE_DESTINATION, GetBot()->m_nBetterWeapon.Get() )
    ADD_TASK( BTASK_AIM, GetBot()->m_nBetterWeapon.Get() )
    ADD_TASK( BTASK_USE, NULL )
    ADD_TASK( BTASK_RESTORE_LOCATION, NULL )

    ADD_INTERRUPT( BCOND_REPEATED_DAMAGE )
    ADD_INTERRUPT( BCOND_HEAVY_DAMAGE )
    ADD_INTERRUPT( BCOND_TOO_CLOSE_TO_ATTACK )
    ADD_INTERRUPT( BCOND_LOW_HEALTH )
    ADD_INTERRUPT( BCOND_DEJECTED )
END_SCHEDULE()

//================================================================================
//================================================================================
float CurrentSchedule::GetDesire()
{
	if ( !CanMove() )
		return BOT_DESIRE_NONE;

	if ( HasCondition(BCOND_BETTER_WEAPON_AVAILABLE) )
		return 0.41f;

	return BOT_DESIRE_NONE;
}

//================================================================================
//================================================================================
void CurrentSchedule::TaskStart()
{
	BotTaskInfo_t *pTask = GetActiveTask();

	switch( pTask->task )
	{
        case BTASK_USE:
        {
            InjectButton( IN_USE );
            break;
        }

        default:
        {
            BaseClass::TaskStart();
            break;
        }
    }
}

//================================================================================
//================================================================================
void CurrentSchedule::TaskRun()
{
	CBaseWeapon *pWeapon = GetBot()->m_nBetterWeapon.Get();

	// El arma ha dejado de existir
	if ( !pWeapon )
	{
		Fail("The weapon dont exists.");
		return;
	}

	// Ya tiene un dueño
	if ( pWeapon->GetOwner() )
	{
		// Alguien más la ha tomado...
		if ( pWeapon->GetOwner() != GetHost() )
		{
			Fail("The weapon has been taken");
			return;
		}
	}

	BotTaskInfo_t *pTask = GetActiveTask();

	switch( pTask->task )
	{
        case BTASK_USE:
        {
            if ( pWeapon->GetOwner() && pWeapon->GetOwner() == GetHost() )
            {
                TaskComplete();
            }

            break;
        }

        default:
        {
            BaseClass::TaskRun();
            break;
        }
    }
}