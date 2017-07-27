//==== Woots 2017. http://creativecommons.org/licenses/by/2.5/mx/ ===========//
//
// Panel base para la creación de elementos de la interfaz que usen Awesomium
// 
// Advertencia: Awesomium es un proyecto abandonado hace 2 años. Se recomienda
// usar GAMEUI_CEF para usar Chromium Embedded Framework. 
//

#ifdef GAMEUI_AWESOMIUM

#ifndef BASE_WEB_PANEL_H
#define BASE_WEB_PANEL_H

#ifdef _WIN32
#pragma once
#endif

#include "VAwesomium.h"

class CBaseWebPanel : public VAwesomium
{
    DECLARE_CLASS_SIMPLE( CBaseWebPanel, VAwesomium );

public:
    CBaseWebPanel( Panel *parent, const char *panelName );
    CBaseWebPanel( vgui::VPANEL parent, const char *panelName );
    ~CBaseWebPanel();

    virtual void OpenFile( const char *address );
    virtual void Think();

    static unsigned ThreadUpdate( void *params );

    virtual bool ShouldUpdate();
    virtual void Update();

public:
    virtual void OnDocumentReady( Awesomium::WebView* caller, const Awesomium::WebURL& url );
    virtual void OnMethodCall( Awesomium::WebView* caller, unsigned int remote_object_id, const Awesomium::WebString& method_name, const Awesomium::JSArray& args );
    virtual Awesomium::JSValue OnMethodCallWithReturnValue( Awesomium::WebView* caller, unsigned int remote_object_id, const Awesomium::WebString& method_name, const Awesomium::JSArray& args );

protected:
    Awesomium::JSObject m_GameObject;
	Awesomium::JSObject m_SteamObject;
	Awesomium::JSObject m_PlayerObject;
    Awesomium::JSObject m_WeaponObject;

    ThreadHandle_t m_nThread;

    float m_flNextThink;

    const char *m_pSteamID;
    const char *m_pPersonaName;
    int m_iPersonaState;

    int m_iSoundGUID;
    bool m_bThinking;
}; 

#endif // BASE_WEB_PANEL_H

#endif // GAMEUI_AWESOMIUM