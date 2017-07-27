//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Sunlight shadow control entity.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "c_baseplayer.h"

#include "c_projectedlight.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar r_flashlightdepthres;
extern ConVar r_flashlightdepthreshigh;

//====================================================================
// Comandos
//====================================================================

ConVar cl_globallight_freeze( "cl_globallight_freeze", "0" );
ConVar cl_globallight_xoffset( "cl_globallight_xoffset", "0" );
ConVar cl_globallight_yoffset( "cl_globallight_yoffset", "0" );

ConVar mat_shadow_level( "mat_shadow_level", "3", FCVAR_ARCHIVE );

//====================================================================
// Purpose : Sunlights shadow control entity
//====================================================================
class C_GlobalLight : public C_BaseEntity
{
public:
    DECLARE_CLASS( C_GlobalLight, C_BaseEntity );
    DECLARE_CLIENTCLASS();

    C_GlobalLight();
    ~C_GlobalLight();

    virtual void OnDataChanged( DataUpdateType_t updateType );

    virtual void Spawn();
    virtual bool ShouldDraw();

    virtual void ClientThink();

private:
    int m_iLightID;

    Vector m_shadowDirection;
    bool m_bEnabled;
    char m_TextureName[MAX_PATH];
    color32	m_LightColor;
    Vector m_CurrentLinearFloatLightColor;
    float m_flCurrentLinearFloatLightAlpha;
    float m_flColorTransitionTime;
    float m_flSunDistance;
    float m_flFOV;
    float m_flNearZ;
    float m_flNorthOffset;
    bool m_bEnableShadows;

    bool m_bHighQualityShadows;
    float m_flShadowsFilter;

    CUtlVector<CInternalLight *> m_nLights;
};

//====================================================================
// Información y Red
//====================================================================

IMPLEMENT_CLIENTCLASS_DT( C_GlobalLight, DT_GlobalLight, CGlobalLight )
    //RecvPropInt( RECVINFO( m_iLightID ) ),
    RecvPropVector( RECVINFO( m_shadowDirection ) ),
    RecvPropBool( RECVINFO( m_bEnabled ) ),
    RecvPropString( RECVINFO( m_TextureName ) ),
    RecvPropInt( RECVINFO( m_LightColor ), 0, RecvProxy_Int32ToColor32 ),
    RecvPropFloat( RECVINFO( m_flColorTransitionTime ) ),
    RecvPropFloat( RECVINFO( m_flSunDistance ) ),
    RecvPropFloat( RECVINFO( m_flFOV ) ),
    RecvPropFloat( RECVINFO( m_flNearZ ) ),
    RecvPropFloat( RECVINFO( m_flNorthOffset ) ),
    RecvPropBool( RECVINFO( m_bEnableShadows ) ),
    RecvPropBool( RECVINFO( m_bHighQualityShadows ) ),
    RecvPropFloat( RECVINFO( m_flShadowsFilter ) ),
END_RECV_TABLE()

//====================================================================
// Constructor
//====================================================================
C_GlobalLight::C_GlobalLight()
{
    m_nLights.EnsureCapacity( MAX_SPLITSCREEN_PLAYERS );

    for ( int hh = 0; hh < MAX_SPLITSCREEN_PLAYERS; ++hh ) {
        m_nLights.AddToTail( new CInternalLight( entindex(), NULL ) );
    }

    m_iLightID = 1;
}

//====================================================================
// Destructor
//====================================================================
C_GlobalLight::~C_GlobalLight()
{
    /*for ( int hh = 0; hh < MAX_SPLITSCREEN_PLAYERS; ++hh ) {
        CInternalLight *pLight = m_nLights.Element( hh );

        if ( pLight ) {
            delete pLight;
            m_nLights.Remove( hh );
        }
    }*/

    m_nLights.PurgeAndDeleteElements();
}

//====================================================================
//====================================================================
void C_GlobalLight::OnDataChanged( DataUpdateType_t updateType )
{
    BaseClass::OnDataChanged( updateType );
}

//====================================================================
// Creaci�n
//====================================================================
void C_GlobalLight::Spawn()
{
    BaseClass::Spawn();
    SetNextClientThink( CLIENT_THINK_ALWAYS );
}

//====================================================================
// We don't draw...
//====================================================================
bool C_GlobalLight::ShouldDraw()
{
    return false;
}

//====================================================================
//====================================================================
void C_GlobalLight::ClientThink()
{
    VPROF( "C_GlobalLight::ClientThink" );

    // Congelado
    if ( cl_globallight_freeze.GetBool() )
        return;

    if ( m_bEnabled ) {
        for ( int hh = 0; hh < MAX_SPLITSCREEN_PLAYERS; ++hh ) {
            ACTIVE_SPLITSCREEN_PLAYER_GUARD( hh );

            CInternalLight *pLight = m_nLights.Element( hh );

            if ( !pLight )
                continue;

            if ( !C_BasePlayer::GetLocalPlayer() )
                continue;

            pLight->UpdateTexture( m_TextureName );
            pLight->SetQuadratic( 0.0f );
            pLight->SetLinear( m_flSunDistance * 2.0f );
            pLight->SetConstant( 0.0f );
            pLight->SetFar( m_flSunDistance * 2.0f );
            pLight->SetFOV( 90.0f );
            pLight->SetNear( m_flNearZ );
            pLight->SetIsGlobalLight( true, m_flFOV );
            pLight->SetBrightScale( 1.5f );
            pLight->SetColor( m_LightColor.r, m_LightColor.g, m_LightColor.b, m_LightColor.a );
            pLight->SetShadows( m_bEnableShadows, m_bHighQualityShadows, (int)m_bHighQualityShadows, m_flShadowsFilter );
            pLight->TurnOn();

            Vector vDirection = m_shadowDirection;
            VectorNormalize( vDirection );

            Vector vSunDirection2D = vDirection;
            vSunDirection2D.z = 0.0f;

            Vector vPos;
            QAngle EyeAngles;
            float flZNear, flZFar, flFov;

            C_BasePlayer::GetLocalPlayer()->CalcView( vPos, EyeAngles, flZNear, flZFar, flFov );
            vPos = (vPos + vSunDirection2D * m_flNorthOffset) - vDirection * m_flSunDistance;
            vPos += Vector( cl_globallight_xoffset.GetFloat(), cl_globallight_yoffset.GetFloat(), 0.0f );

            //vPos = C_BasePlayer::GetLocalPlayer()->GetAbsOrigin();
            //vPos = (vPos + vSunDirection2D * m_flNorthOffset) - vDirection * m_flSunDistance;

            QAngle angAngles;
            VectorAngles( vDirection, angAngles );

            Vector vForward, vRight, vUp;
            AngleVectors( angAngles, &vForward, &vRight, &vUp );
            BasisToQuaternion( vForward, vRight, vUp, pLight->m_nState.m_quatOrientation );
            pLight->Update( vPos, vForward, vRight, vUp );
        }
    }
    else {
        for ( int hh = 0; hh < MAX_SPLITSCREEN_PLAYERS; ++hh ) {
            m_nLights.Element( hh )->TurnOff();
        }
    }

    //g_pClientShadowMgr->SetShadowFromWorldLightsEnabled( !bSupressWorldLights );
    BaseClass::ClientThink();
}