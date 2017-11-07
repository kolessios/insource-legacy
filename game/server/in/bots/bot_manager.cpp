//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#include "cbase.h"
#include "bots\bot_manager.h"

#include "bots\bot.h"

#ifdef INSOURCE_DLL
#include "in_utils.h"
#else
#include "bots\in_utils.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CBotManager g_BotManager;
CBotManager *TheBots = &g_BotManager;

void Bot_RunAll() {
    for ( int it = 0; it <= gpGlobals->maxClients; ++it ) {
        CPlayer *pPlayer = ToInPlayer( UTIL_PlayerByIndex(it) );

        if ( !pPlayer )
            continue;

        if ( !pPlayer->GetBotController() )
            continue;

        pPlayer->GetBotController()->Update();
    }
}

//================================================================================
//================================================================================
CBotManager::CBotManager() : CAutoGameSystemPerFrame("BotManager")
{
}

//================================================================================
//================================================================================
bool CBotManager::Init()
{
    Utils::InitBotTrig();
    return true;
}

//================================================================================
//================================================================================
void CBotManager::LevelInitPostEntity()
{
    
}

//================================================================================
//================================================================================
void CBotManager::LevelShutdownPreEntity()
{
#ifdef INSOURCE_DLL
    for ( int it = 0; it <= gpGlobals->maxClients; ++it ) {
        CPlayer *pPlayer = ToInPlayer( UTIL_PlayerByIndex( it ) );

        if ( !pPlayer )
            continue;

        if ( !pPlayer->IsBot() )
            continue;

        pPlayer->Kick();
    }

    engine->ServerExecute();
#endif
}

//================================================================================
//================================================================================
void CBotManager::FrameUpdatePreEntityThink()
{
#ifdef INSOURCE_DLL
    Bot_RunAll();
#endif
}

//================================================================================
//================================================================================
void CBotManager::FrameUpdatePostEntityThink()
{

}

//================================================================================
//================================================================================
bool CBotManager::IsSpotReserved(const Vector & vecSpot, CPlayer * pPlayer) const
{
    if ( !pPlayer ) {
        return IsSpotReserved(vecSpot, TEAM_UNASSIGNED);
    }

    return IsSpotReserved(vecSpot, pPlayer->GetTeamNumber(), pPlayer);
}

//================================================================================
//================================================================================
bool CBotManager::IsSpotReserved(const Vector & vecSpot, int team, CPlayer *pIgnore) const
{
    const float tolerance = 70.0f;

    for ( int it = 0; it <= gpGlobals->maxClients; ++it ) {
        CPlayer *pPlayer = ToInPlayer(UTIL_PlayerByIndex(it));

        if ( !pPlayer )
            continue;

        if ( pPlayer == pIgnore )
            continue;

        if ( team != TEAM_UNASSIGNED && pPlayer->GetTeamNumber() != team )
            continue;

        if ( !pPlayer->GetBotController() )
            continue;

        IBot *pBot = pPlayer->GetBotController();

        if ( !pBot->GetMemory() )
            continue;

        CDataMemory *memory = pBot->GetMemory()->GetDataMemory("ReservedSpot");

        if ( !memory )
            continue;

        if ( memory->GetVector() == vecSpot || memory->GetVector().DistTo(vecSpot) <= tolerance ) {
            return true;
        }
    }

    return false;
}
