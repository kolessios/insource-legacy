//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#ifndef SOUND_FMOD_EMITTER_SYSTEM
#define SOUND_FMOD_EMITTER_SYSTEM

#pragma once

#include "sound_emitter_system.h"

//================================================================================
//================================================================================
class CFMODSoundEmitterSystem : public CSoundEmitterSystem
{
public:
    DECLARE_CLASS_GAMEROOT(CFMODSoundEmitterSystem, CSoundEmitterSystem);

    virtual char const *Name()
    {
        return "CFMODSoundEmitterSystem";
    }

    CFMODSoundEmitterSystem(const char *name) : BaseClass(name)
    {

    }

public:
    virtual void PrecacheWave(const char *soundwave);

    virtual void EmitSoundByHandle(IRecipientFilter& filter, int entindex, const EmitSound_t & ep, HSOUNDSCRIPTHANDLE& handle);
    virtual void EmitSound(IRecipientFilter& filter, int entindex, const EmitSound_t & ep);
};

#endif // SOUND_FMOD_EMITTER_SYSTEM