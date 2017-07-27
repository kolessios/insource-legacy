//==== Woots 2017. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef ENV_SOUND_H
#define ENV_SOUND_H

#pragma once

#include "sound_instance.h"

// Comandos
enum
{
	ENV_SOUND_CREATE = 1,
	ENV_SOUND_PLAY,
	ENV_SOUND_STOP,
	ENV_SOUND_FADEOUT,
	ENV_SOUND_CHANGE_VOLUME,
	ENV_SOUND_CHANGE_PITCH,
	ENV_SOUND_CHANGE_SOUNDNAME,
	ENV_SOUND_CHANGE_TEAM,
	ENV_SOUND_CHANGE_ORIGIN,
	ENV_SOUND_CHANGE_LAYER,
	ENV_SOUND_CHANGE_CHANNEL
};

#ifdef CLIENT_DLL
	#define CEnvSound C_EnvSound
#endif

#define SF_START_PLAYING 1
#define SF_IS_LOOPED 2
#define SF_SERVER_SIDE 4

//================================================================================
// Entidad para poder crear y controlar un CSoundInstance
//================================================================================
class CEnvSound : public CBaseEntity
{
public:
	DECLARE_CLASS( CEnvSound, CBaseEntity );
#ifndef CLIENT_DLL
	DECLARE_DATADESC();
#endif
	DECLARE_NETWORKCLASS();

	// Principales
	virtual void Spawn();
	virtual void CreateSound();

	// Reproducción
	virtual void Play();
	virtual void Stop();
	virtual void Fadeout( float deltaTime = 0.5f );

	virtual void SetVolume( float volume = -1.0f );
	virtual void RestoreVolume();

	virtual void SetPitch( int pitch = -1.0f );
	virtual void RestorePitch( float deltaTime = 0.0f );

	virtual void SetSoundName( const char *pSoundName, bool isLoop = false );
	virtual void SetTeam( int team );
	virtual void SetOrigin( CBaseEntity *pEntity );
	virtual void SetChannel( int channel );

#ifndef CLIENT_DLL
	virtual int UpdateTransmitState();
	virtual void Think();

	// Inputs
	void InputPlay( inputdata_t &inputdata );
	void InputStop( inputdata_t &inputdata );
	void InputFadeout( inputdata_t &inputdata );

	void InputSetVolume( inputdata_t &inputdata );
	void InputRestoreVolume( inputdata_t &inputdata );

	void InputSetPitch( inputdata_t &inputdata );
	void InputRestorePitch( inputdata_t &inputdata );

	void InputSetSoundName( inputdata_t &inputdata );
	void InputSetTeam( inputdata_t &inputdata );
	void InputSetOrigin( inputdata_t &inputdata );
	void InputSetChannel( inputdata_t &inputdata );
#else
	virtual void PostDataUpdate( DataUpdateType_t updateType );
	virtual void ClientThink();
	virtual void ReceiveMessage( int classID, bf_read &msg );
#endif

protected:
	CNetworkVar( float, m_flStartVolume );
	CNetworkVar( float, m_flFadeVolume );

	CNetworkVar( int, m_iStartPitch );
	CNetworkVar( float, m_flFadePitch );

	CNetworkVar( float, m_flVolume );
	CNetworkVar( int, m_iPitch );

	CNetworkVar( int, m_iTeam );

	CNetworkHandle( CBaseEntity, m_nOriginEntity );
	string_t m_nOriginEntityKeyfield;

	CNetworkVar( int, m_iChannel );

	CNetworkString( m_nSoundName, 260 );
	string_t m_nSoundNameKeyfield;

	CSoundInstance *m_nSound;
};

#endif