//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "c_projectedlight.h"

#include "dlight.h"
#include "iefx.h"
#include "iviewrender.h"
#include "view.h"
#include "engine/ivdebugoverlay.h"
#include "tier0/vprof.h"
#include "tier1/KeyValues.h"
#include "toolframework_client.h"

#include "in_shareddefs.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
// Comandos
//================================================================================

DECLARE_COMMAND( r_projectedtexture_filter, "0.5", "", FCVAR_ARCHIVE )

DECLARE_CHEAT_COMMAND( r_projectedtexture_visualizetrace, "0", "" )
DECLARE_CHEAT_COMMAND( r_projectedtexture_ladderdist, "40", "" )
DECLARE_CHEAT_COMMAND( r_projectedtexture_lockposition, "0", "" )
DECLARE_CHEAT_COMMAND( r_projectedtexture_shadowatten, "0.35", "" )

DECLARE_CHEAT_COMMAND( r_projectedtexture_muzzleflashfov, "120", "" )

DECLARE_CHEAT_COMMAND( r_projectedtexture_nearoffsetscale, "1.0", "" )
DECLARE_CHEAT_COMMAND( r_projectedtexture_tracedistcutoff, "128", "" )
DECLARE_CHEAT_COMMAND( r_projectedtexture_backtraceoffset, "0.4", "" )

//================================================================================
// Constructor
//================================================================================
CInternalLight::CInternalLight( int index, const char *pTextureName )
{
    m_nLightHandle = CLIENTSHADOW_INVALID_HANDLE;
    m_bIsOn = false;
    m_iEntIndex = index;
    m_flCurrentPullBackDist = 1.0f;
    m_flDie.Invalidate();

    // Iniciamos la textura de la luz
    if ( pTextureName )
        UpdateTexture( pTextureName );
}

//================================================================================
// Destructor
//================================================================================
CInternalLight::~CInternalLight()
{
    TurnOff();
}

//================================================================================
// Enciende la luz
//================================================================================
void CInternalLight::TurnOn()
{
    // Ya esta encendida
    if ( IsOn() )
        return;

    m_bIsOn = true;
    m_flCurrentPullBackDist = 1.0f;
}

//================================================================================
// Apaga y destruye la luz
//================================================================================
void CInternalLight::TurnOff()
{
    // No esta encendida
    if ( !IsOn() )
        return;

    m_bIsOn = false;

    // Destruimos la luz
    if ( m_nLightHandle != CLIENTSHADOW_INVALID_HANDLE ) {
        g_pClientShadowMgr->DestroyFlashlight( m_nLightHandle );
        m_nLightHandle = CLIENTSHADOW_INVALID_HANDLE;
    }

#ifndef NO_TOOLFRAMEWORK
    if ( clienttools->IsInRecordingMode() ) {
        KeyValues *msg = new KeyValues( "FlashlightState" );
        msg->SetFloat( "time", gpGlobals->curtime );
        msg->SetInt( "entindex", m_iEntIndex );
        msg->SetInt( "flashlightHandle", m_nLightHandle );
        msg->SetPtr( "flashlightState", NULL );
        ToolFramework_PostToolMessage( HTOOLHANDLE_INVALID, msg );
        msg->deleteThis();
    }
#endif
}

//================================================================================
// Pensamiento
//================================================================================
void CInternalLight::Think()
{
    // Debemos apagarnos
    if ( m_flDie.HasStarted() && m_flDie.IsElapsed() ) {
        TurnOff();
        m_flDie.Invalidate();
    }
}

