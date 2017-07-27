//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef C_PROJECTEDLIGHT_H
#define C_PROJECTEDLIGHT_H

#ifdef _WIN32
#pragma once
#endif

//================================================================================
//================================================================================
class CTraceFilterSkipPlayerAndViewModel : public CTraceFilter
{
public:
    CTraceFilterSkipPlayerAndViewModel( C_BasePlayer *pPlayer, bool bTracePlayers )
    {
        m_pPlayer = pPlayer;
        m_bSkipPlayers = !bTracePlayers;
        m_pLowerBody = NULL;
    }

    virtual bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
    {
        C_BaseEntity *pEntity = EntityFromEntityHandle( pServerEntity );

        if ( !pEntity )
            return true;

        if ( (ToBaseViewModel( pEntity ) != NULL) ||
            (m_bSkipPlayers && pEntity->IsPlayer()) ||
             pEntity == m_pPlayer ||
             pEntity == m_pLowerBody ||
             pEntity->GetCollisionGroup() == COLLISION_GROUP_DEBRIS ||
             pEntity->GetCollisionGroup() == COLLISION_GROUP_INTERACTIVE_DEBRIS ) {
            return false;
        }

        return true;
    }

private:
    C_BaseEntity *m_pPlayer;
    C_BaseEntity *m_pLowerBody;

    bool m_bSkipPlayers;
};

//================================================================================
// Una luz proyectada capaz de generar sombras en tiempo real
//================================================================================
class CInternalLight
{
public:
    DECLARE_CLASS_NOBASE( CInternalLight );

    CInternalLight( int index, const char *pTextureName );
    ~CInternalLight();

    virtual bool IsOn() { return m_bIsOn; }
    virtual void TurnOn();
    virtual void TurnOff();

    virtual void Think();
    virtual void Update( const Vector &vecPos, const Vector &vecForward, const Vector &vecRight, const Vector &vecUp );

    virtual void UpdateTexture( const char* pTextureName );
    virtual void UpdateLightProjection( FlashlightState_t &state );

    virtual bool UpdateState( FlashlightState_t& state, const Vector &vecPos, const Vector &vecDir, const Vector &vecRight, const Vector &vecUp );
    virtual bool ComputeLightPosAndOrientation( const Vector &vecPos, const Vector &vecDir, const Vector &vecRight, const Vector &vecUp, Vector& vecFinalPos, Quaternion& quatOrientation );

    virtual void UpdateLightTopDown( const Vector &vecPos, const Vector &vecForward, const Vector &vecRight, const Vector &vecUp );

    ClientShadowHandle_t GetFlashlightHandle() { return m_nLightHandle; }
    virtual void SetFlashlightHandle( ClientShadowHandle_t handle ) { m_nLightHandle = handle; }

public:
    virtual void SetDie( float flValue ) { m_flDie.Start( flValue ); }

    virtual void SetFOV( float flValue )
    {
        m_nState.m_fHorizontalFOVDegrees = m_nState.m_fVerticalFOVDegrees = flValue;
    }

    virtual void SetNear( float flValue )
    {
        m_nState.m_NearZ = flValue;
    }

    virtual void SetFar( float flValue )
    {
        m_nState.m_FarZ = m_nState.m_FarZAtten = flValue;
    }

    virtual void SetConstant( float flValue )
    {
        m_nState.m_fConstantAtten = flValue;
    }

    virtual void SetQuadratic( float flValue )
    {
        m_nState.m_fQuadraticAtten = flValue;
    }

    virtual void SetLinear( float flValue )
    {
        m_nState.m_fLinearAtten = flValue;
    }

    virtual void SetColor( byte r, byte g, byte b, byte a )
    {
        float flAlpha = a * (1.0f / 255.0f);

        m_nState.m_Color[0] = r * (1.0f / 255.0f) * flAlpha;
        m_nState.m_Color[1] = g * (1.0f / 255.0f) * flAlpha;
        m_nState.m_Color[2] = b * (1.0f / 255.0f) * flAlpha;
        m_nState.m_Color[3] = 0.0f;
    }

    virtual void SetBrightScale( float flValue )
    {
        m_nState.m_fBrightnessScale = flValue;
    }

    virtual void SetIsGlobalLight( bool value, float fov = 90.0f )
    {
        m_nState.m_bGlobalLight = value;
        m_nState.m_bOrtho = value;

        if ( value ) {
            m_nState.m_fOrthoLeft = m_nState.m_fOrthoTop = -fov;
            m_nState.m_fOrthoRight = m_nState.m_fOrthoBottom = fov;
        }
    }

    virtual void SetShadows( bool bEnabled, bool bHighRes = true, int shadowQuality = 1, float flFilterSize = -1.0f )
    {
        m_nState.m_bEnableShadows = bEnabled;
        m_nState.m_bShadowHighRes = bHighRes;
        m_nState.m_nShadowQuality = shadowQuality;
        m_nState.m_flShadowFilterSize = flFilterSize;
    }

    const char *GetFlashlightTextureName() const
    {
        return m_textureName;
    }

    int GetEntIndex() const
    {
        return m_iEntIndex;
    }

public:
    ClientShadowHandle_t m_nLightHandle;
    CTextureReference m_nLightTexture;

    bool m_bIsOn;
    int m_iEntIndex;

    float m_flCurrentPullBackDist;
    char m_textureName[64];

    FlashlightState_t m_nState;
    CountdownTimer m_flDie;
};

//================================================================================
// Una luz proyectada capaz de generar sombras en tiempo real
// Implementación para funcionamiento en pantallas divididas
//================================================================================
class CProjectedLight
{
    DECLARE_CLASS_NOBASE( CInternalLight );

public:

    CProjectedLight( int index, const char *pTextureName );
    ~CProjectedLight();

    virtual bool IsOn() { return m_bIsOn; }
    virtual void TurnOn();
    virtual void TurnOff();

    virtual void Think();
    virtual void Update( const Vector &vecPos, const Vector &vecForward, const Vector &vecRight, const Vector &vecUp );
    virtual void UpdateTexture( const char* pTextureName );

    virtual void SetDie( float flValue );
    virtual void SetFOV( float flValue ); 
    virtual void SetNear( float flValue );
    virtual void SetFar( float flValue );
    virtual void SetConstant( float flValue );
    virtual void SetQuadratic( float flValue );
    virtual void SetLinear( float flValue );
    virtual void SetColor( byte r, byte g, byte b, byte a );
    virtual void SetBrightScale( float flValue );
    virtual void SetIsGlobalLight( bool value, float fov = 90.0f );
    virtual void SetShadows( bool bEnabled, bool bHighRes = true, int shadowQuality = 1, float flFilterSize = -1.0f );

    const char *GetFlashlightTextureName() const;
    int GetEntIndex() const;

protected:
    bool m_bIsOn;
    CUtlVector<CInternalLight *> m_Lights;
};

#endif