//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#include "cbase.h"
#include "scp_player_features.h"

#include "in_buttons.h"
#include "physics_prop_ragdoll.h"
#include "util_shared.h"

#include "players_system.h"

#include "nav.h"
#include "nav_area.h"

#include "scp_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
// Commands
//================================================================================

DECLARE_DEBUG_CMD(sv_scp173_debug, "1", "")

//================================================================================
//================================================================================
void C173BehaviorComponent::Init()
{
    m_bDangerZone = false;
    m_bLocked = false;
}

//================================================================================
//================================================================================
void C173BehaviorComponent::Update()
{
    // Verificamos por peligro
    CheckDanger();

    // Aplicamos las restricciones de movimiento
    ApplyDanger();
}

//================================================================================
// Check if a human player is in the PVS or they are looking directly at him.
//================================================================================
void C173BehaviorComponent::CheckDanger()
{
    m_bDangerZone = false;
    m_bLocked = false;

    // Players in the PVS
    CPVSFilter filter(GetAbsOrigin());

    for ( int i = 0; i < filter.GetRecipientCount(); ++i ) {
        CSCP_Player *pPlayer = ToScpPlayer(filter.GetRecipientIndex(i));

        if ( !pPlayer || !pPlayer->IsActive() )
            continue;

        if ( pPlayer->IsMonster() )
            continue;

        // Danger Zone!
        m_bDangerZone = true;
        break;
    }

    for ( int it = 1; it <= gpGlobals->maxClients; ++it ) {
        CSCP_Player *pPlayer = ToScpPlayer(it);

        if ( !pPlayer || !pPlayer->IsActive() )
            continue;

        // We ignore objectives in NoTarget
        if ( (pPlayer->GetFlags() & FL_NOTARGET) )
            continue;

        if ( pPlayer->IsMonster() )
            continue;

        // This player can not see the SCP
        if ( !pPlayer->IsAbleToSee(GetPlayer()) )
            continue;

        // TODO: Verify visibility by func_reflective_glass

        // They are watching us!
        m_bLocked = true;
        break;
    }
}

//================================================================================
// Apply movement restrictions depending on the danger
//================================================================================
void C173BehaviorComponent::ApplyDanger()
{
    CSCP_Player *pPlayer = ToScpPlayer(GetPlayer());

    // There are no human players nearby, you can walk freely.
    if ( !m_bDangerZone ) {
        pPlayer->EnableMovement();
        pPlayer->EnableAiming();
        return;
    }

    // You can not move near human players
    pPlayer->DisableMovement();

    // Someone is watching us, we are... frozen...
    if ( m_bLocked ) {
        pPlayer->DisableAiming();
    }
    else {
        pPlayer->EnableAiming();
    }

    // We are in a danger zone but nobody can see us.
    if ( m_bDangerZone && !m_bLocked ) {
        Vector forward, start, end;
        pPlayer->EyeVectors(&forward);

        start = pPlayer->EyePosition();
        end = start + forward * 300.0f;

        trace_t tr;
        UTIL_TraceLine(start, end, MASK_PLAYERSOLID, pPlayer, COLLISION_GROUP_NONE, &tr);

        if ( tr.fraction == 1.0f ) {
            NDebugOverlay::Line( tr.startpos, tr.endpos, 255, 0, 0, true, 0.1f );
            return;
        }

        // Evitamos quedarnos atrapados en el suelo
        tr.endpos += 10.0f;

        trace_t hulltr;
        UTIL_TraceHull(tr.endpos, tr.endpos, NAI_Hull::Mins(pPlayer->GetHullType()), NAI_Hull::Maxs(pPlayer->GetHullType()), MASK_PLAYERSOLID, pPlayer, COLLISION_GROUP_NONE, &hulltr);

        if ( hulltr.startsolid || hulltr.fraction < 1.0 ) {
            NDebugOverlay::Line( tr.startpos, tr.endpos, 255, 0, 0, true, 0.1f );
            return;
        }

        if ( sv_scp173_debug.GetBool() )
            NDebugOverlay::VertArrow(tr.endpos + Vector(0, 0, 15.0f), tr.endpos, 15.0f / 4.0f, 0, 255, 0, 10.0f, true, 0.1f);

        // El jugador quiere teletransportarse a esta ubicación
        if ( pPlayer->IsButtonPressed(IN_ATTACK) ) {
            UTIL_ScreenFade(pPlayer, {0,0,0,150}, 0.5f, 0.1f, FFADE_IN);
            pPlayer->Teleport(&tr.endpos, NULL, NULL);

            // 
            pPlayer->EmitSound("SCP173.Rattle");
        }
    }
}