//================================================================================
// Pensamiento
//================================================================================
void CInternalLight::Update( const Vector &vecPos, const Vector &vecForward, const Vector &vecRight, const Vector &vecUp )
{
    VPROF_BUDGET( __FUNCTION__, VPROF_BUDGETGROUP_SHADOW_DEPTH_TEXTURING );

    // No esta encendida
    if ( !IsOn() )
        return;

    FlashlightState_t state;

    // Actualizamos el estado
    if ( !UpdateState( state, vecPos, vecForward, vecRight, vecUp ) )
        return;

    // Actualizamos la proyección de la luz
    UpdateLightProjection( state );

#ifndef NO_TOOLFRAMEWORK
    if ( clienttools->IsInRecordingMode() ) {
        KeyValues *msg = new KeyValues( "FlashlightState" );
        msg->SetFloat( "time", gpGlobals->curtime );
        msg->SetInt( "entindex", m_iEntIndex );
        msg->SetInt( "flashlightHandle", m_nLightHandle );
        msg->SetPtr( "flashlightState", &state );
        ToolFramework_PostToolMessage( HTOOLHANDLE_INVALID, msg );
        msg->deleteThis();
    }
#endif
}

//================================================================================
// Crea o actualiza la textura que se usara para proyectar la luz
//================================================================================
void CInternalLight::UpdateTexture( const char* pTextureName )
{
    static const char *pEmptyString = "";

    if ( pTextureName == NULL )
        pTextureName = pEmptyString;

    if ( !m_nLightTexture.IsValid() || V_stricmp( m_textureName, pTextureName ) != 0 ) {
        if ( pTextureName == pEmptyString ) {
            m_nLightTexture.Init( "effects/flashlight001", TEXTURE_GROUP_OTHER, true );
        }
        else {
            m_nLightTexture.Init( pTextureName, TEXTURE_GROUP_OTHER, true );
        }

        V_strncpy( m_textureName, pTextureName, sizeof( m_textureName ) );
    }
}

//===============================================================================
// Crea o actauliza la proyección con la información proporcionada.
//===============================================================================
void CInternalLight::UpdateLightProjection( FlashlightState_t &state )
{
    // Creamos la luz
    if ( m_nLightHandle == CLIENTSHADOW_INVALID_HANDLE ) {
        m_nLightHandle = g_pClientShadowMgr->CreateFlashlight( state );
    }
    else {
        if ( !r_projectedtexture_lockposition.GetBool() ) {
            g_pClientShadowMgr->UpdateFlashlightState( m_nLightHandle, state );
        }
    }

    g_pClientShadowMgr->UpdateProjectedTexture( m_nLightHandle, true );
}

