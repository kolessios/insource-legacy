//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//

#ifndef SOUND_EMITTER_SYSTEM
#define SOUND_EMITTER_SYSTEM

#pragma once

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

	void InternalPrecacheWaves( int soundIndex );
	void InternalPrefetchWaves( int soundIndex );

	HSOUNDSCRIPTHANDLE PrecacheScriptSound( const char *soundname );
	void PrefetchScriptSound( const char *soundname );

public:

    virtual void EmitSoundByHandle(IRecipientFilter& filter, int entindex, const EmitSound_t & ep, HSOUNDSCRIPTHANDLE& handle);
    virtual void EmitSound(IRecipientFilter& filter, int entindex, const EmitSound_t & ep);

	void EmitCloseCaption( IRecipientFilter& filter, int entindex, bool fromplayer, char const *token, CUtlVector< Vector >& originlist, float duration, bool warnifmissing /*= false*/, bool bForceSubtitle = false )
	{
		// Don't use dedicated closecaption ConVar since it will prevent remote clients from getting captions.
		// Okay to use it in SP, since it's the same ConVar, not the FCVAR_USERINFO one
		if ( gpGlobals->maxClients == 1 &&
			 !g_pClosecaption->GetBool() ) {
			return;
		}

		// A negative duration means fill it in from the wav file if possible
		if ( duration < 0.0f ) {
			char const *wav = soundemitterbase->GetWavFileForSound( token, GENDER_NONE );
			if ( wav ) {
				duration = enginesound->GetSoundDuration( wav );
			}
			else {
				duration = 2.0f;
			}
		}

		char lowercase[256];
		Q_strncpy( lowercase, token, sizeof( lowercase ) );
		Q_strlower( lowercase );
		if ( Q_strstr( lowercase, "\\" ) ) {
			Hack_FixEscapeChars( lowercase );
		}

		// NOTE:  We must make a copy or else if the filter is owned by a SoundPatch, we'll end up destructively removing
		//  all players from it!!!!
		CRecipientFilter filterCopy;
		filterCopy.CopyFrom( (CRecipientFilter &)filter );

		// Captions only route to host player (there is only one closecaptioning HUD)
		filterCopy.RemoveSplitScreenPlayers();

		if ( !bForceSubtitle ) {
			// Remove any players who don't want close captions
			CBaseEntity::RemoveRecipientsIfNotCloseCaptioning( (CRecipientFilter &)filterCopy );
		}

#if !defined( CLIENT_DLL )
		{
			// Defined in sceneentity.cpp
			bool AttenuateCaption( const char *token, const Vector& listener, CUtlVector< Vector >& soundorigins );

			if ( filterCopy.GetRecipientCount() > 0 ) {
				int c = filterCopy.GetRecipientCount();
				for ( int i = c - 1; i >= 0; --i ) {
					CBasePlayer *player = UTIL_PlayerByIndex( filterCopy.GetRecipientIndex( i ) );
					if ( !player )
						continue;

					Vector playerEarPosition = player->EarPosition();

					if ( AttenuateCaption( lowercase, playerEarPosition, originlist ) ) {
						filterCopy.RemoveRecipient( player );
					}
				}
			}
		}
#endif
		// Anyone left?
		if ( filterCopy.GetRecipientCount() > 0 ) {

#if !defined( CLIENT_DLL )

			char lowercase_nogender[256];
			Q_strncpy( lowercase_nogender, lowercase, sizeof( lowercase_nogender ) );
			bool bTriedGender = false;

			CBaseEntity *pActor = CBaseEntity::Instance( entindex );
			if ( pActor ) {
				char const *pszActorModel = STRING( pActor->GetModelName() );
				gender_t gender = soundemitterbase->GetActorGender( pszActorModel );

				if ( gender == GENDER_MALE ) {
					Q_strncat( lowercase, "_male", sizeof( lowercase ), COPY_ALL_CHARACTERS );
					bTriedGender = true;
				}
				else if ( gender == GENDER_FEMALE ) {
					Q_strncat( lowercase, "_female", sizeof( lowercase ), COPY_ALL_CHARACTERS );
					bTriedGender = true;
				}
			}

			unsigned int hash = 0u;
			bool bFound = GetCaptionHash( lowercase, true, hash );

			// if not found, try the no-gender version
			if ( !bFound && bTriedGender ) {
				bFound = GetCaptionHash( lowercase_nogender, true, hash );
			}

			if ( bFound ) {
				if ( g_CaptionRepeats.CanEmitCaption( hash ) ) {
					if ( bForceSubtitle ) {
						// Send forced caption and duration hint down to client
						UserMessageBegin( filterCopy, "CloseCaptionDirect" );
						WRITE_LONG( hash );
						WRITE_UBITLONG( clamp( (int)(duration * 10.0f), 0, 65535 ), 15 ),
							WRITE_UBITLONG( fromplayer ? 1 : 0, 1 ),
							MessageEnd();
					}
					else {
						// Send caption and duration hint down to client
						UserMessageBegin( filterCopy, "CloseCaption" );
						WRITE_LONG( hash );
						WRITE_UBITLONG( clamp( (int)(duration * 10.0f), 0, 65535 ), 15 ),
							WRITE_UBITLONG( fromplayer ? 1 : 0, 1 ),
							MessageEnd();
					}
				}
			}
#else
			// Direct dispatch
			CHudCloseCaption *cchud = GET_FULLSCREEN_HUDELEMENT( CHudCloseCaption );
			if ( cchud ) {
				cchud->ProcessCaption( lowercase, duration, fromplayer );
			}
#endif
		}
	}

	void EmitCloseCaption( IRecipientFilter& filter, int entindex, const CSoundParameters & params, const EmitSound_t & ep )
	{
		// Don't use dedicated closecaption ConVar since it will prevent remote clients from getting captions.
		// Okay to use it in SP, since it's the same ConVar, not the FCVAR_USERINFO one
		if ( gpGlobals->maxClients == 1 &&
			 !g_pClosecaption->GetBool() ) {
			return;
		}

		bool bForceSubtitle = false;

		if ( TestSoundChar( params.soundname, CHAR_SUBTITLED ) ) {
			bForceSubtitle = true;
		}

		if ( !bForceSubtitle && !ep.m_bEmitCloseCaption ) {
			return;
		}

		// NOTE:  We must make a copy or else if the filter is owned by a SoundPatch, we'll end up destructively removing
		//  all players from it!!!!
		CRecipientFilter filterCopy;
		filterCopy.CopyFrom( (CRecipientFilter &)filter );

		if ( !bForceSubtitle ) {
			// Remove any players who don't want close captions
			CBaseEntity::RemoveRecipientsIfNotCloseCaptioning( (CRecipientFilter &)filterCopy );
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
			duration = enginesound->GetSoundDuration( params.soundname );
		}

		bool fromplayer = false;
		CBaseEntity *ent = CBaseEntity::Instance( entindex );
		if ( ent ) {
			while ( ent ) {
				if ( ent->IsPlayer() ) {
					fromplayer = true;
					break;
				}

				ent = ent->GetOwnerEntity();
			}
		}
		EmitCloseCaption( filter, entindex, fromplayer, ep.m_pSoundName, ep.m_UtlVecSoundOrigin, duration, ep.m_bWarnOnMissingCloseCaption, bForceSubtitle );
	}

	void EmitAmbientSound( int entindex, const Vector& origin, const char *soundname, float flVolume, int iFlags, int iPitch, float soundtime /*= 0.0f*/, float *duration /*=NULL*/ )
	{
		// Pull data from parameters
		CSoundParameters params;

		if ( !soundemitterbase->GetParametersForSound( soundname, params, GENDER_NONE ) ) {
			return;
		}

		if ( iFlags & SND_CHANGE_PITCH ) {
			params.pitch = iPitch;
		}

		if ( iFlags & SND_CHANGE_VOL ) {
			params.volume = flVolume;
		}

#if defined( CLIENT_DLL )
		enginesound->EmitAmbientSound( params.soundname, params.volume, params.pitch, iFlags, soundtime );
#else
		engine->EmitAmbientSound( entindex, origin, params.soundname, params.volume, params.soundlevel, iFlags, params.pitch, soundtime );
#endif

		bool needsCC = !(iFlags & (SND_STOP | SND_CHANGE_VOL | SND_CHANGE_PITCH));

		float soundduration = 0.0f;

		if ( duration || needsCC ) {
			soundduration = enginesound->GetSoundDuration( params.soundname );
			if ( duration ) {
				*duration = soundduration;
			}
		}

		TraceEmitSound( entindex, "EmitAmbientSound:  '%s' emitted as '%s' (ent %i)\n",
						soundname, params.soundname, entindex );

		// We only want to trigger the CC on the start of the sound, not on any changes or halting of the sound
		if ( needsCC ) {
			CRecipientFilter filter;
			filter.AddAllPlayers();
			filter.MakeReliable();

			CUtlVector< Vector > dummy;
			EmitCloseCaption( filter, entindex, false, soundname, dummy, soundduration, false );
		}

	}

	void StopSoundByHandle( int entindex, const char *soundname, HSOUNDSCRIPTHANDLE& handle, bool bIsStoppingSpeakerSound = false )
	{
		if ( handle == SOUNDEMITTER_INVALID_HANDLE ) {
			handle = (HSOUNDSCRIPTHANDLE)soundemitterbase->GetSoundIndex( soundname );
		}

		if ( handle == SOUNDEMITTER_INVALID_HANDLE )
			return;

		CSoundParametersInternal *params;

		params = soundemitterbase->InternalGetParametersForSound( (int)handle );
		if ( !params ) {
			return;
		}

		// HACK:  we have to stop all sounds if there are > 1 in the rndwave section...
		int c = params->NumSoundNames();
		for ( int i = 0; i < c; ++i ) {
			char const *wavename = soundemitterbase->GetWaveName( params->GetSoundNames()[i].symbol );
			Assert( wavename );

			enginesound->StopSound(
				entindex,
				params->GetChannel(),
				wavename );

			TraceEmitSound( entindex, "StopSound:  '%s' stopped as '%s' (ent %i)\n",
							soundname, wavename, entindex );

#if !defined ( CLIENT_DLL )
			if ( bIsStoppingSpeakerSound == false ) {
				StopSpeakerSounds( wavename );
			}
#endif // !CLIENT_DLL 
		}

	}

	void StopSound( int entindex, const char *soundname )
	{
		HSOUNDSCRIPTHANDLE handle = (HSOUNDSCRIPTHANDLE)soundemitterbase->GetSoundIndex( soundname );
		if ( handle == SOUNDEMITTER_INVALID_HANDLE ) {
			return;
		}

		StopSoundByHandle( entindex, soundname, handle );
	}


	void StopSound( int iEntIndex, int iChannel, const char *pSample, bool bIsStoppingSpeakerSound = false )
	{
		if ( pSample && (Q_stristr( pSample, ".wav" ) || Q_stristr( pSample, ".mp3" ) || pSample[0] == '!') ) {
			enginesound->StopSound( iEntIndex, iChannel, pSample );

			TraceEmitSound( iEntIndex, "StopSound:  Raw wave stopped '%s' (ent %i)\n",
							pSample, iEntIndex );
#if !defined ( CLIENT_DLL )
			if ( bIsStoppingSpeakerSound == false ) {
				StopSpeakerSounds( pSample );
			}
#endif // !CLIENT_DLL 
		}
		else {
			// Look it up in sounds.txt and ignore other parameters
			StopSound( iEntIndex, pSample );
		}
	}

	void EmitAmbientSound( int entindex, const Vector &origin, const char *pSample, float volume, soundlevel_t soundlevel, int flags, int pitch, float soundtime /*= 0.0f*/, float *duration /*=NULL*/ )
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
			dummyorigins );
		if ( bSwallowed )
			return;
