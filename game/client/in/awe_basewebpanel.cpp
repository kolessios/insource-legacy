//==== Woots 2017. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "awe_basewebpanel.h"

#include "in_shareddefs.h"
#include "in_gamerules.h"

#include "steam\steam_api.h"

#include "gameui_interface.h"
#include "engine\IEngineSound.h"
#include "soundsystem\isoundsystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef GAMEUI_AWESOMIUM

using namespace Awesomium;

DECLARE_CHEAT_COMMAND( cl_web_ui_updaterate, "0.5", "" )

//================================================================================
//================================================================================
CBaseWebPanel::CBaseWebPanel( Panel *parent, const char *panelName ) : BaseClass(parent, panelName) 
{
    m_nThread = NULL;
    m_pSteamID = NULL;
    m_pPersonaName = NULL;
    m_iPersonaState = k_EPersonaStateOffline;

    m_flNextThink = gpGlobals->curtime;
}

//================================================================================
//================================================================================
CBaseWebPanel::CBaseWebPanel( vgui::VPANEL parent, const char *panelName ) : BaseClass(NULL, panelName)
{
    m_nThread = NULL;
    m_pSteamID = NULL;
    m_pPersonaName = NULL;
    m_iPersonaState = k_EPersonaStateOffline;

    m_flNextThink = gpGlobals->curtime;
    SetParent( parent );
}

//================================================================================
// Destructor
//================================================================================
CBaseWebPanel::~CBaseWebPanel() 
{
    // Liberamos el hilo de actualización
    if ( m_nThread )
        ReleaseThreadHandle( m_nThread );
}

//================================================================================
// Abre un archivo local en el navegador web
//================================================================================
void CBaseWebPanel::OpenFile( const char *filepath ) 
{
    OpenURL( VarArgs("file://%s\\%s", engine->GetGameDirectory(), filepath) );
    DevMsg("CBaseWebPanel::OpenFile: %s \n", VarArgs("file://%s\\%s", engine->GetGameDirectory(), filepath));
}

//================================================================================
//================================================================================
void CBaseWebPanel::Think() 
{
    if ( m_flNextThink <= gpGlobals->curtime )
    {
        Update();
        BaseClass::Think();
    }
}

//================================================================================
//================================================================================
unsigned CBaseWebPanel::ThreadUpdate( void *instance ) 
{
    CBaseWebPanel * RESTRICT pPanel = reinterpret_cast<CBaseWebPanel *>(instance);
    Assert( pPanel );

    if ( !pPanel )
        return 0;

    while ( true )
    {
        ThreadSleep( cl_web_ui_updaterate.GetFloat() );

        if ( pPanel->m_bThinking )
        {
            try
            {
                if ( pPanel->ShouldUpdate() )
                    pPanel->Update();
            }
            catch( ... ) 
            {
                Warning("Exception\n");
            }
        }
    }

    return 0;
}

//================================================================================
// Devuelve si debemos actualizar las variables JavaScript
//================================================================================
bool CBaseWebPanel::ShouldUpdate() 
{
    return ShouldPaint();
}

