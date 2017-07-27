//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include <KeyValues.h>
#include "weapon_info.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

FileWeaponInfo_t* CreateWeaponInfo()
{
    return new CWeaponInfo;
}

//================================================================================
// Procesa la configuración del arma
//================================================================================
void CWeaponInfo::Parse( KeyValues *pKeyValuesData, const char *szWeaponName ) 
{
    // Base!
    BaseClass::Parse( pKeyValuesData, szWeaponName );

    // Daño por bala
    m_iDamage = pKeyValuesData->GetInt( "Damage", 15 );

    // Balas por disparo
    m_iBullets = pKeyValuesData->GetInt( "Bullets", 1 );

	// Cadencia de tiro
    m_flFireRate = pKeyValuesData->GetFloat( "FireRate", 0.1f );

	// Poder del retroceso
	m_flVerticalPunch = pKeyValuesData->GetFloat("VerticalPunch", 2.0f);

    // Disperción por bala
    m_flSpreadPerShot = pKeyValuesData->GetFloat( "SpreadPerShot", 0.01f );

	// Modificador de distribución: Al estar agachado
	m_flCrouchSpread = pKeyValuesData->GetFloat( "CrouchSpread", 0.7f );

	// Modificador de distribución: Para un Bot
	m_flBotSpread = pKeyValuesData->GetFloat( "BotSpread", 1.3f );

	// Modificador de distribución: Al saltar
	m_flJumpSpread = pKeyValuesData->GetFloat( "JumpSpread", 1.5f );

	// Modificador de distribución: Al estar incapacitado
	m_flDejectedSpread = pKeyValuesData->GetFloat( "DejectedSpread", 1.5f );

	// Modificador de distribución: Al movernos
	m_flMovingSpread = pKeyValuesData->GetFloat( "MovingSpread", 1.1f );

	// MuzzleFlash en primera persona
	Q_strncpy( m_nMuzzleFlashEffect_1stPerson, pKeyValuesData->GetString("MuzzleFlashEffect_1stPerson", "weapon_muzzle_flash_smg_FP"), 125 );

	// MuzzleFlash en tercera persona
	Q_strncpy( m_nMuzzleFlashEffect_3rdPerson, pKeyValuesData->GetString("MuzzleFlashEffect_3rdPerson", "weapon_muzzle_flash_smg_FP"), 125 );

	// Modelo del shell a sacar al disparar
	Q_strncpy( m_nEjectBrassEffect, pKeyValuesData->GetString("EjectBrassEffect", "weapon_shell_casing_rifle"), 125 );

	// Lugar donde se vera el arma inactiva
	Q_strncpy( m_nAddonAttachment, pKeyValuesData->GetString("AddonAttachment", "primary"), 35 );

    // FOV
    m_flWeaponFOV = pKeyValuesData->GetFloat( "FOV", 90.0f );

    // Distancia máxima
    m_flMaxDistance = pKeyValuesData->GetFloat( "Distance", 1024.0f );

    // Distancia maxima ideal para disparar (para un bot)
    m_flIdealDistance = pKeyValuesData->GetFloat( "IdealDistance", 512.0f );

	// Número máximo de paredes/objetos que se pueden penetrar
	m_iPenetrationNumLayers = pKeyValuesData->GetInt( "PenetrationNumLayers", 2 );

    // Distancia maxima de penetración
	// Las paredes/objetos más gruesos detendran la bala
    m_flPenetrationMaxDistance = pKeyValuesData->GetFloat( "PenetrationMaxDistance", 20.0f );
}