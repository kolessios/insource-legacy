#pragma once

#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>
#include <vgui/ISurface.h>

#ifdef GAMEUI_AWESOMIUM

#include <Awesomium/WebCore.h>
#include <Awesomium/BitmapSurface.h>

#include <Awesomium/WebString.h>
#include <Awesomium/WebView.h>
#include <Awesomium/STLHelpers.h>

#include <Awesomium/JSObject.h>
#include <Awesomium/JSValue.h>
#include <Awesomium/JSArray.h>

#define SET_PROPERTY( object, name, value ) object.SetPropertyAsync(WSLit(name), JSValue(value))
#define SET_METHOD( object, name, has_return ) object.SetCustomMethod(WSLit(name), has_return)
#define TO_STRING( object ) ToString( object ).c_str()

// VPROF
#define VPROF_BUDGETGROUP_AWESOMIUM _T("Awesomium") 

// NEVER DO THIS AT HOME KIDS, MULTIPLE INHERITANCE IS BAD BUT I'M YOUR WORST NIGHTMARE
class VAwesomium : public vgui::Panel, public Awesomium::JSMethodHandler, public Awesomium::WebViewListener::Load
{
	DECLARE_CLASS_SIMPLE(VAwesomium, vgui::Panel);

public:
	VAwesomium( vgui::Panel *parent, const char *panelName );
	~VAwesomium();

    virtual void Init();
    Awesomium::WebView *CreateWebView( int width, int height );

	virtual void OpenURL( const char *address );
	virtual void ExecuteJavaScript(const char *script, const char *frame_xpath = "");

	Awesomium::WebView* GetWebView();
    virtual void ResizeView();

protected:
	virtual void OnMethodCall(Awesomium::WebView* caller, unsigned int remote_object_id, const Awesomium::WebString& method_name, const Awesomium::JSArray& args){};
	virtual Awesomium::JSValue OnMethodCallWithReturnValue(Awesomium::WebView* caller, unsigned int remote_object_id, const Awesomium::WebString& method_name, const Awesomium::JSArray& args){return Awesomium::JSValue::Undefined();};
	virtual void OnBeginLoadingFrame(Awesomium::WebView* caller, int64 frame_id, bool is_main_frame, const Awesomium::WebURL& url, bool is_error_page){};
	virtual void OnFailLoadingFrame(Awesomium::WebView* caller, int64 frame_id, bool is_main_frame, const Awesomium::WebURL& url, int error_code, const Awesomium::WebString& error_desc){};
	virtual void OnFinishLoadingFrame(Awesomium::WebView* caller, int64 frame_id, bool is_main_frame, const Awesomium::WebURL& url){};
	virtual void OnDocumentReady(Awesomium::WebView* caller, const Awesomium::WebURL& url){};

protected:
	virtual void PerformLayout();
    virtual void Think();

    virtual bool ShouldPaint();
	virtual void Paint();
	
	virtual void OnMousePressed(vgui::MouseCode code);
	virtual void OnMouseReleased(vgui::MouseCode code);

	virtual void OnKeyCodePressed(vgui::KeyCode code);
	virtual void OnKeyCodeReleased(vgui::KeyCode code);
	virtual void OnKeyTyped(wchar_t unichar);

	virtual void OnCursorMoved(int x, int y);
	virtual void OnMouseWheeled(int delta);
	virtual void OnRequestFocus(vgui::VPANEL subFocus, vgui::VPANEL defaultPanel);

private:
	void MouseButtonHelper(vgui::MouseCode code, bool isUp);
	void KeyboardButtonHelper(vgui::KeyCode code, bool isUp);

	int NearestPowerOfTwo(int v);

public:
	Awesomium::WebView *m_WebView;                                                                                               
	Awesomium::BitmapSurface *m_BitmapSurface;

	int m_iTextureId;
	int	m_iNearestPowerWidth;
	int m_iNearestPowerHeight;
	bool m_bHasLoaded;
	
    ThreadHandle_t m_nThread;
    unsigned char* m_nBuffer;
};

#endif