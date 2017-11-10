//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
//===========================================================================//

#include "cbase.h"
#include "flashlighteffect.h"



#include "dlight.h"
#include "iefx.h"
#include "iviewrender.h"
#include "view.h"
#include "engine/ivdebugoverlay.h"
#include "tier0/vprof.h"
#include "tier1/KeyValues.h"
#include "toolframework_client.h"

#if defined( _X360 )
extern ConVar r_flashlightdepthres;
#else
extern ConVar r_flashlightdepthres;
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
// Comandos
//================================================================================

DECLARE_CHEAT_CMD( r_flashlight_offsetx, "5", "" )
DECLARE_CHEAT_CMD( r_flashlight_offsety, "-5", "" )
DECLARE_CHEAT_CMD( r_flashlight_offsetz, "0", "" )
DECLARE_CHEAT_CMD( r_flashlight_near, "2", "" )
DECLARE_CHEAT_CMD( r_flashlight_constant, "0", "" )
DECLARE_CHEAT_CMD( r_flashlight_quadratic, "0", "" )
DECLARE_CHEAT_CMD( r_flashlight_ambient, "1", "" )

extern ConVar r_flashlightdepthtexture;

//================================================================================
//================================================================================
/*
CFlashlightEffectManager & FlashlightEffectManager( int32 nSplitscreenPlayerOverride )
{
	static CFlashlightEffectManager s_flashlightEffectManagerArray[ MAX_SPLITSCREEN_PLAYERS ];

	if ( nSplitscreenPlayerOverride != -1 )
	{
		return s_flashlightEffectManagerArray[ nSplitscreenPlayerOverride ];
	}

	ASSERT_LOCAL_PLAYER_RESOLVABLE();
	return s_flashlightEffectManagerArray[ GET_ACTIVE_SPLITSCREEN_SLOT() ];
}
*/


//================================================================================
// Purpose: 
// Input  : nEntIndex - The m_nEntIndex of the client entity that is creating us.
//			vecPos - The position of the light emitter.
//			vecDir - The direction of the light emission.
//================================================================================
CFlashlightEffect::CFlashlightEffect( int nEntIndex, const char *pszTextureName ) : BaseClass( nEntIndex, pszTextureName )
{
    
}


//================================================================================
//================================================================================
CFlashlightEffect::~CFlashlightEffect()
{
	TurnOff();
}

//================================================================================
//================================================================================
void CFlashlightEffect::Init()
{
	//SetOffset( r_flashlight_offsetx.GetFloat(), r_flashlight_offsety.GetFloat(), r_flashlight_offsetz.GetFloat() );
	SetNear( r_flashlight_near.GetFloat() );
	SetConstant( r_flashlight_constant.GetFloat() );
	SetQuadratic( r_flashlight_quadratic.GetFloat() );
    SetLinear( 100.0f );
	SetBrightScale( r_flashlight_ambient.GetFloat() );
    SetColor( 255, 255, 255, 155 );
    SetIsGlobalLight( false );
    SetShadows( true );
}

//================================================================================
//================================================================================
CHeadlightEffect::CHeadlightEffect() : BaseClass( 0 )
{

}

//================================================================================
//================================================================================
CHeadlightEffect::~CHeadlightEffect()
{
	TurnOff();
}

//================================================================================
//================================================================================
void CHeadlightEffect::UpdateLight( const Vector &vecPos, const Vector &vecDir, const Vector &vecRight, const Vector &vecUp, int nDistance )
{
    Assert( false );
    return;
    /*
	if ( IsOn() == false )
		 return;

	FlashlightState_t state;
	Vector basisX, basisY, basisZ;
	basisX = vecDir;
	basisY = vecRight;
	basisZ = vecUp;
	VectorNormalize(basisX);
	VectorNormalize(basisY);
	VectorNormalize(basisZ);

	BasisToQuaternion( basisX, basisY, basisZ, state.m_quatOrientation );
		
	state.m_vecLightOrigin = vecPos;

	
	state.m_fHorizontalFOVDegrees = 45.0f;
	state.m_fVerticalFOVDegrees = 30.0f;
	state.m_fQuadraticAtten = r_flashlightquadratic.GetFloat();
	state.m_fLinearAtten = r_flashlightlinear.GetFloat();
	state.m_fConstantAtten = r_flashlightconstant.GetFloat();
	state.m_Color[0] = 1.0f;
	state.m_Color[1] = 1.0f;
	state.m_Color[2] = 1.0f;
	state.m_Color[3] = r_flashlightambient.GetFloat();
	state.m_NearZ = r_flashlightnear.GetFloat();
	state.m_FarZ = r_flashlightfar.GetFloat();
	state.m_bEnableShadows = true;
	state.m_pSpotlightTexture = m_FlashlightTexture;
	state.m_pProjectedMaterial = NULL;
	state.m_nSpotlightTextureFrame = 0;
	
	
	if( GetFlashlightHandle() == CLIENTSHADOW_INVALID_HANDLE )
	{
		SetFlashlightHandle( g_pClientShadowMgr->CreateFlashlight( state ) );
	}
	else
	{
		g_pClientShadowMgr->UpdateFlashlightState( GetFlashlightHandle(), state );
	}
	
	g_pClientShadowMgr->UpdateProjectedTexture( GetFlashlightHandle(), true );
    */
}

/*
void CFlashlightEffectManager::TurnOnFlashlight( int nEntIndex, const char *pszTextureName, float flFov, float flFarZ, float flLinearAtten )
{
	m_pFlashlightTextureName = pszTextureName;
	m_nFlashlightEntIndex = nEntIndex;
	m_flFov = flFov;
	m_flFarZ = flFarZ;
	m_flLinearAtten = flLinearAtten;
	m_bFlashlightOn = true;

	if ( m_bFlashlightOverride )
	{
		// somebody is overriding the flashlight. We're keeping around the params to restore it later.
		return;
	}

	if ( !m_pFlashlightEffect )
	{
		m_pFlashlightEffect = new CFlashlightEffect( m_nFlashlightEntIndex, pszTextureName, flFov, flFarZ, flLinearAtten );

		if( !m_pFlashlightEffect )
		{
			return;
		}
	}

	m_pFlashlightEffect->TurnOn();
}
*/