//===============================================================================
// Actualiza el estado (la configuración) de la proyección
//===============================================================================
bool CInternalLight::UpdateState( FlashlightState_t& state, const Vector &vecPos, const Vector &vecForward, const Vector &vecRight, const Vector &vecUp )
{
    VPROF_BUDGET( __FUNCTION__, VPROF_BUDGETGROUP_SHADOW_DEPTH_TEXTURING );

    // No esta encendida
    if ( !IsOn() )
        return false;

    // Calculamos la posición y la orientación
    if ( !m_nState.m_bGlobalLight ) {
        if ( !ComputeLightPosAndOrientation( vecPos, vecForward, vecRight, vecUp, state.m_vecLightOrigin, state.m_quatOrientation ) )
            return false;
    }

    //
    state.m_fQuadraticAtten = m_nState.m_fQuadraticAtten;
    state.m_fConstantAtten = m_nState.m_fConstantAtten;
    state.m_fLinearAtten = m_nState.m_fLinearAtten;

    // Color de la luz
    state.m_Color[0] = m_nState.m_Color[0];
    state.m_Color[1] = m_nState.m_Color[1];
    state.m_Color[2] = m_nState.m_Color[2];
    state.m_Color[3] = m_nState.m_Color[3];
    state.m_fBrightnessScale = m_nState.m_fBrightnessScale;

    // Distancia y FOV
    state.m_NearZ = m_nState.m_NearZ + r_projectedtexture_nearoffsetscale.GetFloat() * m_flCurrentPullBackDist;
    state.m_FarZ = m_nState.m_FarZ;
    state.m_FarZAtten = m_nState.m_FarZAtten;
    state.m_fHorizontalFOVDegrees = m_nState.m_fHorizontalFOVDegrees;
    state.m_fVerticalFOVDegrees = m_nState.m_fVerticalFOVDegrees;

    // Textura proyectada
    state.m_pSpotlightTexture = m_nLightTexture;
    state.m_pProjectedMaterial = NULL;
    state.m_nSpotlightTextureFrame = 0;

    // Luz Global
    state.m_bGlobalLight = m_nState.m_bGlobalLight;

    if ( state.m_bGlobalLight ) {
        state.m_vecLightOrigin = vecPos;
        state.m_quatOrientation = m_nState.m_quatOrientation;
    }

    // Ortho
    state.m_bOrtho = m_nState.m_bOrtho;

    if ( state.m_bOrtho ) {
        state.m_fOrthoLeft = m_nState.m_fOrthoLeft;
        state.m_fOrthoTop = m_nState.m_fOrthoTop;
        state.m_fOrthoRight = m_nState.m_fOrthoRight;
        state.m_fOrthoBottom = m_nState.m_fOrthoBottom;
    }

    // Propiedades de las sombras generadas
    state.m_bEnableShadows = m_nState.m_bEnableShadows;
    state.m_flShadowSlopeScaleDepthBias = g_pMaterialSystemHardwareConfig->GetShadowSlopeScaleDepthBias();
    state.m_flShadowDepthBias = g_pMaterialSystemHardwareConfig->GetShadowDepthBias();

    // Calidad de las sombras
    if ( state.m_bEnableShadows ) {
        state.m_flShadowAtten = r_projectedtexture_shadowatten.GetFloat();
        state.m_bShadowHighRes = m_nState.m_bShadowHighRes;
        state.m_nShadowQuality = m_nState.m_nShadowQuality;
        state.m_flShadowFilterSize = (m_nState.m_flShadowFilterSize > 0) ? m_nState.m_flShadowFilterSize : r_projectedtexture_filter.GetFloat();

        ConVarRef r_flashlightdepthreshigh( "r_flashlightdepthreshigh" );
        ConVarRef r_flashlightdepthres( "r_flashlightdepthres" );

        if ( state.m_bShadowHighRes ) {
            state.m_flShadowMapResolution = r_flashlightdepthreshigh.GetFloat();
        }
        else {
            state.m_flShadowMapResolution = r_flashlightdepthres.GetFloat();
        }
    }

    /*state.m_flNoiseStrength = 0.8f;
    state.m_nNumPlanes = 64;
    state.m_flPlaneOffset = 0.0f;
    state.m_bVolumetric = true;
    state.m_flVolumetricIntensity = 1.0f;*/

    return true;
}

