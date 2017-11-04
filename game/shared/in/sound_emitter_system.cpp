//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//

#include "cbase.h"
#include "sound_emitter_system.h"

#include "engine/IEngineSound.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"
#include "filesystem.h"
#include "soundchars.h"

#ifndef CLIENT_DLL

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

ConVar sv_soundemitter_trace("sv_soundemitter_trace", "-1", FCVAR_REPLICATED, "Show all EmitSound calls including their symbolic name and the actual wave file they resolved to. (-1 = for nobody, 0 = for everybody, n = for one entity)\n");
ConVar cc_showmissing("cc_showmissing", "0", FCVAR_REPLICATED, "Show missing closecaption entries.");

//================================================================================
//================================================================================

extern ISoundEmitterSystemBase *soundemitterbase;
static ConVar *g_pClosecaption = NULL;

#ifdef _XBOX
int LookupStringFromCloseCaptionToken(char const *token);
const wchar_t *GetStringForIndex(int index);
#endif

bool g_bPermitDirectSoundPrecache = false;

//================================================================================
// Captions
//================================================================================
#ifndef CLIENT_DLL
static ConVar cc_norepeat("cc_norepeat", "5", 0, "In multiplayer games, don't repeat captions more often than this many seconds.");

class CCaptionRepeatMgr
{
public:

    CCaptionRepeatMgr() :
        m_rbCaptionHistory(0, 0, DefLessFunc(unsigned int))
    {
    }

    bool CanEmitCaption(unsigned int hash);

    void Clear();

private:

    void RemoveCaptionsBefore(float t);

    struct CaptionItem_t
    {
        unsigned int	hash;
        float			realtime;

        static bool Less(const CaptionItem_t &lhs, const CaptionItem_t &rhs)
        {
            return lhs.hash < rhs.hash;
        }
    };

    CUtlMap< unsigned int, float > m_rbCaptionHistory;
};

static CCaptionRepeatMgr g_CaptionRepeats;

void CCaptionRepeatMgr::Clear()
{
    m_rbCaptionHistory.Purge();
}

bool CCaptionRepeatMgr::CanEmitCaption(unsigned int hash)
{
    // Don't cull in single player
    if ( gpGlobals->maxClients == 1 )
        return true;

    float realtime = gpGlobals->realtime;

    RemoveCaptionsBefore(realtime - cc_norepeat.GetFloat());

    int idx = m_rbCaptionHistory.Find(hash);
    if ( idx == m_rbCaptionHistory.InvalidIndex() ) {
        m_rbCaptionHistory.Insert(hash, realtime);
        return true;
    }

    float flLastEmitted = m_rbCaptionHistory[idx];
    if ( realtime - flLastEmitted > cc_norepeat.GetFloat() ) {
        m_rbCaptionHistory[idx] = realtime;
        return true;
    }

    return false;
}

void CCaptionRepeatMgr::RemoveCaptionsBefore(float t)
{
    CUtlVector< unsigned int > toRemove;
    FOR_EACH_MAP(m_rbCaptionHistory, i)
    {
        if ( m_rbCaptionHistory[i] < t ) {
            toRemove.AddToTail(m_rbCaptionHistory.Key(i));
        }
    }

    for ( int i = 0; i < toRemove.Count(); ++i ) {
        m_rbCaptionHistory.Remove(toRemove[i]);
    }
}

//================================================================================
//================================================================================
bool CanEmitCaption(unsigned int hash)
{
    return g_CaptionRepeats.CanEmitCaption(hash);
}

void ClearModelSoundsCache();

#endif // !CLIENT_DLL

//================================================================================
// Helpers
//================================================================================

void WaveTrace(char const *wavname, char const *funcname)
{
    if ( IsX360() && !IsDebug() ) {
        return;
    }

    static CUtlSymbolTable s_WaveTrace;

    // Make sure we only show the message once
    if ( UTL_INVAL_SYMBOL == s_WaveTrace.Find(wavname) ) {
        DevMsg("%s directly referenced wave %s (should use game_sounds.txt system instead)\n",
               funcname, wavname);
        s_WaveTrace.AddString(wavname);
    }
}

