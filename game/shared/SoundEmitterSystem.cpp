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
#include "checksum_crc.h"
#include "tier0/icommandline.h"

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

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
//================================================================================

ConVar cc_showmissing( "cc_showmissing", "0", FCVAR_REPLICATED, "Show missing closecaption entries." );

//================================================================================
// Captions
//================================================================================

#if !defined( CLIENT_DLL )
static ConVar cc_norepeat( "cc_norepeat", "5", 0, "In multiplayer games, don't repeat captions more often than this many seconds." );

class CCaptionRepeatMgr
{
public:

	CCaptionRepeatMgr() :
	  m_rbCaptionHistory( 0, 0, DefLessFunc( unsigned int ) )
	{
	}

	bool CanEmitCaption( unsigned int hash );

	void Clear();

private:

	void RemoveCaptionsBefore( float t );

	struct CaptionItem_t
	{
		unsigned int	hash;
		float			realtime;

		static bool Less( const CaptionItem_t &lhs, const CaptionItem_t &rhs )
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

bool CCaptionRepeatMgr::CanEmitCaption( unsigned int hash )
{
	// Don't cull in single player
	if ( gpGlobals->maxClients == 1 )
		return true;

	float realtime = gpGlobals->realtime;

	RemoveCaptionsBefore( realtime - cc_norepeat.GetFloat() );

	int idx = m_rbCaptionHistory.Find( hash );
	if ( idx == m_rbCaptionHistory.InvalidIndex() )
	{
		m_rbCaptionHistory.Insert( hash, realtime );
		return true;
	}

	float flLastEmitted = m_rbCaptionHistory[ idx ];
	if ( realtime - flLastEmitted > cc_norepeat.GetFloat() )
	{
		m_rbCaptionHistory[ idx ] = realtime;
		return true;
	}

	return false;
}

void CCaptionRepeatMgr::RemoveCaptionsBefore( float t )
{
	CUtlVector< unsigned int > toRemove;
	FOR_EACH_MAP( m_rbCaptionHistory, i )
	{
		if ( m_rbCaptionHistory[ i ] < t )
		{
			toRemove.AddToTail( m_rbCaptionHistory.Key( i ) );
		}
	}

	for ( int i = 0; i < toRemove.Count(); ++i )
	{
		m_rbCaptionHistory.Remove( toRemove[ i ] );
	}
}

void ClearModelSoundsCache();

#endif // !CLIENT_DLL

//================================================================================
// Helpers
//================================================================================

void WaveTrace( char const *wavname, char const *funcname )
{
	if ( IsX360() && !IsDebug() )
	{
		return;
	}

	static CUtlSymbolTable s_WaveTrace;

	// Make sure we only show the message once
	if ( UTL_INVAL_SYMBOL == s_WaveTrace.Find( wavname ) )
	{
		DevMsg( "%s directly referenced wave %s (should use game_sounds.txt system instead)\n", 
			funcname, wavname );
		s_WaveTrace.AddString( wavname );
	}
}

void Hack_FixEscapeChars( char *str )
{
	int len = Q_strlen( str ) + 1;
	char *i = str;
	char *o = (char *)stackalloc( len );
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
	Q_strncpy( str, osave, len );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &src - 
//-----------------------------------------------------------------------------
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


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------


#ifdef USE_OPENAL

ConVar openal_allow_base( "openal_allow_base", "0", 0, "Allow sounds to be passed through to the regular sound system if neccessary for playback" );

class COpenALEmitterSystem : public CSoundEmitterSystem
{
public:
	DECLARE_CLASS_GAMEROOT( COpenALEmitterSystem, CSoundEmitterSystem );

	COpenALEmitterSystem( const char *pName ) : BaseClass( pName )
	{

	}

	virtual char const *Name() {
		return "COpenALEmitterSystem";
	}

public:
	void EmitSoundByHandle( IRecipientFilter& filter, int entindex, const EmitSound_t & ep, HSOUNDSCRIPTHANDLE& handle )
	{
		if ( openal_allow_base.GetBool() ) {
			BaseClass::EmitSoundByHandle( filter, entindex, ep, handle );
			return;
		}

#ifdef CLIENT_DLL
		IOpenALSample *pSample = g_OpenALLoader.Load( PSkipSoundChars( ep.m_pSoundName ) );

		if ( pSample != NULL && pSample->IsReady() ) {
			pSample->LinkEntity( CBaseEntity::Instance( entindex ) );

			if ( ep.m_pOrigin != NULL ) {
				pSample->SetPosition( *ep.m_pOrigin );
			}

			pSample->SetGain( ep.m_flVolume );
			pSample->Play();

			return;
		}

		// Pull data from parameters
		CSoundParameters params;

		// Try to deduce the actor's gender
		gender_t gender = GENDER_NONE;
		CBaseEntity *ent = CBaseEntity::Instance( entindex );
		if ( ent ) {
			char const *actorModel = STRING( ent->GetModelName() );
			gender = soundemitterbase->GetActorGender( actorModel );
		}

		if ( !soundemitterbase->GetParametersForSoundEx( ep.m_pSoundName, handle, params, gender, true ) ) {
			return;
		}

		if ( !params.soundname[0] )
			return;

		pSample = g_OpenALLoader.Load( PSkipSoundChars( params.soundname ) );

		if ( pSample != NULL && pSample->IsReady() ) {
			pSample->LinkEntity( CBaseEntity::Instance( entindex ) );

			if ( ep.m_pOrigin != NULL ) {
				pSample->SetPosition( *ep.m_pOrigin );
			}

			pSample->SetGain( ep.m_flVolume );
			pSample->Play();
		}
		else {
			// Old-School system
			BaseClass::EmitSoundByHandle( filter, entindex, ep, handle );
		}
#else
		BaseClass::EmitSoundByHandle( filter, entindex, ep, handle );
#endif
	}

	void EmitSound( IRecipientFilter& filter, int entindex, const EmitSound_t & ep )
	{
		if ( openal_allow_base.GetBool() ) {
			BaseClass::EmitSound( filter, entindex, ep );
			return;
		}

#ifdef CLIENT_DLL
		IOpenALSample *pSample = g_OpenALLoader.Load( PSkipSoundChars( ep.m_pSoundName ) );

		if ( pSample != NULL && pSample->IsReady() ) {
			pSample->LinkEntity( CBaseEntity::Instance( entindex ) );

			if ( ep.m_pOrigin != NULL ) {
				pSample->SetPosition( *ep.m_pOrigin );
			}

			pSample->SetGain( ep.m_flVolume );
			pSample->Play();
			return;
		}

		// Pull data from parameters
		CSoundParameters params;

		// Try to deduce the actor's gender
		gender_t gender = GENDER_NONE;
		CBaseEntity *ent = CBaseEntity::Instance( entindex );
		if ( ent ) {
			char const *actorModel = STRING( ent->GetModelName() );
			gender = soundemitterbase->GetActorGender( actorModel );
		}

		if ( !soundemitterbase->GetParametersForSound( ep.m_pSoundName, params, gender ) ) {
			return;
		}

		if ( !params.soundname[0] )
			return;

		pSample = g_OpenALLoader.Load( PSkipSoundChars( params.soundname ) );
		if ( pSample != NULL && pSample->IsReady() ) {
			pSample->LinkEntity( CBaseEntity::Instance( entindex ) );

			if ( ep.m_pOrigin != NULL ) {
				pSample->SetPosition( *ep.m_pOrigin );
			}

			pSample->SetGain( ep.m_flVolume );

			pSample->Play();
		}
		else {
			BaseClass::EmitSound( filter, entindex, ep );
		}
#else
		BaseClass::EmitSound( filter, entindex, ep );
#endif
	}

	void EmitAmbientSound( int entindex, const Vector& origin, const char *soundname, float flVolume, int iFlags, int iPitch, float soundtime /*= 0.0f*/, float *duration /*=NULL*/ )
	{
		if ( !openal_allow_base.GetBool() ) {
			return;
		}

		BaseClass::EmitAmbientSound( entindex, origin, soundname, flVolume, iFlags, iPitch, soundtime, duration );
	}

	void EmitAmbientSound( int entindex, const Vector &origin, const char *pSample, float volume, soundlevel_t soundlevel, int flags, int pitch, float soundtime /*= 0.0f*/, float *duration /*=NULL*/ )
	{
		if ( !openal_allow_base.GetBool() ) {
			return;
		}

		BaseClass::EmitAmbientSound( entindex, origin, pSample, volume, soundlevel, flags, pitch, soundtime, duration );
	}
};

static COpenALEmitterSystem g_SoundEmitterSystem( "COpenALEmitterSystem" );

#else

static CSoundEmitterSystem g_SoundEmitterSystem( "CSoundEmitterSystem" );

#endif

IGameSystem *SoundEmitterSystem()
{
	return &g_SoundEmitterSystem;
}

void SoundSystemPreloadSounds( void )
{
	g_SoundEmitterSystem.PreloadSounds();
}

#if !defined( CLIENT_DLL )

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




// --------------------------------------------------------------------
// snd_playsounds
//
// This a utility for testing sound values
// --------------------------------------------------------------------

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

//-----------------------------------------------------------------------------
// Purpose:  Non-static override for doing the general case of CPASAttenuationFilter( this ), and EmitSound( filter, entindex(), etc. );
// Input  : *soundname - 
//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
// Purpose:  Non-static override for doing the general case of CPASAttenuationFilter( this ), and EmitSound( filter, entindex(), etc. );
// Input  : *soundname - 
//-----------------------------------------------------------------------------
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

#if !defined ( CLIENT_DLL )
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

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : filter - 
//			iEntIndex - 
//			*soundname - 
//			*pOrigin - 
//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : filter - 
//			iEntIndex - 
//			*soundname - 
//			*pOrigin - 
//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : filter - 
//			iEntIndex - 
//			params - 
//-----------------------------------------------------------------------------
void CBaseEntity::EmitSound( IRecipientFilter& filter, int iEntIndex, const EmitSound_t & params )
{
	VPROF_BUDGET( "CBaseEntity::EmitSound", _T( "CBaseEntity::EmitSound" ) );

	// VPROF( "CBaseEntity::EmitSound" );
	// Call into the sound emitter system...
	g_SoundEmitterSystem.EmitSound( filter, iEntIndex, params );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : filter - 
//			iEntIndex - 
//			params - 
//-----------------------------------------------------------------------------
void CBaseEntity::EmitSound( IRecipientFilter& filter, int iEntIndex, const EmitSound_t & params, HSOUNDSCRIPTHANDLE& handle )
{
	VPROF_BUDGET( "CBaseEntity::EmitSound", _T( "CBaseEntity::EmitSound" ) );

	// VPROF( "CBaseEntity::EmitSound" );
	// Call into the sound emitter system...
	g_SoundEmitterSystem.EmitSoundByHandle( filter, iEntIndex, params, handle );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *soundname - 
//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *soundname - 
//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iEntIndex - 
//			*soundname - 
//-----------------------------------------------------------------------------
void CBaseEntity::StopSound( int iEntIndex, const char *soundname )
{
	g_SoundEmitterSystem.StopSound( iEntIndex, soundname );
}

void CBaseEntity::StopSound( int iEntIndex, int iChannel, const char *pSample, bool bIsStoppingSpeakerSound )
{
	g_SoundEmitterSystem.StopSound( iEntIndex, iChannel, pSample, bIsStoppingSpeakerSound );
}

soundlevel_t CBaseEntity::LookupSoundLevel( const char *soundname )
{
	return soundemitterbase->LookupSoundLevel( soundname );
}


soundlevel_t CBaseEntity::LookupSoundLevel( const char *soundname, HSOUNDSCRIPTHANDLE& handle )
{
	return soundemitterbase->LookupSoundLevelByHandle( soundname, handle );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *entity - 
//			origin - 
//			flags - 
//			*soundname - 
//-----------------------------------------------------------------------------
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

void CBaseEntity::GenderExpandString( char const *in, char *out, int maxlen )
{
	soundemitterbase->GenderExpandString( STRING( GetModelName() ), in, out, maxlen );
}

bool CBaseEntity::GetParametersForSound( const char *soundname, CSoundParameters &params, const char *actormodel )
{
	gender_t gender = soundemitterbase->GetActorGender( actormodel );
	
	return soundemitterbase->GetParametersForSound( soundname, params, gender );
}

bool CBaseEntity::GetParametersForSound( const char *soundname, HSOUNDSCRIPTHANDLE& handle, CSoundParameters &params, const char *actormodel )
{
	gender_t gender = soundemitterbase->GetActorGender( actormodel );
	
	return soundemitterbase->GetParametersForSoundEx( soundname, handle, params, gender );
}

HSOUNDSCRIPTHANDLE CBaseEntity::PrecacheScriptSound( const char *soundname )
{
#if !defined( CLIENT_DLL )
	return g_SoundEmitterSystem.PrecacheScriptSound( soundname );
#else
	return soundemitterbase->GetSoundIndex( soundname );
#endif
}

#if !defined ( CLIENT_DLL )
// Same as server version of above, but signiture changed so it can be deduced by the macros
void CBaseEntity::VScriptPrecacheScriptSound( const char *soundname )
{
	g_SoundEmitterSystem.PrecacheScriptSound( soundname );
}
#endif // !CLIENT_DLL

void CBaseEntity::PrefetchScriptSound( const char *soundname )
{
	g_SoundEmitterSystem.PrefetchScriptSound( soundname );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *soundname - 
// Output : float
//-----------------------------------------------------------------------------
float CBaseEntity::GetSoundDuration( const char *soundname, char const *actormodel )
{
	return enginesound->GetSoundDuration( PSkipSoundChars( UTIL_TranslateSoundName( soundname, actormodel ) ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : filter - 
//			*token - 
//			duration - 
//			warnifmissing - 
//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//			preload - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
//-----------------------------------------------------------------------------
void CBaseEntity::PrefetchSound( const char *name )
{
	 enginesound->PrefetchSound( name );
}

#if !defined( CLIENT_DLL )
bool GetCaptionHash( char const *pchStringName, bool bWarnIfMissing, unsigned int &hash )
{
	return g_SoundEmitterSystem.GetCaptionHash( pchStringName, bWarnIfMissing, hash );
}

bool CanEmitCaption( unsigned int hash )
{
	return g_CaptionRepeats.CanEmitCaption( hash );
}

#endif