//===============================================================================
//===============================================================================
bool CInternalLight::ComputeLightPosAndOrientation( const Vector &vecPos, const Vector &vecForward, const Vector &vecRight, const Vector &vecUp, Vector& vecFinalPos, Quaternion& quatOrientation )
{
    vecFinalPos = vecPos;
    BasisToQuaternion( vecForward, vecRight, vecUp, quatOrientation );
    return true;

    const float flEpsilon = 0.1f;            // Offset flashlight position along vecUp
    float flDistCutoff = r_projectedtexture_tracedistcutoff.GetFloat();
    const float flDistDrag = 0.2;
    bool bDebugVis = r_projectedtexture_visualizetrace.GetBool();

    C_BasePlayer *pPlayer = UTIL_PlayerByIndex( m_iEntIndex );

    if ( !pPlayer ) {
        pPlayer = C_BasePlayer::GetLocalPlayer();

        if ( !pPlayer )
            return false;
    }

    // We will lock some of the flashlight params if player is on a ladder, to prevent oscillations due to the trace-rays
    bool bPlayerOnLadder = (pPlayer->GetMoveType() == MOVETYPE_LADDER);

    CTraceFilterSkipPlayerAndViewModel traceFilter( pPlayer, true );

    //    Vector vOrigin = vecPos + r_flashlightoffsety.GetFloat() * vecUp;
    Vector vecOffset;
    pPlayer->GetFlashlightOffset( vecForward, vecRight, vecUp, &vecOffset );
    Vector vOrigin = vecPos + vecOffset;

    // Not on ladder...trace a hull
    if ( !bPlayerOnLadder ) {
        Vector vecPlayerEyePos = pPlayer->GetRenderOrigin() + pPlayer->GetViewOffset();

        trace_t pmOriginTrace;
        UTIL_TraceHull( vecPlayerEyePos, vOrigin, Vector( -2, -2, -2 ), Vector( 2, 2, 2 ), (MASK_SOLID & ~(CONTENTS_HITBOX)) | CONTENTS_WINDOW | CONTENTS_GRATE, &traceFilter, &pmOriginTrace );//1

        if ( bDebugVis ) {
            debugoverlay->AddBoxOverlay( pmOriginTrace.endpos, Vector( -2, -2, -2 ), Vector( 2, 2, 2 ), QAngle( 0, 0, 0 ), 0, 255, 0, 16, 0 );

            if ( pmOriginTrace.DidHit() || pmOriginTrace.startsolid ) {
                debugoverlay->AddLineOverlay( pmOriginTrace.startpos, pmOriginTrace.endpos, 255, 128, 128, true, 0 );
            }
            else {
                debugoverlay->AddLineOverlay( pmOriginTrace.startpos, pmOriginTrace.endpos, 255, 0, 0, true, 0 );
            }
        }

        if ( pmOriginTrace.DidHit() || pmOriginTrace.startsolid ) {
            vOrigin = pmOriginTrace.endpos;
        }
        else {
            if ( pPlayer->m_vecFlashlightOrigin != vecPlayerEyePos ) {
                vOrigin = vecPos;
            }
        }
    }
    else // on ladder...skip the above hull trace
    {
        vOrigin = vecPos;
    }

    // Now do a trace along the flashlight direction to ensure there is nothing within range to pull back from
    int iMask = MASK_OPAQUE_AND_NPCS;
    iMask &= ~CONTENTS_HITBOX;
    iMask |= CONTENTS_WINDOW | CONTENTS_GRATE | CONTENTS_IGNORE_NODRAW_OPAQUE;

    Vector vTarget = vOrigin + vecForward * m_nState.m_FarZ;

    // Work with these local copies of the basis for the rest of the function
    Vector vDir = vTarget - vOrigin;
    Vector vRight = vecRight;
    Vector vUp = vecUp;
    VectorNormalize( vDir );
    VectorNormalize( vRight );
    VectorNormalize( vUp );

    // Orthonormalize the basis, since the flashlight texture projection will require this later...
    vUp -= DotProduct( vDir, vUp ) * vDir;
    VectorNormalize( vUp );
    vRight -= DotProduct( vDir, vRight ) * vDir;
    VectorNormalize( vRight );
    vRight -= DotProduct( vUp, vRight ) * vUp;
    VectorNormalize( vRight );

    AssertFloatEquals( DotProduct( vDir, vRight ), 0.0f, 1e-3 );
    AssertFloatEquals( DotProduct( vDir, vUp ), 0.0f, 1e-3 );
    AssertFloatEquals( DotProduct( vRight, vUp ), 0.0f, 1e-3 );

    trace_t pmDirectionTrace;
    UTIL_TraceHull( vOrigin, vTarget, Vector( -1.5, -1.5, -1.5 ), Vector( 1.5, 1.5, 1.5 ), iMask, &traceFilter, &pmDirectionTrace );//.5

    if ( bDebugVis ) {
        debugoverlay->AddBoxOverlay( pmDirectionTrace.endpos, Vector( -4, -4, -4 ), Vector( 4, 4, 4 ), QAngle( 0, 0, 0 ), 0, 0, 255, 16, 0 );
        debugoverlay->AddLineOverlay( vOrigin, pmDirectionTrace.endpos, 255, 0, 0, false, 0 );
    }

    float flTargetPullBackDist = 0.0f;
    float flDist = (pmDirectionTrace.endpos - vOrigin).Length();

    if ( flDist < flDistCutoff ) {
        // We have an intersection with our cutoff range
        // Determine how far to pull back, then trace to see if we are clear
        float flPullBackDist = bPlayerOnLadder ? r_projectedtexture_ladderdist.GetFloat() : flDistCutoff - flDist;    // Fixed pull-back distance if on ladder

        flTargetPullBackDist = flPullBackDist;

        if ( !bPlayerOnLadder ) {
            trace_t pmBackTrace;

            // start the trace away from the actual trace origin a bit, to avoid getting stuck on small, close "lips"
            UTIL_TraceHull( vOrigin - vDir * (flDistCutoff * r_projectedtexture_backtraceoffset.GetFloat()), vOrigin - vDir * (flPullBackDist - flEpsilon),
                            Vector( -1.5f, -1.5f, -1.5f ), Vector( 1.5f, 1.5f, 1.5f ), iMask, &traceFilter, &pmBackTrace );

            if ( bDebugVis ) {
                debugoverlay->AddLineOverlay( pmBackTrace.startpos, pmBackTrace.endpos, 255, 0, 255, true, 0 );
            }

            if ( pmBackTrace.DidHit() ) {
                // We have an intersection behind us as well, so limit our flTargetPullBackDist
                float flMaxDist = (pmBackTrace.endpos - vOrigin).Length() - flEpsilon;
                flTargetPullBackDist = MIN( flMaxDist, flTargetPullBackDist );
                //m_flCurrentPullBackDist = MIN( flMaxDist, m_flCurrentPullBackDist );    // possible pop
            }
        }
    }

    if ( bDebugVis ) {
        // visualize pullback
        debugoverlay->AddBoxOverlay( vOrigin - vDir * m_flCurrentPullBackDist, Vector( -2, -2, -2 ), Vector( 2, 2, 2 ), QAngle( 0, 0, 0 ), 255, 255, 0, 16, 0 );
        debugoverlay->AddBoxOverlay( vOrigin - vDir * flTargetPullBackDist, Vector( -1, -1, -1 ), Vector( 1, 1, 1 ), QAngle( 0, 0, 0 ), 128, 128, 0, 16, 0 );
    }

    m_flCurrentPullBackDist = Lerp( flDistDrag, m_flCurrentPullBackDist, flTargetPullBackDist );
    m_flCurrentPullBackDist = MIN( m_flCurrentPullBackDist, flDistCutoff );    // clamp to max pullback dist
    vOrigin = vOrigin - vDir * m_flCurrentPullBackDist;

    vecFinalPos = vOrigin;
    BasisToQuaternion( vDir, vRight, vUp, quatOrientation );

    return true;
}

