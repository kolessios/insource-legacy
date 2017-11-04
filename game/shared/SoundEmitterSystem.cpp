//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include <ctype.h>
#include <KeyValues.h>
#include "engine/IEngineSound.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"
#include "igamesystem.h"
#include "soundchars.h"
#include "filesystem.h"
#include "tier0/vprof.h"
#include "tier0/icommandline.h"

#include "sound_emitter_system.h"

#ifdef USE_OPENAL
#include "sound_openal_emitter_system.h"
#elif USE_FMOD
#include "sound_fmod_emitter_system.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
//================================================================================

extern bool g_bPermitDirectSoundPrecache;
extern void WaveTrace(char const *wavname, char const *funcname);
extern ISoundEmitterSystemBase *soundemitterbase;

//================================================================================
//================================================================================
EmitSound_t::EmitSound_t( const CSoundParameters &src )
{
	m_nChannel = src.channel;
	m_pSoundName = src.soundname;
	m_flVolume = src.volume;
	m_SoundLevel = src.soundlevel;
	m_nFlags = 0;
	m_nPitch = src.pitch;
	m_pOrigin = 0;
	m_flSoundTime = ( src.delay_msec == 0 ) ? 0.0f : gpGlobals->curtime + ( (float)src.delay_msec / 1000.0f );
	m_pflSoundDuration = 0;
	m_bEmitCloseCaption = true;
	m_bWarnOnMissingCloseCaption = false;
	m_bWarnOnDirectWaveReference = false;
	m_nSpeakerEntity = -1;
}

//================================================================================
// Declares the sound system that the engine will use.
//================================================================================

#ifdef USE_OPENAL

static CSoundOpenALEmitterSystem g_SoundEmitterSystem( "CSoundOpenALEmitterSystem" );

#elif USE_FMOD

static CFMODSoundEmitterSystem g_SoundEmitterSystem("CFMODSoundEmitterSystem");

#else

static CSoundEmitterSystem g_SoundEmitterSystem( "CSoundEmitterSystem" );

#endif

//================================================================================
//================================================================================
IGameSystem *SoundEmitterSystem()
{
	return &g_SoundEmitterSystem;
}

//================================================================================
//================================================================================
void SoundSystemPreloadSounds( void )
{
	g_SoundEmitterSystem.PreloadSounds();
}

#ifndef CLIENT_DLL

void ClearModelSoundsCache();

CON_COMMAND( sv_soundemitter_flush, "Flushes the sounds.txt system (server only)" )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	// save the current soundscape
	// kill the system
	g_SoundEmitterSystem.Flush();

	// Redo precache all wave files... (this should work now that we have dynamic string tables)
	g_SoundEmitterSystem.LevelInitPreEntity();

	// These store raw sound indices for faster precaching, blow them away.
	ClearModelSoundsCache();
	// TODO:  when we go to a handle system, we'll need to invalidate handles somehow
}

CON_COMMAND( sv_soundemitter_filecheck, "Report missing wave files for sounds and game_sounds files." )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	int missing = soundemitterbase->CheckForMissingWavFiles( true );
	DevMsg( "---------------------------\nTotal missing files %i\n", missing );
}

CON_COMMAND( sv_findsoundname, "Find sound names which reference the specified wave files." )
{	
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	if ( args.ArgC() != 2 )
		return;

	int c = soundemitterbase->GetSoundCount();
	int i;

	char const *search = args[ 1 ];
	if ( !search )
		return;

	for ( i = 0; i < c; i++ )
	{
		CSoundParametersInternal *internal = soundemitterbase->InternalGetParametersForSound( i );
		if ( !internal )
			continue;

		int waveCount = internal->NumSoundNames();
		if ( waveCount > 0 )
		{
			for( int wave = 0; wave < waveCount; wave++ )
			{
				char const *wavefilename = soundemitterbase->GetWaveName( internal->GetSoundNames()[ wave ].symbol );

				if ( Q_stristr( wavefilename, search ) )
				{
					char const *soundname = soundemitterbase->GetSoundName( i );
					char const *scriptname = soundemitterbase->GetSourceFileForSound( i );

					Msg( "Referenced by '%s:%s' -- %s\n", scriptname, soundname, wavefilename );
				}
			}
		}
	}
}

