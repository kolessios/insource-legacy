//========= Copyright © 1996-2008, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=====================================================================================//

#include "cbase.h"
#include "weapon_sdkbase.h"

#if defined( CLIENT_DLL )

	#define CWeaponMP5 C_WeaponMP5
	#include "c_in_player.h"

#else

	#include "in_player.h"

#endif


class CWeaponMP5 : public CWeaponSDKBase
{
public:
	DECLARE_CLASS( CWeaponMP5, CWeaponSDKBase );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();
	
	CWeaponMP5();

private:

	CWeaponMP5( const CWeaponMP5 & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponMP5, DT_WeaponMP5 )

BEGIN_NETWORK_TABLE( CWeaponMP5, DT_WeaponMP5 )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponMP5 )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_mp5, CWeaponMP5 );
PRECACHE_WEAPON_REGISTER( weapon_mp5 );



CWeaponMP5::CWeaponMP5()
{
}

acttable_t CWeaponMP5::m_acttable[] = 
{
	{ ACT_MP_STAND_IDLE,					ACT_IDLE_SMG,				false },
	{ ACT_MP_CROUCH_IDLE,					ACT_CROUCHIDLE_SMG,			false },

	{ ACT_MP_RUN,							ACT_RUN_SMG,				false },
	{ ACT_MP_WALK,							ACT_WALK_SMG,				false },
	{ ACT_MP_CROUCHWALK,					ACT_RUN_CROUCH_SMG,			false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,		ACT_PRIMARYATTACK_SMG,		false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,		ACT_PRIMARYATTACK_SMG,		false },

	{ ACT_MP_RELOAD_STAND,					ACT_RELOAD_SMG,				false },
	{ ACT_MP_RELOAD_CROUCH,					ACT_RELOAD_SMG,				false },

};

IMPLEMENT_ACTTABLE( CWeaponMP5 );

