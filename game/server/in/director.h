//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#ifndef DIRECTOR_H
#define DIRECTOR_H

#ifdef _WIN32
#pragma once
#endif

#include "sound_manager.h"
#include "directordefs.h"

#include "info_director.h"

enum
{
	HORDE_DEVIDDLE = 0,
	HORDE_DOBRO,
	HORDE_DRUMS,
	HORDE_ORGANS,
	HORDE_VIOLIN,

	LAST_HORDE_MUSIC
};

static const char *g_hordeMusic[] = {
	"Music.Director.Deviddle",
	"Music.Director.Dobro",
	"Music.Director.Drums",
	"Music.Director.Organs",
	"Music.Director.Violin"
};

//================================================================================
// El Director
//================================================================================
class Director : public CAutoGameSystemPerFrame
{
public:
    Director();
	DECLARE_ENT_SCRIPTDESC();

    // CAutoGameSystemPerFrame
    virtual void PostInit();
    virtual void Shutdown();

    virtual void LevelInitPreEntity();
    virtual void LevelInitPostEntity();

    virtual void LevelShutdownPreEntity();
    virtual void LevelShutdownPostEntity();

    virtual void FrameUpdatePostEntityThink();

    // Devolución
    virtual bool IsDisabled() {
        return m_bDisabled;
    }

    virtual bool IsOnPanicEvent() {
        return (GetStatus() == STATUS_PANIC || GetStatus() == STATUS_FINALE);
    }

	// Del Manager
	virtual void SetPopulation( const char *type );
    
    // Principales
	virtual void Resume();
    virtual void Stop();

    virtual void Reset();

	virtual void StartMap();
    virtual void StartCampaign();

    virtual void FindDirectorEntity();

    virtual void Think();

    virtual void PreUpdate();
    virtual void Update();
	virtual void PostUpdate();

    virtual void UpdatePhase();
    virtual void UpdateAngry();
    virtual void UpdatePanicEvent();

	virtual bool ShouldPhaseEnd( DirectorPhase phase );
    virtual void OnPhaseStart( DirectorPhase phase );
	virtual void OnPhaseEnd( DirectorPhase phase );

	virtual float GetPanicEventDelay();
	virtual int GetPanicEventHordes();
    virtual bool ShouldStartPanicEvent();

    virtual void StartPanic( int hordes );
    virtual void EndPanicHorde();

	virtual void OnPanicStart();
	virtual void OnPanicHordeEnd( int hordeNumber, int left );
	virtual void OnPanicEnd();

	// Música
	virtual void CreateMusic();
	virtual float SoundDesire( const char *soundname, int channel );

	virtual void OnSoundPlay( const char *soundname );
	virtual void OnSoundStop( const char *soundname ) { }

	// Creación
	virtual bool CanSpawnMinions();
    virtual bool CanSpawnMinions( MinionType type );
    virtual bool CanSpawnMinion( CMinionInfo *info );

	virtual bool ScriptCanSpawnMinions() { return CanSpawnMinions(); }
	virtual bool ScriptCanSpawnMinionsByType( int type ) { return CanSpawnMinions( (MinionType)type ); }

    // Configuración
    virtual int GetDifficulty();

    virtual float GetMaxDistance();
    virtual float GetMinDistance();

    virtual float GetMinStress();
    virtual float GetMaxStress();
    virtual float GetStress();

    virtual int GetMaxUnits();
	virtual int GetMaxUnits( MinionType type );

	virtual int ScriptGetMaxUnits() { return GetMaxUnits(); }
	virtual int ScriptGetMaxUnitsByType( int type ) { return GetMaxUnits((MinionType)type); }

	// Utilidades
    virtual void Disclose();
    virtual void KillAll( bool onlyNoVisible = true );

	// Depuración
	virtual void DebugDisplay();
    virtual void DebugScreenText( const char *pText, ... );

    // Estado
    virtual void Set( DirectorStatus status, DirectorPhase phase, float duration );
	virtual void ScriptSet( int status, int phase, float duration );

    DirectorStatus GetStatus() { return m_iStatus; }
    virtual bool IsStatus( DirectorStatus status ) { return (GetStatus() == status); }
    virtual void SetStatus( DirectorStatus status );

	int ScriptGetStatus() { return (int)m_iStatus; }
    virtual bool ScriptIsStatus( int status ) { return (ScriptGetStatus() == status); }
    virtual void ScriptSetStatus( int status ) { SetStatus( (DirectorStatus)status ); }

    DirectorPhase GetPhase() { return m_iPhase; }
    virtual bool IsPhase( DirectorPhase phase ) { return (GetPhase() == phase); }
    virtual void SetPhase( DirectorPhase phase, float duration = -1.0f );

	int ScriptGetPhase() { return (int)m_iPhase; }
    virtual bool ScriptIsPhase( int phase ) { return (ScriptGetPhase() == phase); }
    virtual void ScriptSetPhase( int phase, float duration = -1.0f ) { SetPhase( (DirectorPhase)phase, duration ); }

    DirectorAngry GetAngry() { return m_iAngry; }
    virtual void SetAngry( DirectorAngry status );

	int ScriptGetAngry() { return (int)m_iAngry; }
    virtual void ScriptSetAngry( int status ) { SetAngry( (DirectorAngry)status ); }

    // Virtual Machine
    virtual void StartVirtualMachine();

    virtual bool CallScriptFunction( const char *pFunctionName, ScriptVariant_t *pFunctionReturn );

    template <typename ARG_TYPE_1>
    bool CallScriptFunction( const char *pFunctionName, ScriptVariant_t *pFunctionReturn, ARG_TYPE_1 arg1 );

protected:
    DirectorStatus m_iStatus;
    DirectorPhase m_iPhase;
    DirectorAngry m_iAngry;

    CountdownTimer m_PhaseTimer;
    CountdownTimer m_ThinkTimer;

    CountdownTimer m_SustainTimer;
    CountdownTimer m_PanicTimer;

    IntervalTimer m_MapTimer;
    IntervalTimer m_GameTimer;

    CInfoDirector *m_pEntity;

    bool m_bDisabled;
    int m_iDebugLine;
    int m_iPanicEventHordesLeft;

protected:
    CountdownTimer m_BackgroundMusicTimer;
    CountdownTimer m_ChoirTimer;

    CSoundManager *m_MusicManager;
    CSoundInstance *m_HordeMusicList[LAST_HORDE_MUSIC];
    CSoundInstance *m_HordeMusic = NULL;

protected:
    CScriptScope m_ScriptScope;
    HSCRIPT m_hScriptInstance;
    string_t m_szScriptID;

    friend class DirectorManager;
	friend class CInfoDirector;
};

extern Director *TheDirector;

template<typename ARG_TYPE_1>
inline bool Director::CallScriptFunction( const char * pFunctionName, ScriptVariant_t * pFunctionReturn, ARG_TYPE_1 arg1 ) 
{
	if ( !m_ScriptScope.IsInitialized() )
		return false;

	HSCRIPT hFunc = m_ScriptScope.LookupFunction( pFunctionName );

	if( hFunc )
	{
		m_ScriptScope.Call<ARG_TYPE_1>( hFunc, pFunctionReturn, arg1 );
		m_ScriptScope.ReleaseFunction( hFunc );

		return true;
	}

	return false;
}


#endif // DIRECTOR_H