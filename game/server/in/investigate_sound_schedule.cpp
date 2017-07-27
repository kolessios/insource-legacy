//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "bot.h"

#include "in_utils.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
//================================================================================
BEGIN_SETUP_SCHEDULE( CInvestigateSoundSchedule )
    if ( HasCondition( BCOND_HEAR_SPOOKY ) )
        ADD_TASK( BTASK_RUN, NULL )
    else
        ADD_TASK( BTASK_WALK, NULL )

	ADD_TASK( BTASK_SAVE_LOCATION,	  NULL )
    ADD_TASK( BTASK_MOVE_DESTINATION,  m_vecSoundPosition )
	ADD_TASK( BTASK_WAIT,             RandomFloat(3.0f, 6.0f) )
	ADD_TASK( BTASK_RESTORE_LOCATION, NULL )

    ADD_INTERRUPT( BCOND_NEW_ENEMY )
    ADD_INTERRUPT( BCOND_SEE_ENEMY )
    ADD_INTERRUPT( BCOND_SEE_FEAR )
    ADD_INTERRUPT( BCOND_LIGHT_DAMAGE )
    ADD_INTERRUPT( BCOND_HEAVY_DAMAGE )
    ADD_INTERRUPT( BCOND_LOW_HEALTH )
    ADD_INTERRUPT( BCOND_BETTER_WEAPON_AVAILABLE )
    ADD_INTERRUPT( BCOND_DEJECTED )
    ADD_INTERRUPT( BCOND_HEAR_MOVE_AWAY )
END_SCHEDULE()

//================================================================================
//================================================================================
float CurrentSchedule::GetDesire()
{
    if ( !GetBot()->ShouldInvestigateSound() )
        return BOT_DESIRE_NONE;

    if ( HasCondition( BCOND_HEAR_COMBAT ) || HasCondition( BCOND_HEAR_ENEMY ) )
        return 0.2f;

    return BOT_DESIRE_NONE;
}

//================================================================================
//================================================================================
void CurrentSchedule::TaskStart()
{
    switch ( GetActiveTask()->task ) {
        // Guarda la ubicación actual del Bot
        // Y también obtiene la ubicación del sonido de combate
        case BTASK_SAVE_LOCATION:
        {
            CSound *pSound = GetHost()->GetBestSound( SOUND_COMBAT | SOUND_PLAYER );

            if ( !pSound ) {
                Fail( "Invalid Sound" );
                return;
            }

            m_vecSoundPosition = pSound->GetSoundReactOrigin();

            BaseClass::TaskStart();
            break;
        }

        default:
        {
            BaseClass::TaskStart();
            break;
        }
    }
}