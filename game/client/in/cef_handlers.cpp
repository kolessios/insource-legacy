//==== Woots 2017. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "cef_basepanel.h"

#include "in_shareddefs.h"
#include "in_gamerules.h"

#include "inputsystem/iinputsystem.h"
#include "steam\steam_api.h"

#include "gameui_interface.h"
#include "engine\IEngineSound.h"
#include "soundsystem\isoundsystem.h"

#include <vgui_controls/Controls.h>
#include <vgui/ILocalize.h>

#ifdef GAMEUI_CEF

#include <cef/tests/shared/browser/client_app_browser.h>
#include <cef/tests/shared/browser/main_message_loop_external_pump.h>
#include <cef/tests/shared/browser/main_message_loop_std.h>
#include <cef/tests/shared/common/client_app_other.h>
#include <cef/tests/shared/common/client_switches.h>
#include <cef/tests/shared/renderer/client_app_renderer.h>

extern CUtlVector<char *> g_QueueCommands;

//================================================================================
// JavaScript nos pide ejecutar una función
//================================================================================
bool CDefaultV8Handler::Execute( const CefString & name, CefRefPtr<CefV8Value> object, const CefV8ValueList & arguments, CefRefPtr<CefV8Value>& retval, CefString & exception )
{
    VPROF_BUDGET( __FUNCTION__, VPROF_BUDGETGROUP_CEF );

    // Emitir un sonido
    if ( name == "emitSound" )
    {
        if ( arguments.size() != 1 )
        {
            Warning( "[GameUI] engine.emitSound: Invalid arguments \n" );
            return false;
        }

        if ( !arguments[0]->IsString() )
        {
            Warning( "[GameUI] engine.emitSound: Invalid argument.\n" );
            return false;
        }

        std::string string = arguments[0]->GetStringValue();
        const char *pSoundName = string.c_str();

        CSoundParameters params;

        if ( !soundemitterbase->GetParametersForSound( pSoundName, params, GENDER_NONE ) )
        {
            Warning( "[GameUI] engine.emitSound: Invalid sound (%s) \n", pSoundName );
            return false;
        }

        enginesound->EmitAmbientSound( params.soundname, params.volume, params.pitch );

        retval = CefV8Value::CreateInt( enginesound->GetGuidForLastSoundEmitted() );

        DevMsg( "[GameUI] engine.emitSound: %s \n", pSoundName );
        return true;
    }

    // Emitir un sonido
    if ( name == "stopSound" )
    {
        if ( arguments.size() != 1 )
        {
            Warning( "[GameUI] engine.stopSound: Invalid arguments \n" );
            return false;
        }

        if ( !arguments[0]->IsInt() )
        {
            Warning( "[GameUI] engine.stopSound: Invalid argument.\n" );
            return false;
        }

        enginesound->StopSoundByGuid( arguments[0]->GetIntValue() );
        return true;
    }

    // Ejecutar un comando en el motor
    if ( name == "command" )
    {
        if ( arguments.size() != 1 )
        {
            Warning( "[GameUI] engine.command: Invalid arguments \n" );
            return false;
        }

        if ( !arguments[0]->IsString() )
        {
            Warning( "[GameUI] engine.command: Invalid argument.\n" );
            return false;
        }

        std::string string = arguments[0]->GetStringValue();
        const char *pCommand = string.c_str();

        if ( !ThreadInMainThread() )
        {
            //Warning( "[GameUI] engine.command: Not in Main Thread.\n" );
            
            g_QueueCommands.AddToTail( strdup(pCommand) );
            DevMsg( "[GameUI] engine.command: Queue: %s\n", pCommand );

            return true;
        }      

        DevMsg( "[GameUI] engine.command: %s\n", pCommand );
        engine->ExecuteClientCmd( pCommand );


        return true;
    }

    // Devuelve el valor de un comando
    if ( name == "getCommand" )
    {
        if ( arguments.size() != 1 )
        {
            Warning( "[GameUI] engine.getCommand: Invalid arguments \n" );
            return false;
        }

        if ( !arguments[0]->IsString() )
        {
            Warning( "[GameUI] engine.getCommand: Invalid argument.\n" );
            return false;
        }

        std::string string = arguments[0]->GetStringValue();
        const char *pCommand = string.c_str();

        ConVarRef command( pCommand );

        if ( !command.IsValid() )
            return false;

        CefString value;
        value.FromASCII( command.GetString() );

        retval = CefV8Value::CreateString( value );
        DevMsg( "[GameUI] engine.getCommand: %s\n", pCommand );

        return true;
    }

    // Devuelve el valor de un comando
    if ( name == "translate" )
    {
        if ( arguments.size() != 1 )
        {
            Warning( "[GameUI] engine.translation: Invalid arguments \n" );
            return false;
        }

        if ( !arguments[0]->IsString() )
        {
            Warning( "[GameUI] engine.translation: Invalid argument.\n" );
            return false;
        }

        g_pVGuiLocalize->AddFile( "Resource/gameui_%language%.txt", "GAME", true );

        std::string string = arguments[0]->GetStringValue();
        const char *pString = string.c_str();

        // Tratamos de traducir el mensaje
        LocalizeStringIndex_t index = g_pVGuiLocalize->FindIndex( pString );

        // Sin traducción
        if ( index == LOCALIZE_INVALID_STRING_INDEX )
        {
            retval = CefV8Value::CreateString( pString );
            return true;
        }

        const wchar_t *translated = g_pVGuiLocalize->GetValueByIndex( index );
        WCHAR_TO_STRING( translated );

        retval = CefV8Value::CreateString( cc_translated );
        return true;
    }

    // Mostrar un mensaje en el motor
    if ( name == "log" )
    {
        Msg( "%s\n", arguments[0]->GetStringValue().ToString().c_str() );
        return true;
    }

    return false;
}

