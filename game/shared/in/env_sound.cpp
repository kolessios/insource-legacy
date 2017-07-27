//==== Woots 2017. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "env_sound.h"

#ifndef CLIENT_DLL
#include "eventqueue.h"
#else

#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
// Información y Red
//================================================================================

LINK_ENTITY_TO_CLASS( env_sound, CEnvSound );
IMPLEMENT_NETWORKCLASS_ALIASED( EnvSound, DT_EnvSound );

BEGIN_NETWORK_TABLE( CEnvSound, DT_EnvSound )
#ifndef CLIENT_DLL
    SendPropFloat( SENDINFO( m_flStartVolume ) ),
    SendPropFloat( SENDINFO( m_flFadeVolume ) ),

    SendPropInt( SENDINFO( m_iStartPitch ) ),
    SendPropFloat( SENDINFO( m_flFadePitch ) ),

    SendPropFloat( SENDINFO( m_flVolume ) ),
    SendPropInt( SENDINFO( m_iPitch ) ),
    SendPropInt( SENDINFO( m_iTeam ) ),

    SendPropEHandle( SENDINFO( m_nOriginEntity ) ),
    SendPropInt( SENDINFO( m_iChannel ) ),

    SendPropString( SENDINFO( m_nSoundName ) ),
#else
    RecvPropFloat( RECVINFO( m_flStartVolume ) ),
    RecvPropFloat( RECVINFO( m_flFadeVolume ) ),

    RecvPropInt( RECVINFO( m_iStartPitch ) ),
    RecvPropFloat( RECVINFO( m_flFadePitch ) ),

    RecvPropFloat( RECVINFO( m_flVolume ) ),
    RecvPropInt( RECVINFO( m_iPitch ) ),
    RecvPropInt( RECVINFO( m_iTeam ) ),

    RecvPropEHandle( RECVINFO( m_nOriginEntity ) ),
    RecvPropInt( RECVINFO( m_iChannel ) ),

    RecvPropString( RECVINFO( m_nSoundName ) ),
#endif
END_NETWORK_TABLE()

#ifndef CLIENT_DLL
BEGIN_DATADESC( CEnvSound )
    DEFINE_KEYFIELD( m_flStartVolume, FIELD_FLOAT, "StartVolume" ),
    DEFINE_KEYFIELD( m_flFadeVolume, FIELD_FLOAT, "FadeVolume" ),

    DEFINE_KEYFIELD( m_iStartPitch, FIELD_INTEGER, "StartPitch" ),
    DEFINE_KEYFIELD( m_flFadePitch, FIELD_FLOAT, "FadePitch" ),

    DEFINE_KEYFIELD( m_flVolume, FIELD_FLOAT, "Volume" ),
    DEFINE_KEYFIELD( m_iPitch, FIELD_INTEGER, "Pitch" ),

    DEFINE_KEYFIELD( m_iTeam, FIELD_INTEGER, "Team" ),
    DEFINE_KEYFIELD( m_nOriginEntityKeyfield, FIELD_STRING, "OriginEntity" ),
    DEFINE_KEYFIELD( m_iChannel, FIELD_INTEGER, "Channel" ),

    DEFINE_KEYFIELD( m_nSoundNameKeyfield, FIELD_SOUNDNAME, "SoundName" ),

    // Inputs
    DEFINE_INPUTFUNC( FIELD_VOID, "Play", InputPlay ),
    DEFINE_INPUTFUNC( FIELD_VOID, "Stop", InputStop ),
    DEFINE_INPUTFUNC( FIELD_FLOAT, "Fadeout", InputFadeout ),
    DEFINE_INPUTFUNC( FIELD_FLOAT, "SetVolume", InputSetVolume ),
    DEFINE_INPUTFUNC( FIELD_VOID, "RestoreVolume", InputRestoreVolume ),
    DEFINE_INPUTFUNC( FIELD_FLOAT, "SetPitch", InputSetPitch ),
    DEFINE_INPUTFUNC( FIELD_VOID, "RestorePitch", InputRestorePitch ),
    DEFINE_INPUTFUNC( FIELD_STRING, "SetSoundName", InputSetSoundName ),
    DEFINE_INPUTFUNC( FIELD_INTEGER, "SetTeam", InputSetTeam ),
    DEFINE_INPUTFUNC( FIELD_STRING, "SetOrigin", InputSetOrigin ),
    DEFINE_INPUTFUNC( FIELD_INTEGER, "SetChannel", InputSetChannel ),
END_DATADESC()
#endif

