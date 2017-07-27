//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "scp_player_features.h"

#include "in_buttons.h"
#include "physics_prop_ragdoll.h"
#include "util_shared.h"

#include "players_manager.h"

#include "nav.h"
#include "nav_area.h"

#include "scp_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
// Comandos
//================================================================================

DECLARE_DEBUG_COMMAND( sv_scp173_debug, "1", "" )

//================================================================================
//================================================================================
void C173Behavior::Init()
{
    m_bDangerZone = false;
    m_bLocked = false;
}

//================================================================================
//================================================================================
void C173Behavior::OnUpdate()
{
    // Verificamos por peligro
    CheckDanger();

    // Aplicamos las restricciones de movimiento
    ApplyDanger();
}

//================================================================================
// Verifica si hay algún jugador se encuentra en el PVS de SCP-173 o lo estan
// mirando directamente.
//================================================================================
void C173Behavior::CheckDanger()
{
    m_bDangerZone = false;
    m_bLocked = false;

    // Obtenemos los jugadores en el PVS de SCP-173
    CPVSFilter filter( GetAbsOrigin() );

    for ( int i = 0; i < filter.GetRecipientCount(); ++i )
    {
        CSCP_Player *pPlayer = ToScpPlayer( UTIL_PlayerByIndex( filter.GetRecipientIndex( i ) ) );

        // Jugador inválido
        if ( !pPlayer || !pPlayer->IsActive() )
            continue;

        // Es otro SCP
        if ( pPlayer->IsMonster() )
            continue;

        // Zona de peligro
        m_bDangerZone = true;
        break;
    }

    for ( int it = 0; it <= gpGlobals->maxClients; ++it )
    {
        CSCP_Player *pPlayer = ToScpPlayer( UTIL_PlayerByIndex( it ) );

        // Jugador inválido
        if ( !pPlayer || !pPlayer->IsActive() )
            continue;

        // Tiene NOTARGET activo, lo ignoramos
        if ( (pPlayer->GetFlags() & FL_NOTARGET) )
            continue;

        // Es otro SCP
        if ( pPlayer->IsMonster() )
            continue;

        // SCP-173 no es visible para este jugador
        if ( !pPlayer->FEyesVisible( GetPlayer(), MASK_BLOCKLOS ) )
            continue;

        // TODO: Checar visibilidad por func_reflective_glass

        // ¡Nos estan viendo!
        m_bLocked = true;
        break;
    }
}

//================================================================================
// Aplica las restricciones de movimiento dependiendo del peligro
//================================================================================
void C173Behavior::ApplyDanger()
{
    CSCP_Player *pPlayer = ToScpPlayer( GetPlayer() );

    if ( !m_bDangerZone )
    {
        pPlayer->EnableMovement();
        pPlayer->EnableAiming();
        return;
    }    

    // Desactivamos el movimiento de SCP-173
    pPlayer->DisableMovement();

    // Alguién nos esta viendo directamente
    // desactivamos el movimiento de nuestra cámara
    if ( m_bLocked )
        pPlayer->DisableAiming();
    else
        pPlayer->EnableAiming();

    // Estamos en zona de peligro, pero nadie nos puede ver
    if ( m_bDangerZone && !m_bLocked )
    {
        // Obtenemos el vector que indica enfrente de mi
        Vector forward, start, end;
        pPlayer->EyeVectors( &forward );

        // Una línea hacia enfrente
        start = pPlayer->EyePosition();
        end = start + forward * 300.0f;

        trace_t tr;
        UTIL_TraceLine( start, end, MASK_PLAYERSOLID, pPlayer, COLLISION_GROUP_NONE, &tr );

        // Impacto inválido
        if ( tr.fraction == 1.0f )
        {
            //NDebugOverlay::Line( tr.startpos, tr.endpos, 255, 0, 0, true, 0.1f );
            return;
        }

        // Evitamos quedarnos atrapados en el suelo
        tr.endpos += 10.0f;

        trace_t hulltr;
        UTIL_TraceHull( tr.endpos, tr.endpos, NAI_Hull::Mins( pPlayer->GetHullType() ), NAI_Hull::Maxs( pPlayer->GetHullType() ), MASK_PLAYERSOLID, pPlayer, COLLISION_GROUP_NONE, &hulltr );

        // El jugador no cabe en esta posición
        if ( hulltr.startsolid || hulltr.fraction < 1.0 )
        {
            //NDebugOverlay::Line( tr.startpos, tr.endpos, 255, 0, 0, true, 0.1f );
            return;
        }

        if ( sv_scp173_debug.GetBool() )
            NDebugOverlay::VertArrow( tr.endpos + Vector( 0, 0, 15.0f ), tr.endpos, 15.0f / 4.0f, 0, 255, 0, 10.0f, true, 0.1f );

        // El jugador quiere teletransportarse a esta ubicación
        if ( pPlayer->IsButtonPressed( IN_ATTACK ) )
        {
            UTIL_ScreenFade( pPlayer, { 0,0,0,150 }, 0.5f, 0.1f, FFADE_IN );
            pPlayer->Teleport( &tr.endpos, NULL, NULL );

            // 
            pPlayer->EmitSound("SCP173.Rattle");
        }
    }
}