#endif

		if ( pSample && (Q_stristr( pSample, ".wav" ) || Q_stristr( pSample, ".mp3" )) ) {
#if defined( CLIENT_DLL )
			enginesound->EmitAmbientSound( pSample, volume, pitch, flags, soundtime );
#else
			engine->EmitAmbientSound( entindex, origin, pSample, volume, soundlevel, flags, pitch, soundtime );
#endif

			if ( duration ) {
				*duration = enginesound->GetSoundDuration( pSample );
			}

			TraceEmitSound( entindex, "EmitAmbientSound:  Raw wave emitted '%s' (ent %i)\n",
							pSample, entindex );
		}
		else {
			EmitAmbientSound( entindex, origin, pSample, volume, flags, pitch, soundtime, duration );
		}
	}


#if !defined( CLIENT_DLL )
	bool GetCaptionHash( char const *pchStringName, bool bWarnIfMissing, unsigned int &hash )
	{
		// hash the string, find in dictionary or return 0u if not there!!!
		CUtlVector< AsyncCaption_t >& directories = m_ServerCaptions;

		CaptionLookup_t search;
		search.SetHash( pchStringName );
		hash = search.hash;

		int idx = -1;
		int i;
		int dc = directories.Count();
		for ( i = 0; i < dc; ++i ) {
			idx = directories[i].m_CaptionDirectory.Find( search );
			if ( idx == directories[i].m_CaptionDirectory.InvalidIndex() )
				continue;

			break;
		}

		if ( i >= dc || idx == -1 ) {
			if ( bWarnIfMissing && cc_showmissing.GetBool() ) {
				static CUtlRBTree< unsigned int > s_MissingHashes( 0, 0, DefLessFunc( unsigned int ) );
				if ( s_MissingHashes.Find( hash ) == s_MissingHashes.InvalidIndex() ) {
					s_MissingHashes.Insert( hash );
					Msg( "Missing caption for %s\n", pchStringName );
				}
			}
			return false;
		}

		// Anything marked as L"" by content folks doesn't need to transmit either!!!
		CaptionLookup_t &entry = directories[i].m_CaptionDirectory[idx];
		if ( entry.length <= sizeof( wchar_t ) ) {
			return false;
		}

		return true;
	}
	void StopSpeakerSounds( const char *wavename )
	{
		// Stop sound on any speakers playing this wav name
		// but don't recurse in if this stopsound is happening on a speaker
		CEnvMicrophone::OnSoundStopped( wavename );
	}
#endif
};

#endif