//===============================================================================
//===============================================================================
void CInternalLight::UpdateLightTopDown( const Vector &vecPos, const Vector &vecForward, const Vector &vecRight, const Vector &vecUp )
{
    VPROF_BUDGET( __FUNCTION__, VPROF_BUDGETGROUP_SHADOW_DEPTH_TEXTURING );

    FlashlightState_t state;

    state.m_vecLightOrigin = vecPos;

    Vector vTarget = vecPos + vecForward * m_nState.m_FarZ;

    // Work with these local copies of the basis for the rest of the function
    Vector vDir = vTarget - vecPos;
    Vector vRight = vecRight;
    Vector vUp = vecUp;
    VectorNormalize( vDir );
    VectorNormalize( vRight );
    VectorNormalize( vUp );

    // Orthonormalize the basis, since the flashlight texture projection will require this later...
    vUp -= DotProduct( vDir, vUp ) * vDir;
    VectorNormalize( vUp );
    vRight -= DotProduct( vDir, vRight ) * vDir;
    VectorNormalize( vRight );
    vRight -= DotProduct( vUp, vRight ) * vUp;
    VectorNormalize( vRight );

    AssertFloatEquals( DotProduct( vDir, vRight ), 0.0f, 1e-3 );
    AssertFloatEquals( DotProduct( vDir, vUp ), 0.0f, 1e-3 );
    AssertFloatEquals( DotProduct( vRight, vUp ), 0.0f, 1e-3 );

    BasisToQuaternion( vDir, vRight, vUp, state.m_quatOrientation );

    //
    state.m_fQuadraticAtten = m_nState.m_fQuadraticAtten;
    state.m_fConstantAtten = m_nState.m_fConstantAtten;

    // Color de la luz
    state.m_Color[0] = m_nState.m_Color[0];
    state.m_Color[1] = m_nState.m_Color[1];
    state.m_Color[2] = m_nState.m_Color[2];
    state.m_Color[3] = m_nState.m_Color[3];

    // Distancia y FOV
    state.m_NearZ = m_nState.m_NearZ + r_projectedtexture_nearoffsetscale.GetFloat() * m_flCurrentPullBackDist;
    state.m_FarZ = m_nState.m_FarZ;
    state.m_FarZAtten = m_nState.m_FarZAtten;
    state.m_fHorizontalFOVDegrees = m_nState.m_fHorizontalFOVDegrees;
    state.m_fVerticalFOVDegrees = m_nState.m_fVerticalFOVDegrees;

    state.m_pSpotlightTexture = m_nLightTexture;
    state.m_nSpotlightTextureFrame = 0;
    state.m_fLinearAtten = m_nState.m_fLinearAtten;

    // Propiedades de las sombras generadas
    state.m_bShadowHighRes = m_nState.m_bShadowHighRes;
    state.m_nShadowQuality = m_nState.m_nShadowQuality;
    state.m_flShadowFilterSize = r_projectedtexture_filter.GetFloat();

    // Propiedades de las sombras generadas
    state.m_bEnableShadows = m_nState.m_bEnableShadows;
    state.m_flShadowAtten = r_projectedtexture_shadowatten.GetFloat();
    state.m_flShadowSlopeScaleDepthBias = g_pMaterialSystemHardwareConfig->GetShadowSlopeScaleDepthBias();
    state.m_flShadowDepthBias = g_pMaterialSystemHardwareConfig->GetShadowDepthBias();

    // 
    UpdateLightProjection( state );

#ifndef NO_TOOLFRAMEWORK
    if ( clienttools->IsInRecordingMode() ) {
        KeyValues *msg = new KeyValues( "FlashlightState" );
        msg->SetFloat( "time", gpGlobals->curtime );
        msg->SetInt( "entindex", m_iEntIndex );
        msg->SetInt( "flashlightHandle", m_nLightHandle );
        msg->SetPtr( "flashlightState", &state );
        ToolFramework_PostToolMessage( HTOOLHANDLE_INVALID, msg );
        msg->deleteThis();
    }
#endif
}


