#include "cbase.h"
#include "inputsystem/iinputsystem.h"

#ifdef GAMEUI_AWESOMIUM
#include <VAwesomium.h>
#include <vgui_controls/Controls.h>



#define DEPTH 4

using namespace vgui;
using namespace Awesomium;

//================================================================================
//================================================================================
int g_iNumberOfViews = 0;
static Awesomium::WebSession *g_WebSession;

//================================================================================
// Comandos
//================================================================================
DECLARE_CHEAT_CMD( cl_web_ui, "1", "" );

//================================================================================
// Transforma el buffer de WebView a una textura que podemos usar para mostrar
// en la pantalla.
// Debido a que esta función es muy costosa debemos ejecutarla en un hilo
// distinto al que trabaja el motor, de otra forma perderiamos 30-40 frames
//================================================================================
unsigned AllocateViewBuffer( void *instance ) 
{
    VPROF_BUDGET( "VAwesomium::AllocateViewBuffer", VPROF_BUDGETGROUP_AWESOMIUM );

    VAwesomium * RESTRICT pPanel = reinterpret_cast<VAwesomium *>(instance);

    while ( true )
    {
        // 60 frames en un segundo
        ThreadSleep( (1000.0f / 20.0f) );

        pPanel->m_BitmapSurface = (BitmapSurface*)pPanel->m_WebView->surface();

	    if ( pPanel->m_BitmapSurface && pPanel->m_iNearestPowerWidth + pPanel->m_iNearestPowerHeight > 0 )
	    {
            // Creamos el buffer, un arreglo
		    if ( !pPanel->m_nBuffer )
                pPanel->m_nBuffer = new unsigned char[pPanel->m_iNearestPowerWidth * pPanel->m_iNearestPowerHeight * DEPTH];
	
            // Copiamos el buffer de WebView al que tenemos localmente
	        pPanel->m_BitmapSurface->CopyTo( pPanel->m_nBuffer, pPanel->m_BitmapSurface->width() * DEPTH, DEPTH, true, false );

            // Copiamos el buffer a una textura
            vgui::surface()->DrawSetTextureRGBA(pPanel->m_iTextureId, pPanel->m_nBuffer, pPanel->m_BitmapSurface->width(), pPanel->m_BitmapSurface->height());
	    }
    }

    return 0;
}

//================================================================================
// Hilo para el manejo de Awesomium.
// http://www.awesomium.com/
//
// Awesomium es una librería que permite usar WebKit como interfaz gráfica para
// software en C++ y .NET. Usar Awesomium fácilita muchisimo el desarrollo
// de una interfaz para los juegos, sin embargo es como tener un navegador web integrado
// y puede ser muy costoso para el motor, necesitamos ejecutar algunas de sus
// funciones en un hilo separado.
//================================================================================
class CAwesomiumThread : public CWorkerThread 
{
public:
    CAwesomiumThread()
    {
        m_nCreatedWebView = NULL;
        SetName("AwesomiumThread");
    }

    ~CAwesomiumThread()
    {
    }

    // Funciones que no son Thread-safe y
    // ejecutarlas en el hilo principal causa un error.
    enum
    {
        CREATE_WEBVIEW,
        LOAD_URL,
        EXECUTE_JAVASCRIPT,
        INJECT_MOUSE,
        REQUEST_FOCUS,
        MOUSE_BUTTON,
        MOUSE_WHEEL,
        RESIZE,
        EXIT
    };

    void CreateWebView( int width, int height )
    {
        m_iWebViewWide = width;
        m_iWebViewTall = height;
        CallWorker( CREATE_WEBVIEW );
    }

    void LoadURL( WebView *webView, WebURL url )
    {
        m_nWebView = webView;
        m_nUrl     = url;
        CallWorker( LOAD_URL );
    }

    void ExecuteJavaScript( WebView *webView, const char *script, const char *frame_xpath = "" ) 
    {
        m_nWebView = webView;
        m_nScript  = script;
        m_nFrame   = frame_xpath;
        CallWorker( EXECUTE_JAVASCRIPT );
    }

    void Shutdown( WebView *webView )
    {
        m_nWebView = webView;
        CallWorker( EXIT );
    }