void Hack_FixEscapeChars(char *str)
{
    int len = Q_strlen(str) + 1;
    char *i = str;
    char *o = (char *)stackalloc(len);
    char *osave = o;
    while ( *i ) {
        if ( *i == '\\' ) {
            switch ( *(i + 1) ) {
                case 'n':
                    *o = '\n';
                    ++i;
                    break;
                default:
                    *o = *i;
                    break;
            }
        }
        else {
            *o = *i;
        }

        ++i;
        ++o;
    }
    *o = 0;
    Q_strncpy(str, osave, len);
}

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
            PrecacheWaves(i);
        }
    }
}

//================================================================================
//================================================================================
HSOUNDSCRIPTHANDLE CSoundEmitterSystem::PrecacheScriptSound(const char * soundname)
{
    int soundIndex = soundemitterbase->GetSoundIndex(soundname);

    if ( !soundemitterbase->IsValidIndex(soundIndex) ) {
        char ext[8];
        V_ExtractFileExtension(soundname, ext, sizeof(ext));

        if ( Q_stristr(ext, "wav") || Q_strstr(ext, "mp3") ) {
            PrecacheWave(soundname);
            return SOUNDEMITTER_INVALID_HANDLE;
        }

#ifndef CLIENT_DLL
        if ( soundname[0] ) {
            static CUtlSymbolTable s_PrecacheScriptSoundFailures;

            // Make sure we only show the message once
            if ( UTL_INVAL_SYMBOL == s_PrecacheScriptSoundFailures.Find(soundname) ) {
                Warning("[CSoundEmitterSystem] PrecacheScriptSound '%s' failed, no such sound script entry\n", soundname);
                s_PrecacheScriptSoundFailures.AddString(soundname);
            }
        }
#endif
        return (HSOUNDSCRIPTHANDLE)soundIndex;
    }

#ifndef CLIENT_DLL
    LogPrecache(soundname);
#endif

    PrecacheWaves(soundIndex);
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

    PrefetchWaves(soundIndex);
}

//================================================================================
//================================================================================
void CSoundEmitterSystem::PrecacheWaves(int soundIndex)
{
    CSoundParametersInternal *internal = soundemitterbase->InternalGetParametersForSound(soundIndex);

    if ( !internal )
        return;

    int waveCount = internal->NumSoundNames();

    if ( !waveCount ) {
        DevMsg("[CSoundEmitterSystem] sounds.txt entry '%s' has no waves listed under 'wave' or 'rndwave' key!!!\n",
               soundemitterbase->GetSoundName(soundIndex));
    }
    else {
        for ( int wave = 0; wave < waveCount; wave++ ) {
            PrecacheWave(soundemitterbase->GetWaveName(internal->GetSoundNames()[wave].symbol));
        }
    }
}

