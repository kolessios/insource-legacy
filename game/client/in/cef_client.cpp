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

#ifdef GAMEUI_CEF

#include <cef/tests/shared/browser/client_app_browser.h>
#include <cef/tests/shared/browser/main_message_loop_external_pump.h>
#include <cef/tests/shared/browser/main_message_loop_std.h>
#include <cef/tests/shared/common/client_app_other.h>
#include <cef/tests/shared/common/client_switches.h>
#include <cef/tests/shared/renderer/client_app_renderer.h>

extern ConVar cl_web_ui_fps;

extern CUtlVector<CefBasePanel *> g_CefPanels;

//================================================================================
//================================================================================
void SwapBufferFromBgraToRgba( void* _dest, const void* _src, int width, int height ) {
    int32_t* dest = (int32_t*)_dest;
    int32_t* src = (int32_t*)_src;
    int32_t rgba;
    int32_t bgra;
    int length = width*height;
    for ( int i = 0; i < length; i++ ) {
        bgra = src[i];
        // BGRA in hex = 0xAARRGGBB.
        rgba = (bgra & 0x00ff0000) >> 16 // Red >> Blue.
            | (bgra & 0xff00ff00) // Green Alpha.
            | (bgra & 0x000000ff) << 16; // Blue >> Red.
        dest[i] = rgba;
    }
}

//================================================================================
//================================================================================
unsigned UpdateCefThread( void *instance )
{
    VPROF_BUDGET( "UpdateCefThread", VPROF_BUDGETGROUP_CEF );

    // CCefClient que manejaremos
    CCefClient * RESTRICT client = reinterpret_cast<CCefClient *>(instance);

    // Ancho y altura del panel
    int width = client->GetPanel()->GetWide();
    int height = client->GetPanel()->GetTall();

    // Buffer correjido
    byte *buffer = new unsigned char[width * height * DEPTH];

    while ( true )
    {
        int framerate = client->GetPanel()->GetFramerate();

        // Dependiendo de la calidad
        ThreadSleep( (1000.0f / framerate) );

        // Establecemos la frecuencia de renderizado en CEF
        client->GetBrowser()->GetHost()->SetWindowlessFrameRate( framerate );

        // El buffer no esta preparado
        if ( !client->m_nBuffer )
            continue;
        
        // No agobiemos pintando lo que no se tiene que pintar
        if ( !client->GetPanel()->ShouldPaint() )
            continue;

        // La pantalla sigue igual
        if ( !client->NeedPaint() )
            continue;

        // El buffer de CEF esta en BGRA, debemos convertirlo a RGBA
        // http://www.magpcss.org/ceforum/viewtopic.php?f=6&t=11396#p19854
        SwapBufferFromBgraToRgba( buffer, client->m_nBuffer, width, height );

        // Copiamos el buffer a la textura
        vgui::surface()->DrawSetTextureRGBA( client->GetPanel()->m_iTextureId, buffer, width, height );

        //client->m_bNeedPaint = false;
    }

    return 0;
}

//================================================================================
//================================================================================
CCefApplication::CCefApplication()
{
}

//================================================================================
// Realiza ajustes antes de iniciar la aplicación
//================================================================================
void CCefApplication::OnBeforeCommandLineProcessing( const CefString & process_type, CefRefPtr<CefCommandLine> command_line )
{
    // Desactivando estos parámetros obtenemos una mejora de rendimiento
    // http://www.magpcss.org/ceforum/viewtopic.php?f=6&t=11953

    //CefString accelerated( L"disable-accelerated-compositing" );
    //command_line->AppendSwitch( accelerated );

    CefString gpu( L"disable-gpu" );
    command_line->AppendSwitch( gpu );    

    CefString gpu_compositing( L"disable-gpu-compositing" );
    command_line->AppendSwitch( gpu_compositing );

    CefString frame( L"enable-begin-frame-scheduling" );
    command_line->AppendSwitch( frame );

    CefString d3d11( L"disable-d3d11" );
    command_line->AppendSwitch( d3d11 );

    //CefString threaded( L"disable-threaded-compositing" );
    //command_line->AppendSwitch( threaded );

    CefString transparent( L"transparent-painting-enabled" );
    command_line->AppendSwitch( transparent );
}

//================================================================================
// El contexto de JavaScript ha sido creado.
// Declaramos objetos y variables necesarios globalmente y para cada panel
//================================================================================
void CCefApplication::OnContextCreated( CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context )
{
    // Handler predeterminado
    CefRefPtr<CefV8Handler> handler = new CDefaultV8Handler();

    // Objetos globales
    CefRefPtr<CefV8Value> global = context->GetGlobal();

    // Accessor
    DeclareAccessor( game, CGameV8 );
    DeclareAccessor( steam, CSteamV8 );
    DeclareAccessor( player, CPlayerV8 );


    // Creamos los objetos
    DeclareObject( engineObj, NULL );
    DeclareObject( gameObj, game );
    DeclareObject( steamObj, steam );
    DeclareObject( playerObj, player );
    DeclareObject( weaponObj, player );

    // Engine
    DeclareFunction( "emitSound" );
    DeclareFunction( "stopSound" );
    DeclareFunction( "command" );
    DeclareFunction( "log" );
    DeclareFunction( "getCommand" );
    DeclareFunction( "translate" );
    SetGlobalObject( "engine", engineObj );

    // Variables de Game
    DeclareProperty( gameObj, "gameMode" );
    DeclareProperty( gameObj, "inGame" );
    DeclareProperty( gameObj, "isPaused" );
    DeclareProperty( gameObj, "maxClients" );
    SetGlobalObject( "game", gameObj );

    // Variables de Steam
    DeclareProperty( steamObj, "playerID" );
    DeclareProperty( steamObj, "playerName" );
    DeclareProperty( steamObj, "playerState" );
    DeclareProperty( steamObj, "serverTime" );
    DeclareProperty( steamObj, "country" );
    DeclareProperty( steamObj, "batteryPower" );
    DeclareProperty( steamObj, "secondsSinceActive" );
    SetGlobalObject( "steam", steamObj );

    // Variables de Weapon
    DeclareProperty( weaponObj, "active" );
    DeclareProperty( weaponObj, "clip1" );
    DeclareProperty( weaponObj, "clip1Ammo" );

    // Variables de Player
    DeclareProperty( playerObj, "health" );
    DeclareProperty( playerObj, "info" );
    SetProperty( playerObj, "weapon", weaponObj );
    SetGlobalObject( "player", playerObj );

    // Dejamos que cada panel cree sus objetos
    for ( int it = 0; it < g_CefPanels.Count(); ++it )
    {
        CefBasePanel *panel = g_CefPanels.Element( it );

        // El panel es de otro navegador
        if ( panel->GetClient()->GetBrowser()->GetIdentifier() != browser->GetIdentifier() )
            continue;

        panel->OnContextCreated( context );
    }
}

