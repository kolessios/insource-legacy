//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "sound_instance.h"

#ifndef CLIENT_DLL
#include "director.h"
#endif

#include "sound_instance_manager.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ISoundEmitterSystemBase *soundemitterbase;

//================================================================================
// Comandos
//================================================================================

DECLARE_DEBUG_CMD( sv_debug_sounds, "0", "" )

//================================================================================
// Constructor
//================================================================================
CSoundInstance::CSoundInstance( const char *pSoundName, int team, CBaseEntity *pOrigin, int target, CPlayer *pOwner )
{
    SoundInfo *info = new SoundInfo( pSoundName, team, pOrigin, pOwner, target );
    Setup( info );
}

//================================================================================
// Constructor
//================================================================================
CSoundInstance::CSoundInstance( const char * pSoundName, CBaseEntity * pOrigin, CPlayer * pOwner )
{
    SoundInfo *info = new SoundInfo( pSoundName, TEAM_ANY, pOrigin, pOwner, TARGET_NONE );
    Setup( info );
}

//================================================================================
// Constructor
//================================================================================
CSoundInstance::CSoundInstance( SoundInfo *info, CSoundInstance *master )
{
    Setup( info, master );
}

//================================================================================
// Destructor
//================================================================================
CSoundInstance::~CSoundInstance()
{
    if ( sv_debug_sounds.GetBool() )
        DevMsg( "[CSoundInstance] Destroy! / %s \n", GetSoundName() );

    Stop();

    if ( m_nSound ) {
        SOUND_CONTROLLER.Shutdown( m_nSound );
        m_nSound = NULL;
    }

    if ( m_nTagSound ) {
        delete m_nTagSound;
    }

    TheSoundSystem->Remove( this );
}

//================================================================================
//================================================================================
void CSoundInstance::Setup( SoundInfo *info, CSoundInstance *master )
{
    m_nSoundName = info->soundname;
    m_iTeam = info->team;			// TEAM_ANY
    m_nOriginEntity = info->originEntity.Get();	// NULL
    m_iTarget = info->target;			// TARGET_NONE
    m_bIsPlaying = false;
    m_bIsLoop = info->isLoop;
    m_nSound = NULL;
    m_nTagSound = info->tag;
    m_bIsTag = false;
    m_iChannel = info->channel;
    m_nOwner = info->owner.Get();
    m_bStopOnFinish = !m_bIsLoop;

    TheSoundSystem->Add( this );
    CBaseEntity::PrecacheScriptSound( m_nSoundName );

    if ( master ) {
        master->SetTag( this );
        //SetStopOnFinish( true );
        m_bIsTag = true;
    }
}

//================================================================================
// Inicialización: Debe ser llamado cada vez en Play() para los SoundScripts con
// sonidos al azar.
//================================================================================
bool CSoundInstance::Init()
{
    // Obtenemos la información del sonido
    if ( !soundemitterbase->GetParametersForSound( m_nSoundName, m_nParams, GENDER_NONE ) )
        return false;

    if ( m_nSound ) {
        Stop();
        SOUND_CONTROLLER.Shutdown( m_nSound );
        m_nSound = NULL;
    }

    m_flSoundDuration = enginesound->GetSoundDuration( m_nParams.soundname );

    int iEntIndex = (GetOriginEntity()) ? GetOriginEntity()->entindex() : 0;

#ifdef CLIENT_DLL
    // Por ahora todos los jugadores podrán escuchar el sonido
    // El administrador se encargara de subir o bajar el volumen.
    C_RecipientFilter filter;

    for ( int i = 1; i <= gpGlobals->maxClients; i++ ) {
        CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );

        if ( !pPlayer )
            continue;

        filter.AddRecipient( pPlayer );
    }
#else
    // Obtenemos el filtro
    CRecipientFilter filter;
    ShouldListen( NULL, filter );
#endif

    if ( sv_debug_sounds.GetBool() ) {
        DevMsg( "\n-----------------------------------------------------------\n" );
        DevMsg( "%s (%s)\n", m_nSoundName, m_nParams.soundname );

        for ( int i = 0; i < filter.GetRecipientCount(); ++i ) {
            CPlayer *pSpec = ToInPlayer( UTIL_PlayerByIndex( filter.GetRecipientIndex( i ) ) );
            DevMsg( "	- %s\n", pSpec->GetPlayerName() );
        }

        DevMsg( "-----------------------------------------------------------\n\n" );
    }

    m_nInfo.m_nChannel = m_nParams.channel;
    m_nInfo.m_pSoundName = m_nParams.soundname;
    m_nInfo.m_flVolume = m_nParams.volume;
    m_nInfo.m_SoundLevel = m_nParams.soundlevel;
    m_nInfo.m_nPitch = m_nParams.pitch;
    m_nInfo.m_pflSoundDuration = &m_flSoundDuration;
    m_nInfo.m_bEmitCloseCaption = false;
    m_nSound = SOUND_CONTROLLER.SoundCreate( filter, iEntIndex, m_nInfo );

    if ( !m_nSound )
        return false;

    return true;
}

