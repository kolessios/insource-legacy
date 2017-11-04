//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#include "cbase.h"
#include "sound_fmod_emitter_system.h"

#include "engine/IEngineSound.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"

#include "filesystem.h"
#include "tier2/fileutils.h"
#include "soundchars.h"

#include "fmod_system.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
// Commands
//================================================================================

ConVar snd_fmod_disable("snd_fmod_disable", "0", FCVAR_REPLICATED, "");

//================================================================================
//================================================================================

extern ISoundEmitterSystemBase *soundemitterbase;

//================================================================================
//================================================================================
void CFMODSoundEmitterSystem::PrecacheWave(const char *soundwave)
{
    BaseClass::PrecacheWave(soundwave);

    /*bool result = g_pFullFileSystem->Precache(UTIL_VarArgs("sound/%s", PSkipSoundChars(soundwave)), "GAME");

    if ( !result ) {
        Warning("[CFMODSoundEmitterSystem] Precache failed: %s \n", soundwave);
    }
    else {
        Msg("[CFMODSoundEmitterSystem] Precache: %s \n", soundwave);
    }*/
}

//================================================================================
//================================================================================
void CFMODSoundEmitterSystem::EmitSoundByHandle(IRecipientFilter & filter, int entindex, const EmitSound_t & ep, HSOUNDSCRIPTHANDLE & handle)
{
    // Disabled
    if ( snd_fmod_disable.GetBool() ) {
        BaseClass::EmitSoundByHandle(filter, entindex, ep, handle);
        return;
    }

    // Pull data from parameters
    CSoundParameters params;

    // Try to deduce the actor's gender
    gender_t gender = GENDER_NONE;
    CBaseEntity *ent = CBaseEntity::Instance(entindex);

    if ( ent ) {
        char const *actorModel = STRING(ent->GetModelName());
        gender = soundemitterbase->GetActorGender(actorModel);
    }

    if ( !soundemitterbase->GetParametersForSoundEx(ep.m_pSoundName, handle, params, gender, true) ) {
        return;
    }

    if ( !params.soundname[0] )
        return;

    if ( ep.m_nFlags & SND_CHANGE_PITCH ) {
        params.pitch = ep.m_nPitch;
    }

    if ( ep.m_nFlags & SND_CHANGE_VOL ) {
        params.volume = ep.m_flVolume;
    }

    TheFMODSoundSystem->EmitSound(filter, entindex, params, ep);
    TraceEmitSound(entindex, "EmitSoundByHandle: (%i) '%s' emitted as '%s' (ent %i)\n", handle, ep.m_pSoundName, params.soundname, entindex);
}

//================================================================================
//================================================================================
void CFMODSoundEmitterSystem::EmitSound(IRecipientFilter & filter, int entindex, const EmitSound_t & ep)
{
    // Disabled
    if ( snd_fmod_disable.GetBool() ) {
        BaseClass::EmitSound(filter, entindex, ep);
        return;
    }

    TraceEmitSound(entindex, "%f EmitSound:  NOT HANDLED: '%s' (ent %i) (vol %f)\n", gpGlobals->curtime, ep.m_pSoundName, entindex, ep.m_flVolume);
}
