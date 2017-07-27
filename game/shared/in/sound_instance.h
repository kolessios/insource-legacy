//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef SOUND_INSTANCE_H
#define SOUND_INSTANCE_H

#ifdef _WIN32
#pragma once
#endif

#ifndef CLIENT_DLL
	#include "in_player.h"
	#include "players_system.h"
#else
	#include "c_in_player.h"
#endif

#ifndef CLIENT_DLL
	#include "soundent.h"
#else
	class CSoundPatch;

	#define CRecipientFilter C_RecipientFilter
	#define CPlayer C_Player

	#define FOR_EACH_PLAYER( code ) for ( int it = 1; it <= gpGlobals->maxClients; ++it )  {\
		C_Player *pPlayer = ToInPlayer(UTIL_PlayerByIndex( it ));\
		if ( !pPlayer ) continue;\
		code\
	}
#endif

#include "soundenvelope.h"
#include "engine/IEngineSound.h"

#define SOUND_CONTROLLER (CSoundEnvelopeController::GetController())

//================================================================================
//================================================================================
enum
{
	TARGET_NONE = 0,
	TARGET_ONLY,
	TARGET_EXCEPT,

	LAST_TARGET
};

class CSoundInstance;

//================================================================================
//================================================================================
class SoundInfo
{
public:
    SoundInfo()
    {
        team = TEAM_ANY;
        originEntity = NULL;
        owner = NULL;
        target = TARGET_NONE;
        tag = NULL;
        isLoop = false;
        channel = CHANNEL_ANY;
    }

    SoundInfo( const char *pSoundName, bool looping = false, int teamm = TEAM_ANY, CBaseEntity *pOrigin = NULL, CBaseEntity *pOwner = NULL, int targett = TARGET_NONE, CSoundInstance *soundTag = NULL )
    {
        soundname = pSoundName;
        team = teamm;
        originEntity = pOrigin;
        owner = pOwner;
        target = targett;
        tag = soundTag;
        isLoop = looping;
        channel = CHANNEL_ANY;
    }

    SoundInfo( const char *pSoundName, int teamm = TEAM_ANY, CBaseEntity *pOrigin = NULL, CBaseEntity *pOwner = NULL, int targett = TARGET_NONE, CSoundInstance *soundTag = NULL )
    {
        soundname = pSoundName;
        team = teamm;
        originEntity = pOrigin;
        owner = pOwner;
        target = targett;
        tag = soundTag;
        isLoop = false;
        channel = CHANNEL_ANY;
    }

public:
    const char *soundname;
    int team;
    EHANDLE originEntity;
    EHANDLE owner;
    int target;
    CSoundInstance *tag;
    bool isLoop;
    int channel;
};

#define SOUND_INFO new SoundInfo

//================================================================================
// Clase ideal para crear sonido/música y controlarla por código
//================================================================================
class CSoundInstance
{
public:
	DECLARE_CLASS_NOBASE( CSoundInstance );

	CSoundInstance( const char *pSoundName, int team = TEAM_ANY, CBaseEntity *pOrigin = NULL, int target = TARGET_NONE, CPlayer *pOwner = NULL );
    CSoundInstance( const char *pSoundName, CBaseEntity *pOrigin = NULL, CPlayer *pOwner = NULL );
	CSoundInstance( SoundInfo *info, CSoundInstance *master = NULL );
    ~CSoundInstance();

    virtual void Setup( SoundInfo *info, CSoundInstance *master = NULL );
	virtual bool Init();

	virtual void Update();
	virtual float GetDesire();

	virtual bool ShouldListen( CPlayer *pPlayer, CRecipientFilter &filter );
	virtual bool ShouldListen( CPlayer *pPlayer = NULL );

	virtual void Play( float volume = -1.0f, int pitch = -1 );
	virtual void Stop();

	virtual void Fadeout( float deltaTime = 0.5f, bool stop = true );

	virtual float GetVolume() { return m_nParams.volume; }
	virtual float GetSoundVolume() { return m_nInfo.m_flVolume; }
	virtual void SetVolume( float volume = -1.0f, float deltaTime = 0.0f );
	virtual void RestoreVolume( float deltaTime = 0.0f );

	virtual int GetPitch() { return m_nParams.pitch; }
	virtual int GetSoundPitch() { return m_nInfo.m_nPitch; }
	virtual void SetPitch( int pitch = -1.0f, float deltaTime = 0.0f );
	virtual void RestorePitch( float deltaTime = 0.0f );

	virtual void SetSoundName( const char *pSoundName, bool isLoop = false );
	virtual void SetTag( CSoundInstance *pSound ) { m_nTagSound = pSound; }

	virtual void SetStopOnFinish( bool result ) { m_bStopOnFinish = result; }

	virtual void SetTeam( int team );
	virtual void SetOrigin( CBaseEntity *pEntity );
	virtual void SetExcept( CPlayer *pEntity );
	virtual void SetOwner( CBaseEntity *pEntity );
	virtual void SetTarget( int target );

	//virtual void SetLayer( int layer ) { m_iLayer = layer; }
	virtual void SetChannel( int channel ) { m_iChannel = channel; }

public:
	virtual CBaseEntity *GetOriginEntity() { return m_nOriginEntity.Get(); }
	virtual CPlayer *GetExceptPlayer() { return m_nExceptPlayer.Get(); }
	virtual CBaseEntity *GetOwner() { return m_nOwner.Get(); }

	virtual bool IsPlaying() { return m_bIsPlaying; }
	virtual bool IsLooping() { return m_bIsLoop; }
	virtual bool IsTag() { return m_bIsTag; }
    virtual float GetFinishTime() { return m_flFinishTime; }

	virtual bool StopOnFinish() { return m_bStopOnFinish; }

	virtual float GetDuration() { return m_flSoundDuration; }
	virtual const char *GetSoundName() { return m_nSoundName; }
	virtual CSoundInstance *GetTag() { return m_nTagSound; }

	virtual int GetChannel() { return m_iChannel; }
	//virtual int GetLayer() { return m_iLayer; }

protected:
	float m_flSoundDuration;
	bool m_bIsLoop;
	bool m_bIsPlaying;

	bool m_bFinished;
	float m_flFinishTime;
	bool m_bStopOnFinish;

	bool m_bIsTag;

	//int m_iLayer;
	int m_iChannel;

	const char *m_nSoundName;
	int m_iTeam;
	EHANDLE m_nOwner;
	CHandle<CPlayer> m_nExceptPlayer;
	EHANDLE m_nOriginEntity;
	int m_iTarget;

	CSoundPatch *m_nSound;
	CSoundInstance *m_nTagSound;

public:
	EmitSound_t m_nInfo;
	CSoundParameters m_nParams;
};

#endif // SOUND_INSTANCE_H