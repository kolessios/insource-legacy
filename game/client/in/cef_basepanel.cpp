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

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

CUtlVector<CefBasePanel *> g_CefPanels;
CUtlVector<char *> g_QueueCommands;

//================================================================================
// Comandos
//================================================================================

DECLARE_CHEAT_COMMAND( cl_web_ui, "1", "" );
ConVar cl_web_ui_fps( "cl_web_ui_fps", "60", FCVAR_ARCHIVE, "", true, 24, true, 120 );

//================================================================================
//================================================================================

#define IsKeyDown( virtualkey ) inputsystem->IsButtonDown( inputsystem->VirtualKeyToButtonCode(virtualkey) ) 

int GetCefKeyboardModifiers( WPARAM wparam )
{
    int modifiers = 0;
    if ( IsKeyDown( VK_SHIFT ) )
        modifiers |= EVENTFLAG_SHIFT_DOWN;
    if ( IsKeyDown( VK_CONTROL ) )
        modifiers |= EVENTFLAG_CONTROL_DOWN;
    if ( IsKeyDown( VK_MENU ) )
        modifiers |= EVENTFLAG_ALT_DOWN;

    // Low bit set from GetKeyState indicates "toggled".
    if ( ::GetKeyState( VK_NUMLOCK ) & 1 )
        modifiers |= EVENTFLAG_NUM_LOCK_ON;
    if ( ::GetKeyState( VK_CAPITAL ) & 1 )
        modifiers |= EVENTFLAG_CAPS_LOCK_ON;

    switch ( wparam )
    {
    case VK_RETURN:
        //if ( (lparam >> 16) & KF_EXTENDED )
        //modifiers |= EVENTFLAG_IS_KEY_PAD;
        break;
    case VK_INSERT:
    case VK_DELETE:
    case VK_HOME:
    case VK_END:
    case VK_PRIOR:
    case VK_NEXT:
    case VK_UP:
    case VK_DOWN:
    case VK_LEFT:
    case VK_RIGHT:
        //if ( !((lparam >> 16) & KF_EXTENDED) )
        //modifiers |= EVENTFLAG_IS_KEY_PAD;
        break;
    case VK_NUMLOCK:
    case VK_NUMPAD0:
    case VK_NUMPAD1:
    case VK_NUMPAD2:
    case VK_NUMPAD3:
    case VK_NUMPAD4:
    case VK_NUMPAD5:
    case VK_NUMPAD6:
    case VK_NUMPAD7:
    case VK_NUMPAD8:
    case VK_NUMPAD9:
    case VK_DIVIDE:
    case VK_MULTIPLY:
    case VK_SUBTRACT:
    case VK_ADD:
    case VK_DECIMAL:
    case VK_CLEAR:
        modifiers |= EVENTFLAG_IS_KEY_PAD;
        break;
    case VK_SHIFT:
        if ( IsKeyDown( VK_LSHIFT ) )
            modifiers |= EVENTFLAG_IS_LEFT;
        else if ( IsKeyDown( VK_RSHIFT ) )
            modifiers |= EVENTFLAG_IS_RIGHT;
        break;
    case VK_CONTROL:
        if ( IsKeyDown( VK_LCONTROL ) )
            modifiers |= EVENTFLAG_IS_LEFT;
        else if ( IsKeyDown( VK_RCONTROL ) )
            modifiers |= EVENTFLAG_IS_RIGHT;
        break;
    case VK_MENU:
        if ( IsKeyDown( VK_LMENU ) )
            modifiers |= EVENTFLAG_IS_LEFT;
        else if ( IsKeyDown( VK_RMENU ) )
            modifiers |= EVENTFLAG_IS_RIGHT;
        break;
    case VK_LWIN:
        modifiers |= EVENTFLAG_IS_LEFT;
        break;
    case VK_RWIN:
        modifiers |= EVENTFLAG_IS_RIGHT;
        break;
    }
    return modifiers;
}

//================================================================================
// Constructor
//================================================================================
CefBasePanel::CefBasePanel( vgui::Panel *parent, const char *panelName ) : BaseClass(parent, panelName)
{    
    // Creamos una nueva textura
    m_iTextureId = surface()->CreateNewTextureID(true);

    m_nClient = NULL;

    // Cliente
    //m_nClient = new CCefClient( this );
    
    // Reinicio de variables
    m_bMouseLeaved = false;

    g_CefPanels.AddToTail( this );
}

