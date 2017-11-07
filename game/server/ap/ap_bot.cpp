//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#include "cbase.h"
#include "ap_bot.h"

#include "bots\bot_manager.h"

#include "ap_bot_schedules.h"
#include "in_gamerules.h"
#include "ai_hint.h"

#include "in_utils.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
// Create the components that the bot will have
//================================================================================
void CAP_Bot::SetUpComponents()
{
    ADD_COMPONENT(CBotVision);
    ADD_COMPONENT(CBotFollow);
    ADD_COMPONENT(CBotLocomotion);
    ADD_COMPONENT(CBotMemory);
    ADD_COMPONENT(CBotAttack);
    ADD_COMPONENT(CAP_BotDecision); // This component is mandatory!
}

//================================================================================
// Create the schedules that the bot will have
//================================================================================
void CAP_Bot::SetUpSchedules()
{
    ADD_COMPONENT(CHuntEnemySchedule);
    ADD_COMPONENT(CReloadSchedule);
    ADD_COMPONENT(CCoverSchedule);
    ADD_COMPONENT(CHideSchedule);
    ADD_COMPONENT(CChangeWeaponSchedule);
    ADD_COMPONENT(CHideAndHealSchedule);
    ADD_COMPONENT(CHideAndReloadSchedule);
    ADD_COMPONENT(CMoveAsideSchedule);
    ADD_COMPONENT(CCallBackupSchedule);
    ADD_COMPONENT(CDefendSpawnSchedule);
    ADD_COMPONENT(CHelpDejectedFriendSchedule);

    // Survival: We must look for resources to survive
    if ( TheGameRules->IsGameMode(GAME_MODE_SURVIVAL) ) {
        ADD_COMPONENT(CSearchResourcesSchedule);

        // Soldiers: If we see a building, we must clean up any threat
        if ( IsSoldier() ) {
            ADD_COMPONENT(CCleanBuildingSchedule);
        }
    }

    // Assault: Soldiers must protect their lands!
    if ( TheGameRules->IsGameMode(GAME_MODE_ASSAULT) ) {
        // Some soldiers must remain in a cover position
        if ( IsSoldier() ) {
            if ( GetPlayer()->GetPlayerClass() == PLAYER_CLASS_SOLDIER_LEVEL1 ) {
                ADD_COMPONENT(CMaintainCoverSchedule);
            }
        }
    }
}

//================================================================================
// Player Spawn
//================================================================================
void CAP_Bot::Spawn()
{
    BaseClass::Spawn();

    m_BuildingList.PurgeAndDeleteElements();
    m_pCleaningBuilding = NULL;
    m_pScanningArea = NULL;
}

//================================================================================
// Run the custom artificial intelligence of the Bot.
//================================================================================
void CAP_Bot::RunCustomAI()
{
    BaseClass::RunCustomAI();

    if ( IsSoldier() ) {
        if ( TheGameRules->IsGameMode(GAME_MODE_SURVIVAL) ) {
            FindBuilding();
        }
    }
}

//================================================================================
// 
//================================================================================
void CAP_Bot::FindBuilding()
{
    if ( !GetMemory() )
        return;

    CBaseEntity *pEntrance = GetDataMemoryEntity("BuildingEntrance");

    if ( pEntrance )
        return;

    CSpotCriteria criteria;
    criteria.SetMaxRange(GetHost()->GetSenses()->GetDistLook());
    criteria.SetPlayer(GetHost());
    criteria.SetTacticalMode(GetTacticalMode());
    criteria.SetFlags(FLAG_USE_NEAREST | FLAG_ONLY_VISIBLE);

    CHintCriteria hintCriteria;
    hintCriteria.AddHintType(HINT_TACTICAL_PINCH);
    hintCriteria.AddIncludePosition(criteria.GetOrigin(), criteria.GetMaxRange());

    CUtlVector<CAI_Hint *> collector;
    CAI_HintManager::FindAllHints(criteria.GetOrigin(), hintCriteria, &collector);

    FOR_EACH_VEC(collector, it)
    {
        CAI_Hint *pHint = collector[it];

        if ( !pHint )
            continue;

        Vector position = pHint->GetAbsOrigin();

        if ( !Utils::IsValidSpot(position, criteria) )
            continue;

        if ( HasBuildingCleaned(STRING(pHint->GetGroup())) )
            continue;

        GetMemory()->UpdateDataMemory("BuildingEntrance", pHint, 10.0f);
        break;
    }
}

//================================================================================
// Returns if the soldier is cleaning a building
//================================================================================
bool CAP_Bot::IsCleaningBuilding()
{
    if ( m_pCleaningBuilding )
        return true;

    return IsMinionCleaningBuilding();
}

//================================================================================
// Returns if the squad leader has ordered to clean a building
//================================================================================
bool CAP_Bot::IsMinionCleaningBuilding()
{
    CPlayer *pLeader = GetSquadLeader();

    if ( !pLeader )
        return false;

    if ( pLeader == GetHost() )
        return false;

    // TODO: Humans Leader
    if ( !pLeader->IsBot() )
        return false;

    return (pLeader->GetBotController()->GetActiveScheduleID() == SCHEDULE_CLEAN_BUILDING);
}