//================================================================================
//================================================================================
void CBaseWebPanel::Update() 
{
    VPROF_BUDGET( "CBaseWebPanel::Update", VPROF_BUDGETGROUP_AWESOMIUM );

     // Variables de Game
    if ( TheGameRules )
        SET_PROPERTY( m_GameObject, "gameMode", TheGameRules->GetGameMode() );

    SET_PROPERTY( m_GameObject, "inGame",       engine->IsInGame() );
    SET_PROPERTY( m_GameObject, "isPaused",     engine->IsPaused() );
    SET_PROPERTY( m_GameObject, "maxClients",   engine->GetMaxClients() );
    //SET_PROPERTY( m_GameObject, "isLoading", GameUI().m_bIsLoading );

    // Variables de Steam
    if ( IsPC() && steamapicontext )
    {
        // Intentamos cargar el ID de Steam del jugador
        if ( !m_pSteamID )
        {
            if ( steamapicontext->SteamUser() )
            {
                CSteamID steamID	= steamapicontext->SteamUser()->GetSteamID();
				uint64 mySteamID	= steamID.ConvertToUint64();
				m_pSteamID			= VarArgs("%llu", mySteamID);
            }

            if ( m_pSteamID )
            {
                SET_PROPERTY( m_SteamObject, "playerID", WSLit(m_pSteamID) );
                ExecuteJavaScript("$(document).trigger('player_loaded');");
            }
        }
        else
        {
            SET_PROPERTY( m_SteamObject, "playerID", WSLit(m_pSteamID) );
        }

        if ( steamapicontext->SteamFriends() )
        {
            if ( !m_pPersonaName )
            {
                m_pPersonaName = steamapicontext->SteamFriends()->GetPersonaName();
            }

            SET_PROPERTY( m_SteamObject, "playerName",  WSLit(m_pPersonaName) );
            SET_PROPERTY( m_SteamObject, "playerState", steamapicontext->SteamFriends()->GetPersonaState() );
        }

        if ( steamapicontext->SteamUtils() )
        {
            uint32 serverTime	= steamapicontext->SteamUtils()->GetServerRealTime();
			const char *country = steamapicontext->SteamUtils()->GetIPCountry();
			uint8 battery		= steamapicontext->SteamUtils()->GetCurrentBatteryPower();
			uint32 computer		= steamapicontext->SteamUtils()->GetSecondsSinceComputerActive();

            SET_PROPERTY( m_SteamObject, "serverTime",          (int)serverTime );
            SET_PROPERTY( m_SteamObject, "country",             WSLit(country) );
            SET_PROPERTY( m_SteamObject, "batteryPower",        (int)battery );
            SET_PROPERTY( m_SteamObject, "secondsSinceActive",  (int)computer );
        }
    }

    C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

    // Estamos en una partida
    if ( pPlayer )
    {
        C_BaseCombatWeapon *pWeapon = pPlayer->GetActiveWeapon();

        // Variables de Player
        SET_PROPERTY( m_PlayerObject, "health", pPlayer->GetHealth() );

        // Variables de Weapon
        if ( pWeapon )
        {
            SET_PROPERTY( m_WeaponObject, "active",     WSLit(pWeapon->GetClassname()) );
            SET_PROPERTY( m_WeaponObject, "clip1",      pWeapon->Clip1() );
            SET_PROPERTY( m_WeaponObject, "clip1Ammo",  pPlayer->GetAmmoCount(pWeapon->GetPrimaryAmmoType()) );
        }
        else
        {
            SET_PROPERTY( m_WeaponObject, "active", NULL );
            SET_PROPERTY( m_WeaponObject, "clip1", 0 );
            SET_PROPERTY( m_WeaponObject, "clip1Ammo", 0 );
        }        
    }

    // Pensamiento en JavaScript
    ExecuteJavaScript("$(document).trigger('think');");

    m_flNextThink = gpGlobals->curtime + cl_web_ui_updaterate.GetFloat();
}

//================================================================================
// El documento se ha cargado y esta listo
//================================================================================
void CBaseWebPanel::OnDocumentReady( Awesomium::WebView *caller, const Awesomium::WebURL & url ) 
{
    m_pSteamID = NULL;

    JSValue Game    = caller->CreateGlobalJavascriptObject( WSLit("Game") );
	JSValue Steam	= caller->CreateGlobalJavascriptObject( WSLit("Steam") );
	JSValue Player	= caller->CreateGlobalJavascriptObject( WSLit("Player") );
    JSValue Weapon	= caller->CreateGlobalJavascriptObject( WSLit("Weapon") );

    m_GameObject   = Game.ToObject();
    m_SteamObject  = Steam.ToObject();
    m_PlayerObject = Player.ToObject();
    m_WeaponObject = Weapon.ToObject();

    // Métodos de Game
    SET_METHOD( m_GameObject, "emitSound", false );
    SET_METHOD( m_GameObject, "runCommand", false );
    SET_METHOD( m_GameObject, "reload", false );
    SET_METHOD( m_GameObject, "log", false );
    SET_METHOD( m_GameObject, "convar", true );

    // Variables de Game
    SET_PROPERTY( m_GameObject, "gameMode", GAME_MODE_NONE );
    SET_PROPERTY( m_GameObject, "inGame", false );
    SET_PROPERTY( m_GameObject, "isPaused", false );
    SET_PROPERTY( m_GameObject, "maxClients", 32 );
    SET_PROPERTY( m_GameObject, "isLoaded", false );

    // Variables de Steam
    SET_PROPERTY( m_SteamObject, "playerID", 0 );
    SET_PROPERTY( m_SteamObject, "playerName", WSLit("") );
    SET_PROPERTY( m_SteamObject, "playerState", k_EPersonaStateOffline );
    SET_PROPERTY( m_SteamObject, "serverTime", 0 );
    SET_PROPERTY( m_SteamObject, "country", WSLit("") );
    SET_PROPERTY( m_SteamObject, "batteryPower", 255 );
    SET_PROPERTY( m_SteamObject, "secondsSinceActive", 0 );

    // Variables de Player
    SET_PROPERTY( m_PlayerObject, "health", 0 );
    SET_PROPERTY( m_PlayerObject, "info", 0 );

    // Variables de Weapon
    SET_PROPERTY( m_WeaponObject, "active", NULL );
    SET_PROPERTY( m_WeaponObject, "clip1", 0 );
    SET_PROPERTY( m_WeaponObject, "clip1Ammo", 0 );

    SET_PROPERTY( m_PlayerObject, "weapon", m_WeaponObject );

    // Si ya existe, lo liberamos
    if ( m_nThread )
        ReleaseThreadHandle( m_nThread );

    // Creamos el hilo que se encargara de actaulizar las propiedades JavaScript
    // con la información proporcionada por el juego y Steam
    //m_nThread = CreateSimpleThread( ThreadUpdate, this );

    BaseClass::OnDocumentReady( caller, url );
}

