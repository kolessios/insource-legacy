//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//

#include "cbase.h"
#include "sound_emitter_system.h"

#include "engine/IEngineSound.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"
#include "filesystem.h"

#ifndef CLIENT_DLL
#include "envmicrophone.h"
#include "sceneentity.h"
#include "closedcaptions.h"
#else
#include <vgui_controls/Controls.h>
#include <vgui/IVgui.h>
#include "hud_closecaption.h"

#ifdef USE_OPENAL
#include "openal/openal.h"
#include "openal/openal_oggsample.h"
#include "openal/openal_sample_pool.h"
#include "soundchars.h"
#endif

#ifdef GAMEUI_UISYSTEM2_ENABLED
#include "gameui.h"
#endif

#define CRecipientFilter C_RecipientFilter
#endif

//================================================================================
// Commands
//================================================================================

ConVar sv_soundemitter_trace("sv_soundemitter_trace", "-1", FCVAR_REPLICATED, "Show all EmitSound calls including their symbolic name and the actual wave file they resolved to. (-1 = for nobody, 0 = for everybody, n = for one entity)\n");

//================================================================================
//================================================================================

extern ISoundEmitterSystemBase *soundemitterbase;
static ConVar *g_pClosecaption = NULL;

#ifdef _XBOX
int LookupStringFromCloseCaptionToken(char const *token);
const wchar_t *GetStringForIndex(int index);
#endif

static bool g_bPermitDirectSoundPrecache = false;

#ifndef CLIENT_DLL
//================================================================================
//================================================================================
CSoundEmitterSystem::CSoundEmitterSystem(char const *pszName) :
    m_bLogPrecache(false),
    m_hPrecacheLogFile(FILESYSTEM_INVALID_HANDLE)
{
}

//================================================================================
//================================================================================
void CSoundEmitterSystem::LogPrecache(char const *soundname)
{
    if ( !m_bLogPrecache )
        return;

    // Make sure we only show the message once
    if ( UTL_INVAL_SYMBOL != m_PrecachedScriptSounds.Find(soundname) )
        return;

    if ( m_hPrecacheLogFile == FILESYSTEM_INVALID_HANDLE ) {
        StartLog();
    }

    m_PrecachedScriptSounds.AddString(soundname);

    if ( m_hPrecacheLogFile != FILESYSTEM_INVALID_HANDLE ) {
        filesystem->Write("\"", 1, m_hPrecacheLogFile);
        filesystem->Write(soundname, Q_strlen(soundname), m_hPrecacheLogFile);
        filesystem->Write("\"\n", 2, m_hPrecacheLogFile);
    }
    else {
        Warning("Disabling precache logging due to file i/o problem!!!\n");
        m_bLogPrecache = false;
    }
}

//================================================================================
//================================================================================
void CSoundEmitterSystem::StartLog()
{
    m_PrecachedScriptSounds.RemoveAll();

    if ( !m_bLogPrecache )
        return;

    if ( FILESYSTEM_INVALID_HANDLE != m_hPrecacheLogFile ) {
        return;
    }

    filesystem->CreateDirHierarchy("reslists", "DEFAULT_WRITE_PATH");

    // open the new level reslist
    char path[_MAX_PATH];
    Q_snprintf(path, sizeof(path), "reslists\\%s.snd", gpGlobals->mapname.ToCStr());
    m_hPrecacheLogFile = filesystem->Open(path, "wt", "MOD");
    if ( m_hPrecacheLogFile == FILESYSTEM_INVALID_HANDLE ) {
        Warning("Unable to open %s for precache logging\n", path);
    }
}

//================================================================================
//================================================================================
void CSoundEmitterSystem::FinishLog()
{
    if ( FILESYSTEM_INVALID_HANDLE != m_hPrecacheLogFile ) {
        filesystem->Close(m_hPrecacheLogFile);
        m_hPrecacheLogFile = FILESYSTEM_INVALID_HANDLE;
    }

    m_PrecachedScriptSounds.RemoveAll();
}
#endif

