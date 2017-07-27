//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "bot_soldier.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

/*
//================================================================================
//================================================================================
BEGIN_SETUP_SCHEDULE( CSoldierCoverSchedule )
    ADD_TASK( TASK_SET_COVER )
    ADD_INTERRUPT( BCOND_DEJECTED )
END_SCHEDULE()

//================================================================================
//================================================================================
float CurrentSchedule::GetDesire()
{
	if ( !CanMove() )
		return BOT_DESIRE_NONE;

	// Estamos en una batalla
	if ( IsCombating() || IsAlerted() )
	{
		// Debemos cubrirnos si
		// No estamos ya en una posición de cobertura
		// El enemigo es marcado como peligroso
		// Hay una cobertura cerca
		bool shouldCover = ( !BOT->IsInCoverPosition() && BOT->IsDangerousEnemy() && BOT->GetNearestCover() );

		if ( !HasCondition(BCOND_REPEATED_DAMAGE) && !HasCondition(BCOND_HEAVY_DAMAGE) )
			shouldCover = false;

		// Estamos acercandonos a nuestro líder
		if ( BOT->Follow() && BOT->Follow()->ShouldFollow() )
			shouldCover = false;

		// Debemos colocarnos en una posición de cobertura
		if ( shouldCover )
		{
			if ( IsCombating() )
				return BOT_DESIRE_VERYHIGH;
			else
				return BOT_DESIRE_LOW;
		}
	}

	return BOT_DESIRE_NONE;
}

//================================================================================
//================================================================================
void CurrentSchedule::TaskStart()
{
    switch( GetActiveTask() )
    {
        // Encontramos un lugar seguro para cubrirnos
        case TASK_SET_COVER:
        {
            // Debemos ir rápido
            BOT->Run();
			BOT->Crouch();
            break;
        }
    }
}

//================================================================================
//================================================================================
void CurrentSchedule::TaskRun()
{
	// Mantenemos la posición del enemigo en nuestra memoria
    if ( BOT->Friends() )
        BOT->Friends()->MaintainEnemy();

    switch( GetActiveTask() )
    {
        // Encontramos un lugar seguro para cubrirnos
        case TASK_SET_COVER:
        {
			Vector vecPosition;

            // Hemos llegado
            if ( !BOT->GetNearestCover(vecPosition) || BOT->IsInCoverPosition() )
            {
				BOT->StandUp();
                TaskComplete();
                return;
            }
			

            BOT->Navigation()->SetDestination( vecPosition, PRIORITY_CRITICAL );
            break;
        }
    }
}
*/