//================================================================================
// Para la reproducción del sonido al acabarse
// Esta función se ejecuta automaticamente gracias a SoundGlobalSystem
//================================================================================
void CSoundInstance::Update()
{
    if ( IsPlaying() ) {
        if ( !IsLooping() && gpGlobals->curtime >= m_flFinishTime ) {
            m_bFinished = true;

            if ( sv_debug_sounds.GetBool() )
                DevMsg( "[CSoundInstance] Finished / %s (%.2f >= %.2f)\n", GetSoundName(), gpGlobals->curtime, m_flFinishTime );

            if ( StopOnFinish() )
                Stop();
        }
        else {
            m_bFinished = false;
        }
    }
}

//================================================================================
// Devuelve el deseo de reproducir
//================================================================================
float CSoundInstance::GetDesire()
{
    if ( GetOwner() ) {
        if ( GetOwner()->IsPlayer() ) {
            return ToInPlayer( GetOwner() )->SoundDesire( GetSoundName(), GetChannel() );
        }
#ifndef CLIENT_DLL
        else if ( GetOwner()->IsWorld() ) {
            return TheDirector->SoundDesire( GetSoundName(), GetChannel() );
        }
#endif
        else {
            Warning( "El sonido '%s' tiene un canal pero su dueño no es válido.\n", GetSoundName() );
        }
    }

    return 0.0f;
}

//================================================================================
// Devuelve si el jugador especificado puede escuchar el sonido
//================================================================================
bool CSoundInstance::ShouldListen( CPlayer *pSearch, CRecipientFilter &filter )
{
    CBaseEntity *pEntity = GetOriginEntity();

    // Tiene como destino una entidad
    /*if ( pEntity )
    {
    #ifdef CLIENT_DLL
    filter.AddAllPlayers();
    #else
    CPASAttenuationFilter filter( pEntity, m_nParams.soundlevel );
    #endif
    }
    else
    {
    filter.AddAllPlayers();
    }*/

    filter.AddAllPlayers();

    if ( m_iTeam != TEAM_ANY ) {
        FOR_EACH_PLAYER( {
            if ( pPlayer->GetTeamNumber() != m_iTeam )
            filter.RemoveRecipient( pPlayer );
        } )
    }

    if ( pEntity && pEntity->IsPlayer() ) {
        CBasePlayer *pPlayer = ToBasePlayer( pEntity );

        if ( m_iTarget == TARGET_ONLY ) {
#ifndef CLIENT_DLL
            filter.RemoveAllRecipients();
#else
            FOR_EACH_PLAYER( {
                filter.RemoveRecipient( pPlayer );
            } )
#endif

            filter.AddRecipient( pPlayer );
        }
        else if ( m_iTarget == TARGET_EXCEPT ) {
            filter.RemoveRecipient( pPlayer );
        }
    }

    if ( GetExceptPlayer() )
        filter.RemoveRecipient( GetExceptPlayer() );

    if ( !pSearch )
        return false;

    if ( filter.GetRecipientCount() == 0 )
        return false;

    for ( int i = 0; i < filter.GetRecipientCount(); ++i ) {
        CPlayer *pItem = ToInPlayer( UTIL_PlayerByIndex( filter.GetRecipientIndex( i ) ) );

        if ( pItem == pSearch )
            return true;
    }

    return false;
}

//================================================================================
// Devuelve si el jugador especificado puede escuchar el sonido
//================================================================================
bool CSoundInstance::ShouldListen( CPlayer *pPlayer )
{
#ifdef CLIENT_DLL
    // Usamos el jugador local
    if ( !pPlayer )
        pPlayer = C_Player::GetLocalInPlayer();

    if ( pPlayer ) {
        if ( pPlayer->IsObserver() && (pPlayer->GetObserverMode() == OBS_MODE_CHASE || pPlayer->GetObserverMode() == OBS_MODE_IN_EYE) && pPlayer->GetObserverTarget() )
            pPlayer = ToInPlayer( pPlayer->GetObserverTarget() );
    }
#endif

    CRecipientFilter dummy;
    return ShouldListen( pPlayer, dummy );
}

//================================================================================
// Reproduce el sonido
//================================================================================
void CSoundInstance::Play( float volume, int pitch )
{
    // Ya estamos reproduciendolo
    if ( IsPlaying() )
        return;

    if ( !Init() )
        return;

    // Volumen original
    if ( volume <= -1.0f )
        volume = m_nInfo.m_flVolume;

    // Pitch original
    if ( pitch <= -1 )
        pitch = m_nInfo.m_nPitch;

    // Reproducimos
    SOUND_CONTROLLER.Play( m_nSound, volume, pitch );

    m_nParams.volume = volume;
    m_nParams.pitch = pitch;
    m_bIsPlaying = true;

    // Tiempo en que el sonido termina
    m_flFinishTime = gpGlobals->curtime + m_flSoundDuration;

    if ( sv_debug_sounds.GetBool() )
        DevMsg( "[CSoundInstance] Play / %s (%.2f)\n", GetSoundName(), m_flSoundDuration );

    // Sonido "tag"
    if ( m_nTagSound )
        m_nTagSound->Play();
}

