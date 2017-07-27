//==== Woots 2017. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef INFO_WEAPON_SPAWN_H
#define INFO_WEAPON_SPAWN_H

#ifdef _WIN32
#pragma once
#endif

#include "weapon_base.h"

//================================================================================
// Spawn Flags
//================================================================================

#define SF_SPAWN_IMMEDIATELY 1
#define SF_HIDE_FROM_PLAYERS 2
#define SF_ONLY_ONE_ACTIVE 4
#define SF_STATIC_SPAWNER 8
#define SF_ADAPTATIVE 16

//================================================================================
// Una entidad para crear un Bot
//================================================================================
class CWeaponSpawn : public CBaseEntity
{
public:
    DECLARE_CLASS( CWeaponSpawn, CBaseEntity );
    DECLARE_DATADESC();

	~CWeaponSpawn();

    virtual int ObjectCaps();

    virtual void Spawn();
	virtual void Think();

	virtual bool IsStatic();
	virtual bool IsAdaptative();
	virtual bool IsDistanceHandled();

	virtual const char *GetWeaponClass();

    virtual bool CanSpawn();
    virtual void SpawnWeapon();

	virtual void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

    // Inputs
    void InputSpawn( inputdata_t &inputdata );
    void InputEnable( inputdata_t &inputdata );
    void InputDisable( inputdata_t &inputdata );
    void InputToggle( inputdata_t &inputdata );

    void InputSetTier( inputdata_t &inputdata );
    void InputSetAmount( inputdata_t &inputdata );
    void InputFill( inputdata_t &inputdata );

protected:
    bool m_bDisabled;

	int m_iTier;
	int m_iAmount;
	int m_iActualAmount;

	int m_iOnlyInSkill;
	CHandle<CBaseWeapon> m_nWeapon;

	const char *m_pWeaponClass;

    // Outputs
    COutputEvent m_OnSpawn;
    COutputEvent m_OnDepleted;
};

#endif // BOT_MAKER_H