//==== Woots 2017. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "sound_manager.h"
#include "sound_instance_manager.h"

#ifdef CLIENT_DLL
#include "c_in_player.h"
#else
#include "in_player.h"
#include "director.h"
#endif

#include "in_shareddefs.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
// Comandos
//================================================================================

DECLARE_REPLICATED_COMMAND( sv_debug_sound_manager, "0", "" );

//================================================================================
// Constructor
//================================================================================
CSoundManager::CSoundManager()
{
    TheSoundSystem->Add( this );
    m_nSounds.EnsureCapacity( 100 );
}

//================================================================================
// Destructor
//================================================================================
CSoundManager::~CSoundManager()
{
    TheSoundSystem->Remove( this );
}

//================================================================================
// Pensamiento
//================================================================================
void CSoundManager::Update()
{
    UpdateChannel();
}

//================================================================================
// Actualiza los sonidos por canal
//================================================================================
void CSoundManager::UpdateChannel()
{
    // Lista de sonidos por canal
    CUtlVector<CSoundInstance *> sounds[LAST_CHANNEL];

    // Primero separamos por canal los sonidos
    FOR_EACH_VEC( m_nSounds, it )
    {
        CSoundInstance *pSound = m_nSounds.Element( it );
        if ( pSound->GetChannel() < CHANNEL_ANY ) continue;

        sounds[pSound->GetChannel()].AddToTail( pSound );
    }

    // Revisamos cada canal
    for ( int channel = CHANNEL_1; channel < LAST_CHANNEL; ++channel ) {
        if ( sounds[channel].Count() == 0 ) 
            continue;

        if ( sv_debug_sound_manager.GetBool() )
            DevMsg( "\n\n CSoundManager - Channel %i\n", channel );

        CSoundInstance *pIdeal = NULL;
        float flDesire = 0.0f;

        // Tomamos el mejor sonido para este canal
        FOR_EACH_VEC( sounds[channel], it )
        {
            CSoundInstance *pSound = sounds[channel].Element( it );
            CBaseEntity *pOwner = pSound->GetOwner();

            Assert( pOwner );

            if ( !pOwner ) {
                Warning( "El sonido '%s' tiene un canal pero no un dueño.\n", pSound->GetSoundName() );
                continue;
            }

            float desire = pSound->GetDesire();

            if ( sv_debug_sound_manager.GetBool() )
                DevMsg( "	- %s = %.2f \n", pSound->GetSoundName(), desire );

            // El nuevo preferido, tiene mayor deseo
            if ( desire > flDesire ) {
                pIdeal = pSound;
                flDesire = desire;
            }
        }

        if ( pIdeal ) {
            float flVolume = -1.0f;

#ifdef CLIENT_DLL
            // Si no debería escucharlo aún entonces lo reproducimos en silencio
            if ( !pIdeal->ShouldListen() )
                flVolume = 0.0f;
#endif

            if ( !pIdeal->IsPlaying() ) {
                pIdeal->Play( flVolume );
                OnPlay( pIdeal );
            }
            else {
                pIdeal->SetVolume( flVolume );
            }

            if ( sv_debug_sound_manager.GetBool() )
                DevMsg( "Reproduciendo %s - Gano con %.2f \n", pIdeal->GetSoundName(), flDesire );
        }

        // Silenciamos todos los demás sonidos
        FOR_EACH_VEC( sounds[channel], it )
        {
            CSoundInstance *pSound = sounds[channel].Element( it );

            if ( pSound == pIdeal ) 
                continue;

            if ( pSound->IsPlaying() && pSound->IsLooping() ) {
                pSound->Fadeout( 0.1f, false );
                //pSound->Stop();
                OnStop( pSound );
            }
        }

        if ( sv_debug_sound_manager.GetBool() )
            DevMsg( "\n\n" );
    }

    // Revisamos CHANNEL_ANY
    // CHANNEL_ANY tiene un comportamiento diferente, reproduce si la función de deseo devuelve > 0
    // si no para el sonido, sin importar los demás sonidos
    if ( sounds[CHANNEL_ANY].Count() > 0 ) {
        FOR_EACH_VEC( sounds[CHANNEL_ANY], it )
        {
            CSoundInstance *pSound = sounds[CHANNEL_ANY].Element( it );
            CBaseEntity *pOwner = pSound->GetOwner();

            Assert( pOwner );

            if ( !pOwner ) {
                Warning( "El sonido '%s' tiene un canal pero no un dueño.\n", pSound->GetSoundName() );
                continue;
            }

            float desire = pSound->GetDesire();

            if ( sv_debug_sound_manager.GetBool() )
                DevMsg( "	- %s = %.2f \n", pSound->GetSoundName(), desire );

            // Tiene deseo
            if ( desire > 0.0f ) {
                // En este caso el nivel de deseo es el nivel de volumen
                float flVolume = desire;

                // Si es mayor 1, entonces usamos el volumen predeterminado
                // del sonido.
                if ( flVolume > 1.0f )
                    flVolume = -1.0f;

#ifdef CLIENT_DLL
                // Si no debería escucharlo aún entonces lo reproducimos en silencio
                if ( !pSound->ShouldListen() )
                    flVolume = 0.0f;
#endif

                if ( !pSound->IsPlaying() ) {
                    pSound->Play( flVolume );
                    OnPlay( pSound );
                }
                else {
                    pSound->SetVolume( flVolume );
                }
            }
            else {
                if ( pSound->IsPlaying() && pSound->IsLooping() ) {
                    pSound->Fadeout();
                    OnStop( pSound );
                }
            }
        }
    }
}

//================================================================================
// El sonido especificado debe reproducirse
//================================================================================
void CSoundManager::OnPlay( CSoundInstance *pSound )
{
    if ( pSound->GetOwner() ) {
        if ( pSound->GetOwner()->IsPlayer() ) {
            ToInPlayer( pSound->GetOwner() )->OnSoundPlay( pSound->GetSoundName() );
        }
#ifndef CLIENT_DLL
        else if ( pSound->GetOwner()->IsWorld() ) {
            TheDirector->OnSoundPlay( pSound->GetSoundName() );
        }
#endif
    }
}

//================================================================================
// El sonido especificado debe parar
//================================================================================
void CSoundManager::OnStop( CSoundInstance *pSound )
{
    if ( pSound->GetOwner() ) {
        if ( pSound->GetOwner()->IsPlayer() ) {
            ToInPlayer( pSound->GetOwner() )->OnSoundStop( pSound->GetSoundName() );
        }
#ifndef CLIENT_DLL
        else if ( pSound->GetOwner()->IsWorld() ) {
            TheDirector->OnSoundStop( pSound->GetSoundName() );
        }
#endif
    }
}

//================================================================================
//================================================================================
void CSoundManager::Add( CSoundInstance *pSound )
{
    // Ya existe
    if ( m_nSounds.Find( pSound ) != -1 )
        return;

    m_nSounds.AddToTail( pSound );
}

//================================================================================
//================================================================================
void CSoundManager::Remove( CSoundInstance *pSound )
{
    int index = m_nSounds.Find( pSound );

    if ( index == -1 )
        return;

    m_nSounds.Remove( index );
}
