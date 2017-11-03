//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//

#include "cbase.h"
#include "sound_openal_emitter_system.h"

#include "engine/IEngineSound.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"
#include "filesystem.h"

#ifdef USE_OPENAL
#include "openal/openal.h"
#include "openal/openal_oggsample.h"
#include "openal/openal_sample_pool.h"
#include "soundchars.h"
#endif

#ifndef CLIENT_DLL
#include "envmicrophone.h"
#include "sceneentity.h"
#include "closedcaptions.h"
#else
#include <vgui_controls/Controls.h>
#include <vgui/IVgui.h>
#include "hud_closecaption.h"

#ifdef GAMEUI_UISYSTEM2_ENABLED
#include "gameui.h"
#endif

#define CRecipientFilter C_RecipientFilter
#endif

//================================================================================
// Commands
//================================================================================

ConVar snd_openal_disable("snd_openal_disable", "0", 0, "");

//================================================================================
//================================================================================

extern ISoundEmitterSystemBase *soundemitterbase;

//================================================================================
//================================================================================
void CSoundOpenALEmitterSystem::EmitSoundByHandle(IRecipientFilter & filter, int entindex, const EmitSound_t & ep, HSOUNDSCRIPTHANDLE & handle)
{
    // Disabled
    if ( snd_openal_disable.GetBool() ) {
        BaseClass::EmitSoundByHandle(filter, entindex, ep, handle);
        return;
    }

    IOpenALSample *pSample = g_OpenALLoader.Load(PSkipSoundChars(ep.m_pSoundName));

    if ( pSample && pSample->IsReady() ) {
        pSample->LinkEntity(CBaseEntity::Instance(entindex));

        if ( ep.m_pOrigin != NULL ) {
            pSample->SetPosition(*ep.m_pOrigin);
        }

        pSample->SetGain(ep.m_flVolume);
        pSample->Play();
        TraceEmitSound(entindex, "[OpenAL] EmitSound:  '%s' emitted (ent %i)\n", ep.m_pSoundName, entindex);
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

    pSample = g_OpenALLoader.Load(PSkipSoundChars(params.soundname));

    if ( pSample && pSample->IsReady() ) {
        pSample->LinkEntity(CBaseEntity::Instance(entindex));

        if ( ep.m_pOrigin != NULL ) {
            pSample->SetPosition(*ep.m_pOrigin);
        }

        pSample->SetGain(ep.m_flVolume);
        pSample->Play();
        TraceEmitSound(entindex, "[OpenAL] EmitSound:  '%s' emitted as '%s' (ent %i)\n", ep.m_pSoundName, params.soundname, entindex);
    }
    else {
        Assert(!"A problem has occurred when playing the sound.");
        Warning("[OpenAL] A problem has occurred when playing the sound, using engine sound...\n");

        BaseClass::EmitSoundByHandle(filter, entindex, ep, handle);
    }
}

//================================================================================
//================================================================================
void CSoundOpenALEmitterSystem::EmitSound(IRecipientFilter & filter, int entindex, const EmitSound_t & ep)
{
    // Disabled
    if ( snd_openal_disable.GetBool() ) {
        BaseClass::EmitSound(filter, entindex, ep);
        return;
    }

#ifdef CLIENT_DLL
    IOpenALSample *pSample = g_OpenALLoader.Load(PSkipSoundChars(ep.m_pSoundName));

    if ( pSample && pSample->IsReady() ) {
        pSample->LinkEntity(CBaseEntity::Instance(entindex));

        if ( ep.m_pOrigin != NULL ) {
            pSample->SetPosition(*ep.m_pOrigin);
        }

        pSample->SetGain(ep.m_flVolume);
        pSample->Play();
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

    if ( !soundemitterbase->GetParametersForSound(ep.m_pSoundName, params, gender) ) {
        return;
    }

    if ( !params.soundname[0] )
        return;

    pSample = g_OpenALLoader.Load(PSkipSoundChars(params.soundname));

    if ( pSample && pSample->IsReady() ) {
        pSample->LinkEntity(CBaseEntity::Instance(entindex));

        if ( ep.m_pOrigin != NULL ) {
            pSample->SetPosition(*ep.m_pOrigin);
        }

        pSample->SetGain(ep.m_flVolume);

        pSample->Play();
    }
    else {
        Assert(!"A problem has occurred when playing the sound.");
        Warning("[OpenAL] A problem has occurred when playing the sound, using engine sound...\n");

        BaseClass::EmitSound(filter, entindex, ep);
    }
#else
    //BaseClass::EmitSound(filter, entindex, ep);
#endif
}