//================================================================================
// Devuelve el valor de una variable en el objeto Game
//================================================================================
bool CGameV8::Get( const CefString & name, const CefRefPtr<CefV8Value> object, CefRefPtr<CefV8Value>& retval, CefString & exception )
{
    VPROF_BUDGET( __FUNCTION__, VPROF_BUDGETGROUP_CEF );

    // Modo de juego
    if ( name == "gameMode" )
    {
        if ( TheGameRules )
            retval = CefV8Value::CreateInt( TheGameRules->GetGameMode() );
        else
            retval = CefV8Value::CreateInt( GAME_MODE_NONE );

        return true;
    }

    // ¿Esta jugando?
    if ( name == "inGame" )
    {
        retval = CefV8Value::CreateBool( engine->IsInGame() );
        return true;
    }

    // ¿Esta pausado?
    if ( name == "isPaused" )
    {
        retval = CefV8Value::CreateBool( engine->IsPaused() );
        return true;
    }

    // Limite de jugadores
    if ( name == "maxClients" )
    {
        retval = CefV8Value::CreateInt( engine->GetMaxClients() );
        return true;
    }

    return false;
}

//================================================================================
// Devuelve el valor de una variable en el objeto Steam
//================================================================================
bool CSteamV8::Get( const CefString & name, const CefRefPtr<CefV8Value> object, CefRefPtr<CefV8Value>& retval, CefString & exception )
{
    VPROF_BUDGET( __FUNCTION__, VPROF_BUDGETGROUP_CEF );

    if ( !steamapicontext || !steamapicontext->SteamFriends() || !steamapicontext->SteamUtils() )
        return false;

    // ID de Steam
    if ( name == "playerID" )
    {
        // Intentamos consultar a Steam por el ID
        if ( steamapicontext->SteamUser() )
        {
            CSteamID steamID = steamapicontext->SteamUser()->GetSteamID();
            uint64 mySteamID = steamID.ConvertToUint64();
            m_pSteamID = VarArgs( "%llu", mySteamID );
        }

        CefString steamid;

        if ( m_pSteamID )
            steamid.FromASCII( m_pSteamID );
        else
            steamid.FromASCII( "Unknown" );

        retval = CefV8Value::CreateString( steamid );
        return true;
    }

    // Nombre de jugador
    if ( name == "playerName" )
    {
        CefString playername;
        playername.FromASCII( steamapicontext->SteamFriends()->GetPersonaName() );

        retval = CefV8Value::CreateString( playername );
        return true;
    }

    // Estado del jugador
    if ( name == "playerState" )
    {
        retval = CefV8Value::CreateInt( (int)steamapicontext->SteamFriends()->GetPersonaState() );
        return true;
    }

    // Tiempo real
    if ( name == "serverTime" )
    {
        uint32 serverTime = steamapicontext->SteamUtils()->GetServerRealTime();

        retval = CefV8Value::CreateInt( (int)serverTime );
        return true;
    }

    // País
    if ( name == "country" )
    {
        CefString country;
        country.FromASCII( steamapicontext->SteamUtils()->GetIPCountry() );

        retval = CefV8Value::CreateString( country );
        return true;
    }

    // Nivel de bateria
    if ( name == "batteryPower" )
    {
        uint8 battery = steamapicontext->SteamUtils()->GetCurrentBatteryPower();

        retval = CefV8Value::CreateInt( (int)battery );
        return true;
    }

    // Segundos inactivo
    if ( name == "secondsSinceActive" )
    {
        uint32 computer = steamapicontext->SteamUtils()->GetSecondsSinceComputerActive();

        retval = CefV8Value::CreateInt( (int)computer );
        return true;
    }

    return false;
}

//================================================================================
// Devuelve el valor de una variable en el objeto Player
//================================================================================
bool CPlayerV8::Get( const CefString & name, const CefRefPtr<CefV8Value> object, CefRefPtr<CefV8Value>& retval, CefString & exception )
{
    VPROF_BUDGET( __FUNCTION__, VPROF_BUDGETGROUP_CEF );

    C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

    if ( !pPlayer )
        return false;

    // Salud
    if ( name == "health" )
    {
        retval = CefV8Value::CreateInt( pPlayer->GetHealth() );
        return true;
    }

    return false;
}

#endif // GAMEUI_CEF