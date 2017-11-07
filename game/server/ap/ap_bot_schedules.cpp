//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017


#include "cbase.h"

#include "ap_bot.h"
#include "ap_bot_schedules.h"
#include "in_utils.h"

#include "ai_hint.h"

#include "nav.h"
#include "nav_area.h"
#include "nav_mesh.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
//================================================================================
SET_SCHEDULE_TASKS(CSearchResourcesSchedule)
{
    ADD_TASK(BTASK_RELOAD, true);
    ADD_TASK(BTASK_GET_RESOURCES_SPOT, NULL);
    ADD_TASK(BTASK_MOVE_DESTINATION, NULL);
}

SET_SCHEDULE_INTERRUPTS(CSearchResourcesSchedule)
{
    ADD_INTERRUPT(BCOND_NEW_ENEMY);
    ADD_INTERRUPT(BCOND_SEE_ENEMY);
    ADD_INTERRUPT(BCOND_SEE_FEAR);
    ADD_INTERRUPT(BCOND_LIGHT_DAMAGE);
    ADD_INTERRUPT(BCOND_HEAVY_DAMAGE);
    ADD_INTERRUPT(BCOND_LOW_HEALTH);
    ADD_INTERRUPT(BCOND_DEJECTED);
    ADD_INTERRUPT(BCOND_BETTER_WEAPON_AVAILABLE);
}

//================================================================================
//================================================================================
CSearchResourcesSchedule::CSearchResourcesSchedule(IBot *bot) : BaseClass(bot)
{
	m_nTimer.Start( 10 );
}

//================================================================================
//================================================================================
float CSearchResourcesSchedule::GetDesire() const
{
	if ( !GetDecision()->CanMove() )
		return BOT_DESIRE_NONE;

    if ( IsCombating() || IsAlerted() )
		return BOT_DESIRE_NONE;

	if ( GetLocomotion()->HasDestination() )
		return BOT_DESIRE_NONE;

	if ( GetFollow()->IsFollowingActive() )
		return BOT_DESIRE_NONE;

	if ( IsIdle() && m_nTimer.IsElapsed() )
		return 0.15f;

	return 0.0f;
}

//================================================================================
//================================================================================
void CSearchResourcesSchedule::Finish()
{
	BaseClass::Finish();
	m_nTimer.Start( RandomInt(5, 6) );
}

