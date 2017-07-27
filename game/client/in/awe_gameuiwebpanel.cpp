//==== Woots 2017. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "awe_gameuiwebpanel.h"

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

#include "steam\steam_api.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//setup in GameUI_Interface.cpp
extern class IMatchSystem *matchsystem;
extern const char *COM_GetModDirectory( void );
//extern IGameConsole *IGameConsole();

#ifdef GAMEUI_AWESOMIUM

CWebGameUI *TheGameUIPanel = NULL;

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

    m_bIsPaused = false;

    TheGameUIPanel = this;
}

//================================================================================
// Incialización
//================================================================================
void CWebGameUI::Init() 
{
    BaseClass::Init();

    // Cargamos la página
    OpenFile("resource\\gameui\\menu.html");
}

//================================================================================
//================================================================================
bool CWebGameUI::ShouldPaint() 
{
    // Estamos jugando
    //if ( engine->IsInGame() && !m_bIsPaused )
      //  return false;

    return BaseClass::ShouldPaint();
}

#endif
