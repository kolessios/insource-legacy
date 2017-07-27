//==== Woots 2017. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "bullet_manager.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CBulletManager g_BulletManager;
CBulletManager *TheBulletManager = &g_BulletManager;

//================================================================================
// 
//================================================================================
void CBulletManager::LevelShutdownPreEntity()
{
	m_nBullets.PurgeAndDeleteElements();
}

#ifdef CLIENT_DLL
void CBulletManager::Update( float frametime )
{
	Simulate();
}
#else
void CBulletManager::FrameUpdatePostEntityThink()
{
	Simulate();
}
#endif

//================================================================================
// Simula la trayectoria de todas las balas regístradas
//================================================================================
void CBulletManager::Simulate() 
{
	FOR_EACH_VEC( m_nBullets, it )
	{
		bool alive = m_nBullets[it]->Simulate();

		if ( !alive )
			m_nBullets.Remove( it );
	}
}

//================================================================================
// Registra una bala realista
//================================================================================
void CBulletManager::Add( CBullet *pBullet ) 
{
	m_nBullets.AddToTail( pBullet );
}

//================================================================================
// Elimina una bala realista
//================================================================================
void CBulletManager::Remove( CBullet *pBullet ) 
{
	m_nBullets.FindAndRemove( pBullet );
}