//================================================================================
//================================================================================
bool CSoundEmitterSystem::Init()
{
    Assert(soundemitterbase);

#ifndef CLIENT_DLL
    m_bLogPrecache = CommandLine()->CheckParm("-makereslists") ? true : false;
#endif

    g_pClosecaption = cvar->FindVar("closecaption");
    Assert(g_pClosecaption);

#ifndef CLIENT_DLL
    // Server keys off of english file!!!
    char dbfile[512];
    Q_snprintf(dbfile, sizeof(dbfile), "resource/closecaption_%s.dat", "english");

    m_ServerCaptions.Purge();

    if ( IsX360() ) {
        char fullpath[MAX_PATH];
        char fullpath360[MAX_PATH];
        filesystem->RelativePathToFullPath(dbfile, "GAME", fullpath, sizeof(fullpath));
        UpdateOrCreateCaptionFile(fullpath, fullpath360, sizeof(fullpath360));
        Q_strncpy(fullpath, fullpath360, sizeof(fullpath));
    }

    int idx = m_ServerCaptions.AddToTail();
    AsyncCaption_t& entry = m_ServerCaptions[idx];

    if ( !entry.LoadFromFile(dbfile) ) {
        m_ServerCaptions.Remove(idx);
    }
#endif

    return true;
}

//================================================================================
//================================================================================
void CSoundEmitterSystem::Shutdown()
{
    Assert(soundemitterbase);

#ifndef CLIENT_DLL
    FinishLog();
#endif
}

//================================================================================
//================================================================================
void CSoundEmitterSystem::LevelInitPreEntity()
{
    char mapname[256];
#ifndef CLIENT_DLL
    StartLog();
    Q_snprintf(mapname, sizeof(mapname), "maps/%s", STRING(gpGlobals->mapname));
#else
    Q_strncpy(mapname, engine->GetLevelName(), sizeof(mapname));
#endif

    Q_FixSlashes(mapname);
    Q_strlower(mapname);

    // Load in any map specific overrides
    char scriptfile[512];
    Q_StripExtension(mapname, scriptfile, sizeof(scriptfile));
    Q_strncat(scriptfile, "_level_sounds.txt", sizeof(scriptfile), COPY_ALL_CHARACTERS);

    if ( filesystem->FileExists(scriptfile, "GAME") ) {
        soundemitterbase->AddSoundOverrides(scriptfile);
    }

#ifndef CLIENT_DLL
    PreloadSounds();
    g_CaptionRepeats.Clear();
#endif
}

//================================================================================
//================================================================================
void CSoundEmitterSystem::LevelShutdownPostEntity()
{
    soundemitterbase->ClearSoundOverrides();

#ifndef CLIENT_DLL
    FinishLog();
    g_CaptionRepeats.Clear();
#endif
}

//================================================================================
//================================================================================
void CSoundEmitterSystem::TraceEmitSound(int originEnt, char const * fmt, ...)
{
    if ( sv_soundemitter_trace.GetInt() == -1 )
        return;

    if ( sv_soundemitter_trace.GetInt() != 0 && sv_soundemitter_trace.GetInt() != originEnt )
        return;

    va_list	argptr;
    char string[256];
    va_start(argptr, fmt);
    Q_vsnprintf(string, sizeof(string), fmt, argptr);
    va_end(argptr);

    // Spew to console
    Msg("%s %s", CBaseEntity::IsServer() ? "(sv)" : "(cl)", string);
}

//================================================================================
//================================================================================
void CSoundEmitterSystem::Flush()
{
    Shutdown();
    soundemitterbase->Flush();

#ifdef CLIENT_DLL
#ifdef GAMEUI_UISYSTEM2_ENABLED
    g_pGameUIGameSystem->ReloadSounds();
#endif
#endif

    Init();
}