CON_COMMAND( sv_soundemitter_spew, "Print details about a sound." )
{
	if ( args.ArgC() != 2 )
	{
		Msg( "Usage:  soundemitter_spew < sndname >\n" );
		return;
	}

	soundemitterbase->DescribeSound( args.Arg( 1 ) );
}

#else

//================================================================================
//================================================================================
void Playgamesound_f( const CCommand &args )
{
	CBasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pPlayer )
	{
		if ( args.ArgC() > 2 )
		{
			Vector position = pPlayer->EyePosition();
			Vector forward;
			pPlayer->GetVectors( &forward, NULL, NULL );
			position += atof( args[2] ) * forward;
			ABS_QUERY_GUARD( true );
			CPASAttenuationFilter filter( pPlayer );
			EmitSound_t params;
			params.m_pSoundName = args[1];
			params.m_pOrigin = &position;
			params.m_flVolume = 0.0f;
			params.m_nPitch = 0;
			g_SoundEmitterSystem.EmitSound( filter, 0, params );
		}
		else
		{
			pPlayer->EmitSound( args[1] );
		}
	}
	else
	{
		Msg("Can't play until a game is started.\n");
		// UNDONE: Make something like this work?
		//CBroadcastRecipientFilter filter;
		//g_SoundEmitterSystem.EmitSound( filter, 1, args[1], 0.0, 0, 0, &vec3_origin, 0, NULL );
	}
}

//================================================================================
//================================================================================
static int GamesoundCompletion( const char *partial, char commands[ COMMAND_COMPLETION_MAXITEMS ][ COMMAND_COMPLETION_ITEM_LENGTH ] )
{
	int current = 0;

	const char *cmdname = "playgamesound";
	char *substring = NULL;
	int substringLen = 0;
	if ( Q_strstr( partial, cmdname ) && strlen(partial) > strlen(cmdname) + 1 )
	{
		substring = (char *)partial + strlen( cmdname ) + 1;
		substringLen = strlen(substring);
	}
	
	for ( int i = soundemitterbase->GetSoundCount()-1; i >= 0 && current < COMMAND_COMPLETION_MAXITEMS; i-- )
	{
		const char *pSoundName = soundemitterbase->GetSoundName( i );
		if ( pSoundName )
		{
			if ( !substring || !Q_strncasecmp( pSoundName, substring, substringLen ) )
			{
				Q_snprintf( commands[ current ], sizeof( commands[ current ] ), "%s %s", cmdname, pSoundName );
				current++;
			}
		}
	}

	return current;
}

static ConCommand Command_Playgamesound( "playgamesound", Playgamesound_f, "Play a sound from the game sounds txt file", FCVAR_CLIENTCMD_CAN_EXECUTE | FCVAR_SERVER_CAN_EXECUTE, GamesoundCompletion );




//================================================================================
// This a utility for testing sound values
//================================================================================
static int GamesoundCompletion2( const char *partial, char commands[ COMMAND_COMPLETION_MAXITEMS ][ COMMAND_COMPLETION_ITEM_LENGTH ] )
{
	int current = 0;

	const char *cmdname = "snd_playsounds";
	char *substring = NULL;
	int substringLen = 0;
	if ( Q_strstr( partial, cmdname ) && strlen(partial) > strlen(cmdname) + 1 )
	{
		substring = (char *)partial + strlen( cmdname ) + 1;
		substringLen = strlen(substring);
	}
	
	for ( int i = soundemitterbase->GetSoundCount()-1; i >= 0 && current < COMMAND_COMPLETION_MAXITEMS; i-- )
	{
		const char *pSoundName = soundemitterbase->GetSoundName( i );
		if ( pSoundName )
		{
			if ( !substring || !Q_strncasecmp( pSoundName, substring, substringLen ) )
			{
				Q_snprintf( commands[ current ], sizeof( commands[ current ] ), "%s %s", cmdname, pSoundName );
				current++;
			}
		}
	}

	return current;
}

