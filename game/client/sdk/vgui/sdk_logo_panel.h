#ifndef SDK_LOGO_PANEL_H
#define SDK_LOGO_PANEL_H

#ifdef _WIN32
#pragma once
#endif

#include <vgui/VGUI.h>
#include <vgui_controls/ImagePanel.h>

class ChatEchoPanel;

//-----------------------------------------------------------------------------
// Purpose: Panel that holds a single image - shows the backdrop image in the briefing
//-----------------------------------------------------------------------------
class CLogo_Panel : public vgui::ImagePanel
{
	DECLARE_CLASS_SIMPLE( CLogo_Panel, vgui::ImagePanel );
public:
	CLogo_Panel(vgui::Panel *parent, const char *name);
	virtual ~CLogo_Panel();

	virtual void PerformLayout();
};


#endif // SDK_LOGO_PANEL_H
