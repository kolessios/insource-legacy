//==== Woots 2017. http://creativecommons.org/licenses/by/2.5/mx/ ===========//
//
// Panel base para la creación de elementos de la interfaz que usen CEF
//
// https://bitbucket.org/chromiumembedded/cef
//

#ifdef GAMEUI_CEF

#ifndef CEF_PANEL_H
#define CEF_PANEL_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>
#include <vgui/ISurface.h>

#include <cef/include/cef_app.h>
#include <cef/include/cef_client.h>
#include <cef/include/cef_render_handler.h>
#include <cef/include/cef_request_handler.h>
#include <cef/include/cef_render_process_handler.h>
#include <cef/include/cef_dom.h>

//================================================================================
// Macros
//================================================================================

#define VPROF_BUDGETGROUP_CEF _T("CEF")

#define DeclareObject( name, value ) CefRefPtr<CefV8Value> name = CefV8Value::CreateObject( value, NULL );
#define DeclareAccessor( name, classname ) CefRefPtr<CefV8Accessor> name = new classname()

#define SetGlobalObject( name, value ) global->SetValue( name, value, V8_PROPERTY_ATTRIBUTE_NONE );

#define SetProperty( object, name, value ) object->SetValue( name, value, V8_PROPERTY_ATTRIBUTE_NONE )

#define DeclareProperty( object, name ) object->SetValue( name, V8_ACCESS_CONTROL_DEFAULT, V8_PROPERTY_ATTRIBUTE_NONE )
#define DeclareFunction( name ) engineObj->SetValue( name, CefV8Value::CreateFunction( name, handler ), V8_PROPERTY_ATTRIBUTE_NONE )

// Todo el proceso para convertir un wchar a const char *
// TODO: Mover a un lugar global
#define WCHAR_TO_STRING( varname ) CefString cefstring_##varname( varname ); std::string str_##varname = cefstring_##varname.ToString(); const char *cc_##varname = str_##varname.c_str();

#define TO_STRING( object ) ToString( object ).c_str()
#define DEPTH 4

class CefBasePanel;
class CCefClient;

//================================================================================
// Handler predeterminado para las funciones de javascript
//================================================================================
class CDefaultV8Handler : public CefV8Handler
{
public:
    virtual bool Execute( const CefString& name,
        CefRefPtr<CefV8Value> object,
        const CefV8ValueList& arguments,
        CefRefPtr<CefV8Value>& retval,
        CefString& exception );

private:
    IMPLEMENT_REFCOUNTING( CDefaultV8Handler );
};

//================================================================================
// Handlers predeterminadso para obtener información importante desde JavaScript
//================================================================================
class CGameV8 : public CefV8Accessor
{
public:
    virtual bool Get( const CefString& name,
        const CefRefPtr<CefV8Value> object,
        CefRefPtr<CefV8Value>& retval,
        CefString& exception );

    virtual bool Set( const CefString& name,
        const CefRefPtr<CefV8Value> object,
        const CefRefPtr<CefV8Value> value,
        CefString& exception ) {
        return false;
    }

private:
    IMPLEMENT_REFCOUNTING( CGameV8 );
};
class CSteamV8 : public CefV8Accessor
{
public:
    virtual bool Get( const CefString& name,
        const CefRefPtr<CefV8Value> object,
        CefRefPtr<CefV8Value>& retval,
        CefString& exception );

    virtual bool Set( const CefString& name,
        const CefRefPtr<CefV8Value> object,
        const CefRefPtr<CefV8Value> value,
        CefString& exception ) {
        return false;
    }

protected:
    const char *m_pSteamID = NULL;

private:
    IMPLEMENT_REFCOUNTING( CSteamV8 );
};
class CPlayerV8 : public CefV8Accessor
{
public:
    virtual bool Get( const CefString& name,
        const CefRefPtr<CefV8Value> object,
        CefRefPtr<CefV8Value>& retval,
        CefString& exception );

    virtual bool Set( const CefString& name,
        const CefRefPtr<CefV8Value> object,
        const CefRefPtr<CefV8Value> value,
        CefString& exception ) {
        return false;
    }

private:
    IMPLEMENT_REFCOUNTING( CPlayerV8 );
};

//================================================================================
// Aplicación global.
//================================================================================
class CCefApplication : public CefApp, public CefRenderProcessHandler
{
public:
    CCefApplication();

public:
    // CefApp Implementation
    virtual CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() {
        return this;
    }

    virtual void OnBeforeCommandLineProcessing(
        const CefString& process_type,
        CefRefPtr<CefCommandLine> command_line );

public:
    // CefRenderProcessHandler Implementation
    virtual void OnContextCreated( CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame> frame,
        CefRefPtr<CefV8Context> context );

protected:

private:
    IMPLEMENT_REFCOUNTING( CCefApplication );
};