CProjectedLight::CProjectedLight( int index, const char *pTextureName )
{
    m_Lights.EnsureCapacity( MAX_SPLITSCREEN_PLAYERS );

    for ( int hh = 0; hh < MAX_SPLITSCREEN_PLAYERS; ++hh ) {
        m_Lights.AddToTail( new CInternalLight( index, pTextureName ) );
    }

    m_bIsOn = false;
}

CProjectedLight::~CProjectedLight()
{
    m_Lights.PurgeAndDeleteElements();
}

void CProjectedLight::TurnOn()
{
    //ASSERT_LOCAL_PLAYER_RESOLVABLE();
    //m_Lights.Element( GET_ACTIVE_SPLITSCREEN_SLOT() )->TurnOn();

    for ( int hh = 0; hh < MAX_SPLITSCREEN_PLAYERS; ++hh ) {
        m_Lights.Element(hh)->TurnOn();
    }

    m_bIsOn = true;
}

void CProjectedLight::TurnOff()
{
    //ASSERT_LOCAL_PLAYER_RESOLVABLE();
    //m_Lights.Element( GET_ACTIVE_SPLITSCREEN_SLOT() )->TurnOff();

    for ( int hh = 0; hh < MAX_SPLITSCREEN_PLAYERS; ++hh ) {
        ACTIVE_SPLITSCREEN_PLAYER_GUARD( hh );
        m_Lights.Element( hh )->TurnOff();
    }

    m_bIsOn = false;
}

