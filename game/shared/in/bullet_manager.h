//==== Woots 2017. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef BULLET_MANAGER_H
#define BULLET_MANAGER_H

#ifdef _WIN32
#pragma once
#endif

#include "bullet.h"

//================================================================================
// Encargado de simular la trayectoria de las balas realistas
//================================================================================
class CBulletManager : public CAutoGameSystemPerFrame
{
public:
	DECLARE_CLASS_GAMEROOT( CBulletManager, CAutoGameSystemPerFrame );

	// CAutoGameSystemPerFrame
	virtual void LevelShutdownPreEntity();

#ifdef CLIENT_DLL
	virtual void Update( float frametime );
#else
	virtual void FrameUpdatePostEntityThink();
#endif

	//
	virtual void Simulate();

	virtual void Add( CBullet *pBullet );
	virtual void Remove( CBullet *pBullet );

protected:
	CUtlVector<CBullet *> m_nBullets;
};

extern CBulletManager *TheBulletManager;

#endif