    int Run()
    {
        WebCore *webCore = WebCore::instance();

        if ( !webCore )
        {
            WebConfig config;
            config.user_agent = WSLit("Awesomium - InSource Embeded Browser");

            #ifdef DEBUG
            config.log_level = kLogLevel_Verbose;
            config.remote_debugging_port = 1337;
            config.reduce_memory_usage_on_navigation = true;
            #endif

            // Creamos el núcleo
		    webCore = WebCore::Initialize( config );

            // Configuración para la sesión
            WebPreferences preferences;
            preferences.enable_javascript = true;
            preferences.enable_dart = true;
            preferences.enable_plugins = false;
            preferences.enable_local_storage = true;
            preferences.enable_databases = true;
            preferences.enable_web_audio = true;
            preferences.enable_web_gl = false;
            preferences.enable_web_security = false;
            preferences.enable_remote_fonts = true;
            preferences.enable_smooth_scrolling = true;
            preferences.allow_scripts_to_open_windows = true;
            preferences.allow_scripts_to_close_windows = false;
            preferences.allow_scripts_to_access_clipboard = false;
            preferences.allow_universal_access_from_file_url = true;
            preferences.allow_file_access_from_file_url = true;
            preferences.allow_running_insecure_content = true;
            preferences.accept_charset = WSLit("utf-8");
            preferences.default_encoding = WSLit("utf-8");

            // Creamos la sesión
            g_WebSession = webCore->CreateWebSession( WSLit(""), preferences );
        }

        while( true )
        {
            // Actualizamos el núcleo
            webCore->Update();

            unsigned nCall;

            // Han llamado al hilo
            if ( PeekCall(&nCall) )
            {
                if ( nCall == CREATE_WEBVIEW )
                {
                    m_nCreatedWebView = webCore->CreateWebView( m_iWebViewTall, m_iWebViewWide, g_WebSession );
                    m_nCreatedWebView->SetTransparent( true );
                    Reply( 1 );
                }

                else if ( nCall == LOAD_URL )
                {
                    m_nWebView->LoadURL(WebURL(WSLit("about:blank")));
	                m_nWebView->LoadURL( m_nUrl );
                    Reply( 1 );
                }

                else if ( nCall == EXECUTE_JAVASCRIPT )
                {
                    m_nWebView->ExecuteJavascript(WSLit(m_nScript), WSLit(m_nFrame));
                    Reply( 1 );
                }

                else if ( nCall == INJECT_MOUSE )
                {
                    m_nWebView->InjectMouseMove( m_iMouseX, m_iMouseY );
                    Reply( 1 );
                }

                else if ( nCall == REQUEST_FOCUS )
                {
                    m_nWebView->Focus();
                    Reply( 1 );
                }

                else if ( nCall == MOUSE_BUTTON )
                {
                    ( m_bMouseUp ) ? m_nWebView->InjectMouseUp( m_iMouseButton ) : m_nWebView->InjectMouseDown( m_iMouseButton );
                    Reply( 1 );
                }

                else if ( nCall == MOUSE_WHEEL )
                {
                    m_nWebView->InjectMouseWheel(m_flMouseDelta * WHEEL_DELTA, 0);
                    Reply( 1 );
                }

                else if ( nCall == RESIZE )
                {
                    m_nWebView->Resize( m_iWebViewWide, m_iWebViewTall );
                    Reply(1);
                }

                else if ( nCall == EXIT )
                {
                    if ( m_nWebView )
                        m_nWebView->Destroy();

                    // Todos los componentes se han destruido
                    if ( g_iNumberOfViews <= 0 )
                    {
                        // Liberamos la sesión
                        if ( g_WebSession )
                        {
                            g_WebSession->Release();
                            g_WebSession = NULL;
                        }

                        // Apagamos el núcleo
                        if ( webCore )
                        {
                            webCore->Shutdown();
                            webCore = NULL;
                        }

                        // Paramos el hilo
                        break;
                    }
                }
            }
        }

        return 0;
    }

public:
    Awesomium::WebView *m_nCreatedWebView;
    int m_iWebViewTall;
    int m_iWebViewWide;

    Awesomium::WebView *m_nWebView;

    const char *m_nScript;
    const char *m_nFrame;

    int m_iMouseX;
    int m_iMouseY;

    MouseButton m_iMouseButton;
    bool m_bMouseUp;

    float m_flMouseDelta;

    Awesomium::WebURL m_nUrl;
};