//================================================================================
// 
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
// 
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
            int randomInt = RandomInt(1, 10);

            // Depending on the difficulty level:
            // Sometimes we fail finding resources.
            if ( GetProfile()->IsEasy() ) {
                wander = (randomInt > 2);
            }
            else if ( GetProfile()->IsMedium() ) {
                wander = (randomInt > 4);
            }
            else if ( GetProfile()->IsHard() ) {
                wander = (randomInt > 6);
            }
            else {
                wander = (randomInt > 8);
            }

            // Clean
            m_CandidateAreas.Purge();

            // We found a place to wander or a place with delicious resources.
            if ( wander ) {
                GetWander();
            }
            else {
                GetResources();

                // We have not found anything...
                // We wander a little
                if ( m_CandidateAreas.Count() == 0 ) {
                    GetWander();
                }
            }

            Assert(m_CandidateAreas.Count() > 0);

            // We have not found anything...
            if ( m_CandidateAreas.Count() == 0 ) {
                Fail( "We have not found anything..." );
                return;
            }

            // We only found one place
            if ( m_CandidateAreas.Count() == 1 ) {
                CNavArea *pArea = m_CandidateAreas[0];

                SavePosition(pArea->GetRandomPoint());
                TaskComplete();
            }
            else {
                int key = RandomInt( 0, m_CandidateAreas.Count() - 1 );

                CNavArea *pArea = m_CandidateAreas[key];
                Assert( pArea );

                SavePosition(pArea->GetRandomPoint());
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

SET_SCHEDULE_TASKS(CCleanBuildingSchedule)
{
    CAP_Bot *pSoldier = dynamic_cast<CAP_Bot *>(GetBot());
    AssertOnce(pSoldier);

    if ( pSoldier->IsMinionCleaningBuilding() ) {
        // We follow our leader
        ADD_TASK(BTASK_WALK, NULL);
        ADD_TASK(BTASK_WAIT_LEADER, NULL);
        ADD_TASK(BTASK_SNEAK, NULL);
        ADD_TASK(BTASK_TACTICAL_COVER, NULL);
    }
    else {
        if ( !pSoldier->IsCleaningBuilding() ) {
            CBaseEntity *pEntrance = GetDataMemoryEntity("BuildingEntrance");

            ADD_TASK(BTASK_SET_TOLERANCE, RandomFloat(400.0f, 480.0f));
            ADD_TASK(BTASK_MOVE_DESTINATION, pEntrance);

            ADD_TASK(BTASK_SNEAK, NULL);
            ADD_TASK(BTASK_SET_TOLERANCE, RandomFloat(250.0f, 380.0f));
            ADD_TASK(BTASK_MOVE_DESTINATION, pEntrance);

            ADD_TASK(BTASK_CROUCH, NULL);
            ADD_TASK(BTASK_MOVE_DESTINATION, pEntrance);
            ADD_TASK(BTASK_PLAY_GESTURE, ACT_SIGNAL_HALT);
            ADD_TASK(BTASK_WAIT, RandomFloat(2.0f, 5.0f));
        }

        ADD_TASK(BTASK_STANDUP, NULL);
        ADD_TASK(BTASK_CLEAN_AREA, NULL);
    }
}

SET_SCHEDULE_INTERRUPTS(CCleanBuildingSchedule)
{
    ADD_INTERRUPT(BCOND_NEW_ENEMY);
    ADD_INTERRUPT(BCOND_SEE_ENEMY);
    ADD_INTERRUPT(BCOND_SEE_FEAR);
    ADD_INTERRUPT(BCOND_LIGHT_DAMAGE);
    ADD_INTERRUPT(BCOND_HEAVY_DAMAGE);
    ADD_INTERRUPT(BCOND_LOW_HEALTH);
    ADD_INTERRUPT(BCOND_DEJECTED);
    ADD_INTERRUPT(BCOND_BETTER_WEAPON_AVAILABLE);
}

//================================================================================
//================================================================================
CCleanBuildingSchedule::CCleanBuildingSchedule(IBot *bot) : BaseClass(bot)
{
     //m_pHint = NULL;
}

//================================================================================
//================================================================================
float CCleanBuildingSchedule::GetDesire() const
{
    CAP_Bot *pSoldier = dynamic_cast<CAP_Bot *> (GetBot());
    AssertOnce( pSoldier );

    if ( !pSoldier )
        return BOT_DESIRE_NONE;

    if ( !GetDecision()->CanMove() )
        return BOT_DESIRE_NONE;

    if ( IsCombating() || IsAlerted() )
        return BOT_DESIRE_NONE;

    if ( GetBot()->GetSquad() && !GetBot()->IsSquadLeader() ) {
        // My leader has ordered that we must clean a building, I follow it loyally.
        if ( pSoldier->IsMinionCleaningBuilding() )
            return 0.28f;

        return BOT_DESIRE_NONE;
    }

    CBaseEntity *pEntrance = GetDataMemoryEntity("BuildingEntrance");

    // We found an uncleaned building
    if ( pEntrance )
        return 0.28f;

    return BOT_DESIRE_NONE;
}

//================================================================================
//================================================================================
void CCleanBuildingSchedule::Start()
{
    BaseClass::Start();

    if ( GetBot()->IsSquadLeader() ) {
        CAI_Hint *pEntrance = dynamic_cast<CAI_Hint*>(GetDataMemoryEntity("BuildingEntrance"));
        Assert(pEntrance);

        ((CAP_Bot *)GetBot())->StartBuildingClean(pEntrance);
    }
}

//================================================================================
//================================================================================
void CCleanBuildingSchedule::Finish()
{
    BaseClass::Finish();
}

//================================================================================
//================================================================================
void CCleanBuildingSchedule::Update()
{
    BaseClass::Update();

    CAP_Bot *pSoldier = (CAP_Bot *)GetBot();

    if ( GetBot()->IsSquadLeader() ) {
        CBuildingInfo *info = pSoldier->GetBuildingInfo();
        
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

//================================================================================
//================================================================================
void CCleanBuildingSchedule::LookAround()
{
    if ( !GetVision() || !GetMemory() )
        return;

    // We block aiming to other sides a moment
    GetMemory()->UpdateDataMemory("BlockLookAround", 1, 2.0f);

    CSpotCriteria criteria;
    criteria.SetMaxRange( 700.0f );
    criteria.SetPlayer( GetHost() );
    criteria.SetTacticalMode( GetBot()->GetTacticalMode() );
    criteria.SetFlags(FLAG_USE_NEAREST | FLAG_IGNORE_RESERVED | FLAG_INTERESTING_SPOT);

    Vector vecSpot;
    vecSpot.Invalidate();

    SpotVector list;
    Utils::GetSpotCriteria(NULL, criteria, &list);

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

    GetVision()->LookAt( "Tactical Spot", vecSpot, PRIORITY_HIGH, 1.0f );
}

//================================================================================
//================================================================================
void CCleanBuildingSchedule::TaskStart()
{
    BotTaskInfo_t *pTask = GetActiveTask();
    CAP_Bot *pSoldier = (CAP_Bot *)GetBot();

    CBuildingInfo *info = pSoldier->GetBuildingInfo();
    Assert( info );

    if ( !info ) {
        Fail("Trying to clean a building without knowing which");
        return;
    }

    switch ( pTask->task ) {
        case BTASK_CLEAN_AREA:
        {
            // We have examined all areas of this building
            if ( info->areas.Count() == 0 ) {
                pSoldier->FinishBuildingClean();
                GetHost()->DoAnimationEvent( PLAYERANIMEVENT_CUSTOM_GESTURE, ACT_SIGNAL_GROUP );

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
                pSoldier->FinishBuildingClean();
                GetHost()->DoAnimationEvent( PLAYERANIMEVENT_CUSTOM_GESTURE, ACT_SIGNAL_GROUP );

                TaskComplete();
                return;
            }

            info->areas.Remove( key );
            pSoldier->SetScanningArea( pArea );

            ADD_TASK(BTASK_SNEAK, NULL);
            ADD_TASK( BTASK_PLAY_GESTURE, ACT_SIGNAL_FORWARD );
            ADD_TASK( BTASK_MOVE_DESTINATION, pArea->GetCenter() );
            ADD_TASK( BTASK_CROUCH, NULL );
            ADD_TASK( BTASK_TACTICAL_LOOK_AROUND, NULL );
            ADD_TASK( BTASK_WAIT, RandomFloat( 3.0f, 7.0f ) );
            ADD_TASK( BTASK_STANDUP, NULL );
            ADD_TASK( BTASK_CLEAN_AREA, NULL );

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
            GetFollow()->Enable();
            break;
        }

        case BTASK_TACTICAL_COVER:
        {
            GetFollow()->Disable();
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
void CCleanBuildingSchedule::TaskRun()
{
    CAP_Bot *pSoldier = dynamic_cast<CAP_Bot *>(GetBot());
    BotTaskInfo_t *pTask = GetActiveTask();

    CPlayer *pLeader = GetBot()->GetSquadLeader();
    CAP_Bot *pSoldierLeader = dynamic_cast<CAP_Bot *>(pLeader->GetBotController());

    CBuildingInfo *info = pSoldier->GetBuildingInfo();
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
            if ( !pSoldier->IsMinionCleaningBuilding() ) {
                TaskComplete();
                return;
            }

            CSpotCriteria criteria;
            criteria.SetMaxRange( 500.0f );
            criteria.SetPlayer( pLeader );
            criteria.AvoidTeam( GetHost()->GetTeamNumber() );
            criteria.SetMinRangeFromAvoid( 80.0f );
            criteria.SetFlags(FLAG_USE_NEAREST | FLAG_COVER_SPOT);

            SpotVector list;
            Utils::GetSpotCriteria(NULL, criteria, &list);

            Assert( list.Count() > 0 );

            if ( list.Count() == 0 )
                return;

            Vector vecLocation;

            FOR_EACH_VEC( list, it )
            {
                if ( GetBot()->GetSquad()->IsSomeoneGoing( list[it], GetHost() ) )
                    continue;

                vecLocation = list[it];
                break;
            }

            Assert(vecLocation.IsValid() );

            if ( !vecLocation.IsValid() )
                return;

            float flDistance = GetAbsOrigin().DistTo(vecLocation);
            float flTolerance = GetLocomotion()->GetDistanceToDestination();

            if ( flDistance <= flTolerance ) {
                GetLocomotion()->Crouch();
                return;
            }

            GetLocomotion()->DriveTo("Tactical Cover", vecLocation, PRIORITY_HIGH );
            break;
        }

        default:
        {
            BaseClass::TaskRun();
            break;
        }
    }
}