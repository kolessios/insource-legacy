//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#include "cbase.h"
#include "bots\bot_manager.h"

#include "bots\bot.h"
#include "in_utils.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CBotManager g_BotManager;
CBotManager *TheBots = &g_BotManager;

//================================================================================
// Comandos
//================================================================================


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
    for ( int it = 0; it <= gpGlobals->maxClients; ++it ) {
        CPlayer *pPlayer = ToInPlayer( it );

        if ( !pPlayer )
            continue;

        if ( !pPlayer->IsBot() )
            continue;

        pPlayer->Kick();
    }

    engine->ServerExecute();
}

//================================================================================
//================================================================================
void CBotManager::FrameUpdatePreEntityThink()
{
    for ( int it = 0; it <= gpGlobals->maxClients; ++it ) {
        CPlayer *pPlayer = ToInPlayer( it );

        if ( !pPlayer )
            continue;

        if ( !pPlayer->IsBot() )
            continue;

        pPlayer->GetAI()->Think();
    }
}

//================================================================================
//================================================================================
void CBotManager::FrameUpdatePostEntityThink()
{

}