//================================================================================
//================================================================================
void CSoundEmitterSystem::PrefetchWaves(int soundIndex)
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
void CSoundEmitterSystem::PrecacheWave(const char *soundwave)
{
    g_bPermitDirectSoundPrecache = true;
    CBaseEntity::PrecacheSound(soundwave);
    g_bPermitDirectSoundPrecache = false;
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

//================================================================================
//================================================================================
void CSoundEmitterSystem::EmitAmbientSound(int entindex, const Vector & origin, const char * pSample, float volume, soundlevel_t soundlevel, int flags, int pitch, float soundtime, float * duration)
{
#if !defined( CLIENT_DLL )
    CUtlVector< Vector > dummyorigins;

    // Loop through all registered microphones and tell them the sound was just played
    // NOTE: This means that pitch shifts/sound changes on the original ambient will not be reflected in the re-broadcasted sound
    bool bSwallowed = CEnvMicrophone::OnSoundPlayed(
        entindex,
        pSample,
        soundlevel,
        volume,
        flags,
        pitch,
        &origin,
        soundtime,
        dummyorigins);

    if ( bSwallowed )
        return;
#endif

    if ( pSample && (Q_stristr(pSample, ".wav") || Q_stristr(pSample, ".mp3")) ) {
#ifdef CLIENT_DLL
        enginesound->EmitAmbientSound(pSample, volume, pitch, flags, soundtime);
#else
        engine->EmitAmbientSound(entindex, origin, pSample, volume, soundlevel, flags, pitch, soundtime);
#endif

        if ( duration ) {
            *duration = enginesound->GetSoundDuration(pSample);
        }

        TraceEmitSound(entindex, "EmitAmbientSound:  Raw wave emitted '%s' (ent %i)\n", pSample, entindex);
    }
    else {
        EmitAmbientSound(entindex, origin, pSample, volume, flags, pitch, soundtime, duration);
    }
}

//================================================================================
//================================================================================
void CSoundEmitterSystem::EmitAmbientSound(int entindex, const Vector & origin, const char * soundname, float flVolume, int iFlags, int iPitch, float soundtime, float * duration)
{
    // Pull data from parameters
    CSoundParameters params;

    if ( !soundemitterbase->GetParametersForSound(soundname, params, GENDER_NONE) ) {
        return;
    }

    if ( iFlags & SND_CHANGE_PITCH ) {
        params.pitch = iPitch;
    }

    if ( iFlags & SND_CHANGE_VOL ) {
        params.volume = flVolume;
    }

#ifdef CLIENT_DLL
    enginesound->EmitAmbientSound(params.soundname, params.volume, params.pitch, iFlags, soundtime);
#else
    engine->EmitAmbientSound(entindex, origin, params.soundname, params.volume, params.soundlevel, iFlags, params.pitch, soundtime);
#endif

    bool needsCC = !(iFlags & (SND_STOP | SND_CHANGE_VOL | SND_CHANGE_PITCH));
    float soundduration = 0.0f;

    if ( duration || needsCC ) {
        soundduration = enginesound->GetSoundDuration(params.soundname);

        if ( duration ) {
            *duration = soundduration;
        }
    }

    TraceEmitSound(entindex, "EmitAmbientSound:  '%s' emitted as '%s' (ent %i)\n", soundname, params.soundname, entindex);

    // We only want to trigger the CC on the start of the sound, not on any changes or halting of the sound
    if ( needsCC ) {
        CRecipientFilter filter;
        filter.AddAllPlayers();
        filter.MakeReliable();

        CUtlVector< Vector > dummy;
        EmitCloseCaption(filter, entindex, false, soundname, dummy, soundduration, false);
    }
}

//================================================================================
//================================================================================
void CSoundEmitterSystem::StopSoundByHandle(int entindex, const char * soundname, HSOUNDSCRIPTHANDLE & handle, bool bIsStoppingSpeakerSound)
{
    if ( handle == SOUNDEMITTER_INVALID_HANDLE ) {
        handle = (HSOUNDSCRIPTHANDLE)soundemitterbase->GetSoundIndex(soundname);
    }

    if ( handle == SOUNDEMITTER_INVALID_HANDLE )
        return;

    CSoundParametersInternal *params;

    params = soundemitterbase->InternalGetParametersForSound((int)handle);

    if ( !params ) {
        return;
    }

    // HACK:  we have to stop all sounds if there are > 1 in the rndwave section...
    int c = params->NumSoundNames();

    for ( int i = 0; i < c; ++i ) {
        char const *wavename = soundemitterbase->GetWaveName(params->GetSoundNames()[i].symbol);
        Assert(wavename);

        enginesound->StopSound(entindex, params->GetChannel(), wavename);
        TraceEmitSound(entindex, "StopSound:  '%s' stopped as '%s' (ent %i)\n", soundname, wavename, entindex);

#ifndef CLIENT_DLL
        if ( bIsStoppingSpeakerSound == false ) {
            StopSpeakerSounds(wavename);
        }
#endif
    }
}

//================================================================================
//================================================================================
void CSoundEmitterSystem::StopSound(int entindex, const char * soundname)
{
    HSOUNDSCRIPTHANDLE handle = (HSOUNDSCRIPTHANDLE)soundemitterbase->GetSoundIndex(soundname);

    if ( handle == SOUNDEMITTER_INVALID_HANDLE ) {
        return;
    }

    StopSoundByHandle(entindex, soundname, handle);
}

//================================================================================
//================================================================================
void CSoundEmitterSystem::StopSound(int iEntIndex, int iChannel, const char * pSample, bool bIsStoppingSpeakerSound)
{
    if ( pSample && (Q_stristr(pSample, ".wav") || Q_stristr(pSample, ".mp3") || pSample[0] == '!') ) {
        enginesound->StopSound(iEntIndex, iChannel, pSample);
        TraceEmitSound(iEntIndex, "StopSound:  Raw wave stopped '%s' (ent %i)\n", pSample, iEntIndex);

#ifndef CLIENT_DLL
        if ( bIsStoppingSpeakerSound == false ) {
            StopSpeakerSounds(pSample);
        }
#endif
    }
    else {
        // Look it up in sounds.txt and ignore other parameters
        StopSound(iEntIndex, pSample);
    }
}

//================================================================================
//================================================================================
void CSoundEmitterSystem::EmitCloseCaption(IRecipientFilter & filter, int entindex, bool fromplayer, char const * token, CUtlVector<Vector>& originlist, float duration, bool warnifmissing, bool bForceSubtitle)
{
    // Don't use dedicated closecaption ConVar since it will prevent remote clients from getting captions.
    // Okay to use it in SP, since it's the same ConVar, not the FCVAR_USERINFO one
    if ( gpGlobals->maxClients == 1 && !g_pClosecaption->GetBool() ) {
        return;
    }

    // A negative duration means fill it in from the wav file if possible
    if ( duration < 0.0f ) {
        char const *wav = soundemitterbase->GetWavFileForSound(token, GENDER_NONE);
        if ( wav ) {
            duration = enginesound->GetSoundDuration(wav);
        }
        else {
            duration = 2.0f;
        }
    }

    char lowercase[256];
    Q_strncpy(lowercase, token, sizeof(lowercase));
    Q_strlower(lowercase);

    if ( Q_strstr(lowercase, "\\") ) {
        Hack_FixEscapeChars(lowercase);
    }

    // NOTE:  We must make a copy or else if the filter is owned by a SoundPatch, we'll end up destructively removing
    //  all players from it!!!!
    CRecipientFilter filterCopy;
    filterCopy.CopyFrom((CRecipientFilter &)filter);

    // Captions only route to host player (there is only one closecaptioning HUD)
    filterCopy.RemoveSplitScreenPlayers();

    if ( !bForceSubtitle ) {
        // Remove any players who don't want close captions
        CBaseEntity::RemoveRecipientsIfNotCloseCaptioning((CRecipientFilter &)filterCopy);
    }

#ifndef CLIENT_DLL
    // Defined in sceneentity.cpp
    bool AttenuateCaption(const char *token, const Vector& listener, CUtlVector< Vector >& soundorigins);

    if ( filterCopy.GetRecipientCount() > 0 ) {
        int c = filterCopy.GetRecipientCount();
        for ( int i = c - 1; i >= 0; --i ) {
            CBasePlayer *player = UTIL_PlayerByIndex(filterCopy.GetRecipientIndex(i));
            if ( !player )
                continue;

            Vector playerEarPosition = player->EarPosition();

            if ( AttenuateCaption(lowercase, playerEarPosition, originlist) ) {
                filterCopy.RemoveRecipient(player);
            }
        }
    }
#endif

    // Anyone left?
    if ( filterCopy.GetRecipientCount() > 0 ) {
#ifndef CLIENT_DLL
        char lowercase_nogender[256];
        Q_strncpy(lowercase_nogender, lowercase, sizeof(lowercase_nogender));
        bool bTriedGender = false;

        CBaseEntity *pActor = CBaseEntity::Instance(entindex);
        if ( pActor ) {
            char const *pszActorModel = STRING(pActor->GetModelName());
            gender_t gender = soundemitterbase->GetActorGender(pszActorModel);

            if ( gender == GENDER_MALE ) {
                Q_strncat(lowercase, "_male", sizeof(lowercase), COPY_ALL_CHARACTERS);
                bTriedGender = true;
            }
            else if ( gender == GENDER_FEMALE ) {
                Q_strncat(lowercase, "_female", sizeof(lowercase), COPY_ALL_CHARACTERS);
                bTriedGender = true;
            }
        }

        unsigned int hash = 0u;
        bool bFound = GetCaptionHash(lowercase, true, hash);

        // if not found, try the no-gender version
        if ( !bFound && bTriedGender ) {
            bFound = GetCaptionHash(lowercase_nogender, true, hash);
        }

        if ( bFound ) {
            if ( g_CaptionRepeats.CanEmitCaption(hash) ) {
                if ( bForceSubtitle ) {
                    // Send forced caption and duration hint down to client
                    UserMessageBegin(filterCopy, "CloseCaptionDirect");
                    WRITE_LONG(hash);
                    WRITE_UBITLONG(clamp((int)(duration * 10.0f), 0, 65535), 15),
                        WRITE_UBITLONG(fromplayer ? 1 : 0, 1),
                        MessageEnd();
                }
                else {
                    // Send caption and duration hint down to client
                    UserMessageBegin(filterCopy, "CloseCaption");
                    WRITE_LONG(hash);
                    WRITE_UBITLONG(clamp((int)(duration * 10.0f), 0, 65535), 15),
                        WRITE_UBITLONG(fromplayer ? 1 : 0, 1),
                        MessageEnd();
                }
            }
        }
#else
        // Direct dispatch
        CHudCloseCaption *cchud = GET_FULLSCREEN_HUDELEMENT(CHudCloseCaption);
        if ( cchud ) {
            cchud->ProcessCaption(lowercase, duration, fromplayer);
        }
#endif
    }
}

//================================================================================
//================================================================================
void CSoundEmitterSystem::EmitCloseCaption(IRecipientFilter & filter, int entindex, const CSoundParameters & params, const EmitSound_t & ep)
{
    // Don't use dedicated closecaption ConVar since it will prevent remote clients from getting captions.
    // Okay to use it in SP, since it's the same ConVar, not the FCVAR_USERINFO one
    if ( gpGlobals->maxClients == 1 &&
        !g_pClosecaption->GetBool() ) {
        return;
    }

    bool bForceSubtitle = false;

    if ( TestSoundChar(params.soundname, CHAR_SUBTITLED) ) {
        bForceSubtitle = true;
    }

    if ( !bForceSubtitle && !ep.m_bEmitCloseCaption ) {
        return;
    }

    // NOTE:  We must make a copy or else if the filter is owned by a SoundPatch, we'll end up destructively removing
    //  all players from it!!!!
    CRecipientFilter filterCopy;
    filterCopy.CopyFrom((CRecipientFilter &)filter);

    if ( !bForceSubtitle ) {
        // Remove any players who don't want close captions
        CBaseEntity::RemoveRecipientsIfNotCloseCaptioning((CRecipientFilter &)filterCopy);
    }

    // Anyone left?
    if ( filterCopy.GetRecipientCount() <= 0 ) {
        return;
    }

    float duration = 0.0f;
    if ( ep.m_pflSoundDuration ) {
        duration = *ep.m_pflSoundDuration;
    }
    else {
        duration = enginesound->GetSoundDuration(params.soundname);
    }

    bool fromplayer = false;
    CBaseEntity *ent = CBaseEntity::Instance(entindex);
    if ( ent ) {
        while ( ent ) {
            if ( ent->IsPlayer() ) {
                fromplayer = true;
                break;
            }

            ent = ent->GetOwnerEntity();
        }
    }
    EmitCloseCaption(filter, entindex, fromplayer, ep.m_pSoundName, ep.m_UtlVecSoundOrigin, duration, ep.m_bWarnOnMissingCloseCaption, bForceSubtitle);
}

#ifndef CLIENT_DLL
//================================================================================
//================================================================================
bool CSoundEmitterSystem::GetCaptionHash(char const * pchStringName, bool bWarnIfMissing, unsigned int & hash)
{
    // hash the string, find in dictionary or return 0u if not there!!!
    CUtlVector< AsyncCaption_t >& directories = m_ServerCaptions;

    CaptionLookup_t search;
    search.SetHash(pchStringName);
    hash = search.hash;

    int idx = -1;
    int i;
    int dc = directories.Count();
    for ( i = 0; i < dc; ++i ) {
        idx = directories[i].m_CaptionDirectory.Find(search);
        if ( idx == directories[i].m_CaptionDirectory.InvalidIndex() )
            continue;

        break;
    }

    if ( i >= dc || idx == -1 ) {
        if ( bWarnIfMissing && cc_showmissing.GetBool() ) {
            static CUtlRBTree< unsigned int > s_MissingHashes(0, 0, DefLessFunc(unsigned int));
            if ( s_MissingHashes.Find(hash) == s_MissingHashes.InvalidIndex() ) {
                s_MissingHashes.Insert(hash);
                Msg("Missing caption for %s\n", pchStringName);
            }
        }
        return false;
    }

    // Anything marked as L"" by content folks doesn't need to transmit either!!!
    CaptionLookup_t &entry = directories[i].m_CaptionDirectory[idx];
    if ( entry.length <= sizeof(wchar_t) ) {
        return false;
    }

    return true;
}
#endif