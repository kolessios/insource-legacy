#ifndef WEAPON_RAINBOWBASE_COMBATWEAPON_H
#define WEAPON_RAINBOWBASE_COMBATWEAPON_H
#ifdef _WIN32
#pragma once
#endif

#ifdef CLIENT_DLL
	#include "skeleton/c_skeleton_player.h"
#else
	#include "skeleton/skeleton_player.h"
#endif

#include "weapon_rainbowbase.h"

#if defined( CLIENT_DLL )
#define CWeaponRainbowBase_CombatWeapon C_WeaponRainbowBase_CombatWeapon
#endif

class CWeaponRainbowBase_CombatWeapon : public CWeaponRainbowBase
{
#if !defined( CLIENT_DLL )
	DECLARE_DATADESC();
#endif

	DECLARE_CLASS( CWeaponRainbowBase_CombatWeapon, CWeaponRainbowBase );
public:
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CWeaponRainbowBase_CombatWeapon();

	virtual bool	WeaponShouldBeLowered( void );

	virtual bool	Ready( void );
	virtual bool	Lower( void );
	virtual bool	Deploy( void );
	virtual bool	Holster( CBaseCombatWeapon *pSwitchingTo );
	virtual void	WeaponIdle( void );

	virtual void	AddViewmodelBob( CBaseViewModel *viewmodel, Vector &origin, QAngle &angles );
	virtual	float	CalcViewmodelBob( void );

	virtual Vector	GetBulletSpread( WeaponProficiency_t proficiency );
	virtual float	GetSpreadBias( WeaponProficiency_t proficiency );

	virtual const	WeaponProficiencyInfo_t *GetProficiencyValues();
	static const	WeaponProficiencyInfo_t *GetDefaultProficiencyValues();

	virtual void	ItemHolsterFrame( void );

protected:

	bool			m_bLowered;			// Whether the viewmodel is raised or lowered
	float			m_flRaiseTime;		// If lowered, the time we should raise the viewmodel
	float			m_flHolsterTime;	// When the weapon was holstered

private:
	
	CWeaponRainbowBase_CombatWeapon( const CWeaponRainbowBase_CombatWeapon & );
};

#endif // WEAPON_RAINBOWBASE_COMBATWEAPON_H
