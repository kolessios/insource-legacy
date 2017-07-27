//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "bot.h"

#include "in_utils.h"
#include "in_buttons.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
//================================================================================
BEGIN_SETUP_SCHEDULE( CCallBackupSchedule )
    ADD_TASK( BTASK_SAVE_LOCATION,	 NULL )
	ADD_TASK( BTASK_RUN,	             NULL )
    ADD_TASK( BTASK_GET_FAR_COVER,		 1000.0f )
	ADD_TASK( BTASK_CROUCH,			 NULL )
	ADD_TASK( BTASK_HEAL,			 NULL )
	ADD_TASK( BTASK_RELOAD,			 NULL )
    ADD_TASK( BTASK_WAIT,			 RandomFloat(2.0f, 6.0f) )
	ADD_TASK( BTASK_CALL_FOR_BACKUP,	 NULL )
	ADD_TASK( BTASK_WAIT,			 RandomFloat(2.0f, 6.0f) )
	ADD_TASK( BTASK_RESTORE_LOCATION, NULL )

    ADD_INTERRUPT( BCOND_DEJECTED )
END_SCHEDULE()