//================================================================================
//================================================================================
void CEnvSound::Spawn()
{
    SetSolid( SOLID_NONE );
    SetMoveType( MOVETYPE_NONE );
    SetCollisionGroup( COLLISION_GROUP_NONE );
    SetModelName( NULL_STRING );

    AddEffects( EF_NODRAW );
    AddEFlags( EFL_FORCE_CHECK_TRANSMIT );

#ifndef CLIENT_DLL
    // Copiamos el nombre de sonido a la variable en red
    Q_strncpy( m_nSoundName.GetForModify(), STRING( m_nSoundNameKeyfield ), 260 );

    // Obtenemos la entidad de donde vendrá el sonido
    if ( m_nOriginEntityKeyfield != NULL_STRING )
        m_nOriginEntity = gEntList.FindEntityByName( NULL, m_nOriginEntityKeyfield );

    Msg( "m_nSoundName: %s\n", m_nSoundName.Get() );
    Msg( "m_nOriginName: %s\n", STRING( m_nOriginEntityKeyfield ) );

    // Creamos el sonido
    // Si el sonido es client-side no se hará nada
    CreateSound();

    // Empezamos a reproducir
    if ( HasSpawnFlags( SF_START_PLAYING ) )
        g_EventQueue.AddEvent( this, "Play", 1.0f, this, this );

    SetNextThink( gpGlobals->curtime + 0.1f );
#endif	

    // Base
    BaseClass::Spawn();
}

//================================================================================
// Crea el objeto [CSoundInstance]
//================================================================================
void CEnvSound::CreateSound()
{
    if ( HasSpawnFlags( SF_SERVER_SIDE ) ) {
#ifdef CLIENT_DLL
        return;
#endif
    }
    else {
#ifndef CLIENT_DLL
        return;
#endif
    }

    if ( m_flVolume == 0.0f )
        m_flVolume = -1.0f;

    if ( m_iPitch == 0 )
        m_iPitch = -1;

    // Información del sonido
    SoundInfo *info = SOUND_INFO();
    info->soundname = m_nSoundName.Get();
    info->team = m_iTeam.Get();
    info->originEntity = (m_nOriginEntity.Get()) ? m_nOriginEntity.Get() : this;
    info->owner = this;
    info->isLoop = HasSpawnFlags( SF_IS_LOOPED );
    info->channel = m_iChannel.Get();

    // Creamos el sonido
    m_nSound = new CSoundInstance( info );
}

//================================================================================
// Reproduce el sonido
//================================================================================
void CEnvSound::Play()
{
    float volume = m_flVolume;
    int pitch = m_iPitch;

    // Usamos el volumen de comienzo
    if ( m_flStartVolume > 0.0f )
        volume = m_flStartVolume;

    // Usamos el pitch de comienzo
    if ( m_iStartPitch > 0 )
        pitch = m_iStartPitch;

    Assert( m_nSound );
    m_nSound->Play( volume, pitch );

    // Cambiamos el volumen al normal con el tiempo de fade indicado
    if ( m_flStartVolume > 0.0f )
        m_nSound->SetVolume( m_flVolume, m_flFadeVolume );

    // Cambiamos el pitch al normal con el tiempo de fade indicado
    if ( m_iStartPitch > 0 )
        m_nSound->SetPitch( m_iPitch, m_flFadePitch );
}

//================================================================================
// Para la reproducción del sonido
//================================================================================
void CEnvSound::Stop()
{
    Assert( m_nSound );
    m_nSound->Stop();
}

//================================================================================
// Baja el volumen gradualmente hasta parar la reproducción del sonido
//================================================================================
void CEnvSound::Fadeout( float deltaTime )
{
    Assert( m_nSound );
    m_nSound->Fadeout( deltaTime );
}

//================================================================================
// Establece el volumen del sonido
//================================================================================
void CEnvSound::SetVolume( float volume )
{
    Assert( m_nSound );
    m_nSound->SetVolume( volume, m_flFadeVolume );
}

//================================================================================
// Restablece el volumen original del sonido
//================================================================================
void CEnvSound::RestoreVolume()
{
    Assert( m_nSound );
    m_nSound->RestoreVolume( m_flFadeVolume );
}

//================================================================================
// Establece el Pitch del sonido
//================================================================================
void CEnvSound::SetPitch( int pitch )
{
    Assert( m_nSound );
    m_nSound->SetPitch( pitch, m_flFadePitch );
}

//================================================================================
// Restaura el Pitch original del sonido
//================================================================================
void CEnvSound::RestorePitch( float deltaTime )
{
    Assert( m_nSound );
    m_nSound->RestorePitch( deltaTime );
}

