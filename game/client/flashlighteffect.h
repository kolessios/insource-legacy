//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef FLASHLIGHTEFFECT_H
#define FLASHLIGHTEFFECT_H

#ifdef _WIN32
#pragma once
#endif

#include "c_projectedlight.h"

struct dlight_t;

//===============================================================================
//===============================================================================
class CFlashlightEffect : public CProjectedLight
{
public:
	DECLARE_CLASS( CFlashlightEffect, CProjectedLight );

	CFlashlightEffect(int nEntIndex, const char *pszTextureName = NULL );
	virtual ~CFlashlightEffect();

	virtual void Init();
};

//================================================================================
//================================================================================
class CHeadlightEffect : public CFlashlightEffect
{
public:
	DECLARE_CLASS( CHeadlightEffect, CFlashlightEffect );

	CHeadlightEffect();
	~CHeadlightEffect();

	virtual void UpdateLight(const Vector &vecPos, const Vector &vecDir, const Vector &vecRight, const Vector &vecUp, int nDistance);
};

/*
class CFlashlightEffectManager
{
private:
	CFlashlightEffect *m_pFlashlightEffect;
	const char *m_pFlashlightTextureName;
	int m_nFlashlightEntIndex;
	float m_flFov;
	float m_flFarZ;
	float m_flLinearAtten;
	int m_nMuzzleFlashFrameCountdown;
	CountdownTimer m_muzzleFlashTimer;
	float m_flMuzzleFlashBrightness;
	bool m_bFlashlightOn;
	int m_nFXComputeFrame;
	bool m_bFlashlightOverride;

public:
	CFlashlightEffectManager() : m_pFlashlightEffect( NULL ), m_pFlashlightTextureName( NULL ), m_nFlashlightEntIndex( -1 ), m_flFov( 0.0f ),
								m_flFarZ( 0.0f ), m_flLinearAtten( 0.0f ), m_nMuzzleFlashFrameCountdown( 0 ), m_flMuzzleFlashBrightness( 1.0f ),
								m_bFlashlightOn( false ), m_nFXComputeFrame( -1 ), m_bFlashlightOverride( false ) {}

	// @Deferred - Biohazard
	void TurnOnFlashlight( int nEntIndex = 0, const char *pszTextureName = NULL, float flFov = 0.0f, float flFarZ = 0.0f, float flLinearAtten = 0.0f );

	void TurnOffFlashlight( bool bForce = false )
	{
		m_pFlashlightTextureName = NULL;
		m_bFlashlightOn = false;

		if ( bForce )
		{
			m_bFlashlightOverride = false;
			m_nMuzzleFlashFrameCountdown = 0;
			m_muzzleFlashTimer.Invalidate();
			delete m_pFlashlightEffect;
			m_pFlashlightEffect = NULL;
			return;
		}

		if ( m_bFlashlightOverride )
		{
			// don't mess with it while it's overridden
			return;
		}

		if( m_nMuzzleFlashFrameCountdown == 0 && m_muzzleFlashTimer.IsElapsed() )
		{
			delete m_pFlashlightEffect;
			m_pFlashlightEffect = NULL;
		}
	}

	bool IsFlashlightOn() const { return m_bFlashlightOn; }

	void UpdateFlashlight( const Vector &vecPos, const Vector &vecDir, const Vector &vecRight, const Vector &vecUp, float flFov, bool castsShadows,
		float flFarZ, float flLinearAtten, const char* pTextureName = NULL )
	{
		if ( m_bFlashlightOverride )
		{
			// don't mess with it while it's overridden
			return;
		}

		bool bMuzzleFlashActive = ( m_nMuzzleFlashFrameCountdown > 0 ) || !m_muzzleFlashTimer.IsElapsed();

		if ( m_pFlashlightEffect )
		{
			m_flFov = flFov;
			m_flFarZ = flFarZ;
			m_flLinearAtten = flLinearAtten;
			m_pFlashlightEffect->UpdateLight( m_nFlashlightEntIndex, vecPos, vecDir, vecRight, vecUp, flFov, flFarZ, flLinearAtten, castsShadows, pTextureName );
			m_pFlashlightEffect->SetMuzzleFlashEnabled( bMuzzleFlashActive, m_flMuzzleFlashBrightness );
		}

		if ( !bMuzzleFlashActive && !m_bFlashlightOn && m_pFlashlightEffect )
		{
			delete m_pFlashlightEffect;
			m_pFlashlightEffect = NULL;
		}

		if ( bMuzzleFlashActive && !m_bFlashlightOn && !m_pFlashlightEffect )
		{
			m_pFlashlightEffect = new CFlashlightEffect( m_nFlashlightEntIndex );
			m_pFlashlightEffect->SetMuzzleFlashEnabled( bMuzzleFlashActive, m_flMuzzleFlashBrightness );
		}

		if ( bMuzzleFlashActive && m_nFXComputeFrame != gpGlobals->framecount )
		{
			m_nFXComputeFrame = gpGlobals->framecount;
			m_nMuzzleFlashFrameCountdown--;
		}
	}

	void SetEntityIndex( int index )
	{
		m_nFlashlightEntIndex = index;
	}

	void TriggerMuzzleFlash()
	{
		m_nMuzzleFlashFrameCountdown = 2;
		m_muzzleFlashTimer.Start( 0.066f );		// show muzzleflash for 2 frames or 66ms, whichever is longer
		m_flMuzzleFlashBrightness = random->RandomFloat( 0.4f, 2.0f );
	}

	const char *GetFlashlightTextureName( void ) const
	{
		return m_pFlashlightTextureName;
	}

	int GetFlashlightEntIndex( void ) const
	{
		return m_nFlashlightEntIndex;
	}

	void EnableFlashlightOverride( bool bEnable )
	{
		m_bFlashlightOverride = bEnable;

		if ( !m_bFlashlightOverride )
		{
			// make sure flashlight is in its original state
			if ( m_bFlashlightOn && m_pFlashlightEffect == NULL )
			{
				TurnOnFlashlight( m_nFlashlightEntIndex, m_pFlashlightTextureName, m_flFov, m_flFarZ, m_flLinearAtten );
			}
			else if ( !m_bFlashlightOn && m_pFlashlightEffect )
			{
				delete m_pFlashlightEffect;
				m_pFlashlightEffect = NULL;
			}
		}
	}

	void UpdateFlashlightOverride(	bool bFlashlightOn, const Vector &vecPos, const Vector &vecDir, const Vector &vecRight, const Vector &vecUp,
									float flFov, bool castsShadows, ITexture *pFlashlightTexture, const Vector &vecBrightness )
	{
		Assert( m_bFlashlightOverride );
		if ( !m_bFlashlightOverride )
		{
			return;
		}

		if ( bFlashlightOn && !m_pFlashlightEffect )
		{
			m_pFlashlightEffect = new CFlashlightEffect( m_nFlashlightEntIndex );			
		}
		else if ( !bFlashlightOn && m_pFlashlightEffect )
		{
			delete m_pFlashlightEffect;
			m_pFlashlightEffect = NULL;
		}

		if( m_pFlashlightEffect )
		{
			m_pFlashlightEffect->UpdateLight( m_nFlashlightEntIndex, vecPos, vecDir, vecRight, vecUp, flFov, castsShadows, pFlashlightTexture, vecBrightness, false );
		}
	}
};

CFlashlightEffectManager & FlashlightEffectManager( int32 nSplitscreenPlayerOverride = -1 );
*/

#endif // FLASHLIGHTEFFECT_H