//================================================================================
//================================================================================
void S_PlaySounds( const CCommand &args )
{
	CBasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( pPlayer )
	{
		if ( args.ArgC() > 4 )
		{
//			Vector position = pPlayer->EyePosition();
			Vector position;
			//	Vector forward;
		//	pPlayer->GetVectors( &forward, NULL, NULL );
		//	position += atof( args[2] ) * forward;
			position[0] = atof( args[2] );
			position[1] = atof( args[3] );
			position[2] = atof( args[4] );

			ABS_QUERY_GUARD( true );
			CPASAttenuationFilter filter( pPlayer );
			EmitSound_t params;
			params.m_pSoundName = args[1];
			params.m_pOrigin = &position;
			params.m_flVolume = 0.0f;
			params.m_nPitch = 0;
			g_SoundEmitterSystem.EmitSound( filter, 0, params );
		}
		else
		{
			pPlayer->EmitSound( args[1] );
		}
	}
	else
	{
		Msg("Can't play until a game is started.\n");
		// UNDONE: Make something like this work?
		//CBroadcastRecipientFilter filter;
		//g_SoundEmitterSystem.EmitSound( filter, 1, args[1], 0.0, 0, 0, &vec3_origin, 0, NULL );
	}
}

static ConCommand SND_PlaySounds( "snd_playsounds", S_PlaySounds, "Play sounds from the game sounds txt file at a given location", FCVAR_CLIENTCMD_CAN_EXECUTE | FCVAR_SERVER_CAN_EXECUTE, GamesoundCompletion2 );

//================================================================================
//================================================================================
static int GamesoundCompletion3( const char *partial, char commands[ COMMAND_COMPLETION_MAXITEMS ][ COMMAND_COMPLETION_ITEM_LENGTH ] )
{
	int current = 0;

	const char *cmdname = "snd_setsoundparam";
	char *substring = NULL;
	int substringLen = 0;
	if ( Q_strstr( partial, cmdname ) && strlen(partial) > strlen(cmdname) + 1 )
	{
		substring = (char *)partial + strlen( cmdname ) + 1;
		substringLen = strlen(substring);
	}
	
	for ( int i = soundemitterbase->GetSoundCount()-1; i >= 0 && current < COMMAND_COMPLETION_MAXITEMS; i-- )
	{
		const char *pSoundName = soundemitterbase->GetSoundName( i );
		if ( pSoundName )
		{
			if ( !substring || !Q_strncasecmp( pSoundName, substring, substringLen ) )
			{
				Q_snprintf( commands[ current ], sizeof( commands[ current ] ), "%s %s", cmdname, pSoundName );
				current++;
			}
		}
	}

	return current;
}

//================================================================================
//================================================================================
static void S_SetSoundParam( const CCommand &args )
{
	if ( args.ArgC() != 4 )
	{
		DevMsg("Parameters: mix group name, [vol, mute, solo], value");
		return;
	}

	const char *szSoundName = args[1];
	const char *szparam = args[2];
	const char *szValue = args[3];

	// get the sound we're working on
	int soundindex = soundemitterbase->GetSoundIndex( szSoundName);
	if ( !soundemitterbase->IsValidIndex(soundindex) )
		return;

	// Look up the sound level from the soundemitter system
	CSoundParametersInternal *soundparams = soundemitterbase->InternalGetParametersForSound( soundindex );
	if ( !soundparams )
	{
		return;
	}

	// // See if it's writable, if not then bail
	// char const *scriptfile = soundemitter->GetSourceFileForSound( soundindex );
	// if ( !scriptfile || 
		 // !filesystem->FileExists( scriptfile ) ||
		 // !filesystem->IsFileWritable( scriptfile ) )
	// {
		// return;
	// }

	// Copy the parameters
	CSoundParametersInternal newparams;
	newparams.CopyFrom( *soundparams );
				
	if(!Q_stricmp("volume", szparam))
		newparams.VolumeFromString( szValue);
	else if(!Q_stricmp("level", szparam))
		newparams.SoundLevelFromString( szValue );

	// No change
	if ( newparams == *soundparams )
	{
		return;
	}

	soundemitterbase->UpdateSoundParameters( szSoundName , newparams );

}

static ConCommand SND_SetSoundParam( "snd_setsoundparam", S_SetSoundParam, "Set a sound paramater", FCVAR_CLIENTCMD_CAN_EXECUTE | FCVAR_SERVER_CAN_EXECUTE, GamesoundCompletion3 );

#endif // CLIENT_DLL

