//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//

#ifndef SOUND_EMITTER_SYSTEM
#define SOUND_EMITTER_SYSTEM

#pragma once

#ifndef CLIENT_DLL
#include "closedcaptions.h"
#include "envmicrophone.h"
#include "sceneentity.h"
#endif

//================================================================================
//================================================================================
class CSoundEmitterSystem : public CBaseGameSystem
{
public:
	virtual char const *Name() {
		return "CSoundEmitterSystem";
	}

#ifndef CLIENT_DLL
	CSoundEmitterSystem( char const *pszName );

	void LogPrecache( char const *soundname );
	void StartLog();
	void FinishLog();

	bool m_bLogPrecache;
	FileHandle_t	m_hPrecacheLogFile;
	CUtlSymbolTable m_PrecachedScriptSounds;
	CUtlVector< AsyncCaption_t > m_ServerCaptions;
#else
	CSoundEmitterSystem( char const *name )
	{
	}
#endif

	// IServerSystem stuff
	virtual bool Init();
	virtual void Shutdown();

	// Precache all wave files referenced in wave or rndwave keys
	virtual void LevelInitPreEntity();
	virtual void LevelShutdownPostEntity();

	virtual void TraceEmitSound( int originEnt, char const *fmt, ... );

	void Flush();
	void PreloadSounds();

	virtual HSOUNDSCRIPTHANDLE PrecacheScriptSound( const char *soundname );
	virtual void PrefetchScriptSound( const char *soundname );

    virtual void PrecacheWaves(int soundIndex);
    virtual void PrefetchWaves(int soundIndex);

    virtual void PrecacheWave(const char *soundwave);

public:

    virtual void EmitSoundByHandle(IRecipientFilter& filter, int entindex, const EmitSound_t & ep, HSOUNDSCRIPTHANDLE& handle);
    virtual void EmitSound(IRecipientFilter& filter, int entindex, const EmitSound_t & ep);

    virtual void EmitAmbientSound(int entindex, const Vector &origin, const char *pSample, float volume, soundlevel_t soundlevel, int flags, int pitch, float soundtime /*= 0.0f*/, float *duration /*=NULL*/);
    virtual void EmitAmbientSound(int entindex, const Vector& origin, const char *soundname, float flVolume, int iFlags, int iPitch, float soundtime /*= 0.0f*/, float *duration /*=NULL*/);

    void StopSoundByHandle(int entindex, const char *soundname, HSOUNDSCRIPTHANDLE& handle, bool bIsStoppingSpeakerSound = false);

    void StopSound(int entindex, const char *soundname);
    void StopSound(int iEntIndex, int iChannel, const char *pSample, bool bIsStoppingSpeakerSound = false);

    void EmitCloseCaption(IRecipientFilter& filter, int entindex, bool fromplayer, char const *token, CUtlVector< Vector >& originlist, float duration, bool warnifmissing /*= false*/, bool bForceSubtitle = false);
    void EmitCloseCaption(IRecipientFilter& filter, int entindex, const CSoundParameters & params, const EmitSound_t & ep);

#ifndef CLIENT_DLL
    bool GetCaptionHash(char const *pchStringName, bool bWarnIfMissing, unsigned int &hash);
	
    void StopSpeakerSounds( const char *wavename )
	{
		// Stop sound on any speakers playing this wav name
		// but don't recurse in if this stopsound is happening on a speaker
		CEnvMicrophone::OnSoundStopped( wavename );
	}
#endif
};

#endif