//================================================================================
// Destructor
//================================================================================
CefBasePanel::~CefBasePanel()
{
    if ( m_nClient )
    {
        delete m_nClient;
        m_nClient = NULL;
    }
}

//================================================================================
//================================================================================
void CefBasePanel::Init()
{
    if ( GetClient() )
        GetClient()->Init();
}

//================================================================================
// Devuelve el framerate del navegador. Más framerate es igual a una navegación
// más fluida a costa de un rendimiento menor en el motor.
//================================================================================
int CefBasePanel::GetFramerate()
{
    return cl_web_ui_fps.GetInt();
}

//================================================================================
// Devuelve si debería dibujarse el panel
//================================================================================
bool CefBasePanel::ShouldPaint()
{
    return cl_web_ui.GetBool();
}

//================================================================================
// Dibuja la textura creada del buffer en la pantalla
//================================================================================
void CefBasePanel::Paint()
{
    VPROF_BUDGET( __FUNCTION__, VPROF_BUDGETGROUP_CEF );

    BaseClass::Paint();

    if ( !ShouldPaint() )
        return;

    vgui::surface()->DrawSetTexture( m_iTextureId );
    vgui::surface()->DrawSetColor( 255, 255, 255, 255 );
    vgui::surface()->DrawTexturedRect( 0, 0, GetWide(), GetTall() );
}

//================================================================================
//================================================================================
void CefBasePanel::PaintBackground()
{
    VPROF_BUDGET( __FUNCTION__, VPROF_BUDGETGROUP_CEF );

    BaseClass::PaintBackground();

    // Fondo invisible
    SetBgColor( Color( 0, 0, 0, 0 ) );
    SetFgColor( Color( 0, 0, 0, 0 )  );
    SetPaintBackgroundType( 0 );
}

//================================================================================
//================================================================================
void CefBasePanel::OnThink()
{
    VPROF_BUDGET( __FUNCTION__, VPROF_BUDGETGROUP_CEF );

    // Hay comandos que deben ejecutarse
    if ( g_QueueCommands.Count() > 0 )
    {
        for ( int it = 0; it < g_QueueCommands.Count(); ++it )
        {
            const char *pCommand = g_QueueCommands[it];

            engine->ExecuteClientCmd( pCommand );
            DevMsg( "[OnThink] engine.command: %s\n", pCommand );
        }

        g_QueueCommands.Purge();
    }
}

//================================================================================
//================================================================================
void CefBasePanel::PerformLayout()
{
    BaseClass::PerformLayout();
    ResizeView();
}

//================================================================================
//================================================================================
void CefBasePanel::ResizeView()
{
    if ( GetClient() )
        GetClient()->ResizeView();
}

//================================================================================
// Ejecuta JavaScript
//================================================================================
void CefBasePanel::ExecuteJavaScript( const char *script )
{
    if ( !GetClient() )
        return;

    CefString code;
    code.FromASCII( script );

    CefString file;
    GetClient()->GetBrowser()->GetMainFrame()->ExecuteJavaScript( code, file, 0 );
}

//================================================================================
// Abre la dirección web
//================================================================================
void CefBasePanel::OpenURL( const char *address )
{
    if ( !GetClient() )
        return;

    CefString str;
    str.FromASCII( address );

    GetClient()->GetBrowser()->GetMainFrame()->LoadURL( str );
    ResizeView();

    DevMsg( "CefBasePanel::OpenURL: %s \n", address );
}

//================================================================================
// Abre el archivo local
//================================================================================
void CefBasePanel::OpenFile( const char * filepath )
{
    if ( !GetClient() )
        return;

    OpenURL( VarArgs( "file://%s\\%s", engine->GetGameDirectory(), filepath ) );
}

//================================================================================
//================================================================================
void CefBasePanel::OnKeyCodePressed( vgui::KeyCode code )
{
    KeyboardButtonHelper( code, false );
}

//================================================================================
//================================================================================
void CefBasePanel::OnKeyCodeReleased( vgui::KeyCode code )
{
    KeyboardButtonHelper( code, true );
}

//================================================================================
//================================================================================
void CefBasePanel::OnKeyCodeTyped( vgui::KeyCode code )
{
}