//================================================================================
// Order the soldier to clean a building
//================================================================================
void CAP_Bot::StartBuildingClean(CAI_Hint *pHint)
{
    const char *pGroupName = STRING(pHint->GetGroup());

    // This building is clean, that's what we remember
    if ( HasBuildingCleaned(pGroupName) ) {
        Assert(!"It has been ordered to clean a building that is already clean!");
        return;
    }

    CBuildingInfo *info = GetBuildingInfo(pGroupName);

    // We are already cleaning one, but we are ordered to clean another...
    if ( m_pCleaningBuilding && m_pCleaningBuilding != info ) {
        Assert(!"It has been ordered to clean a building, but I am already cleaning one!");
        FinishBuildingClean();
    }

    if ( !info ) {
        // I want this clean building, squad!
        m_pCleaningBuilding = new CBuildingInfo(pHint);
        m_BuildingList.AddToTail(m_pCleaningBuilding);
    }
    else {
        // Tango eliminated, we continue ...
        DebugAddMessage("Resuming Building Clean...");
        m_pCleaningBuilding = info;
    }
}

//================================================================================
// Order the cleaning of the building to be completed
//================================================================================
void CAP_Bot::FinishBuildingClean()
{
    Assert(m_pCleaningBuilding);

    // But we were not cleaning any buildings, huh?
    if ( !m_pCleaningBuilding )
        return;

    SetScanningArea(NULL);

    m_pCleaningBuilding->expired.Start(BUILDING_MEMORY_TIME);
    m_pCleaningBuilding = NULL;
}

//================================================================================
// Returns memory building information
//================================================================================
CBuildingInfo *CAP_Bot::GetBuildingInfo(const char * pName)
{
    if ( m_BuildingList.Count() == 0 )
        return NULL;

    FOR_EACH_VEC(m_BuildingList, it)
    {
        CBuildingInfo *info = m_BuildingList[it];
        Assert(info);

        if ( FStrEq(info->name, pName) )
            return info;
    }

    return NULL;
}

//================================================================================
// Returns the information of the building we are cleaning
//================================================================================
CBuildingInfo * CAP_Bot::GetBuildingInfo()
{
    if ( IsMinionCleaningBuilding() ) {
        CAP_Bot *pAI = dynamic_cast<CAP_Bot *>(GetSquadLeaderAI());
        return pAI->GetBuildingInfo();
    }

    return m_pCleaningBuilding;
}

//================================================================================
// Returns if the building is clean
//================================================================================
bool CAP_Bot::HasBuildingCleaned(const char * pName)
{
    CBuildingInfo *info = GetBuildingInfo(pName);

    if ( !info )
        return false;

    if ( info == m_pCleaningBuilding )
        return false;

    if ( !info->expired.HasStarted() )
        return false;

    return true;
}

//================================================================================
// Scan the navigation areas within the radius of the building.
//================================================================================
void CBuildingInfo::Scan()
{
    CHintCriteria hintCriteria;
    hintCriteria.SetHintType(HINT_TACTICAL_AREA);
    hintCriteria.SetGroup(entrance->GetGroup());

    CAI_Hint *pHint = CAI_HintManager::FindHint(entrance->GetAbsOrigin(), hintCriteria);
    Assert(pHint);

    if ( !pHint )
        return;

    float radius = pHint->GetRadius();

    NavAreaCollector collector;
    collector.m_area.EnsureCapacity(1000);
    TheNavMesh->ForAllAreasInRadius(collector, pHint->GetAbsOrigin(), radius);

    FOR_EACH_VEC(collector.m_area, it)
    {
        CNavArea *pArea = collector.m_area[it];

        // Possible place of resources, we must examine it
        if ( pArea->HasAttributes(NAV_MESH_RESOURCES) ) {
            areas.AddToTail(pArea);
            continue;
        }

        if ( pArea->IsUnderwater() )
            continue;

        if ( pArea->IsBlocked(TEAM_ANY) || pArea->HasAvoidanceObstacle() )
            continue;

        // Very small area, probably not important
        if ( pArea->GetSizeX() < 60.0f || pArea->GetSizeY() < 60.0f )
            continue;

        Vector vecFloor = pArea->GetCenter();
        vecFloor.z += 10.0f;
        vecFloor.z -= 300.0f;

        trace_t tr;
        UTIL_TraceLine(pArea->GetCenter(), vecFloor, MASK_SOLID, NULL, COLLISION_GROUP_NONE, &tr);

        // Invalid area
        if ( strstr(tr.surface.name, "TOOLS/") )
            continue;

        // Invalid area
        if ( strstr(tr.surface.name, "studio") )
            continue;

        areas.AddToTail(pArea);
    }

    Assert(areas.Count() > 0);
}