void CProjectedLight::Think()
{
    //ASSERT_LOCAL_PLAYER_RESOLVABLE();
    //m_Lights.Element( GET_ACTIVE_SPLITSCREEN_SLOT() )->Think();

    for ( int hh = 0; hh < MAX_SPLITSCREEN_PLAYERS; ++hh ) {
        ACTIVE_SPLITSCREEN_PLAYER_GUARD( hh );
        m_Lights.Element( hh )->Think();
    }
}

void CProjectedLight::Update( const Vector & vecPos, const Vector & vecForward, const Vector & vecRight, const Vector & vecUp )
{
    //ASSERT_LOCAL_PLAYER_RESOLVABLE();
    //m_Lights.Element( GET_ACTIVE_SPLITSCREEN_SLOT() )->Update( vecPos, vecForward, vecRight, vecUp );

    for ( int hh = 0; hh < MAX_SPLITSCREEN_PLAYERS; ++hh ) {
        ACTIVE_SPLITSCREEN_PLAYER_GUARD( hh );
        m_Lights.Element( hh )->Update( vecPos, vecForward, vecRight, vecUp );
    }
}

void CProjectedLight::UpdateTexture( const char * pTextureName )
{
    //ASSERT_LOCAL_PLAYER_RESOLVABLE();
    //m_Lights.Element( GET_ACTIVE_SPLITSCREEN_SLOT() )->UpdateTexture( pTextureName );

    for ( int hh = 0; hh < MAX_SPLITSCREEN_PLAYERS; ++hh ) {
        ACTIVE_SPLITSCREEN_PLAYER_GUARD( hh );
        m_Lights.Element( hh )->UpdateTexture( pTextureName );
    }
}

void CProjectedLight::SetDie( float flValue )
{
    //ASSERT_LOCAL_PLAYER_RESOLVABLE();
    //m_Lights.Element( GET_ACTIVE_SPLITSCREEN_SLOT() )->SetDie( flValue );

    for ( int hh = 0; hh < MAX_SPLITSCREEN_PLAYERS; ++hh ) {
        m_Lights.Element( hh )->SetDie( flValue );
    }
}

void CProjectedLight::SetFOV( float flValue )
{
    //ASSERT_LOCAL_PLAYER_RESOLVABLE();
    //m_Lights.Element( GET_ACTIVE_SPLITSCREEN_SLOT() )->SetFOV( flValue );
   
    for ( int hh = 0; hh < MAX_SPLITSCREEN_PLAYERS; ++hh ) {
        m_Lights.Element( hh )->SetFOV( flValue );
    }
}

void CProjectedLight::SetNear( float flValue )
{
    //ASSERT_LOCAL_PLAYER_RESOLVABLE();
    //m_Lights.Element( GET_ACTIVE_SPLITSCREEN_SLOT() )->SetNear( flValue );

    for ( int hh = 0; hh < MAX_SPLITSCREEN_PLAYERS; ++hh ) {
        m_Lights.Element( hh )->SetNear( flValue );
    }
}

