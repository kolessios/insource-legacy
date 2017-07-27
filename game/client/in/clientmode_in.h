#ifndef _INCLUDED_CLIENTMODE_SDK_H
#define _INCLUDED_CLIENTMODE_SDK_H
#ifdef _WIN32
#pragma once
#endif

#include "clientmode_shared.h"
#include <vgui_controls/EditablePanel.h>
#include <vgui/Cursor.h>
#include "ivmodemanager.h"

#include "GameUI/igameui.h"

class CHudViewport;

//================================================================================
// Se encarga de manejar los ClientMode de los jugadores
//================================================================================
class CModeManager : public IVModeManager
{
public:
	DECLARE_CLASS_GAMEROOT( CModeManager, IVModeManager );

    virtual void Init();
    virtual void SwitchMode( bool commander, bool force ) {}
    virtual void LevelInit( const char *newmap );
    virtual void LevelShutdown( void );
    virtual void ActivateMouse( bool isactive ) {}
};

//================================================================================
// Define la interfaz del jugador.
//================================================================================
class BaseClientMode : public ClientModeShared
{
public:
    DECLARE_CLASS( BaseClientMode, ClientModeShared );

    virtual void Init();
    virtual void InitWeaponSelectionHudElement() { return; }
    virtual void InitViewport();
    virtual void Shutdown();

    virtual void LevelInit( const char *newmap );
    virtual void LevelShutdown( void );
    virtual void Update();

    virtual void FireGameEvent( IGameEvent *event );

    virtual void UpdatePostProcessingEffects();
    virtual void DoPostScreenSpaceEffects( const CViewSetup *pSetup );  
};

//================================================================================
// BaseClientModeFullscreen
//================================================================================
class BaseClientModeFullscreen : public BaseClientMode
{
    DECLARE_CLASS_SIMPLE( BaseClientModeFullscreen, BaseClientMode );
public:
    virtual void InitViewport();
    virtual void Init();

    void Shutdown();
};

//================================================================================
//================================================================================
class CHudViewport : public CBaseViewport
{
private:
    DECLARE_CLASS_SIMPLE( CHudViewport, CBaseViewport );

protected:
    virtual void ApplySchemeSettings( vgui::IScheme *pScheme )
    {
        BaseClass::ApplySchemeSettings( pScheme );

        GetHud().InitColors( pScheme );
        SetPaintBackgroundEnabled( false );
    }

    virtual void CreateDefaultPanels( void ) { /* don't create any panels yet*/ };
};

//================================================================================
//================================================================================
class CFullscreenViewport : public CHudViewport
{
private:
    DECLARE_CLASS_SIMPLE( CFullscreenViewport, CHudViewport );

private:
    virtual void InitViewportSingletons( void )
    {
        SetAsFullscreenViewportInterface();
    }
};

//================================================================================
//================================================================================

extern vgui::HScheme g_hVGuiCombineScheme;
extern IClientMode *GetClientModeNormal();
extern BaseClientMode* GetBaseClientMode();

#endif // _INCLUDED_CLIENTMODE_SDK_H