//================================================================================
//================================================================================
void CSoundEmitterSystem::PreloadSounds()
{
    for ( int i = soundemitterbase->First(); i != soundemitterbase->InvalidIndex(); i = soundemitterbase->Next(i) ) {
        CSoundParametersInternal *pParams = soundemitterbase->InternalGetParametersForSound(i);

        if ( pParams->ShouldPreload() ) {
            InternalPrecacheWaves(i);
        }
    }
}

//================================================================================
//================================================================================
void CSoundEmitterSystem::InternalPrecacheWaves(int soundIndex)
{
    CSoundParametersInternal *internal = soundemitterbase->InternalGetParametersForSound(soundIndex);

    if ( !internal )
        return;

    int waveCount = internal->NumSoundNames();

    if ( !waveCount ) {
        DevMsg("CSoundEmitterSystem:  sounds.txt entry '%s' has no waves listed under 'wave' or 'rndwave' key!!!\n",
               soundemitterbase->GetSoundName(soundIndex));
    }
    else {
        g_bPermitDirectSoundPrecache = true;

        for ( int wave = 0; wave < waveCount; wave++ ) {
            CBaseEntity::PrecacheSound(soundemitterbase->GetWaveName(internal->GetSoundNames()[wave].symbol));
        }

        g_bPermitDirectSoundPrecache = false;
    }
}

//================================================================================
//================================================================================
void CSoundEmitterSystem::InternalPrefetchWaves(int soundIndex)
{
    CSoundParametersInternal *internal = soundemitterbase->InternalGetParametersForSound(soundIndex);

    if ( !internal )
        return;

    int waveCount = internal->NumSoundNames();

    if ( !waveCount ) {
        DevMsg("CSoundEmitterSystem:  sounds.txt entry '%s' has no waves listed under 'wave' or 'rndwave' key!!!\n",
               soundemitterbase->GetSoundName(soundIndex));
    }
    else {
        for ( int wave = 0; wave < waveCount; wave++ ) {
            CBaseEntity::PrefetchSound(soundemitterbase->GetWaveName(internal->GetSoundNames()[wave].symbol));
        }
    }
}

//================================================================================
//================================================================================
HSOUNDSCRIPTHANDLE CSoundEmitterSystem::PrecacheScriptSound(const char * soundname)
{
    int soundIndex = soundemitterbase->GetSoundIndex(soundname);

    if ( !soundemitterbase->IsValidIndex(soundIndex) ) {
        if ( Q_stristr(soundname, ".wav") || Q_strstr(soundname, ".mp3") ) {
            g_bPermitDirectSoundPrecache = true;
            CBaseEntity::PrecacheSound(soundname);
            g_bPermitDirectSoundPrecache = false;

            return SOUNDEMITTER_INVALID_HANDLE;
        }

#ifndef CLIENT_DLL
        if ( soundname[0] ) {
            static CUtlSymbolTable s_PrecacheScriptSoundFailures;

            // Make sure we only show the message once
            if ( UTL_INVAL_SYMBOL == s_PrecacheScriptSoundFailures.Find(soundname) ) {
                Warning("PrecacheScriptSound '%s' failed, no such sound script entry\n", soundname);
                s_PrecacheScriptSoundFailures.AddString(soundname);
            }
        }
#endif
        return (HSOUNDSCRIPTHANDLE)soundIndex;
    }

#ifndef( CLIENT_DLL )
    LogPrecache(soundname);
#endif

    InternalPrecacheWaves(soundIndex);
    return (HSOUNDSCRIPTHANDLE)soundIndex;
}

//================================================================================
//================================================================================
void CSoundEmitterSystem::PrefetchScriptSound(const char * soundname)
{
    int soundIndex = soundemitterbase->GetSoundIndex(soundname);

    if ( !soundemitterbase->IsValidIndex(soundIndex) ) {
        if ( Q_stristr(soundname, ".wav") || Q_strstr(soundname, ".mp3") ) {
            CBaseEntity::PrefetchSound(soundname);
        }

        return;
    }

    InternalPrefetchWaves(soundIndex);
}

