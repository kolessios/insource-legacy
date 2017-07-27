#include "cbase.h"
#include "predicted_viewmodel.h"
//#include "skeleton/multiplayer/basenetworkedplayer.h"

class CSkeletonPlayer : public CBasePlayer //CBaseNetworkedPlayer
{
public:
	DECLARE_CLASS( CSkeletonPlayer, CBasePlayer );
	DECLARE_NETWORKCLASS();

	virtual void Spawn();

	// Create a predicted viewmodel
	void CreateViewModel( int index );

	void PostThink();

	// Lag compensate when firing bullets
	void FireBullets ( const FireBulletsInfo_t &info );
};