//================================================================================
//================================================================================
void CefBasePanel::OnKeyTyped( wchar_t unichar )
{
    if ( !GetClient() )
        return;

    CefKeyEvent keyboard_event;
    keyboard_event.type = KEYEVENT_CHAR;
    keyboard_event.windows_key_code = unichar;
    keyboard_event.focus_on_editable_field = true;
    //keyboard_event.modifiers = GetCefKeyboardModifiers( virtualkey );

    GetClient()->GetBrowser()->GetHost()->SendKeyEvent( keyboard_event );
}

//================================================================================
//================================================================================
void CefBasePanel::KeyboardButtonHelper( vgui::KeyCode code, bool isUp )
{
    if ( !GetClient() )
        return;

    UINT virtualkey = inputsystem->ButtonCodeToVirtualKey( code );
    
    CefKeyEvent keyboard_event;
    keyboard_event.type = (isUp) ? KEYEVENT_KEYUP : KEYEVENT_KEYDOWN;
    keyboard_event.windows_key_code = virtualkey;
    keyboard_event.modifiers = GetCefKeyboardModifiers( virtualkey );

    GetClient()->GetBrowser()->GetHost()->SendKeyEvent( keyboard_event );

    /*
    UINT virtualkey = inputsystem->ButtonCodeToVirtualKey( code );
    UINT wScanCode = MapVirtualKey( virtualkey, MAPVK_VK_TO_VSC );

    BYTE kb[256];
    GetKeyboardState( kb );
    WCHAR uc[5] = {};

    ToUnicode( virtualkey, wScanCode, kb, uc, 4, 0 );

    CefKeyEvent keyboard_event;
    keyboard_event.type = (isUp) ? KEYEVENT_KEYUP : KEYEVENT_KEYDOWN;
    keyboard_event.windows_key_code = *uc;
    keyboard_event.modifiers = GetCefKeyboardModifiers( virtualkey );
    GetClient()->GetBrowser()->GetHost()->SendKeyEvent( keyboard_event );
    */
}

//================================================================================
//================================================================================
void CefBasePanel::OnMousePressed( vgui::MouseCode code )
{
    MouseButtonHelper( code, false );
}

//================================================================================
//================================================================================
void CefBasePanel::OnMouseReleased( vgui::MouseCode code )
{
    MouseButtonHelper( code, true );
}

//================================================================================
//================================================================================
void CefBasePanel::MouseButtonHelper( vgui::MouseCode code, bool isUp )
{
    if ( !GetClient() )
        return;

    CefBrowserHost::MouseButtonType buttonType;

    switch ( code )
    {
        case MOUSE_RIGHT:
            buttonType = MBT_RIGHT;
            break;
        case MOUSE_MIDDLE:
            buttonType = MBT_MIDDLE;
            break;
        default: // MOUSE_LEFT:
            buttonType = MBT_LEFT;
            break;
    }

    GetClient()->GetBrowser()->GetHost()->SendMouseClickEvent( GetClient()->m_MouseEvent, buttonType, isUp, 1 );
}

//================================================================================
//================================================================================
void CefBasePanel::OnCursorEntered()
{
    m_bMouseLeaved = false;
}

//================================================================================
//================================================================================
void CefBasePanel::OnCursorExited()
{
    m_bMouseLeaved = true;
}

//================================================================================
//================================================================================
void CefBasePanel::OnCursorMoved( int x, int y )
{
    if ( !GetClient() )
        return;

    GetClient()->m_MouseEvent.x = x;
    GetClient()->m_MouseEvent.y = y;
    GetClient()->GetBrowser()->GetHost()->SendMouseMoveEvent( GetClient()->m_MouseEvent, m_bMouseLeaved );
}

//================================================================================
//================================================================================
void CefBasePanel::OnMouseWheeled( int delta )
{
    if ( !GetClient() )
        return;

    GetClient()->GetBrowser()->GetHost()->SendMouseWheelEvent( GetClient()->m_MouseEvent, 0, delta * WHEEL_DELTA );
}

//================================================================================
//================================================================================
void CefBasePanel::OnRequestFocus( vgui::VPANEL subFocus, vgui::VPANEL defaultPanel )
{
    if ( !GetClient() )
        return;

    BaseClass::OnRequestFocus( subFocus, defaultPanel );
    GetClient()->GetBrowser()->GetHost()->SendFocusEvent( true );
}

#endif // GAMEUI_CEF