void CProjectedLight::SetFar( float flValue )
{
    //ASSERT_LOCAL_PLAYER_RESOLVABLE();
    //m_Lights.Element( GET_ACTIVE_SPLITSCREEN_SLOT() )->SetFar( flValue );

    for ( int hh = 0; hh < MAX_SPLITSCREEN_PLAYERS; ++hh ) {
        m_Lights.Element( hh )->SetFar( flValue );
    }
}

void CProjectedLight::SetConstant( float flValue )
{
    //ASSERT_LOCAL_PLAYER_RESOLVABLE();
    //m_Lights.Element( GET_ACTIVE_SPLITSCREEN_SLOT() )->SetConstant( flValue );

    for ( int hh = 0; hh < MAX_SPLITSCREEN_PLAYERS; ++hh ) {
        m_Lights.Element( hh )->SetConstant( flValue );
    }
}

void CProjectedLight::SetQuadratic( float flValue )
{
    //ASSERT_LOCAL_PLAYER_RESOLVABLE();
    //m_Lights.Element( GET_ACTIVE_SPLITSCREEN_SLOT() )->SetQuadratic( flValue );

    for ( int hh = 0; hh < MAX_SPLITSCREEN_PLAYERS; ++hh ) {
        m_Lights.Element( hh )->SetQuadratic( flValue );
    }
}

void CProjectedLight::SetLinear( float flValue )
{
    //ASSERT_LOCAL_PLAYER_RESOLVABLE();
    //m_Lights.Element( GET_ACTIVE_SPLITSCREEN_SLOT() )->SetLinear( flValue );

    for ( int hh = 0; hh < MAX_SPLITSCREEN_PLAYERS; ++hh ) {
        m_Lights.Element( hh )->SetLinear( flValue );
    }
}

void CProjectedLight::SetColor( byte r, byte g, byte b, byte a )
{
    //ASSERT_LOCAL_PLAYER_RESOLVABLE();
    //m_Lights.Element( GET_ACTIVE_SPLITSCREEN_SLOT() )->SetColor( r, g, b, a );

    for ( int hh = 0; hh < MAX_SPLITSCREEN_PLAYERS; ++hh ) {
        m_Lights.Element( hh )->SetColor( r, g, b, a );
    }
}

void CProjectedLight::SetBrightScale( float flValue )
{
    //ASSERT_LOCAL_PLAYER_RESOLVABLE();
    //m_Lights.Element( GET_ACTIVE_SPLITSCREEN_SLOT() )->SetBrightScale( flValue );

    for ( int hh = 0; hh < MAX_SPLITSCREEN_PLAYERS; ++hh ) {
        m_Lights.Element( hh )->SetBrightScale( flValue );
    }
}

void CProjectedLight::SetIsGlobalLight( bool value, float fov )
{
    //ASSERT_LOCAL_PLAYER_RESOLVABLE();
    //m_Lights.Element( GET_ACTIVE_SPLITSCREEN_SLOT() )->SetIsGlobalLight( value, fov );

    for ( int hh = 0; hh < MAX_SPLITSCREEN_PLAYERS; ++hh ) {
        m_Lights.Element( hh )->SetIsGlobalLight( value, fov );
    }
}

void CProjectedLight::SetShadows( bool bEnabled, bool bHighRes, int shadowQuality, float flFilterSize )
{
    //ASSERT_LOCAL_PLAYER_RESOLVABLE();
    //m_Lights.Element( GET_ACTIVE_SPLITSCREEN_SLOT() )->SetShadows( bEnabled, bHighRes, shadowQuality, flFilterSize );

    for ( int hh = 0; hh < MAX_SPLITSCREEN_PLAYERS; ++hh ) {
        m_Lights.Element( hh )->SetShadows( bEnabled, bHighRes, shadowQuality, flFilterSize );
    }
}

const char *CProjectedLight::GetFlashlightTextureName() const
{
    return m_Lights.Element(0)->GetFlashlightTextureName();
}

int CProjectedLight::GetEntIndex() const
{
    return m_Lights.Element( 0 )->GetEntIndex();
}
