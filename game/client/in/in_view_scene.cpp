//========= InfoSmart 2016. http://creativecommons.org/licenses/by/2.5/mx/ =========//

#include "cbase.h"
#include "view_scene.h"

#include "in_view_scene.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

#ifndef LIGHT_DEFERRED

static CInViewRender g_ViewRender;

IViewRender *GetViewRenderInstance()
{
    return &g_ViewRender;
}

#endif