static CAwesomiumThread g_AwesomiumThread;

//================================================================================
// Constructor
//================================================================================
VAwesomium::VAwesomium(Panel *parent, const char *panelName) : Panel(parent, panelName)
{
    // Corremos el hilo de Awesomium
    if ( !g_AwesomiumThread.IsAlive() )
    {
        g_AwesomiumThread.Start();

        // Esperamos 500ms para que el WebCore sea creado
        ThreadSleep( 500 );
    }

    // Número de componentes creados
	g_iNumberOfViews++;

    // Creamos una nueva textura y establecemos la instancia de WebCore
	m_iTextureId    = surface()->CreateNewTextureID(true);
    m_BitmapSurface = NULL;
    m_nBuffer       = NULL;

    // Creamos el WebView
	m_WebView = CreateWebView( GetTall(), GetWide() );
	m_WebView->set_js_method_handler( this );
	m_WebView->set_load_listener( this );

    // Creamos el hilo que se encargara de transformar el buffer de WebView
    // a una textura que pueda ser usada por Source.
    m_nThread = CreateSimpleThread( AllocateViewBuffer, this );
    ThreadSetPriority( m_nThread, 0 );
}

//================================================================================
// Destructor
//================================================================================
VAwesomium::~VAwesomium()
{
    g_iNumberOfViews--;

    // Liberamos el hilo
    if ( m_nThread )
        ReleaseThreadHandle( m_nThread );

    // Destruimos el WebView
    g_AwesomiumThread.Shutdown( m_WebView );

    // Limpiamos variables
	m_WebView       = NULL;
	m_BitmapSurface = NULL;
    m_nThread       = NULL;
    
    delete m_nBuffer;
    m_nBuffer = NULL;
}

//================================================================================
//================================================================================
void VAwesomium::Init() 
{
    
}

//================================================================================
//================================================================================
Awesomium::WebView *VAwesomium::CreateWebView( int width, int height ) 
{
    // Creamos el WebView en el hilo
    g_AwesomiumThread.CreateWebView( width, height );

    // Esperamos 300ms
    ThreadSleep( 300 );

    Assert( g_AwesomiumThread.m_nCreatedWebView );
    return g_AwesomiumThread.m_nCreatedWebView; 
}

//================================================================================
// Abre la dirección indicada en el WebView
//================================================================================
void VAwesomium::OpenURL( const char *address )
{
    g_AwesomiumThread.LoadURL( m_WebView, WebURL(WSLit(address)) );
	ResizeView();
}

//================================================================================
// Inyecta código JavaScript en el WebView
//================================================================================
void VAwesomium::ExecuteJavaScript( const char *script, const char *frame_xpath )
{
    g_AwesomiumThread.ExecuteJavaScript( m_WebView, script, frame_xpath );	
}

//================================================================================
// Devuelve el WebView, sin embargo no deberías usarlo en el hilo principal
//================================================================================
Awesomium::WebView* VAwesomium::GetWebView()
{
	return m_WebView;
}

//================================================================================
//================================================================================
void VAwesomium::PerformLayout()
{
	BaseClass::PerformLayout();
	ResizeView();
}

//================================================================================
// Pensamiento
//================================================================================
void VAwesomium::Think()
{
    
}

//================================================================================
// Devuelve si debería dibujarse/procesarse el WebView
//================================================================================
bool VAwesomium::ShouldPaint() 
{
   // La página sigue cargando...
	if ( m_WebView->IsLoading() )
		return false;

    return cl_web_ui.GetBool();
}

//================================================================================
// Dibuja la textura creada del buffer de WebView
//================================================================================
void VAwesomium::Paint()
{
    VPROF_BUDGET( "WebView::Paint", VPROF_BUDGETGROUP_AWESOMIUM );
	BaseClass::Paint();

    // No debemos dibujar nada, pausamos la renderación para ahorrar recursos
    if ( !ShouldPaint() )
    {
        m_WebView->PauseRendering();
        return;
    }

    // Seguimos renderizando
    m_WebView->ResumeRendering();
    
    // Dibujamos la textura en la interfaz del juego
    if ( m_BitmapSurface )
    {
        vgui::surface()->DrawSetTexture(m_iTextureId);
	    vgui::surface()->DrawSetColor(255, 255, 255, 255);
	    vgui::surface()->DrawTexturedSubRect(0, 0, m_BitmapSurface->width(), m_BitmapSurface->height(), 0.0f, 0.0f, 1, 1);
    }
}

