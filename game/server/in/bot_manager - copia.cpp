//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "bot_manager.h"

#include "bot.h"
#include "in_utils.h"
#include "players_manager.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CBotManager g_BotManager;
CBotManager *TheBots = &g_BotManager;

//================================================================================
// Comandos
//================================================================================

DECLARE_REPLICATED_COMMAND( bot_threaded_think, "10", "Indica la cantidad de Bots que deben estar activos para procesar la I.A. de forma asincronica." );

//#define ShouldThreaded() m_iActive >= bot_threaded_think.GetInt()
#define ShouldThreaded() false // Ivan: Da más problemas que una buena solución al lag

//================================================================================
//================================================================================
CBotManager::CBotManager() : CAutoGameSystemPerFrame("BotManager")
{
}

//================================================================================
//================================================================================
bool CBotManager::Init()
{
    m_nThinkThread    = NULL;
    m_bThinking        = false;

    m_flManagerDuration = 0.0f;
    m_flThinkDuration = 0.0f;

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
    // Kickeamos a todos los Bots para evitarnos problemas
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

    CFastTimer timer;
    timer.Start();

    // Debemos activar el procesamiento asincronico.
    // -----------------
    // Iván: Lamentablemente el sistema para procesar el input y el movimiento de
    // los jugadores esta desarrollado de forma muy rara y "sincronica".
    // Básicamente estamos obligados a tener que pasar cada jugador por ese sistema y bloquear
    // la ejecución del código hasta que termine para después pasar al siguiente jugador. Es por eso que se causa
    // el terrible "lag" cuando hay muchos jugadores conectados.
    // Probablemente se pueda reescribir todo ese código pero ahora mismo tardaría mucho.
    // -----------------
    // Por ahora solo activamos esta función en el procesamiento de la I.A. del Bot y después
    // llamamos a la función (PlayerMove) para pasar el Input generado por el bot en el código sincronico.
    // Esto nos ahorrara algunos frames de trabajo, pero el lag seguira si hay demasiados bots/jugadores.
    // Tengo esperanza de que en Source 2 todo sea mucho mejor :)
    if ( ShouldThreaded() )
    {
        // Ya terminamos el procesamiento anterior
        if ( !m_bThinking )
        {
            // Creamos un nuevo proceso para procesar la I.A.
            ReleaseThreadHandle( m_nThinkThread );
            m_nThinkThread = CreateSimpleThread( ThinkThread, NULL );
            //ThreadSetAffinity( m_nThinkThread, XBOX_PROCESSOR_0 );
        }
    }
    else
    {
        // Procesamos la I.A.
        FOR_EACH_BOT({
            pBot->Think();

			pBot->PlayerMove();
        });

        TheBots->m_FrameTimer.Start();
    }

    // Llamamos a PlayerMove para
    // enviar el input generado
    /*FOR_EACH_BOT({
        pBot->PlayerMove(); // RIP frames
    });*/        
    
    timer.End();
    m_flManagerDuration = timer.GetDuration().GetMillisecondsF();
    
    // Esta función tardo más en ejecutarse que Think :'(
    if ( ShouldThreaded() && m_flManagerDuration > m_flThinkDuration )
        NDebugOverlay::ScreenText( 0.6f, 0.13f, UTIL_VarArgs("%.2f > %.2f \n", m_flManagerDuration, m_flThinkDuration), 255, 0, 155, 255, 0.1f );
}

//================================================================================
//================================================================================
unsigned CBotManager::ThinkThread( void *params )
{
    CFastTimer timer;
    timer.Start();

    // Procesamos la I.A.
    TheBots->m_bThinking = true;
    FOR_EACH_BOT(
    {
        try
        {
            pBot->Think();
        }
        catch( ... ) { }
    });
    TheBots->m_bThinking = false;

    timer.End();
    TheBots->m_flThinkDuration = timer.GetDuration().GetMillisecondsF();

    //NDebugOverlay::ScreenText( 0.6f, 0.16f, UTIL_VarArgs("Think: %.2f \n", TheBots->m_flThinkDuration), 255, 0, 155, 255, TheBots->GetDeltaT() );

    TheBots->m_FrameTimer.Start();
    return 0;
}