//==== Woots 2017. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef BULLET_H
#define BULLET_H

#ifdef _WIN32
#pragma once
#endif

#include "takedamageinfo.h"
#include "weapon_base.h"

//================================================================================
// Representa una bala
//================================================================================
class CBullet : public CMemZeroOnNew
{
public:
	DECLARE_CLASS_NOBASE( CBullet );

	CBullet( const FireBulletsInfo_t bulletsInfo );
	CBullet( const FireBulletsInfo_t bulletsInfo, CBaseEntity *pOwner );
	CBullet( int shot, const FireBulletsInfo_t bulletsInfo, CBaseEntity *pOwner );

	virtual void Init( int shot, const FireBulletsInfo_t bulletsInfo, CBaseEntity *pOwner );
	virtual void Prepare();

	virtual bool Simulate();

	virtual void Fire( int seed );
	virtual void Fire();

	virtual bool HandleFire();
	virtual bool HandleShotImpact( trace_t &tr );
	virtual void HandleEntityImpact( CBaseEntity *pEntity, trace_t &tr );
	virtual bool HandleWaterImpact( ITraceFilter &filter );

	virtual void DoImpactEffect( trace_t &tr );

	virtual bool IsInWorld();
	virtual bool ShouldDrawWaterImpact() { return true; }
	virtual bool ShouldDrawUnderwaterBulletBubbles() { return true; }

	virtual float GetCurrentDistance() { return m_flCurrentDistance; }
	virtual float GetDistanceLeft() { return m_flDistanceLeft; }
	virtual int GetPenetration() { return m_iPenetrations; }
	virtual bool IsRealistic() { return m_bRealistic; }
	virtual float GetSpeed() { return m_flSpeed; }
	virtual const Vector &GetAbsOrigin() { return m_vecOrigin; }

	virtual CBaseEntity *GetOwner() { return m_nOwner.Get(); }
	virtual CBasePlayer *GetPlayerOwner();
	virtual CBaseWeapon *GetWeapon() { return m_nWeapon.Get(); }

protected:
	FireBulletsInfo_t m_BulletsInfo;
	CTakeDamageInfo m_DamageInfo;

	float m_flMaxDistance;
	float m_flMaxPenetrationDistance;
	float m_iMaxPenetrationLayers;

	float m_flCurrentDistance;
	float m_flDistanceLeft;

	int m_iPenetrations;
	int m_iShot;

	bool m_bIgnorePositionUpdate;

	Vector m_vecShotInitial;
	Vector m_vecShotStart;
	Vector m_vecShotEnd;
	Vector m_vecShotDir;

	EHANDLE m_nOwner;
	CHandle<CBaseWeapon> m_nWeapon;

	// Realistic
	bool m_bRealistic;

	Vector m_vecOrigin;
	float m_flSpeed;
};

#endif
