//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#ifndef __FMOD_SYSTEM
#define __FMOD_SYSTEM

#pragma once

#include "inc/fmod.hpp"

//================================================================================
//================================================================================

#define METERS_TO_VALVEUNITS(u) (valveUnitsPerMeter*(u)) // Converts meters to valve units
#define VALVEUNITS_TO_METERS(u) (0.01905f*(u)) // Converts valve units to meters

// @see http://developer.valvesoftware.com/wiki/Dimensions
const float baseRolloffFactor = 1.0f;
const float valveUnitsPerMeter = 1 / 0.01905;
const float valveSpeedOfSound = METERS_TO_VALVEUNITS(340.29);

//================================================================================
//================================================================================
class CFMODSoundSystem : public CAutoGameSystemPerFrame
{
public:
    DECLARE_CLASS_GAMEROOT(CFMODSoundSystem, CAutoGameSystemPerFrame);

    CFMODSoundSystem();
    ~CFMODSoundSystem();    

#ifdef CLIENT_DLL
    virtual bool Init();
    virtual void Shutdown();

    virtual void Update(float frametime);
    virtual void UpdateListener();
    virtual void ReceiveMessage(bf_read & msg);

    virtual void LevelInitPreEntity();

    virtual void LevelInitPostEntity()
    {
    }

    virtual void LevelShutdownPreEntity()
    {
    }

    virtual void LevelShutdownPostEntity()
    {
    }

    virtual void ErrorCheck(FMOD_RESULT result);
#else
    virtual bool Init()
    {
        return true;
    }
#endif

public:
    virtual const char *GetFullSoundPath(const char *relativePath);
    virtual void EmitSound(IRecipientFilter& filter, int entindex, CSoundParameters params, const EmitSound_t & ep);

#ifdef CLIENT_DLL
    virtual FMOD::System *System()
    {
        return m_pSystem;
    }
#endif

protected:
#ifdef CLIENT_DLL
    FMOD::System *m_pSystem;
#endif
};

extern CFMODSoundSystem *TheFMODSoundSystem;

#endif // __FMOD_SYSTEM