//================================================================================
//================================================================================
void CSoundEmitterSystem::EmitSoundByHandle(IRecipientFilter & filter, int entindex, const EmitSound_t & ep, HSOUNDSCRIPTHANDLE & handle)
{
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

    if ( !Q_strncasecmp(params.soundname, "vo", 2) &&
        !(params.channel == CHAN_STREAM ||
        params.channel == CHAN_VOICE) ) {
        DevMsg("EmitSound:  Voice wave file %s doesn't specify CHAN_VOICE or CHAN_STREAM for sound %s\n", params.soundname, ep.m_pSoundName);
    }

    // handle SND_CHANGEPITCH/SND_CHANGEVOL and other sound flags.etc.
    if ( ep.m_nFlags & SND_CHANGE_PITCH ) {
        params.pitch = ep.m_nPitch;
    }

    if ( ep.m_nFlags & SND_CHANGE_VOL ) {
        params.volume = ep.m_flVolume;
    }

#ifndef CLIENT_DLL
    bool bSwallowed = CEnvMicrophone::OnSoundPlayed(
        entindex,
        params.soundname,
        params.soundlevel,
        params.volume,
        ep.m_nFlags,
        params.pitch,
        ep.m_pOrigin,
        ep.m_flSoundTime,
        ep.m_UtlVecSoundOrigin);

    if ( bSwallowed )
        return;
#endif

#if defined( _DEBUG ) && !defined( CLIENT_DLL )
    if ( !enginesound->IsSoundPrecached(params.soundname) ) {
        Msg("Sound %s:%s was not precached\n", ep.m_pSoundName, params.soundname);
    }
#endif

    float st = ep.m_flSoundTime;

    if ( !st && params.delay_msec != 0 ) {
        st = gpGlobals->curtime + (float)params.delay_msec / 1000.f;
    }

    // TERROR:
    float startTime = Plat_FloatTime();

    enginesound->EmitSound(
        filter,
        entindex,
        params.channel,
        params.soundname,
        params.volume,
        (soundlevel_t)params.soundlevel,
        ep.m_nFlags,
        params.pitch,
        ep.m_pOrigin,
        NULL,
        &ep.m_UtlVecSoundOrigin,
        true,
        st,
        ep.m_nSpeakerEntity);

    if ( ep.m_pflSoundDuration ) {
#ifdef GAME_DLL
        float startTime = Plat_FloatTime();
#endif
        *ep.m_pflSoundDuration = enginesound->GetSoundDuration(params.soundname);

#ifdef GAME_DLL
        float timeSpent = (Plat_FloatTime() - startTime) * 1000.0f;
        const float thinkLimit = 10.0f;

        if ( timeSpent > thinkLimit ) {
            UTIL_LogPrintf("getting sound duration for %s took %f milliseconds\n", params.soundname, timeSpent);
        }
#endif
    }

    // TERROR:
    float timeSpent = (Plat_FloatTime() - startTime) * 1000.0f;
    const float thinkLimit = 50.0f;

    if ( timeSpent > thinkLimit ) {
#ifdef GAME_DLL
        UTIL_LogPrintf("EmitSoundByHandle(%s) took %f milliseconds (server)\n",
                       ep.m_pSoundName, timeSpent);
#else
        DevMsg("EmitSoundByHandle(%s) took %f milliseconds (client)\n",
               ep.m_pSoundName, timeSpent);
#endif
    }

    TraceEmitSound(entindex, "EmitSound:  '%s' emitted as '%s' (ent %i)\n", ep.m_pSoundName, params.soundname, entindex);

    // Don't caption modulations to the sound
    if ( !(ep.m_nFlags & (SND_CHANGE_PITCH | SND_CHANGE_VOL)) ) {
        EmitCloseCaption(filter, entindex, params, ep);
    }
}