//================================================================================
// Cliente. Clase de utilidad para crear un navegador CEF 
//================================================================================
class CCefClient : public CefRenderHandler, public CefRequestHandler, public CefLoadHandler, public CefContextMenuHandler, public CefClient
{
public:
    CCefClient( CefBasePanel *panel );
    ~CCefClient();

    virtual void Init();
    virtual void ResizeView();

    CefRefPtr<CefBrowser> GetBrowser() { return m_Browser; }
    CefMouseEvent GetMouseEvent() { return m_MouseEvent;  }
    CefBasePanel *GetPanel() { return m_Panel; }

    bool NeedPaint() { return m_bNeedPaint; }

public:
    // CefClient Implementation
    virtual CefRefPtr<CefRenderHandler> GetRenderHandler()
    {
        return this;
    }

    virtual CefRefPtr<CefRequestHandler> GetRequestHandler()
    {
        return this;
    }

    virtual CefRefPtr<CefLoadHandler> GetLoadHandler()
    {
        return this;
    }

    virtual CefRefPtr<CefContextMenuHandler> GetContextMenuHandler() 
    {
        return this;
    }

public:
    virtual void OnResourceLoadComplete( CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, CefRefPtr<CefResponse> response, URLRequestStatus status, int64 received_content_length );
    virtual void OnLoadingStateChange( CefRefPtr<CefBrowser> browser, bool isLoading, bool canGoBack, bool canGoForward );

public:
    // CefRenderHandler Implementation
    virtual bool GetViewRect( CefRefPtr<CefBrowser> browser, CefRect& rect );
    virtual void OnPaint( CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList& dirtyRects, const void* buffer, int width, int height );

public:
    virtual void OnBeforeContextMenu( CefRefPtr<CefBrowser> browser,
        CefRefPtr<CefFrame> frame,
        CefRefPtr<CefContextMenuParams> params,
        CefRefPtr<CefMenuModel> model )
    {
        model->Clear();
    }

public:
    IMPLEMENT_REFCOUNTING( CCefClient );

public:
    CefBasePanel *m_Panel;

    CefRefPtr<CefBrowser> m_Browser;
    CefMouseEvent m_MouseEvent;


    byte *m_nBuffer;
    bool m_bNeedPaint;
    ThreadHandle_t m_nThread;

    friend class CefBasePanel;
};

//================================================================================
// Panel CEF. Base para la creación de paneles usando C.E.F.
//================================================================================
class CefBasePanel : public vgui::Panel
{
    DECLARE_CLASS_SIMPLE( CefBasePanel, vgui::Panel );

public:
    CefBasePanel( vgui::Panel *parent, const char *panelName );
    ~CefBasePanel();

    virtual CCefClient *GetClient() {
        return m_nClient;
    }

    virtual void Init();   
    virtual int GetFramerate();

    virtual bool ShouldPaint();
    virtual void Paint();
    virtual void PaintBackground();

    virtual void OnThink();

    virtual void OnContextCreated( CefRefPtr<CefV8Context> context ) { }

    virtual void PerformLayout();
    virtual void ResizeView();

    virtual void ExecuteJavaScript( const char *script );

    virtual void OpenURL( const char *address );
    virtual void OpenFile( const char *filepath );

public:
    virtual void OnKeyCodePressed( vgui::KeyCode code );
    virtual void OnKeyCodeReleased( vgui::KeyCode code );
    virtual void OnKeyCodeTyped( vgui::KeyCode code );
    virtual void OnKeyTyped( wchar_t unichar );
    virtual void KeyboardButtonHelper( vgui::KeyCode code, bool isUp );

    virtual void OnMousePressed( vgui::MouseCode code );
    virtual void OnMouseReleased( vgui::MouseCode code );
    virtual void MouseButtonHelper( vgui::MouseCode code, bool isUp );

    virtual void OnCursorEntered();
    virtual void OnCursorExited();
    virtual void OnCursorMoved( int x, int y );

    virtual void OnMouseWheeled( int delta );
    virtual void OnRequestFocus( vgui::VPANEL subFocus, vgui::VPANEL defaultPanel );

public:
    virtual void OnResourceLoadComplete( CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, CefRefPtr<CefResponse> response, CefRequestHandler::URLRequestStatus status, int64 received_content_length ) { }
    virtual void OnLoadingStateChange( CefRefPtr<CefBrowser> browser, bool isLoading, bool canGoBack, bool canGoForward ) { }

public:
    int m_iTextureId;
    bool m_bMouseLeaved;

    CCefClient *m_nClient;
};

#endif // CEF_PANEL_H

#endif // GAMEUI_CEF