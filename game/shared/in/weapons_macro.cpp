//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "weapon_base.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Global Testing
DECLARE_WEAPON( WeaponCubemap, cubemap, CBaseWeapon, PISTOL )

DECLARE_WEAPON( WeaponAK47, rifle_ak47, CBaseWeapon, RIFLE )
DECLARE_WEAPON( WeaponM16, rifle_m16, CBaseWeapon, RIFLE )
DECLARE_WEAPON( WeaponSMG, smg, CBaseWeapon, SMG )
DECLARE_WEAPON( WeaponP220, pistol_p220, CBaseWeapon, PISTOL )
DECLARE_WEAPON( WeaponCombatShotgun, shotgun_combat, CBaseWeaponShotgun, SHOTGUN )
//DECLARE_WEAPON( WeaponPumpShotgun, shotgun_pump, CBaseWeaponShotgun, PUMPSHOTGUN )
DECLARE_WEAPON( WeaponSniperRifle, rifle_sniper, CBaseWeaponSniper, SHOTGUN )

#ifdef APOCALYPSE
//DECLARE_WEAPON( WeaponAK47, ak47, CBaseWeapon, AR2 )
//DECLARE_WEAPON( WeaponM4A1, m4a1, CBaseWeapon, AR2 )
#endif