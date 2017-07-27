#ifndef ALIENFX_H
#define ALIENFX_H

#ifdef _WIN32
#pragma once
#endif

#undef STDCALL
#include "alienfx\LFX2.h"

enum
{
    LIGHT_PER_FRAME,
    LIGHT_PRIORITY_LOW,
    LIGHT_PRIORITY_MEDIUM,
    LIGHT_PRIORITY_HIGH
};

//================================================================================
// Permite controlar las luces de un dispositivo compatible con AlienFX
//================================================================================
class CAlienFX : CAutoGameSystemPerFrame
{
public:
    CAlienFX();

    virtual bool Init();
    virtual void Shutdown();

    virtual void Update( float frametime );
    virtual void UpdateStatus();
    virtual void UpdatePulse( float interval );

    virtual bool IsEnabled();
    virtual void OnMessage( bf_read &msg );

    virtual bool HasStarted() { return m_bStarted; }
    virtual void SetColor( int device, int color, float duration = -1 );
    virtual void SetActiveColor( int device, int color, float duration = -1 );
    virtual void SetActionColor( int device, int color, int action );

protected:
    LFX2INITIALIZE FXInit;
    LFX2RESET FXReset;
    LFX2LIGHT FXLight;
    LFX2UPDATE FXUpdate;
    LFX2RELEASE FXRelease;
    LFX2ACTIONCOLOR FXLightActionColor;

    bool m_bStarted;

    CountdownTimer m_LightTimer;
    CountdownTimer m_PulseTimer;
    int m_PulseState;
};

extern CAlienFX *TheAlienFX;

#endif // ALIENFX_H