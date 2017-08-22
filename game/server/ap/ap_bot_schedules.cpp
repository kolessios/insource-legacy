//==== Woots 2017. http://creativecommons.org/licenses/by/2.5/mx/ ===========//


#include "cbase.h"

/*
#include "ap_bot.h"
#include "ap_bot_schedules.h"
#include "in_utils.h"

#include "ai_hint.h"

#include "nav.h"
#include "nav_area.h"
#include "nav_mesh.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_SCHEDULE( CSearchResourcesSchedule )
	//ADD_TASK( BTASK_RUN,                NULL )
	ADD_TASK( BTASK_RELOAD_ASYNC,       NULL )
	ADD_TASK( BTASK_GET_RESOURCES_SPOT, NULL )
	ADD_TASK( BTASK_MOVE_DESTINATION,   NULL )

	ADD_INTERRUPT( BCOND_NEW_ENEMY )
    ADD_INTERRUPT( BCOND_SEE_ENEMY )
	ADD_INTERRUPT( BCOND_SEE_FEAR )
    ADD_INTERRUPT( BCOND_LIGHT_DAMAGE )
    ADD_INTERRUPT( BCOND_HEAVY_DAMAGE )
    ADD_INTERRUPT( BCOND_LOW_HEALTH )
    ADD_INTERRUPT( BCOND_DEJECTED )
    ADD_INTERRUPT( BCOND_BETTER_WEAPON_AVAILABLE )
END_SCHEDULE()

//================================================================================
//================================================================================
CSearchResourcesSchedule::CSearchResourcesSchedule() 
{
	m_nTimer.Start( 10 );
}

//================================================================================
//================================================================================
float CSearchResourcesSchedule::GetDesire()
{
	if ( !CanMove() )
		return 0.0f;

    if ( IsCombating() || IsAlerted() )
		return 0.0f;

	if ( GetBot()->HasDestination() )
		return 0.0f;

	if ( GetBot()->IsFollowingSomeone() )
		return 0.0f;

	if ( IsIdle() && m_nTimer.IsElapsed() )
		return 0.15f;

	return 0.0f;
}

//================================================================================
//================================================================================
void CSearchResourcesSchedule::OnEnd()
{
	BaseClass::OnEnd();
	m_nTimer.Start( RandomInt(5, 6) );
}

//================================================================================
//================================================================================
void CSearchResourcesSchedule::GetWander() 
{
	int count = TheNavMesh->GetNavAreaCount();

	if ( count == 0 )
		return;

    FOR_EACH_VEC( TheNavAreas, it )
    {
        CNavArea *pArea = TheNavAreas[it];

		if ( !pArea )
			continue;

		if ( pArea->IsUnderwater() )
			continue;

		if ( pArea->GetDanger(TEAM_ANY) > 10.0f )
			continue;
    
		if ( m_CandidateAreas.HasElement(pArea) )
			continue;

		m_CandidateAreas.AddToTail( pArea );
	}
}

//================================================================================
//================================================================================
void CSearchResourcesSchedule::GetResources() 
{
	int count = TheNavMesh->GetNavAreaCount();

	if ( count == 0 )
		return;

    FOR_EACH_VEC( TheNavAreas, it )
    {
        CNavArea *pArea = TheNavAreas[it];

		if ( !pArea )
			continue;

		if ( pArea->IsUnderwater() )
			continue;

		if ( pArea->GetDanger(TEAM_ANY) > 10.0f )
			continue;

		if ( !pArea->HasAttributes(NAV_MESH_RESOURCES) )
			continue;
    
		if ( m_CandidateAreas.HasElement(pArea) )
			continue;

		m_CandidateAreas.AddToTail( pArea );
	}
}

//================================================================================
//================================================================================
void CSearchResourcesSchedule::TaskStart()
{
    BotTaskInfo_t *pTask = GetActiveTask();

    switch ( pTask->task ) {
        case BTASK_GET_RESOURCES_SPOT:
        {
            bool wander = false;

            // Dependiendo del nivel de dificultad, podemos
            // vagar en vez de ir por los recursos
            if ( GetSkill()->IsEasy() )
                wander = (RandomInt( 0, 10 ) > 4);
            if ( GetSkill()->IsMedium() )
                wander = (RandomInt( 0, 10 ) > 6);
            if ( GetSkill()->IsHard() )
                wander = (RandomInt( 0, 10 ) > 8);

            // Limpiamos las areas candidatas para ir
            m_CandidateAreas.Purge();

            if ( wander ) {
                GetWander();
            }
            else {
                GetResources();
            }

            // No hay lugares candidatos para el mapa!
            if ( m_CandidateAreas.Count() == 0 ) {
                Fail( "m_CandidateAreas == 0!" );
                return;
            }

            // Solo hay una
            if ( m_CandidateAreas.Count() == 1 ) {
                CNavArea *pArea = m_CandidateAreas[0];

                m_vecLocation = pArea->GetRandomPoint();
                TaskComplete();
            }
            else {
                int key = RandomInt( 0, m_CandidateAreas.Count() - 1 );

                CNavArea *pArea = m_CandidateAreas[key];
                Assert( pArea );

                m_vecLocation = pArea->GetRandomPoint();
                TaskComplete();
            }

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
void CSearchResourcesSchedule::TaskRun()
{
	BotTaskInfo_t *pTask = GetActiveTask();

	switch( pTask->task )
	{
		case BTASK_GET_RESOURCES_SPOT:
		{
			break;
		}

		default:
		{
			BaseClass::TaskRun();
			break;
		}
	}
}

//--------------------------------------------------------------------------------------------------

BEGIN_SCHEDULE( CCleanBuildingSchedule )
    CAP_BotSoldier *pSoldierAI = (CAP_BotSoldier *)GetBot();
    AssertOnce( pSoldierAI );

    if ( pSoldierAI->IsMinionCleaningBuilding() ) {
        ADD_TASK( BTASK_WALK, NULL )
        ADD_TASK( BTASK_WAIT_LEADER, NULL )
        ADD_TASK( BTASK_SNEAK, NULL )
        ADD_TASK( BTASK_TACTICAL_COVER, NULL )
    }
    else {
        if ( !pSoldierAI->IsCleaningBuilding() ) {
            ADD_TASK( BTASK_SET_TOLERANCE, RandomFloat( 400.0f, 480.0f ) )
            ADD_TASK( BTASK_MOVE_DESTINATION, m_pHint )

            ADD_TASK( BTASK_SNEAK, NULL )
            ADD_TASK( BTASK_SET_TOLERANCE, RandomFloat( 250.0f, 380.0f ) )
            ADD_TASK( BTASK_MOVE_DESTINATION, m_pHint )

            ADD_TASK( BTASK_CROUCH, NULL )
            ADD_TASK( BTASK_MOVE_DESTINATION, m_pHint )
            ADD_TASK( BTASK_PLAY_GESTURE, ACT_SIGNAL_HALT )
            ADD_TASK( BTASK_WAIT, RandomFloat( 2.0f, 5.0f ) )
        }

        ADD_TASK( BTASK_STANDUP, NULL )
        ADD_TASK( BTASK_CLEAN_AREA, NULL )
    }

    ADD_INTERRUPT( BCOND_NEW_ENEMY )
    ADD_INTERRUPT( BCOND_SEE_ENEMY )
    ADD_INTERRUPT( BCOND_SEE_FEAR )
    ADD_INTERRUPT( BCOND_LIGHT_DAMAGE )
    ADD_INTERRUPT( BCOND_HEAVY_DAMAGE )
    ADD_INTERRUPT( BCOND_LOW_HEALTH )
    ADD_INTERRUPT( BCOND_DEJECTED )
    ADD_INTERRUPT( BCOND_BETTER_WEAPON_AVAILABLE )
END_SCHEDULE()

CCleanBuildingSchedule::CCleanBuildingSchedule()
{
     m_pHint = NULL;
}

float CCleanBuildingSchedule::GetDesire()
{
    CAP_BotSoldier *pSoldierAI = (CAP_BotSoldier *)GetBot();
    AssertOnce( pSoldierAI );

    if ( !CanMove() )
        return BOT_DESIRE_NONE;

    if ( IsCombating() || IsAlerted() )
        return BOT_DESIRE_NONE;

    if ( GetBot()->GetSquad() && !GetBot()->IsSquadLeader() ) {
        // Mi líder entro en proceso de limpiar un edificio
        // Sigamos lo mismo que el.
        if ( pSoldierAI->IsMinionCleaningBuilding() )
            return 0.28f;

        return BOT_DESIRE_NONE;
    }

    if ( !pSoldierAI )
        return BOT_DESIRE_NONE;

    if ( m_pHint )
        return 0.28f;

    CSpotCriteria criteria;
    criteria.OnlyVisible( true );
    criteria.SetMaxRange( GetHost()->GetSenses()->GetDistLook() );
    criteria.SetOrigin( GetAbsOrigin() );
    criteria.SetTacticalMode( GetBot()->GetTacticalMode() );

    CHintCriteria hintCriteria;
    hintCriteria.AddHintType( HINT_TACTICAL_PINCH );
    hintCriteria.AddIncludePosition( criteria.m_vecOrigin, criteria.m_flMaxRange );

    CUtlVector<CAI_Hint *> collector;
    CAI_HintManager::FindAllHints( criteria.m_vecOrigin, hintCriteria, &collector );

    FOR_EACH_VEC( collector, it )
    {
        CAI_Hint *pHint = collector[it];

        if ( !pHint )
            continue;

        Vector position = pHint->GetAbsOrigin();

        if ( !Utils::IsValidSpot( position, criteria.m_vecOrigin, criteria, GetHost() ) )
            continue;

        if ( pSoldierAI->HasBuildingCleaned( STRING( pHint->GetGroup() ) ) )
            continue;

        m_pHint = pHint;
        return 0.28f;
    }

    return BOT_DESIRE_NONE;
}

void CCleanBuildingSchedule::Start()
{
    BaseClass::Start();

    m_vecSpot.Invalidate();

    if ( GetBot()->IsSquadLeader() ) {
        Assert( m_pHint );
        ((CAP_BotSoldier *)GetBot())->StartBuildingClean( m_pHint );
    }
}

void CCleanBuildingSchedule::OnEnd()
{
    BaseClass::OnEnd();
    m_vecSpot.Invalidate();
}

void CCleanBuildingSchedule::Think()
{
    BaseClass::Think();

    CAP_BotSoldier *pSoldierBot = (CAP_BotSoldier *)GetBot();

    if ( GetBot()->IsSquadLeader() ) {
        CBuildingInfo *info = pSoldierBot->GetBuildingInfo();
        
        if ( !info )
            return;

        if ( GetBot()->ShouldShowDebug() ) {
            if ( info->areas.Count() > 0 ) {
                FOR_EACH_VEC( info->areas, it )
                {
                    CNavArea *pArea = info->areas[it];
                    pArea->DrawFilled( 255, 0, 0, 100.0f, 0.1f );
                }
            }
        }
    }
    else {
        LookAround();
    }
}

void CCleanBuildingSchedule::LookAround()
{
    if ( !GetBot()->Aim() )
        return;

    GetBot()->Aim()->BlockLookAround( 2.0f );

    CSpotCriteria criteria;
    criteria.SetMaxRange( 700.0f );
    criteria.OnlyVisible( true );
    criteria.SetTacticalMode( GetBot()->GetTacticalMode() );

    Vector vecSpot;
    vecSpot.Invalidate();

    SpotVector list;
    Utils::FillIntestingPositions( &list, GetHost(), criteria );

    if ( list.Count() == 0 )
        return;

    float closest = MAX_TRACE_LENGTH;

    FOR_EACH_VEC( list, it )
    {
        if ( GetBot()->ShouldShowDebug() )
            NDebugOverlay::Cross3D( list[it], 8.0f, 0, 0, 255, true, 0.15f );

        float distance = GetAbsOrigin().DistTo( list[it] );

        CNavArea *pArea = GetHost()->GetLastKnownArea();

        if ( pArea && !pArea->HasAttributes(NAV_MESH_STAIRS) ) {
            if ( distance < 130.0f )
                continue;
        }

        if ( GetBot()->GetSquad()->IsSomeoneLooking( list[it], GetHost() ) ) {
            if ( GetBot()->ShouldShowDebug() )
                NDebugOverlay::Box( list[it], Vector( -2, -2, -2 ), Vector( 2, 2, 2 ), 255, 0, 0, 150.0f, 0.1f );

            continue;
        }

        if ( distance < closest ) {
            vecSpot = list[it];
            closest = distance;
        }
    }

    if ( GetBot()->ShouldShowDebug() )
        NDebugOverlay::Box( vecSpot, Vector( -2, -2, -2 ), Vector( 2, 2, 2 ), 0, 255, 0, 150.0f, 0.1f );

    if ( !vecSpot.IsValid() )
        return;

    m_vecSpot = vecSpot;
    GetBot()->LookAt( "Tactical Spot", vecSpot, PRIORITY_HIGH, 1.0f );
}

void CCleanBuildingSchedule::TaskStart()
{
    BotTaskInfo_t *pTask = GetActiveTask();
    CAP_BotSoldier *pSoldierBot = (CAP_BotSoldier *)GetBot();

    CBuildingInfo *info = pSoldierBot->GetBuildingInfo();
    Assert( info );

    if ( !info ) {
        Fail("info == NULL");
        return;
    }

    switch ( pTask->task ) {
        case BTASK_CLEAN_AREA:
        {
            // Ya no quedan más areas que examinar, el edificio esta limpio.
            if ( info->areas.Count() == 0 ) {
                pSoldierBot->FinishBuildingClean();
                GetHost()->DoAnimationEvent( PLAYERANIMEVENT_CUSTOM_GESTURE, ACT_SIGNAL_GROUP );
                m_pHint = NULL;

                TaskComplete();
                return;
            }

            CNavArea *pArea = NULL;
            int key = -1;

            if ( info->areas.Count() == 1 ) {
                pArea = info->areas[0];
                key = 0;
            }
            else {
                float closest = MAX_TRACE_LENGTH;
                bool visible = false;

                FOR_EACH_VEC( info->areas, it )
                {
                    float distance = GetAbsOrigin().DistTo( info->areas[it]->GetCenter() );
                    bool isVisible = GetHost()->IsAbleToSee( info->areas[it]->GetCenter(), CBaseCombatCharacter::DISREGARD_FOV );

                    if ( distance > closest )
                        continue;

                    if ( !isVisible && visible )
                        continue;

                    pArea = info->areas[it];
                    closest = distance;
                    visible = isVisible;
                    key = it;
                }
            }

            Assert( pArea );

            // Esto jamás debería ocurrir...
            if ( !pArea ) {
                pSoldierBot->FinishBuildingClean();
                GetHost()->DoAnimationEvent( PLAYERANIMEVENT_CUSTOM_GESTURE, ACT_SIGNAL_GROUP );
                m_pHint = NULL;

                TaskComplete();
                return;
            }

            info->areas.Remove( key );
            pSoldierBot->SetScanningArea( pArea );

            ADD_TASK( BTASK_SNEAK, NULL )
            ADD_TASK( BTASK_PLAY_GESTURE, ACT_SIGNAL_FORWARD )
            ADD_TASK( BTASK_MOVE_DESTINATION, pArea->GetCenter() )
            ADD_TASK( BTASK_CROUCH, NULL )
            ADD_TASK( BTASK_TACTICAL_LOOK_AROUND, NULL )
            ADD_TASK( BTASK_WAIT, RandomFloat( 3.0f, 7.0f ) )
            ADD_TASK( BTASK_STANDUP, NULL )
            ADD_TASK( BTASK_CLEAN_AREA, NULL )

            TaskComplete();
            break;
        }

        case BTASK_TACTICAL_LOOK_AROUND:
        {
            LookAround();
            TaskComplete();
            break;
        }

        case BTASK_WAIT_LEADER:
        {
            GetBot()->ResumeFollow();
            break;
        }

        case BTASK_TACTICAL_COVER:
        {
            GetBot()->PauseFollow();
            break;
        }

        default:
        {
            BaseClass::TaskStart();
            break;
        }
    }
}

void CCleanBuildingSchedule::TaskRun()
{
    CAP_BotSoldier *pSoldierBot = dynamic_cast<CAP_BotSoldier *>(GetBot());
    BotTaskInfo_t *pTask = GetActiveTask();

    CPlayer *pLeader = GetBot()->GetSquadLeader();
    CAP_BotSoldier *pSoldierLeader = dynamic_cast<CAP_BotSoldier *>(pLeader->GetAI());

    CBuildingInfo *info = pSoldierBot->GetBuildingInfo();
    Assert( info );

    if ( !info ) {
        Fail( "info == NULL" );
        return;
    }

    switch ( pTask->task ) {
        case BTASK_WAIT_LEADER:
        {
            CNavArea *pArea = pSoldierLeader->GetScanningArea();

            if ( pArea ) {
                TaskComplete();
                return;
            }

            break;
        }

        case BTASK_TACTICAL_COVER:
        {
            if ( !pSoldierBot->IsMinionCleaningBuilding() ) {
                TaskComplete();
                return;
            }

            CSpotCriteria criteria;
            criteria.SetMaxRange( 500.0f );
            criteria.SetOrigin( pLeader->GetAbsOrigin() );
            criteria.AvoidTeam( GetHost()->GetTeamNumber() );
            criteria.SetMinDistanceAvoid( 80.0f );

            SpotVector list;
            Utils::FillCoverPositions( &list, GetHost(), criteria );

            Assert( list.Count() > 0 );

            if ( list.Count() == 0 )
                return;

            m_vecLocation.Invalidate();

            FOR_EACH_VEC( list, it )
            {
                if ( GetBot()->GetSquad()->IsSomeoneGoing( list[it], GetHost() ) )
                    continue;

                m_vecLocation = list[it];
                break;
            }

            Assert( m_vecLocation.IsValid() );

            if ( !m_vecLocation.IsValid() )
                return;

            float flDistance = GetAbsOrigin().DistTo( m_vecLocation );
            float flTolerance = GetBot()->GetDestinationDistanceTolerance();

            if ( flDistance <= flTolerance ) {
                GetBot()->Crouch();
                return;
            }

            GetBot()->SetDestination( m_vecLocation, PRIORITY_HIGH );
            break;
        }

        default:
        {
            BaseClass::TaskRun();
            break;
        }
    }
}
*/