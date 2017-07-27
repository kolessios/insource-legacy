//==== Woots 2017. http://creativecommons.org/licenses/by/2.5/mx/ ===========//
//
// Panel web base para el GameUI, el menú principal con Awesomium
//
// Advertencia: Awesomium es un proyecto abandonado hace 2 años. Se recomienda
// usar GAMEUI_CEF para usar Chromium Embedded Framework..
//

#ifdef GAMEUI_AWESOMIUM

#ifndef GAMEUI_WEB_PANEL_H
#define GAMEUI_WEB_PANEL_H

#ifdef _WIN32
#pragma once
#endif

#include "awe_basewebpanel.h"

class CWebGameUI : public CBaseWebPanel
{
    DECLARE_CLASS_SIMPLE( CWebGameUI, CBaseWebPanel );

public:
    CWebGameUI();

    virtual void Init();
    virtual bool ShouldPaint();

public:
    virtual void OnGameUIActivated() { }
    virtual void OnGameUIHidden() { }
    virtual void RunFrame() { }
    virtual void OnLevelLoadingStarted( const char *levelName, bool bShowProgressDialog ) { }
    virtual void OnLevelLoadingFinished(bool bError, const char *failureReason, const char *extendedReason) { }
    virtual bool UpdateProgressBar(float progress, const char *statusText) { return true; }

protected:
    bool m_bIsPaused;
};

extern CWebGameUI *TheGameUIPanel;

#endif // GAMEUI_WEB_PANEL_H

#endif // GAMEUI_AWESOMIUM