//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "sound_instance_manager.h"

#include "in_shareddefs.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

SoundGlobalSystem g_SoundGlobalSystem;
SoundGlobalSystem *TheSoundSystem = &g_SoundGlobalSystem;

extern ConVar sv_debug_sound_manager;

//================================================================================
// Constructor
//================================================================================
SoundGlobalSystem::SoundGlobalSystem() : CAutoGameSystemPerFrame("SoundGlobalSystem")
{
}

//================================================================================
// Destruye todos los sonidos
//================================================================================
void SoundGlobalSystem::Shutdown() 
{
    //m_ManagerList.PurgeAndDeleteElements();
    //m_SoundList.PurgeAndDeleteElements();
}

//================================================================================
// Descarga del mapa actual antes de eliminar las entidades
//================================================================================
void SoundGlobalSystem::LevelShutdownPreEntity() 
{
}

void SoundGlobalSystem::LevelShutdownPostEntity()
{
    Shutdown();
}

#ifndef CLIENT_DLL
//================================================================================
//================================================================================
void SoundGlobalSystem::FrameUpdatePreEntityThink() 
{
    FOR_EACH_VEC( m_ManagerList, it )
    {
        CSoundManager *pManager = m_ManagerList.Element( it );
        Assert( pManager );
        pManager->Update();
    }

	FOR_EACH_VEC( m_SoundList, it )
	{
		CSoundInstance *pSound = m_SoundList.Element( it );
		Assert( pSound );
		pSound->Update();
	}

	// Layers
	// SoundMixers!!!! :( https://twitter.com/Kolesias123/status/827403221513564161
	//UpdateLayer();
}
#else
//================================================================================
//================================================================================
void SoundGlobalSystem::Update( float frametime ) 
{
    FOR_EACH_VEC( m_ManagerList, it )
    {
        CSoundManager *pManager = m_ManagerList.Element( it );
        Assert( pManager );
        pManager->Update();
    }

	FOR_EACH_VEC( m_SoundList, it )
	{
		CSoundInstance *pSound = m_SoundList.Element( it );
		Assert( pSound );
		pSound->Update();
	}

	// 
	UpdateListen();
}

//================================================================================
// Silencia los sonidos que el jugador local no puede escuchar
//================================================================================
void SoundGlobalSystem::UpdateListen() 
{
	FOR_EACH_VEC( m_SoundList, it )
	{
		CSoundInstance *pSound = m_SoundList.Element( it );

		// Podemos escucharlo
		if ( pSound->ShouldListen() ) 
            continue;

		// No podemos escucharlo, lo silenciamos
		// por si en el futuro deberíamos oirlo
		pSound->SetVolume( 0.0f );
	}
}
#endif

//================================================================================
// Actualiza el volumen de cada sonido basandose en la prioridad de su capa.
// @OBSOLETE: Para eso estan los SoundMixers
//================================================================================
#if 0
void SoundGlobalSystem::UpdateLayer() 
{
#ifndef CLIENT_DLL
	// Recuerda que los sonidos creados en servidor afectan a todos los usuarios,
	// es recomendable usar este sistema de capas para sonidos en cliente o que usan (env_sound)
#else
	
#endif

	// Capa mayor
	int seniorLayer = 0;

	// Verificamos cada sonido reproduciendose y verificamos
	// cual es el que tiene una capa mayor
	FOR_EACH_VEC( m_nList, it )
	{
		CSoundInstance *pSound = m_nList.Element( it );

		// Sin capa
		if ( pSound->GetLayer() < 0 ) continue;

		// No se esta reproduciendo
		if ( !pSound->IsPlaying() ) continue;

		#ifdef CLIENT_DLL
		// No debería escuchar este sonido
		if ( !pSound->ShouldListen() ) continue;
		#endif

		// Este sonido tiene mayor capa
		if ( pSound->GetLayer() > seniorLayer )
			seniorLayer = pSound->GetLayer();
	}

	if ( sv_debug_sound_manager.GetBool() )
		DevMsg("Capa mayor: %i \n", seniorLayer);

	FOR_EACH_VEC( m_nList, it )
	{
		CSoundInstance *pSound = m_nList.Element( it );

		// Sin capa
		if ( pSound->GetLayer() < 0 ) continue;

		// No se esta reproduciendo
		if ( !pSound->IsPlaying() ) continue;

		#ifdef CLIENT_DLL
		// No debería escuchar este sonido
		if ( !pSound->ShouldListen() )	continue;
		#endif

		// Bajamos el volumen de las capas inferiores
		if ( pSound->GetLayer() < seniorLayer )
		{
			pSound->SetVolume( 0.01f, 0.5f );
		}

		// Restauramos el de las capas superiores
		else 
		{
			if ( pSound->GetVolume() == 0.01f )
				pSound->RestoreVolume( 0.5f );
		}
	}
}
#endif

//================================================================================
//================================================================================
void SoundGlobalSystem::Add( CSoundInstance *pSound )
{
    // Ya existe
    if ( m_SoundList.Find( pSound ) != -1 )
        return;

    m_SoundList.AddToTail( pSound );
}

//================================================================================
//================================================================================
void SoundGlobalSystem::Remove( CSoundInstance *pSound ) 
{
	int index = m_SoundList.Find( pSound );

	if ( index == -1 )
		return;

    m_SoundList.Remove( index );
}

//================================================================================
//================================================================================
void SoundGlobalSystem::Add( CSoundManager * pManager )
{
    if ( m_ManagerList.Find( pManager ) != -1 )
        return;

    m_ManagerList.AddToTail( pManager );
}

//================================================================================
//================================================================================
void SoundGlobalSystem::Remove( CSoundManager * pManager )
{
    int index = m_ManagerList.Find( pManager );

    if ( index == -1 )
        return;

    m_ManagerList.Remove( index );
}