//================================================================================
// Purpose:  Non-static override for doing the general case of CPASAttenuationFilter( this ), and EmitSound( filter, entindex(), etc. );
// Input  : *soundname - 
//================================================================================
void CBaseEntity::EmitSound( const char *soundname, float soundtime /*= 0.0f*/, float *duration /*=NULL*/ )
{
	//VPROF( "CBaseEntity::EmitSound" );
	VPROF_BUDGET( "CBaseEntity::EmitSound", _T( "CBaseEntity::EmitSound" ) );

	ABS_QUERY_GUARD( true );
	CPASAttenuationFilter filter( this, soundname );

	EmitSound_t params;
	params.m_pSoundName = soundname;
	params.m_flSoundTime = soundtime;
	params.m_pflSoundDuration = duration;
	params.m_bWarnOnDirectWaveReference = true;

	EmitSound( filter, entindex(), params );
}

//================================================================================
// Purpose:  Non-static override for doing the general case of CPASAttenuationFilter( this ), and EmitSound( filter, entindex(), etc. );
// Input  : *soundname - 
//================================================================================
void CBaseEntity::EmitSound( const char *soundname, HSOUNDSCRIPTHANDLE& handle, float soundtime /*= 0.0f*/, float *duration /*=NULL*/ )
{
	VPROF_BUDGET( "CBaseEntity::EmitSound", _T( "CBaseEntity::EmitSound" ) );

	// VPROF( "CBaseEntity::EmitSound" );
	ABS_QUERY_GUARD( true );
	CPASAttenuationFilter filter( this, soundname, handle );

	EmitSound_t params;
	params.m_pSoundName = soundname;
	params.m_flSoundTime = soundtime;
	params.m_pflSoundDuration = duration;
	params.m_bWarnOnDirectWaveReference = true;

	EmitSound( filter, entindex(), params, handle );
}

#ifndef CLIENT_DLL
void CBaseEntity::ScriptEmitSound( const char *soundname )
{
	EmitSound( soundname );
}

float CBaseEntity::ScriptSoundDuration( const char *soundname, const char *actormodel )
{
	float duration = CBaseEntity::GetSoundDuration( soundname, actormodel );
	return duration;
}
#endif // !CLIENT

//================================================================================
//================================================================================
void CBaseEntity::EmitSound( IRecipientFilter& filter, int iEntIndex, const char *soundname, const Vector *pOrigin /*= NULL*/, float soundtime /*= 0.0f*/, float *duration /*=NULL*/ )
{
	VPROF_BUDGET( "CBaseEntity::EmitSound", _T( "CBaseEntity::EmitSound" ) );

	// VPROF( "CBaseEntity::EmitSound" );
	EmitSound_t params;
	params.m_pSoundName = soundname;
	params.m_flSoundTime = soundtime;
	params.m_pOrigin = pOrigin;
	params.m_pflSoundDuration = duration;
	params.m_bWarnOnDirectWaveReference = true;

	EmitSound( filter, iEntIndex, params, params.m_hSoundScriptHandle );
}

//================================================================================
//================================================================================
void CBaseEntity::EmitSound( IRecipientFilter& filter, int iEntIndex, const char *soundname, HSOUNDSCRIPTHANDLE& handle, const Vector *pOrigin /*= NULL*/, float soundtime /*= 0.0f*/, float *duration /*=NULL*/ )
{
	VPROF_BUDGET( "CBaseEntity::EmitSound", _T( "CBaseEntity::EmitSound" ) );

	//VPROF( "CBaseEntity::EmitSound" );
	EmitSound_t params;
	params.m_pSoundName = soundname;
	params.m_flSoundTime = soundtime;
	params.m_pOrigin = pOrigin;
	params.m_pflSoundDuration = duration;
	params.m_bWarnOnDirectWaveReference = true;

	EmitSound( filter, iEntIndex, params, handle );
}

//================================================================================
//================================================================================
void CBaseEntity::EmitSound( IRecipientFilter& filter, int iEntIndex, const EmitSound_t & params )
{
	VPROF_BUDGET( "CBaseEntity::EmitSound", _T( "CBaseEntity::EmitSound" ) );

	// VPROF( "CBaseEntity::EmitSound" );
	// Call into the sound emitter system...
	g_SoundEmitterSystem.EmitSound( filter, iEntIndex, params );
}

