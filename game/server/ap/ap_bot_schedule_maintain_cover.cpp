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

#include "director.h"
#include "in_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define MAX_MAINTAIN_TIME (3.0f * 60.0f)

//================================================================================
//================================================================================
SET_SCHEDULE_TASKS(CMaintainCoverSchedule)
{
    //ADD_TASK(BTASK_SAVE_COVER_SPOT, 1300.0f);
    //ADD_TASK(BTASK_MOVE_DESTINATION, NULL);
    ADD_TASK(BTASK_MAINTAIN_COVER, NULL);
}

SET_SCHEDULE_INTERRUPTS(CMaintainCoverSchedule)
{
    ADD_INTERRUPT(BCOND_DEJECTED);
}

//================================================================================
//================================================================================
float CMaintainCoverSchedule::GetDesire() const
{
    VPROF_BUDGET("CMaintainCoverSchedule", VPROF_BUDGETGROUP_BOTS);

    if ( !GetDecision()->CanMove() )
        return BOT_DESIRE_NONE;

    if ( IsIdle() )
        return BOT_DESIRE_NONE;

    if ( GetFollow()->IsFollowingActive() )
        return BOT_DESIRE_NONE;

    if ( HasFailed() && GetElapsedTimeSinceFail() <= 15.0f )
        return BOT_DESIRE_NONE;

    if ( GetBot()->GetTacticalMode() != TACTICAL_MODE_DEFENSIVE ) {
        // I've been hiding a long time, let's go for the action!
        if ( HasStarted() && GetElapsedTime() >= MAX_MAINTAIN_TIME )
            return BOT_DESIRE_NONE;
    }

    // Assault Mode
    if ( TheGameRules->IsGameMode(GAME_MODE_ASSAULT) && TheDirector->IsOnPanicEvent() ) {
        //int maxUnits = TheDirector->GetMaxUnits(CHILD_TYPE_COMMON);
        int inCover = GetSoldiersInCover();

        // We do not want more than 5 of the military to be hiding
        if ( !HasStarted() && inCover >= 5 ) {
            return BOT_DESIRE_NONE;
        }
    }

    // One decimal above HUNT_ENEMY
    // In this way we will never chase the enemy
    return 0.66f;
}

//================================================================================
//================================================================================
int CMaintainCoverSchedule::GetSoldiersInCover() const
{
    int count = 0;

    for ( int it = 0; it <= gpGlobals->maxClients; ++it ) {
        CAP_Player *pPlayer = ToApPlayer(it);

        if ( !pPlayer )
            continue;

        if ( !pPlayer->IsSoldier() )
            continue;

        if ( !pPlayer->GetBotController() )
            continue;

        if ( pPlayer->GetBotController()->GetActiveScheduleID() == SCHEDULE_MAINTAIN_COVER ) {
            ++count;
        }
    }

    return count;
}

//================================================================================
//================================================================================
bool CMaintainCoverSchedule::ShouldCrouch() const
{
    CBaseCombatCharacter *pEnemy = ToBaseCombatCharacter(GetBot()->GetEnemy());

    if ( !pEnemy )
        return true;

    // Location of eyes when standing
    Vector vecEyes = GetHost()->GetAbsOrigin();
    vecEyes.z += VEC_VIEW.z;

    // We will not be able to see our enemy, we cover
    if ( !pEnemy->IsAbleToSee(vecEyes, CBaseCombatCharacter::DISREGARD_FOV) )
        return true;

    return false;
}

//================================================================================
//================================================================================
void CMaintainCoverSchedule::TaskStart()
{
    BotTaskInfo_t *pTask = GetActiveTask();

    switch ( pTask->task ) {
        case BTASK_MAINTAIN_COVER:
        {
            // We force the update to obtain positions with the new criteria.
            GetDecision()->UpdateCoverSpots();
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
void CMaintainCoverSchedule::TaskRun()
{
    BotTaskInfo_t *pTask = GetActiveTask();

    switch ( pTask->task ) {
        case BTASK_MAINTAIN_COVER:
        {
            // We must always be in a cover spot
            if ( !GetDecision()->IsInCoverPosition() ) {
                Vector vecGoal;

                if ( !GetDecision()->GetNearestCover(&vecGoal) ) {
                    Fail("No cover spot found");
                    return;
                }

                Assert(vecGoal.IsValid());
                GetLocomotion()->Run();
                GetLocomotion()->DriveTo("Moving to Cover", vecGoal, PRIORITY_HIGH);
                return;
            }

            GetLocomotion()->Walk();

            // Crouched or standing?
            if ( GetLocomotion() ) {
                if ( ShouldCrouch() ) {
                    GetLocomotion()->Crouch();
                }
                else {
                    GetLocomotion()->StandUp();
                }
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
