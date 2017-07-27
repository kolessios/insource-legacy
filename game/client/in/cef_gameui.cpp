//==== Woots 2017. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "cef_gameui.h"

#include "ienginevgui.h"
#include "engineinterface.h"

#include "./GameUI/IGameUI.h"
#include "ienginevgui.h"
#include "engine/ienginesound.h"
#include "EngineInterface.h"
#include "tier0/dbg.h"
#include "ixboxsystem.h"
#include "GameUI_Interface.h"
#include "game/client/IGameClientExports.h"
#include "gameui/igameconsole.h"
#include "inputsystem/iinputsystem.h"
#include "FileSystem.h"
#include "filesystem/IXboxInstaller.h"
#include "tier2/renderutils.h"

#include <vgui/ILocalize.h>

#include "steam\steam_api.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//setup in GameUI_Interface.cpp
//extern class IMatchSystem *matchsystem;
//extern const char *COM_GetModDirectory( void );
//extern IGameConsole *IGameConsole();

#ifdef GAMEUI_CEF

// Puntero hacia el panel
CWebGameUI *TheGameUiPanel = NULL;

//================================================================================
// Constructor
//================================================================================
CWebGameUI::CWebGameUI() : BaseClass((Panel *)NULL, "CWebGameUI")
{
    #if !defined( _X360 ) && !defined( NOSTEAM )
    if ( steamapicontext && steamapicontext->SteamUtils() )
    {
        steamapicontext->SteamUtils()->SetOverlayNotificationPosition( k_EPositionBottomRight );
    }
    #endif

    // Pertenecemos al GameUI (Menú)
    vgui::VPANEL rootpanel = enginevguifuncs->GetPanel( PANEL_GAMEUIDLL );
    SetParent( rootpanel );

    // Con esto hacemos que el mouse funcione
    MakePopup( false );
    TheGameUiPanel = this;
}

//================================================================================
// Incialización
//================================================================================
void CWebGameUI::Init() 
{
    BaseClass::Init();

    // Reseteo de variables
    m_bHidden = false;
    m_nLoadingStatus = "";

    // Cargamos la página
    OpenFile("resource\\gameui\\main.html");
}

//================================================================================
//================================================================================
bool CWebGameUI::ShouldPaint() 
{
    // No debemos mostrar
    if ( m_bHidden )
        return false;

    return BaseClass::ShouldPaint();
}

void CWebGameUI::OnGameUIActivated()
{
    m_bHidden = false;
}

void CWebGameUI::OnGameUIHidden()
{
    m_bHidden = true;
}

void CWebGameUI::RunFrame()
{

}

void CWebGameUI::OnDisconnectFromServer( uint8 eSteamLoginFailure )
{

}

void CWebGameUI::OnLevelLoadingStarted( const char *levelName, bool bShowProgressDialog )
{
    if ( bShowProgressDialog )
        ExecuteJavaScript( "ui.loading.type = 'dialog';" );
    else
        ExecuteJavaScript( "ui.loading.type = 'fullscreen';" );

    ExecuteJavaScript( UTIL_VarArgs( "ui.loading.levelName = '%s';", levelName ) );
}

void CWebGameUI::OnLevelLoadingFinished( bool bError, const char *failureReason, const char *extendedReason )
{
    // Hemos terminado de cargar
    ExecuteJavaScript( "ui.loading.type = 'none';" );
    ExecuteJavaScript( "ui.loading.levelName = '';" );
    ExecuteJavaScript( "ui.loading.statusText = '';" );
    ExecuteJavaScript( "ui.loading.progress = '0';" );

    // Forzamos el repintado
    if ( GetClient() )
        GetClient()->m_bNeedPaint = true;
}

bool CWebGameUI::UpdateProgressBar( float progress, const char *statusText )
{
    // Debemos actualizar el cuadro de texto
    if ( statusText && strlen( statusText ) > 0 )
    {
        const wchar_t *translateStatus = g_pVGuiLocalize->Find( statusText );
        WCHAR_TO_STRING( translateStatus );

        if ( cc_translateStatus && strlen( cc_translateStatus ) > 0 )
            ExecuteJavaScript( UTIL_VarArgs( "ui.loading.statusText = '%s';", cc_translateStatus ) );
    }

    // Porcentaje de progreso
    ExecuteJavaScript( UTIL_VarArgs( "ui.loading.progress = '%.2f';", round(100 * progress) ) );
    return true;
}

#endif
