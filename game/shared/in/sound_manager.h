//==== Woots 2017. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef SOUND_MANAGER_H
#define SOUND_MANAGER_H

#ifdef _WIN32
#pragma once
#endif

#include "sound_instance.h"

//================================================================================
// Un sencillo adminstrador que controla los sonidos dependiendo de su canal y capa
//================================================================================
class CSoundManager
{
public:
	DECLARE_CLASS_NOBASE( CSoundManager );

    CSoundManager();
    ~CSoundManager();

	virtual void Update();
	virtual void UpdateChannel();

	virtual void OnPlay( CSoundInstance *pSound );
	virtual void OnStop( CSoundInstance *pSound );

	// 
	virtual void Add( CSoundInstance *pSound );
	virtual void Remove( CSoundInstance *pSound );

public:
	CUtlVector<CSoundInstance *> m_nSounds;
};

#endif