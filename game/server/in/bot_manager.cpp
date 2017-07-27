//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "bot_manager.h"

#include "bot.h"
#include "in_utils.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CBotManager g_BotManager;
CBotManager *TheBots = &g_BotManager;

//================================================================================
// Pensamiento de los Bots
// Se debe ejecutar después de procesar el movimiento de los jugadores 
// (gameinterface.cpp:GameFrame)
//================================================================================
void BotThink()
{
    FOR_EACH_BOT( {
        pBot->Think();
    } );
}

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
    // Eliminamos a todos los bots
    engine->ServerCommand("bot_kick\n");
}

//================================================================================
//================================================================================
void CBotManager::FrameUpdatePreEntityThink()
{

}

//================================================================================
//================================================================================
void CBotManager::FrameUpdatePostEntityThink()
{
    VPROF_BUDGET("CBotManager::FrameUpdatePostEntityThink", VPROF_BUDGETGROUP_BOTS);

    // Contamos los Bots activos
    m_iActive = 0;
    FOR_EACH_BOT({
        ++m_iActive;
    });
}