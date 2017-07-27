//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef SOUND_INSTANCE_MANAGER_H
#define SOUND_INSTANCE_MANAGER_H

#ifdef _WIN32
#pragma once
#endif

#include "sound_instance.h"
#include "sound_manager.h"

//================================================================================
//================================================================================
class SoundGlobalSystem : public CAutoGameSystemPerFrame
{
public:
	DECLARE_CLASS_GAMEROOT( SoundGlobalSystem, CAutoGameSystemPerFrame );

	SoundGlobalSystem();

	// CAutoGameSystemPerFrame
    virtual bool Init() { return true; }
    virtual void Shutdown();

	virtual void LevelShutdownPreEntity();
    virtual void LevelShutdownPostEntity();

#ifndef CLIENT_DLL
	virtual void FrameUpdatePreEntityThink();
#else
	virtual void Update( float frametime );
	virtual void UpdateListen();
#endif

#if 0
	virtual void UpdateLayer();
#endif

	virtual void Add( CSoundInstance *pSound );
	virtual void Remove( CSoundInstance *pSound );

    virtual void Add( CSoundManager *pManager );
    virtual void Remove( CSoundManager *pManager );

protected:
	CUtlVector<CSoundInstance *> m_SoundList;
    CUtlVector<CSoundManager *> m_ManagerList;
};

extern SoundGlobalSystem *TheSoundSystem;

#endif