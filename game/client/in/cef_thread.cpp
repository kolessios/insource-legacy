//==== Woots 2017. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "cef_thread.h"

#ifdef GAMEUI_CEF

#include <cef/tests/shared/browser/client_app_browser.h>
#include <cef/tests/shared/browser/main_message_loop_external_pump.h>
#include <cef/tests/shared/browser/main_message_loop_std.h>
#include <cef/tests/shared/common/client_app_other.h>
#include <cef/tests/shared/common/client_switches.h>
#include <cef/tests/shared/renderer/client_app_renderer.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar cl_web_ui_fps;

//================================================================================
//================================================================================
void CefThread::SwapBufferFromBgraToRgba( void* _dest, const void* _src, int width, int height )
{
    VPROF_BUDGET( "SwapBufferFromBgraToRgba", VPROF_BUDGETGROUP_CEF );

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
// El subproceso ha empezado, debemos inicializar C.E.F.
//================================================================================
bool CefThread::Start()
{
    // Hay que iniciar y preparar CEF
    // https://github.com/qwertzui11/cef_osr
    DevMsg( "Starting Chromium Embedded Framework...\n" );

    // Creamos un nuevo objeto para controlar la aplicación
    CefRefPtr<CCefApplication> app( new CCefApplication() );

    CefMainArgs args;
    int cef_result = CefExecuteProcess( args, app.get(), NULL );

    // 
    if ( cef_result >= 0 )
    {
        Warning( "CefThread::Start() failed to execute CEF process.\n" );
        return false;
    }

    const char *userAgent = "InSource - Chromium Embedded Framework";

    CefSettings cef_settings;
    cef_settings.single_process = true;
    cef_settings.no_sandbox = true;
    cef_settings.multi_threaded_message_loop = false;
    cef_settings.windowless_rendering_enabled = true;
    cef_settings.command_line_args_disabled = true;
    cef_settings.ignore_certificate_errors = true;

    CefString( &cef_settings.locale ).FromASCII( "es" );

    if ( IsDebug() )
    {
        cef_settings.log_severity = LOGSEVERITY_WARNING;
        cef_settings.remote_debugging_port = 1337;
    }
    else
    {
        CefString( &cef_settings.user_agent ).FromASCII( userAgent );
    }

    bool cef_init = CefInitialize( args, cef_settings, app.get(), NULL );

    if ( !cef_init )
    {
        Warning( "CefThread::Start() failed to initialize CEF.\n" );
        return false;
    }

    DevMsg( "Chromium Embedded Framework ready.\n" );
    return true;
}

void CefThread::RunTask()
{
    // Inválido
    if ( m_ThreadData.task == CEF_TASK_INVALID )
        return;

    switch ( m_ThreadData.task )
    {
        case CEF_TASK_CREATE_BROWSER:
        {
            break;
        }
    }

    m_ThreadData.task = CEF_TASK_INVALID
}

//================================================================================
// Subproceso dedicado para C.E.F.
// Aquí realizamos todo lo relacionado 
//================================================================================
unsigned int CefThread::ExecThread( void *instance )
{
    VPROF_BUDGET( "UpdateCefThread", VPROF_BUDGETGROUP_CEF );

    if ( !Start() )
    {
        Error("CefThread failed to Start.");
        return 0;
    }

    while ( true )
    {
        // Dependiendo de la calidad
        ThreadSleep( (1000.0f / cl_web_ui_fps.GetFloat()) );

        RunTask();

        // Hay paneles usando CEF
        /*if ( g_CefPanels.Count() > 0 )
        {
            FOR_EACH_VEC( g_CefPanels, it )
            {
                CefBasePanel *panel = g_CefPanels.Element( it );

                // Ancho y altura del panel
                int width = panel->GetWide();
                int height = panel->GetTall();
            }
        }*/
    }

    return 0;
}

#endif // GAMEUI_CEF