//================================================================================
// Establece el nombre del sonido que debe reproducir
//================================================================================
void CEnvSound::SetSoundName( const char *pSoundName, bool isLoop )
{
    Assert( m_nSound );
    m_nSound->SetSoundName( pSoundName, isLoop );
}

//================================================================================
// Establece el equipo que podrá escuchar el sonido
//================================================================================
void CEnvSound::SetTeam( int team )
{
    Assert( m_nSound );
    m_nSound->SetTeam( team );
}

//================================================================================
// Establece el origen del sonido
//================================================================================
void CEnvSound::SetOrigin( CBaseEntity *pEntity )
{
    Assert( m_nSound );
    m_nSound->SetOrigin( pEntity );
}

//================================================================================
// Establece el canal de reproducción del sonido
//================================================================================
void CEnvSound::SetChannel( int channel )
{
    Assert( m_nSound );
    m_nSound->SetChannel( channel );
}

#ifndef CLIENT_DLL
//================================================================================
// Devuelve de que forma se transmitira esta entidad
//================================================================================
int CEnvSound::UpdateTransmitState()
{
    // El sonido se maneja server-side, no es necesario envíar la entidad
    // al cliente.
    if ( HasSpawnFlags( SF_SERVER_SIDE ) )
        return SetTransmitState( FL_EDICT_DONTSEND );

    // Sonido client-side, envíamos cualquier cambio.
    return SetTransmitState( FL_EDICT_ALWAYS );
}

//================================================================================
// Pensamiento
//================================================================================
void CEnvSound::Think()
{
    if ( m_nSound->IsPlaying() ) {
        if ( m_nSound->IsLooping() && gpGlobals->curtime >= m_nSound->GetFinishTime() ) {
            m_nSound->Stop();
            m_nSound->Play();
        }
    }

    SetNextThink( gpGlobals->curtime + 0.1f );
}

void CEnvSound::InputPlay( inputdata_t &inputdata )
{
    if ( HasSpawnFlags( SF_SERVER_SIDE ) ) {
        Play();
    }
    else {
        EntityMessageBegin( this );
        WRITE_BYTE( ENV_SOUND_PLAY );
        MessageEnd();
    }
}

void CEnvSound::InputStop( inputdata_t &inputdata )
{
    if ( HasSpawnFlags( SF_SERVER_SIDE ) ) {
        Stop();
    }
    else {
        EntityMessageBegin( this );
        WRITE_BYTE( ENV_SOUND_STOP );
        MessageEnd();
    }
}

void CEnvSound::InputFadeout( inputdata_t &inputdata )
{
    float deltaTime = inputdata.value.Float();

    if ( HasSpawnFlags( SF_SERVER_SIDE ) ) {
        Fadeout( deltaTime );
    }
    else {
        EntityMessageBegin( this );
        WRITE_BYTE( ENV_SOUND_FADEOUT );
        WRITE_FLOAT( deltaTime );
        MessageEnd();
    }
}

void CEnvSound::InputSetVolume( inputdata_t &inputdata )
{
    float volume = inputdata.value.Float();

    if ( HasSpawnFlags( SF_SERVER_SIDE ) ) {
        SetVolume( volume );
    }
    else {
        EntityMessageBegin( this );
        WRITE_BYTE( ENV_SOUND_CHANGE_VOLUME );
        WRITE_FLOAT( volume );
        MessageEnd();
    }
}

void CEnvSound::InputRestoreVolume( inputdata_t &inputdata )
{
    if ( HasSpawnFlags( SF_SERVER_SIDE ) ) {
        RestoreVolume();
    }
    else {
        EntityMessageBegin( this );
        WRITE_BYTE( ENV_SOUND_CHANGE_VOLUME );
        WRITE_FLOAT( -1.0f );
        MessageEnd();
    }
}

void CEnvSound::InputSetPitch( inputdata_t &inputdata )
{
    float pitch = inputdata.value.Float();

    if ( HasSpawnFlags( SF_SERVER_SIDE ) ) {
        SetPitch( pitch );
    }
    else {
        EntityMessageBegin( this );
        WRITE_BYTE( ENV_SOUND_CHANGE_PITCH );
        WRITE_FLOAT( pitch );
        MessageEnd();
    }
}

void CEnvSound::InputRestorePitch( inputdata_t &inputdata )
{
    if ( HasSpawnFlags( SF_SERVER_SIDE ) ) {
        RestorePitch();
    }
    else {
        EntityMessageBegin( this );
        WRITE_BYTE( ENV_SOUND_CHANGE_PITCH );
        WRITE_FLOAT( -1.0f );
        MessageEnd();
    }
}