//================================================================================
//================================================================================
void VAwesomium::OnCursorMoved(int x, int y)
{
    g_AwesomiumThread.m_iMouseX  = x;
    g_AwesomiumThread.m_iMouseY  = y;
    g_AwesomiumThread.m_nWebView = m_WebView;
    g_AwesomiumThread.CallWorker( CAwesomiumThread::INJECT_MOUSE );
}

//================================================================================
//================================================================================
void VAwesomium::OnRequestFocus(vgui::VPANEL subFocus, vgui::VPANEL defaultPanel)
{
	BaseClass::OnRequestFocus(subFocus, defaultPanel);
	
    g_AwesomiumThread.m_nWebView = m_WebView;
    g_AwesomiumThread.CallWorker( CAwesomiumThread::REQUEST_FOCUS );
}

//================================================================================
//================================================================================
void VAwesomium::OnMousePressed(MouseCode code)
{
	MouseButtonHelper(code, false);
}

//================================================================================
//================================================================================
void VAwesomium::OnMouseReleased(MouseCode code)
{
	MouseButtonHelper(code, true);
}

//================================================================================
//================================================================================
void VAwesomium::MouseButtonHelper(MouseCode code, bool isUp)
{
	switch ( code )
	{
	    case MOUSE_RIGHT:
		    g_AwesomiumThread.m_iMouseButton = kMouseButton_Right;
		    break;
	    case MOUSE_MIDDLE:
		    g_AwesomiumThread.m_iMouseButton = kMouseButton_Middle;
		    break;
	    default: // MOUSE_LEFT:
		    g_AwesomiumThread.m_iMouseButton = kMouseButton_Left;
		    break;
	}

    g_AwesomiumThread.m_bMouseUp = isUp;
    g_AwesomiumThread.m_nWebView = m_WebView;
    g_AwesomiumThread.CallWorker( CAwesomiumThread::MOUSE_BUTTON );
}

//================================================================================
//================================================================================
void VAwesomium::OnMouseWheeled(int delta)
{
    g_AwesomiumThread.m_flMouseDelta = delta;
    g_AwesomiumThread.m_nWebView     = m_WebView;
    g_AwesomiumThread.CallWorker( CAwesomiumThread::MOUSE_WHEEL );
}

//================================================================================
//================================================================================
void VAwesomium::OnKeyTyped(wchar_t unichar)
{
	WebKeyboardEvent event;

	event.text[0] = unichar;
	event.type = WebKeyboardEvent::kTypeChar;
	m_WebView->InjectKeyboardEvent(event);
}

//================================================================================
//================================================================================
void VAwesomium::KeyboardButtonHelper(KeyCode code, bool isUp)
{
	WebKeyboardEvent event;

	event.virtual_key_code = inputsystem->ButtonCodeToVirtualKey(code);
	event.type = isUp ? WebKeyboardEvent::kTypeKeyUp : WebKeyboardEvent::kTypeKeyDown;

	m_WebView->InjectKeyboardEvent(event);
}

//================================================================================
//================================================================================
void VAwesomium::OnKeyCodePressed(KeyCode code)
{
	KeyboardButtonHelper(code, false);
}

//================================================================================
//================================================================================
void VAwesomium::OnKeyCodeReleased(KeyCode code)
{
	KeyboardButtonHelper(code, true);
}

//================================================================================
//================================================================================
int VAwesomium::NearestPowerOfTwo(int v)
{
	// http://stackoverflow.com/questions/466204/rounding-off-to-nearest-power-of-2
	v--;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	v |= v >> 16;
	v++;
	return v;
}

//================================================================================
//================================================================================
void VAwesomium::ResizeView()
{
	m_iNearestPowerWidth  = NearestPowerOfTwo(GetWide());
	m_iNearestPowerHeight = NearestPowerOfTwo(GetTall());

    g_AwesomiumThread.m_iWebViewTall = GetTall();
    g_AwesomiumThread.m_iWebViewWide = GetWide();
    g_AwesomiumThread.m_nWebView     = m_WebView;
    g_AwesomiumThread.CallWorker( CAwesomiumThread::RESIZE );
}

#endif