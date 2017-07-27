//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef WEAPON_INFO_H
#define WEAPON_INFO_H

#ifdef _WIN32
#pragma once
#endif

#include "weapon_parse.h"
#include "networkvar.h"

//================================================================================
// Información extra de un arma
//================================================================================
class CWeaponInfo : public FileWeaponInfo_t
{
public:
    DECLARE_CLASS_GAMEROOT( CWeaponInfo, FileWeaponInfo_t );

    //CWeaponInfo();
    virtual void Parse( ::KeyValues *, const char * );

public:
    int m_iDamage;
    int m_iBullets;

    float m_flFireRate;

	float m_flVerticalPunch;

    float m_flSpreadPerShot;
	float m_flCrouchSpread;
	float m_flBotSpread;
	float m_flJumpSpread;
	float m_flDejectedSpread;
	float m_flMovingSpread;

	char m_nMuzzleFlashEffect_1stPerson[125];
	char m_nMuzzleFlashEffect_3rdPerson[125];
	char m_nEjectBrassEffect[125];
	char m_nAddonAttachment[35];

    float m_flWeaponFOV;
    float m_flMaxDistance;
    float m_flIdealDistance;

	int m_iPenetrationNumLayers;
    float m_flPenetrationMaxDistance;
};

#endif