void CEnvSound::InputSetSoundName( inputdata_t &inputdata )
{
    const char *name = inputdata.value.String();

    if ( HasSpawnFlags( SF_SERVER_SIDE ) ) {
        SetSoundName( name, HasSpawnFlags( SF_IS_LOOPED ) );
    }
    else {
        EntityMessageBegin( this );
        WRITE_BYTE( ENV_SOUND_CHANGE_SOUNDNAME );
        WRITE_STRING( name );
        MessageEnd();
    }
}

void CEnvSound::InputSetTeam( inputdata_t &inputdata )
{
    int team = inputdata.value.Int();

    if ( HasSpawnFlags( SF_SERVER_SIDE ) ) {
        SetTeam( team );
    }
    else {
        EntityMessageBegin( this );
        WRITE_BYTE( ENV_SOUND_CHANGE_TEAM );
        WRITE_BYTE( team );
        MessageEnd();
    }
}

void CEnvSound::InputSetOrigin( inputdata_t &inputdata )
{
    const char *name = inputdata.value.String();
    CBaseEntity *pEntity = gEntList.FindEntityByName( NULL, MAKE_STRING( name ) );

    if ( !pEntity ) {
        Warning( "%s no corresponde a ninguna entidad.\n", name );
        return;
    }

    if ( HasSpawnFlags( SF_SERVER_SIDE ) ) {
        SetOrigin( pEntity );
    }
    else {
        EntityMessageBegin( this );
        WRITE_BYTE( ENV_SOUND_CHANGE_ORIGIN );
        WRITE_EHANDLE( pEntity );
        MessageEnd();
    }
}

void CEnvSound::InputSetChannel( inputdata_t &inputdata )
{
    int channel = inputdata.value.Int();

    if ( HasSpawnFlags( SF_SERVER_SIDE ) ) {
        SetChannel( channel );
    }
    else {
        EntityMessageBegin( this );
        WRITE_BYTE( ENV_SOUND_CHANGE_CHANNEL );
        WRITE_BYTE( channel );
        MessageEnd();
    }
}

#else
//================================================================================
// Se ha recibido información del servidor
//================================================================================
void CEnvSound::PostDataUpdate( DataUpdateType_t updateType )
{
    // La entidad ha sido creada en el cliente
    if ( updateType == DATA_UPDATE_CREATED ) {
        // Creamos el sonido
        Msg( "m_nSoundName: %s\n", m_nSoundName.Get() );
        CreateSound();
    }

    // Base!
    BaseClass::PostDataUpdate( updateType );
}

//================================================================================
// Pensamiento
//================================================================================
void CEnvSound::ClientThink()
{
    // ??

    SetNextClientThink( CLIENT_THINK_ALWAYS );
}

//================================================================================
// Se ha recibido un mensaje del servidor
//================================================================================
void CEnvSound::ReceiveMessage( int classID, bf_read & msg )
{
    // Mensaje para la clase padre
    if ( classID != GetClientClass()->m_ClassID ) {
        BaseClass::ReceiveMessage( classID, msg );
        return;
    }

    Assert( m_nSound );

    int messageType = msg.ReadByte();

    switch ( messageType ) {
        // Reproducir
        case ENV_SOUND_PLAY:
        {
            Play();
            break;
        }

        // Parar
        case ENV_SOUND_STOP:
        {
            Stop();
            break;
        }

        // Para progresivamente
        case ENV_SOUND_FADEOUT:
        {
            float value = msg.ReadBitFloat();
            Fadeout( value );
            break;
        }

        // Cambiar/Restaurar volumen
        case ENV_SOUND_CHANGE_VOLUME:
        {
            float value = msg.ReadBitFloat();

            if ( value >= 0.0f ) {
                SetVolume( value );
            }
            else {
                RestoreVolume();
            }

            break;
        }

        // Cambiar/Restaurar pitch
        case ENV_SOUND_CHANGE_PITCH:
        {
            float value = msg.ReadBitFloat();

            if ( value >= 0.0f ) {
                SetPitch( value );
            }
            else {
                RestorePitch();
            }

            break;
        }

        case ENV_SOUND_CHANGE_SOUNDNAME:
        {
            const char *soundname = msg.ReadAndAllocateString();
            SetSoundName( soundname );
            break;
        }

        case ENV_SOUND_CHANGE_TEAM:
        {
            int value = msg.ReadByte();
            SetTeam( value );
            break;
        }

        case ENV_SOUND_CHANGE_ORIGIN:
        {
            AssertOnce( !"TODO!" );
            //msg.ReadEn
            break;
        }

        case ENV_SOUND_CHANGE_CHANNEL:
        {
            int value = msg.ReadByte();
            SetChannel( value );
            break;
        }
    }
}
#endif