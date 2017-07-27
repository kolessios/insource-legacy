//========= Copyright © 1996-2008, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=====================================================================================//

#include "cbase.h"
#include "weapon_sdkbase.h"

#if defined( CLIENT_DLL )

	#define CWeaponPistol C_WeaponPistol
	#include "c_in_player.h"

#else

	#include "in_player.h"

#endif


class CWeaponPistol : public CWeaponSDKBase
{
public:
	DECLARE_CLASS( CWeaponPistol, CWeaponSDKBase );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();
	DECLARE_ACTTABLE();
	
	CWeaponPistol();

private:

	CWeaponPistol( const CWeaponPistol & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponPistol, DT_WeaponPistol )

BEGIN_NETWORK_TABLE( CWeaponPistol, DT_WeaponPistol )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponPistol )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_pistol, CWeaponPistol );
PRECACHE_WEAPON_REGISTER( weapon_pistol );



CWeaponPistol::CWeaponPistol()
{
}

//Tony; todo; add ACT_MP_PRONE* activities, so we have them.
acttable_t CWeaponPistol::m_acttable[] = 
{
	{ ACT_MP_STAND_IDLE,					ACT_IDLE_PISTOL,				false },
	{ ACT_MP_CROUCH_IDLE,					ACT_CROUCHIDLE_PISTOL,			false },

	{ ACT_MP_RUN,							ACT_RUN_PISTOL,					false },
	{ ACT_MP_WALK,							ACT_WALK_PISTOL,				false },
	{ ACT_MP_CROUCHWALK,					ACT_RUN_CROUCH_PISTOL,			false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,		ACT_PRIMARYATTACK_PISTOL,		false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,		ACT_PRIMARYATTACK_PISTOL,		false },

	{ ACT_MP_RELOAD_STAND,					ACT_RELOAD_PISTOL,				false },
	{ ACT_MP_RELOAD_CROUCH,					ACT_RELOAD_PISTOL,				false },
};

IMPLEMENT_ACTTABLE( CWeaponPistol );