//================================================================================
// Constructor
//================================================================================
CCefClient::CCefClient( CefBasePanel *panel )
{
    m_Panel = panel;
    m_Browser = NULL;
    m_nBuffer = NULL;
    m_bNeedPaint = false;
}

//================================================================================
// Destructor
//================================================================================
CCefClient::~CCefClient()
{
    // Cerramos el navegador
    if ( m_Browser )
    {
        m_Browser->GetHost()->CloseBrowser( true );
        m_Browser = NULL;
    }

    if ( m_nThread )
    {
        ReleaseThreadHandle( m_nThread );
        m_nThread = NULL;
    }

    delete m_nBuffer;
    m_nBuffer = NULL;
}

//================================================================================
// Crea el navegador y empieza a renderizar
//================================================================================
void CCefClient::Init()
{
    // Configuración de la ventana
    CefWindowInfo info;
    info.width = GetPanel()->GetWide();
    info.height = GetPanel()->GetTall();
    info.SetAsWindowless( GetDesktopWindow(), true );

    // Configuración del navegador
    CefBrowserSettings settings;
    settings.file_access_from_file_urls = STATE_ENABLED;
    settings.javascript_access_clipboard = STATE_DISABLED;
    settings.javascript_close_windows = STATE_DISABLED;
    settings.javascript_open_windows = STATE_DISABLED;
    settings.plugins = STATE_DISABLED;
    settings.remote_fonts = STATE_ENABLED;
    settings.webgl = STATE_DISABLED;
    settings.web_security = STATE_DISABLED;
    settings.universal_access_from_file_urls = STATE_ENABLED;
    settings.windowless_frame_rate = cl_web_ui_fps.GetFloat();
    settings.image_loading = STATE_ENABLED;

    CefString( &settings.default_encoding ).FromASCII( "UTF-8" );

    // Creamos el navegador
    m_Browser = CefBrowserHost::CreateBrowserSync( info, this, "", settings, NULL );

    // Creamos el hilo que se encargara de copiar el buffer en la textura que queremos
    m_nThread = CreateSimpleThread( UpdateCefThread, this );
    ThreadSetPriority( m_nThread, 0 );
}

//================================================================================
//================================================================================
void CCefClient::ResizeView()
{
    // Volvemos a crear el buffer con el nuevo tamaño
    m_nBuffer = new unsigned char[GetPanel()->GetWide() * GetPanel()->GetTall() * DEPTH];

    // Notificamos al navegador del cambio de tamaño
    GetBrowser()->GetHost()->WasResized();
}

//================================================================================
// Establece las dimensiones del navegador
//================================================================================
bool CCefClient::GetViewRect( CefRefPtr<CefBrowser> browser, CefRect &rect )
{
    rect = CefRect( 0, 0, GetPanel()->GetWide(), GetPanel()->GetTall() );
    return true;
}

//================================================================================
// El navegador ha sido dibujado, copiamos el buffer.
//================================================================================
void CCefClient::OnPaint( CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList & dirtyRects, const void * buffer, int width, int height )
{
    //VPROF_BUDGET( __FUNCTION__, VPROF_BUDGETGROUP_CEF );

    if ( !m_nBuffer )
        m_nBuffer = new unsigned char[width * height * DEPTH];

    // Copiamos el buffer del navegador al que tenemos localmente
    // Nota: ¡el buffer de CEF esta en BGRA! Azul y rojo cambiados.
    memcpy( m_nBuffer, buffer, width * height * DEPTH );

    //if ( ThreadInMainThread() )
    //    DevMsg( "OnPaint (ThreadInMainThread)!! \n" );
    //else
    //    DevMsg( "OnPaint!! \n" );
    
    // Necesitamos pintar los nuevos pixeles
    m_bNeedPaint = true;
}

//================================================================================
//================================================================================
void CCefClient::OnResourceLoadComplete( CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, CefRefPtr<CefResponse> response, URLRequestStatus status, int64 received_content_length )
{
    GetPanel()->OnResourceLoadComplete( browser, frame, request, response, status, received_content_length );
}

//================================================================================
//================================================================================
void CCefClient::OnLoadingStateChange( CefRefPtr<CefBrowser> browser, bool isLoading, bool canGoBack, bool canGoForward )
{
    GetPanel()->OnLoadingStateChange( browser, isLoading, canGoBack, canGoForward );
}

#endif // GAMEUI_CEF