//==== Woots 2017. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifdef GAMEUI_CEF

#ifndef GAMEUI_WEB_PANEL_H
#define GAMEUI_WEB_PANEL_H

#ifdef _WIN32
#pragma once
#endif

#include "cef_basepanel.h"

//================================================================================
// Interfaz de usuario para el menú
//================================================================================
class CWebGameUI : public CefBasePanel
{
    DECLARE_CLASS_SIMPLE( CWebGameUI, CefBasePanel );

public:
    CWebGameUI();

    virtual void Init();
    virtual bool ShouldPaint();

public:
    virtual void OnGameUIActivated();
    virtual void OnGameUIHidden();

    virtual void RunFrame();

    virtual void OnDisconnectFromServer( uint8 eSteamLoginFailure );
    virtual void OnLevelLoadingStarted( const char *levelName, bool bShowProgressDialog );
    virtual void OnLevelLoadingFinished( bool bError, const char *failureReason, const char *extendedReason );
    virtual bool UpdateProgressBar( float progress, const char *statusText );

protected:
    bool m_bHidden;
    const char *m_nLoadingStatus;
};

extern CWebGameUI *TheGameUiPanel;

#endif // GAMEUI_WEB_PANEL_H
#endif // GAMEUI_CEF
