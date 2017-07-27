//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef BOT_MANAGER_H
#define BOT_MANAGER_H

#ifdef _WIN32
#pragma once
#endif

//================================================================================
// Sistema de bots
//================================================================================
class CBotManager : public CAutoGameSystemPerFrame
{
public:
    CBotManager();

    virtual bool Init();

    virtual void LevelInitPostEntity();
    virtual void LevelShutdownPreEntity();

    virtual void FrameUpdatePreEntityThink();
    virtual void FrameUpdatePostEntityThink();

    virtual float GetDeltaT() { return gpGlobals->frametime; }

public:
    int m_iActive;
};

extern CBotManager *TheBots;

#endif // BOT_MANAGER_H
