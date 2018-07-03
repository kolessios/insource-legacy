//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#include "cbase.h"
#include "ap_bot.h"

#include "in_gamerules.h"
#include "in_utils.h"
#include "players_system.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
//================================================================================
bool CAP_BotDecision::CanLookNoVisibleSpots() const
{
    // We remove the disadvantage of not being able to see interesting places without having vision
    return (TheGameRules->IsGameMode(GAME_MODE_SURVIVAL) || TheGameRules->IsGameMode(GAME_MODE_ASSAULT));
}

//================================================================================
//================================================================================
bool CAP_BotDecision::ShouldLookSquadMember() const
{
    // Apocalypse seeks to be realistic, 
    // and it is strange that they look at each other even if they are far away.
    return false;
}

//================================================================================
//================================================================================
bool CAP_BotDecision::ShouldOnlyFeelPlayers() const
{
    if ( TheGameRules->IsGameMode(GAME_MODE_ASSAULT) ) {
        return true;
    }

    return BaseClass::ShouldOnlyFeelPlayers();
}

//================================================================================
//================================================================================
float CAP_BotDecision::GetUpdateCoverRate() const
{
    if ( GetBot()->GetActiveScheduleID() == SCHEDULE_MAINTAIN_COVER )
        return 8.0f;

    return 1.0f;
}

//================================================================================
//================================================================================
void CAP_BotDecision::GetCoverCriteria(CSpotCriteria & criteria)
{
    BaseClass::GetCoverCriteria(criteria);

    if ( GetBot()->GetActiveScheduleID() == SCHEDULE_MAINTAIN_COVER ) {
        criteria.SetMaxRange(1800.0f);
        criteria.SetMinRangeFromAvoid(800.0f);
        criteria.SetOrigin(GetBot()->GetEnemy());
        criteria.ClearFlags(FLAG_USE_NEAREST);
    }
}
