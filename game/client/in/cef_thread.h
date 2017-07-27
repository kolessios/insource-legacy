//==== Woots 2017. http://creativecommons.org/licenses/by/2.5/mx/ ===========//
//
// https://bitbucket.org/chromiumembedded/cef
//

#ifdef GAMEUI_CEF

#ifndef CEF_THREAD_H
#define CEF_THREAD_H

#ifdef _WIN32
#pragma once
#endif

#include "cef_basepanel.h"

//================================================================================
//================================================================================
enum CefTask_t
{
    CEF_TASK_INVALID = 0,
    CEF_TASK_CREATE_BROWSER
};

//================================================================================
//================================================================================

//================================================================================
// Controla el subproceso de C.E.F.
//================================================================================
class CefThread
{
public:
    DECLARE_CLASS_NOBASE( CefThread );

    static void SwapBufferFromBgraToRgba( void* _dest, const void* _src, int width, int height );

    static bool Start();
    static void RunTask();

    static unsigned int ExecThread( void *instance );

public:
    static struct CefThreadProcessData
    {
        bool IsLocked() { return task > CEF_TASK_INVALID; }

        CefTask_t task = CEF_TASK_INVALID;
    } m_ThreadData;
};

#endif // CEF_THREAD_H
#endif // GAMEUI_CEF