//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#ifndef __FMOD_SYSTEM
#define __FMOD_SYSTEM

#pragma once

#include "inc/fmod_studio.hpp"

//================================================================================
//================================================================================

#define METERS_TO_VALVEUNITS(u) (valveUnitsPerMeter*(u)) // Converts meters to valve units
#define VALVEUNITS_TO_METERS(u) (metersPerValveUnit*(u)) // Converts valve units to meters

// @see http://developer.valvesoftware.com/wiki/Dimensions
const float baseRolloffFactor = 1.0f;
const float metersPerValveUnit = 0.01905f;
const float valveUnitsPerMeter = 1 / metersPerValveUnit;
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
    virtual FMOD::Studio::System *System()
    {
        return m_pSystem;
    }

    virtual FMOD::System *LowLevelSystem()
    {
        return m_pLowLevelSystem;
    }

    virtual void FAILCHECK(FMOD_RESULT result);
    virtual void WARNCHECK(FMOD_RESULT result);

    virtual bool Init();
    virtual void Setup();

    virtual void Shutdown();
    
    virtual void LoadBanks();

    virtual void Update(float frametime);
    virtual void UpdateListener();

    virtual void SetupScene();
    virtual void CreateGeometry();

    virtual void LevelInitPreEntity();
    virtual void LevelInitPostEntity();
    virtual void LevelShutdownPreEntity();
    virtual void LevelShutdownPostEntity();

    void ParseAllEntities(const char *pMapData);
    const char *ParseEntity(const char *pEntData);
#else
    virtual bool Init()
    {
        return true;
    }
#endif

public:
    virtual const char *GetFullPath(const char *relativePath);
    virtual const char *GetFullSoundPath(const char *relativePath);

    virtual void EmitSound(IRecipientFilter& filter, int entindex, CSoundParameters params, const EmitSound_t & ep);

#ifdef CLIENT_DLL
    virtual void ReceiveMessage(bf_read & msg);
#endif

protected:
#ifdef CLIENT_DLL
    FMOD::Studio::System *m_pSystem;
    FMOD::System *m_pLowLevelSystem;

    bool m_bScenePrepared;
#endif
};

extern CFMODSoundSystem *TheFMODSoundSystem;

#endif // __FMOD_SYSTEM