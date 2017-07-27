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
BEGIN_SETUP_SCHEDULE( CHelpDejectedFriendSchedule )
    ADD_TASK( BTASK_SAVE_LOCATION,	 NULL )
	ADD_TASK( BTASK_RUN,	         NULL )
	ADD_TASK( BTASK_MOVE_DESTINATION, GetBot()->m_nDejectedFriend.Get() )
	ADD_TASK( BTASK_AIM,			 GetBot()->m_nDejectedFriend.Get() )
	ADD_TASK( BTASK_HELP,		     NULL )
	ADD_TASK( BTASK_RESTORE_LOCATION, NULL )

    ADD_INTERRUPT( BCOND_REPEATED_DAMAGE )
    ADD_INTERRUPT( BCOND_HEAVY_DAMAGE )
    ADD_INTERRUPT( BCOND_LOW_HEALTH )
    ADD_INTERRUPT( BCOND_DEJECTED )
END_SCHEDULE()

//================================================================================
//================================================================================
bool CurrentSchedule::ShouldHelp()
{
	CPlayer *pFriend = GetBot()->m_nDejectedFriend.Get();

	if ( !pFriend )
		return false;

	// Estamos en modo defensivo, si estamos en combate solo ayudarlo si esta cerca
    if ( GetBot()->GetTacticalMode() == TACTICAL_MODE_DEFENSIVE )
	{
		float distance = GetAbsOrigin().DistTo( pFriend->GetAbsOrigin() );
		const float tolerance = 400.0f;

		if ( !IsIdle() && distance >= tolerance )
			return false;
	}

	return true;
}

//================================================================================
//================================================================================
float CurrentSchedule::GetDesire()
{
	if ( !CanMove() )
		return 0.0f;

	// No estamos viendo a nadie incapacitado
	if ( !HasCondition(BCOND_SEE_DEJECTED_FRIEND) )
		return 0.0f;

	// Estamos en combate
	if ( IsCombating() )
		return 0.0f;

	// No debemos ayudar
	if ( !ShouldHelp() )
		return 0.0f;

	return 0.51f;
}

//================================================================================
//================================================================================
void CurrentSchedule::TaskRun()
{
    CPlayer *pFriend = GetBot()->m_nDejectedFriend.Get();
	BotTaskInfo_t *pTask = GetActiveTask();

    switch( pTask->task )
    {
        // Recargamos
        case BTASK_HELP:
        {
			// Miramos a nuestro amigo
            GetBot()->LookAt( "Dejected Friend", pFriend->GetAbsOrigin(), PRIORITY_CRITICAL, 1.0f );

            // Empezamos a ayudarlo
            if ( GetBot()->IsAimingReady() )
                InjectButton( IN_USE );

			// Ya no esta vivo o se ha levantado
			if ( !pFriend || !pFriend->IsAlive() || pFriend->IsBeingHelped() || !pFriend->IsDejected() )
			{
				TaskComplete();
			}

            break;
        }

		default:
		{
			// Nuestro amigo ha muerto o 
			// ya esta siendo ayudado
			if ( !pFriend || !pFriend->IsAlive() || pFriend->IsBeingHelped() || !pFriend->IsDejected() )
			{
				GetBot()->m_nDejectedFriend = NULL;
				Fail("The friend.");
				return;
			}

			BaseClass::TaskRun();
			break;
		}
    }
}