//================================================================================
//================================================================================
void CBaseEntity::EmitSound( IRecipientFilter& filter, int iEntIndex, const EmitSound_t & params, HSOUNDSCRIPTHANDLE& handle )
{
	VPROF_BUDGET( "CBaseEntity::EmitSound", _T( "CBaseEntity::EmitSound" ) );

	// VPROF( "CBaseEntity::EmitSound" );
	// Call into the sound emitter system...
	g_SoundEmitterSystem.EmitSoundByHandle( filter, iEntIndex, params, handle );
}

//================================================================================
//================================================================================
void CBaseEntity::StopSound( const char *soundname )
{
#if defined( CLIENT_DLL )
	if ( entindex() == -1 )
	{
		// If we're a clientside entity, we need to use the soundsourceindex instead of the entindex
		StopSound( GetSoundSourceIndex(), soundname );
		return;
	}
#endif

	StopSound( entindex(), soundname );
}

//================================================================================
//================================================================================
void CBaseEntity::StopSound( const char *soundname, HSOUNDSCRIPTHANDLE& handle )
{
#if defined( CLIENT_DLL )
	if ( entindex() == -1 )
	{
		// If we're a clientside entity, we need to use the soundsourceindex instead of the entindex
		StopSound( GetSoundSourceIndex(), soundname );
		return;
	}
#endif

	g_SoundEmitterSystem.StopSoundByHandle( entindex(), soundname, handle );
}

//================================================================================
//================================================================================
void CBaseEntity::StopSound( int iEntIndex, const char *soundname )
{
	g_SoundEmitterSystem.StopSound( iEntIndex, soundname );
}

//================================================================================
//================================================================================
void CBaseEntity::StopSound( int iEntIndex, int iChannel, const char *pSample, bool bIsStoppingSpeakerSound )
{
	g_SoundEmitterSystem.StopSound( iEntIndex, iChannel, pSample, bIsStoppingSpeakerSound );
}

//================================================================================
//================================================================================
soundlevel_t CBaseEntity::LookupSoundLevel( const char *soundname )
{
	return soundemitterbase->LookupSoundLevel( soundname );
}

//================================================================================
//================================================================================
soundlevel_t CBaseEntity::LookupSoundLevel( const char *soundname, HSOUNDSCRIPTHANDLE& handle )
{
	return soundemitterbase->LookupSoundLevelByHandle( soundname, handle );
}

//================================================================================
//================================================================================
void CBaseEntity::EmitAmbientSound( int entindex, const Vector& origin, const char *soundname, int flags, float soundtime /*= 0.0f*/, float *duration /*=NULL*/ )
{
	g_SoundEmitterSystem.EmitAmbientSound( entindex, origin, soundname, 0.0, flags, 0, soundtime, duration );
}

// HACK HACK:  Do we need to pull the entire SENTENCEG_* wrapper over to the client .dll?
#if defined( CLIENT_DLL )
int SENTENCEG_Lookup(const char *sample)
{
	return engine->SentenceIndexFromName( sample + 1 );
}
#endif

//================================================================================
//================================================================================
void UTIL_EmitAmbientSound( int entindex, const Vector &vecOrigin, const char *samp, float vol, soundlevel_t soundlevel, int fFlags, int pitch, float soundtime /*= 0.0f*/, float *duration /*=NULL*/ )
{
	if (samp && *samp == '!')
	{
		int sentenceIndex = SENTENCEG_Lookup(samp);
		if (sentenceIndex >= 0)
		{
			char name[32];
			Q_snprintf( name, sizeof(name), "!%d", sentenceIndex );
#if !defined( CLIENT_DLL )
			engine->EmitAmbientSound( entindex, vecOrigin, name, vol, soundlevel, fFlags, pitch, soundtime );
#else
			enginesound->EmitAmbientSound( name, vol, pitch, fFlags, soundtime );
#endif
			if ( duration )
			{
				*duration = enginesound->GetSoundDuration( name );
			}

			g_SoundEmitterSystem.TraceEmitSound( entindex, "UTIL_EmitAmbientSound:  Sentence emitted '%s' (ent %i)\n",
				name, entindex );
		}
	}
	else
	{
		g_SoundEmitterSystem.EmitAmbientSound( entindex, vecOrigin, samp, vol, soundlevel, fFlags, pitch, soundtime, duration );
	}
}

