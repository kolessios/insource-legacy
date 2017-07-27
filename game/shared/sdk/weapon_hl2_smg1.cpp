//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"

#ifdef CLIENT_DLL
	#include "rainbow/c_rainbow_player.h"
#else
	#include "rainbow/rainbow_player.h"
	#include "basegrenade_shared.h"
#endif

#include "weapon_rainbowbase.h"
#include "weapon_rainbowbase_machinegun.h"

#ifdef CLIENT_DLL
#define CWeapon_HL2_SMG1 C_Weapon_HL2_SMG1
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CWeapon_HL2_SMG1 : public CWeaponRainbowBase_MachineGun
{
public:
	DECLARE_CLASS( CWeapon_HL2_SMG1, CWeaponRainbowBase_MachineGun );

	CWeapon_HL2_SMG1();

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	
	void	Precache( void );
	void	AddViewKick( void );

	int		GetMinBurst() { return 2; }
	int		GetMaxBurst() { return 5; }

	virtual void Equip( CBaseCombatCharacter *pOwner );
	bool	Reload( void );

	float	GetFireRate( void ) { return 0.075f; }	// 13.3hz
	Activity	GetPrimaryAttackActivity( void );

	virtual const Vector& GetBulletSpread( void )
	{
		static const Vector cone = VECTOR_CONE_5DEGREES;
		return cone;
	}

	const WeaponProficiencyInfo_t *GetProficiencyValues();

	DECLARE_ACTTABLE();
	
private:
	CWeapon_HL2_SMG1( const CWeapon_HL2_SMG1 & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( Weapon_HL2_SMG1, DT_Weapon_HL2_SMG1 )

BEGIN_NETWORK_TABLE( CWeapon_HL2_SMG1, DT_Weapon_HL2_SMG1 )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeapon_HL2_SMG1 )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_hl2_smg1, CWeapon_HL2_SMG1 );
PRECACHE_WEAPON_REGISTER(weapon_hl2_smg1);

acttable_t	CWeapon_HL2_SMG1::m_acttable[] = 
{
	{ ACT_MP_STAND_IDLE,				        ACT_MP_STAND_PRIMARY,					    false },
	{ ACT_MP_CROUCH_IDLE,				        ACT_MP_CROUCH_PRIMARY,			            false },

	{ ACT_MP_RUN,						        ACT_MP_RUN_PRIMARY,					        false },
	{ ACT_MP_CROUCHWALK,				        ACT_MP_CROUCHWALK_PRIMARY,			        false },

	{ ACT_MP_ATTACK_STAND_PRIMARYFIRE,	        ACT_MP_ATTACK_STAND_PRIMARYFIRE,            false },
	{ ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,	        ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,           false },

	{ ACT_MP_JUMP_START,						ACT_MP_JUMP_START_PRIMARY,					false },
	{ ACT_MP_JUMP_LAND,						    ACT_MP_JUMP_LAND_PRIMARY,					false },
	{ ACT_MP_AIRWALK,						    ACT_MP_AIRWALK_PRIMARY,					    false },
};

IMPLEMENT_ACTTABLE(CWeapon_HL2_SMG1);

//=========================================================
CWeapon_HL2_SMG1::CWeapon_HL2_SMG1( )
{
	m_fMinRange1		= 0;
	m_fMaxRange1		= 1400;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeapon_HL2_SMG1::Precache( void )
{
	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: Give this weapon longer range when wielded by an ally NPC.
//-----------------------------------------------------------------------------
void CWeapon_HL2_SMG1::Equip( CBaseCombatCharacter *pOwner )
{
	m_fMaxRange1 = 1400;
	BaseClass::Equip( pOwner );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Activity
//-----------------------------------------------------------------------------
Activity CWeapon_HL2_SMG1::GetPrimaryAttackActivity( void )
{
	return ACT_VM_PRIMARYATTACK;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CWeapon_HL2_SMG1::Reload( void )
{
	bool fRet;

	fRet = DefaultReload( GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD );
	if ( fRet )
	{
		WeaponSound( RELOAD );
		ToRainbowPlayer(GetOwner())->DoAnimationEvent( PLAYERANIMEVENT_RELOAD );

	}

	return fRet;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeapon_HL2_SMG1::AddViewKick( void )
{
	#define	EASY_DAMPEN			0.5f
	#define	MAX_VERTICAL_KICK	2.0f	//Degrees
	#define	SLIDE_LIMIT			3.0f	//Seconds
	
	//Get the view kick
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if ( pPlayer == NULL )
		return;

	DoMachineGunKick( pPlayer, EASY_DAMPEN, MAX_VERTICAL_KICK, m_fFireDuration, SLIDE_LIMIT );
}

//-----------------------------------------------------------------------------
const WeaponProficiencyInfo_t *CWeapon_HL2_SMG1::GetProficiencyValues()
{
	static WeaponProficiencyInfo_t proficiencyTable[] =
	{
		{ 7.0,		0.75	},
		{ 5.00,		0.75	},
		{ 10.0/3.0, 0.75	},
		{ 5.0/3.0,	0.75	},
		{ 1.00,		1.0		},
	};

	COMPILE_TIME_ASSERT( ARRAYSIZE(proficiencyTable) == WEAPON_PROFICIENCY_PERFECT + 1);

	return proficiencyTable;
}
