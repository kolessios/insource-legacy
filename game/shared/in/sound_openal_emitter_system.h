//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//

#ifndef SOUND_OPENAL_EMITTER_SYSTEM
#define SOUND_OPENAL_EMITTER_SYSTEM

#pragma once

#include "sound_emitter_system.h"

//================================================================================
//================================================================================
class CSoundOpenALEmitterSystem : public CSoundEmitterSystem
{
public:
    DECLARE_CLASS_GAMEROOT(CSoundOpenALEmitterSystem, CSoundEmitterSystem);

	virtual char const *Name() {
		return "CSoundOpenALEmitterSystem";
	}

    CSoundOpenALEmitterSystem(const char *name) : BaseClass(name)
    {

    }

public:
    virtual void EmitSoundByHandle(IRecipientFilter& filter, int entindex, const EmitSound_t & ep, HSOUNDSCRIPTHANDLE& handle);
    virtual void EmitSound(IRecipientFilter& filter, int entindex, const EmitSound_t & ep);

    //virtual void EmitAmbientSound(int entindex, const Vector& origin, const char *soundname, float flVolume, int iFlags, int iPitch, float soundtime /*= 0.0f*/, float *duration /*=NULL*/);
    //virtual void EmitAmbientSound(int entindex, const Vector &origin, const char *pSample, float volume, soundlevel_t soundlevel, int flags, int pitch, float soundtime /*= 0.0f*/, float *duration /*=NULL*/);
};

#endif