//================================================================================
//================================================================================
static const char *UTIL_TranslateSoundName( const char *soundname, const char *actormodel )
{
	Assert( soundname );

	if ( Q_stristr( soundname, ".wav" ) || Q_stristr( soundname, ".mp3" ) )
	{
		if ( Q_stristr( soundname, ".wav" ) )
		{
			WaveTrace( soundname, "UTIL_TranslateSoundName" );
		}
		return soundname;
	}

	return soundemitterbase->GetWavFileForSound( soundname, actormodel );
}

//================================================================================
//================================================================================
void CBaseEntity::GenderExpandString( char const *in, char *out, int maxlen )
{
	soundemitterbase->GenderExpandString( STRING( GetModelName() ), in, out, maxlen );
}

//================================================================================
//================================================================================
bool CBaseEntity::GetParametersForSound( const char *soundname, CSoundParameters &params, const char *actormodel )
{
	gender_t gender = soundemitterbase->GetActorGender( actormodel );
	
	return soundemitterbase->GetParametersForSound( soundname, params, gender );
}

//================================================================================
//================================================================================
bool CBaseEntity::GetParametersForSound( const char *soundname, HSOUNDSCRIPTHANDLE& handle, CSoundParameters &params, const char *actormodel )
{
	gender_t gender = soundemitterbase->GetActorGender( actormodel );
	
	return soundemitterbase->GetParametersForSoundEx( soundname, handle, params, gender );
}

//================================================================================
//================================================================================
HSOUNDSCRIPTHANDLE CBaseEntity::PrecacheScriptSound( const char *soundname )
{
#if !defined( CLIENT_DLL )
	return g_SoundEmitterSystem.PrecacheScriptSound( soundname );
#else
	return soundemitterbase->GetSoundIndex( soundname );
#endif
}

#if !defined ( CLIENT_DLL )
//================================================================================
// Same as server version of above, but signiture changed so it can be deduced by the macros
//================================================================================
void CBaseEntity::VScriptPrecacheScriptSound( const char *soundname )
{
	g_SoundEmitterSystem.PrecacheScriptSound( soundname );
}
#endif // !CLIENT_DLL

//================================================================================
//================================================================================
void CBaseEntity::PrefetchScriptSound( const char *soundname )
{
	g_SoundEmitterSystem.PrefetchScriptSound( soundname );
}

//================================================================================
//================================================================================
float CBaseEntity::GetSoundDuration( const char *soundname, char const *actormodel )
{
	return enginesound->GetSoundDuration( PSkipSoundChars( UTIL_TranslateSoundName( soundname, actormodel ) ) );
}

//================================================================================
//================================================================================
void CBaseEntity::EmitCloseCaption( IRecipientFilter& filter, int entindex, char const *token, CUtlVector< Vector >& soundorigin, float duration, bool warnifmissing /*= false*/ )
{
	bool fromplayer = false;
	CBaseEntity *ent = CBaseEntity::Instance( entindex );
	while ( ent )
	{
		if ( ent->IsPlayer() )
		{
			fromplayer = true;
			break;
		}
		ent = ent->GetOwnerEntity();
	}

	g_SoundEmitterSystem.EmitCloseCaption( filter, entindex, fromplayer, token, soundorigin, duration, warnifmissing );
}

//================================================================================
//================================================================================
bool CBaseEntity::PrecacheSound( const char *name )
{
	if ( IsPC() && !g_bPermitDirectSoundPrecache )
	{
		Warning( "Direct precache of %s\n", name );
	}

	// If this is out of order, warn
	if ( !CBaseEntity::IsPrecacheAllowed() )
	{
		if ( !enginesound->IsSoundPrecached( name ) )
		{
			Assert( !"CBaseEntity::PrecacheSound:  too late" );

			Warning( "Late precache of %s\n", name );
		}
	}

	bool bret = enginesound->PrecacheSound( name, true );
	return bret;
}

//================================================================================
//================================================================================
void CBaseEntity::PrefetchSound( const char *name )
{
	 enginesound->PrefetchSound( name );
}

#if !defined( CLIENT_DLL )
//================================================================================
//================================================================================
bool GetCaptionHash( char const *pchStringName, bool bWarnIfMissing, unsigned int &hash )
{
	return g_SoundEmitterSystem.GetCaptionHash( pchStringName, bWarnIfMissing, hash );
}
#endif
