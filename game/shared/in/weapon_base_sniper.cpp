//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "weapon_base.h"

#include "in_buttons.h"
#include "in_shareddefs.h"

#include "datacache/imdlcache.h"
#include "tier0/vprof.h"

#ifndef CLIENT_DLL
    #include "in_player.h"
    #include "player_lagcompensation.h"
    #include "soundent.h"
    #include "squad.h"
#else
    #include "fx_impact.h"
    #include "c_in_player.h"
    #include "sdk_input.h"
    #include "cl_animevent.h"
    #include "c_te_legacytempents.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
// Comandos
//================================================================================

//================================================================================
// Información y Red
//================================================================================

IMPLEMENT_NETWORKCLASS_ALIASED( BaseWeaponSniper, DT_BaseWeaponSniper )

BEGIN_NETWORK_TABLE( CBaseWeaponSniper, DT_BaseWeaponSniper )
#ifdef CLIENT_DLL
    RecvPropBool( RECVINFO( m_bInZoom ) ),
#else
    SendPropBool( SENDINFO( m_bInZoom ) ),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CBaseWeaponSniper )
    DEFINE_PRED_FIELD( m_bInZoom, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()
#endif

//================================================================================
//================================================================================
CBaseWeaponSniper::CBaseWeaponSniper()
{
    m_bInZoom = false;
}

//================================================================================
//================================================================================
float CBaseWeaponSniper::GetSpreadPerShot() 
{
    float flSpread = GetWeaponInfo().m_flSpreadPerShot;
    CPlayer *pPlayer = GetPlayerOwner();

    // Nos esta usando un Jugador
    // Cambiamos la distribución dependiendo de ciertas acciones
    if ( pPlayer ) 
    {
        // Agachado
        if ( pPlayer->IsCrouching() ) 
        {
            flSpread *= 0.5f;
        }

        // Saltando
        if ( !pPlayer->IsInGround() ) 
        {
            flSpread *= 1.5f;
        }

        // Incapacitado
        if ( pPlayer->IsDejected() )
        {
            flSpread *= 1.5f;
        }

        // Moviendose
        #ifndef CLIENT_DLL
        if ( pPlayer->IsMoving() )
        {
            flSpread *= 1.3f;
        }
        #endif

        // Zoom
        if ( IsWeaponZoomed() ) 
        {
            flSpread = VECTOR_CONE_1DEGREES.x;
        }
    }

    flSpread = clamp( flSpread, VECTOR_CONE_PRECALCULATED.x, VECTOR_CONE_20DEGREES.x );
    return flSpread;
}

//================================================================================
//================================================================================
void CBaseWeaponSniper::ItemPostFrame() 
{
    BaseClass::ItemPostFrame();

    if ( m_bInZoom && !CanSecondaryAttack() )
        DisableZoom();
}

//================================================================================
//================================================================================
bool CBaseWeaponSniper::CanSecondaryAttack() 
{
    if ( IsReloading() )
        return false;

    return true;
}

//================================================================================
//================================================================================
void CBaseWeaponSniper::SecondaryAttack() 
{
    ToggleZoom();
}

//================================================================================
//================================================================================
void CBaseWeaponSniper::ToggleZoom() 
{
    if ( m_bInZoom )
        DisableZoom();
    else
        EnableZoom();
}

//================================================================================
//================================================================================
void CBaseWeaponSniper::EnableZoom() 
{
    if ( m_bInZoom )
        return;

    CPlayer *pPlayer = GetPlayerOwner();
    Assert( pPlayer );

    if ( pPlayer->SetFOV(this, 20, 0.1f) )
    {
        m_bInZoom = true;
    }
}

//================================================================================
//================================================================================
void CBaseWeaponSniper::DisableZoom() 
{
    if ( !m_bInZoom )
        return;

    CPlayer *pPlayer = GetPlayerOwner();
    Assert( pPlayer );

    if ( pPlayer->SetFOV(this, 0, 0.2f) )
    {
        m_bInZoom = false;
    }
}