//================================================================================
// Para la reproducción del sonido
//================================================================================
void CSoundInstance::Stop()
{
    // No se esta reproduciendo
    if ( !IsPlaying() )
        return;

    Assert( m_nSound );

    // Paramos
    SOUND_CONTROLLER.SoundDestroy( m_nSound );
    m_bIsPlaying = false;

    if ( sv_debug_sounds.GetBool() )
        DevMsg( "[CSoundInstance] Stop / %s\n", GetSoundName() );
}

//================================================================================
// Para gradualmente la reproducción del sonido
//================================================================================
void CSoundInstance::Fadeout( float deltaTime, bool stop )
{
    // No se esta reproduciendo
    if ( !IsPlaying() )
        return;

    Assert( m_nSound );

    // Paramos
    SOUND_CONTROLLER.SoundFadeOut( m_nSound, deltaTime, stop );
    m_bIsPlaying = false;

    if ( sv_debug_sounds.GetBool() )
        DevMsg( "[CSoundInstance] SoundFadeOut / %s = %.2f\n", GetSoundName(), deltaTime );

    // Sonido "tag"
    if ( m_nTagSound )
        m_nTagSound->Fadeout( deltaTime, stop );
}

//================================================================================
// Establece el volumen del sonido
//================================================================================
void CSoundInstance::SetVolume( float volume, float deltaTime )
{
    if ( !IsPlaying() )
        return;

    Assert( m_nSound );

    // Mismo valor
    if ( volume == m_nParams.volume )
        return;

    if ( volume == -1 )
        volume = m_nInfo.m_flVolume;

    if ( sv_debug_sounds.GetBool() )
        DevMsg( "[CSoundInstance] SoundChangeVolume / %s = %.2f -> %.2f\n", GetSoundName(), m_nParams.volume, volume );

    SOUND_CONTROLLER.SoundChangeVolume( m_nSound, volume, deltaTime );
    m_nParams.volume = volume;
}

//================================================================================
// Restablece el volumen original del sonido
//================================================================================
void CSoundInstance::RestoreVolume( float deltaTime )
{
    SetVolume( m_nInfo.m_flVolume, deltaTime );
}

//================================================================================
// Establece el Pitch del sonido
//================================================================================
void CSoundInstance::SetPitch( int pitch, float deltaTime )
{
    if ( !IsPlaying() )
        return;

    Assert( m_nSound );

    // Mismo valor
    if ( pitch == m_nParams.pitch )
        return;

    if ( pitch == -1 )
        pitch = m_nInfo.m_nPitch;

    if ( sv_debug_sounds.GetBool() )
        DevMsg( "[CSoundInstance] SoundChangePitch / %s = %.2f -> %.2f\n", GetSoundName(), m_nParams.pitch, pitch );

    SOUND_CONTROLLER.SoundChangePitch( m_nSound, pitch, deltaTime );
    m_nParams.pitch = pitch;

    // Sonido "tag"
    if ( m_nTagSound )
        m_nTagSound->SetPitch( pitch, deltaTime );
}

//================================================================================
// Restaura el Pitch original del sonido
//================================================================================
void CSoundInstance::RestorePitch( float deltaTime )
{
    SetPitch( m_nInfo.m_nPitch, deltaTime );
}

//================================================================================
// Establece el nombre del sonido que debe reproducir
//================================================================================
void CSoundInstance::SetSoundName( const char *pSoundName, bool isLoop )
{
    m_nSoundName = pSoundName;
    m_bIsLoop = isLoop;

    CBaseEntity::PrecacheScriptSound( m_nSoundName );

    // Es el mismo
    if ( FStrEq( pSoundName, m_nSoundName ) )
        return;

    // Seguimos reproduciendo
    if ( IsPlaying() ) {
        Stop();
        Play();
    }
}

//================================================================================
// Establece el equipo que podrá escuchar el sonido
//================================================================================
void CSoundInstance::SetTeam( int team )
{
    m_iTeam = team;

    // Seguimos reproduciendo
    if ( IsPlaying() ) {
        Stop();
        Play();
    }
}

//================================================================================
// Establece el origen del sonido
//================================================================================
void CSoundInstance::SetOrigin( CBaseEntity * pEntity )
{
    m_nOriginEntity = pEntity;

    // Seguimos reproduciendo
    if ( IsPlaying() ) {
        Stop();
        Play();
    }
}

//================================================================================
// Establece una entidad que no debe escuchar el sonido
//================================================================================
void CSoundInstance::SetExcept( CPlayer *pEntity )
{
    m_nExceptPlayer = pEntity;

    // Seguimos reproduciendo
    if ( IsPlaying() ) {
        Stop();
        Play();
    }
}

//================================================================================
//================================================================================
void CSoundInstance::SetOwner( CBaseEntity *pEntity )
{
    m_nOwner = pEntity;
}

//================================================================================
// Establece el objetivo de quien escuchara el sonido
//================================================================================
void CSoundInstance::SetTarget( int target )
{
    m_iTarget = target;

    // Seguimos reproduciendo
    if ( IsPlaying() ) {
        Stop();
        Play();
    }
}