//================================================================================
//================================================================================
void CSoundEmitterSystem::EmitSound(IRecipientFilter & filter, int entindex, const EmitSound_t & ep)
{
    VPROF("CSoundEmitterSystem::EmitSound (calls engine)");

    if ( ep.m_pSoundName &&
        (Q_stristr(ep.m_pSoundName, ".wav") ||
        Q_stristr(ep.m_pSoundName, ".mp3") ||
        ep.m_pSoundName[0] == '!') ) {
#ifndef CLIENT_DLL
        bool bSwallowed = CEnvMicrophone::OnSoundPlayed(
            entindex,
            ep.m_pSoundName,
            ep.m_SoundLevel,
            ep.m_flVolume,
            ep.m_nFlags,
            ep.m_nPitch,
            ep.m_pOrigin,
            ep.m_flSoundTime,
            ep.m_UtlVecSoundOrigin);

        if ( bSwallowed )
            return;
#endif

        // TERROR:
        float startTime = Plat_FloatTime();

        if ( ep.m_bWarnOnDirectWaveReference &&
            Q_stristr(ep.m_pSoundName, ".wav") ) {
            WaveTrace(ep.m_pSoundName, "Emitsound");
        }

#if defined( _DEBUG ) && !defined( CLIENT_DLL )
        if ( !enginesound->IsSoundPrecached(ep.m_pSoundName) ) {
            Msg("Sound %s was not precached\n", ep.m_pSoundName);
        }
#endif

        enginesound->EmitSound(
            filter,
            entindex,
            ep.m_nChannel,
            ep.m_pSoundName,
            ep.m_flVolume,
            ep.m_SoundLevel,
            ep.m_nFlags,
            ep.m_nPitch,
            ep.m_pOrigin,
            NULL,
            &ep.m_UtlVecSoundOrigin,
            true,
            ep.m_flSoundTime,
            ep.m_nSpeakerEntity);

        try {
            if ( ep.m_pflSoundDuration ) {
                // TERROR:
#ifdef GAME_DLL
                UTIL_LogPrintf("getting wav duration for %s\n", ep.m_pSoundName);
#endif
                VPROF("CSoundEmitterSystem::EmitSound GetSoundDuration (calls engine)");
                *ep.m_pflSoundDuration = enginesound->GetSoundDuration(ep.m_pSoundName);
            }
        }
        catch ( ... ) {
            // TODO: FIXME
            Assert(!"ep.m_pflSoundDuration Exception");
            return;
        }

        TraceEmitSound(entindex, "%f EmitSound:  Raw wave emitted '%s' (ent %i) (vol %f)\n", gpGlobals->curtime, ep.m_pSoundName, entindex, ep.m_flVolume);

        // TERROR:
        float timeSpent = (Plat_FloatTime() - startTime) * 1000.0f;
        const float thinkLimit = 50.0f;

        if ( timeSpent > thinkLimit ) {
#ifdef GAME_DLL
            UTIL_LogPrintf("CSoundEmitterSystem::EmitSound(%s) took %f milliseconds (server)\n",
                           ep.m_pSoundName, timeSpent);
#else
            DevMsg("CSoundEmitterSystem::EmitSound(%s) took %f milliseconds (client)\n",
                   ep.m_pSoundName, timeSpent);
#endif
        }

        return;
    }

    if ( ep.m_hSoundScriptHandle == SOUNDEMITTER_INVALID_HANDLE ) {
        ep.m_hSoundScriptHandle = (HSOUNDSCRIPTHANDLE)soundemitterbase->GetSoundIndex(ep.m_pSoundName);
    }

    if ( ep.m_hSoundScriptHandle == -1 )
        return;

    EmitSoundByHandle(filter, entindex, ep, ep.m_hSoundScriptHandle);
}