//================================================================================
// Se ha llamado a un método
//================================================================================
void CBaseWebPanel::OnMethodCall( Awesomium::WebView *caller, unsigned int remote_object_id, const Awesomium::WebString &method_name, const Awesomium::JSArray &args ) 
{
    DevMsg("[CBaseWebPanel::OnMethodCall] %i - %s(): ", remote_object_id, TO_STRING(method_name));

    // Game
    if ( m_GameObject.remote_id() == remote_object_id )
    {
        // Emitir un sonido
        if ( method_name == WSLit("emitSound") )
        {
            const char *pSoundName = TO_STRING( args[0].ToString() );

            CSoundParameters params;	    
            if ( !soundemitterbase->GetParametersForSound( pSoundName, params, GENDER_NONE ) )
		        return;

            enginesound->EmitAmbientSound( params.soundname, params.volume, params.pitch );
            m_iSoundGUID = enginesound->GetGuidForLastSoundEmitted();

            DevMsg("%s", pSoundName);
        }

        // Ejecutar un comando
        else if ( method_name == WSLit("runCommand") )
        {
            const char *pCommand = TO_STRING( args[0].ToString() );

            engine->ClientCmd_Unrestricted( pCommand );
            DevMsg("%s", pCommand);
        }

        // Recargar la página
        else if ( method_name == WSLit("reload") )
        {
            ExecuteJavaScript("document.location.reload();");
        }

        // Mensaje
        else if ( method_name == WSLit("log") )
        {
            DevMsg("\n");
            Msg("%s \n", TO_STRING(args[0].ToString()));
        }

        // Alerta
        else if ( method_name == WSLit("warning") )
        {
            DevMsg("\n");
            Warning("%s \n", TO_STRING(args[0].ToString()));
        }
    }

    DevMsg("\n");
    BaseClass::OnMethodCall( caller, remote_object_id, method_name, args );
}

//================================================================================
//================================================================================
Awesomium::JSValue CBaseWebPanel::OnMethodCallWithReturnValue( Awesomium::WebView * caller, unsigned int remote_object_id, const Awesomium::WebString & method_name, const Awesomium::JSArray & args ) 
{
    DevMsg("[CBaseWebPanel::OnMethodCallWithReturnValue] %i - %s() \n", remote_object_id, TO_STRING(method_name));

    // Game
    if ( m_GameObject.remote_id() == remote_object_id )
    {
        // Obtener el valor de un comando
        if ( method_name == WSLit("convar") )
        {
            const char *pCommand = TO_STRING( args[0].ToString() );

            ConVarRef command( pCommand );

            if ( !command.IsValid() )
                return JSValue(NULL);

            return JSValue( WSLit(command.GetString()) );
        }
    }

    return BaseClass::OnMethodCallWithReturnValue( caller, remote_object_id, method_name, args );
}

#endif // GAMEUI_AWESOMIUM
