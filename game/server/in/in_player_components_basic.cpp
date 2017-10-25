//==== Woots 2017. ===========//

#include "cbase.h"
#include "in_player_components_basic.h"

#include "in_buttons.h"
#include "physics_prop_ragdoll.h"
#include "util_shared.h"

#include "nav.h"
#include "nav_area.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
// Comandos
//================================================================================

// Regeneración de salud
DECLARE_NOTIFY_COMMAND( sv_player_health_regeneration, "0", "" )
DECLARE_NOTIFY_COMMAND( sv_player_health_regeneration_rate, "1", "" )
DECLARE_NOTIFY_COMMAND( sv_player_health_regeneration_amount, "0", "" )

// Aguante
DECLARE_NOTIFY_COMMAND( sv_player_stamina_drain, "10", "" )
DECLARE_CHEAT_COMMAND( sv_player_stamina_infinite, "0", "" )

// Estres
DECLARE_REPLICATED_COMMAND( sv_player_stress_drain, "2", "" )

// Caida
DECLARE_REPLICATED_COMMAND( sv_player_check_fall, "1", "" )
DECLARE_REPLICATED_COMMAND( sv_player_check_fall_tolerance, "500", "" )


//================================================================================
//================================================================================
void CPlayerHealthComponent::Init()
{
    m_RegenerationTimer.Invalidate();
}

//================================================================================
//================================================================================
void CPlayerHealthComponent::Update()
{
    CPlayer *pPlayer = GetPlayer();
    CAttribute *pAttribute = pPlayer->GetAttribute( "health" );

    AssertMsgOnce( pAttribute, "Without health attribute" );

    // Sin atributo de regeneración de salud
    if ( !pAttribute ) {
        return;
    }

    if ( !sv_player_health_regeneration.GetBool() ) {
        return;
    }

    if ( pPlayer->IsDejected() ) {
        pPlayer->AddAttributeModifier( "health_dejected" );
    }
    else {
        // Agregamos manualmente un modificador de regeneración
        AttributeInfo modifier;
        Q_strncpy( modifier.name, "health_skill", sizeof( modifier.name ) );
        modifier.rate = sv_player_health_regeneration_rate.GetFloat();
        modifier.value = sv_player_health_regeneration_amount.GetFloat();
        pAttribute->AddModifier( modifier );
    }

    if ( m_RegenerationTimer.HasStarted() && !m_RegenerationTimer.IsElapsed() )
        return;

    // Nueva salud
    int newHealth = GetHealth() + (int)pAttribute->GetValue();
    newHealth = clamp( newHealth, 0, pPlayer->GetMaxHealth() );

    if ( newHealth > 0 ) {
        pPlayer->SetHealth( newHealth );
    }
    else {
        pPlayer->SetHealth( 1 );
        pPlayer->TakeDamage( CTakeDamageInfo( pPlayer, pPlayer, 1, DMG_GENERIC ) );
    }

    m_RegenerationTimer.Start( pAttribute->GetRate() );
}

//================================================================================
//================================================================================
void CPlayerEffectsComponent::Update()
{
    CPlayer *pPlayer = GetPlayer();

    if ( pPlayer->GetPlayerStatus() == PLAYER_STATUS_DEJECTED ) {
        pPlayer->SnapEyeAnglesZ( 10 );
        return;
    }

    if ( GetHealth() <= 30 ) {
        pPlayer->SnapEyeAnglesZ( 3 );
    }
    else {
        pPlayer->SnapEyeAnglesZ( 0 );
    }
}

//================================================================================
//================================================================================
void CPlayerDejectedComponent::Update()
{
    if ( !GetPlayer()->IsAlive() )
        return;

    UpdateDejected();
    UpdateFall();
}

//================================================================================
//================================================================================
void CPlayerDejectedComponent::UpdateDejected()
{
    CPlayer *pPlayer = GetPlayer();

    if ( pPlayer->IsDejected() ) {
        pPlayer->SetAbsVelocity( Vector( 0, 0, 0 ) );
        pPlayer->AddAttributeModifier( "stress_dejected" );

        // Terminamos debajo del agua...
        /*if ( pPlayer->GetWaterLevel() > WL_Feet ) {
            pPlayer->TakeDamage( CTakeDamageInfo( pPlayer, pPlayer, 999, DMG_DROWN ) );
            return;
        }*/

        // Dejaron de ayudarte compañero..
        //if ( pPlayer->GetHelpProgress() > 0 && pPlayer->m_RaiseHelpTimer.IsGreaterThen( 0.05f ) )
            //pPlayer->OnPlayerStatus( pPlayer->GetPlayerStatus(), pPlayer->GetPlayerStatus() );
    }

    // Estamos cayendo...
    // Solo restauramos todo y la gravedad se ocupara de hacer el daño.
    if ( pPlayer->GetPlayerStatus() == PLAYER_STATUS_FALLING ) {
        if ( pPlayer->IsOnGround() ) {
            pPlayer->SetPlayerStatus( PLAYER_STATUS_NONE );
        }
    }

    // Colgando por nuestra vida
    if ( pPlayer->GetPlayerStatus() == PLAYER_STATUS_CLIMBING ) {
        if ( pPlayer->IsButtonPressed( IN_USE ) ) {
            pPlayer->m_flClimbingHold += 0.2f;
        }

        pPlayer->m_flClimbingHold -= 5 * gpGlobals->frametime;

        if ( pPlayer->m_flClimbingHold <= 0 ) {
            pPlayer->SetPlayerStatus( PLAYER_STATUS_FALLING );
        }
    }
}

//================================================================================
// Verifica si el salto terminará en una gran caida hacia la muerte
// TODO: Resolver varios problemas de detección de borde.
//================================================================================
void CPlayerDejectedComponent::UpdateFall()
{
    CPlayer *pPlayer = GetPlayer();

    if ( !sv_player_check_fall.GetBool() )
        return;

    if ( pPlayer->GetPlayerStatus() == PLAYER_STATUS_CLIMBING || pPlayer->GetPlayerStatus() == PLAYER_STATUS_FALLING )
        return;

    if ( pPlayer->IsOnGround() )
        return;

    if ( pPlayer->IsOnGodMode() || pPlayer->GetMoveType() != MOVETYPE_WALK )
        return;

    if ( !pPlayer->GetLastKnownArea() )
        return;

    trace_t frontTrace;
    trace_t frontDownTrace;
    trace_t downTrace;

    QAngle angles = pPlayer->GetAbsAngles();
    angles.x = 0;

    // Obtenemos los vectores de las distintas posiciones
    Vector vecForward, vecRight, vecUp, vecOrigin;
    AngleVectors( angles, &vecForward, &vecRight, &vecUp );
    //pPlayer->GetVectors( &vecForward, &vecRight, &vecUp );

    // Nuestra ubicación
    vecOrigin = GetAbsOrigin();

    // Trazamos una linea de 40 delante nuestra
    UTIL_TraceLine( vecOrigin, vecOrigin + vecForward * 40, MASK_PLAYERSOLID, pPlayer, COLLISION_GROUP_NONE, &frontTrace );
    //NDebugOverlay::Line( vecOrigin, vecOrigin + vecForward * 40, 255, 0, 0, true, 0.5f );

    // Trazamos una línea de 600 hacia abajo de donde termina la línea delante
    UTIL_TraceLine( frontTrace.endpos, frontTrace.endpos + -vecUp * sv_player_check_fall_tolerance.GetFloat(), MASK_PLAYERSOLID, pPlayer, COLLISION_GROUP_NONE, &frontDownTrace );
    //NDebugOverlay::Line( frontTrace.endpos, frontTrace.endpos + -vecUp * sv_player_check_fall_tolerance.GetFloat(), 0, 255, 0, true, 0.5f );

    // Trazamos una línea de 600 hacia abajo de nosotros
    UTIL_TraceLine( vecOrigin, vecOrigin + -vecUp * sv_player_check_fall_tolerance.GetFloat(), MASK_PLAYERSOLID, pPlayer, COLLISION_GROUP_NONE, &downTrace );
    //NDebugOverlay::Line( vecOrigin, vecOrigin + -vecUp * sv_player_check_fall_tolerance.GetFloat(), 0, 0, 255, true, 0.5f );

    if ( frontDownTrace.startsolid || downTrace.startsolid )
        return;

    // El salto es seguro
    if ( frontDownTrace.fraction != 1.0 || downTrace.fraction != 1.0 )
        return;

    // La última NavArea donde estuvimos
    CNavArea *pArea = pPlayer->GetLastKnownArea();

    //pArea->DrawFilled( 255, 0, 0, 100.0f, 10.0f );

    // Obtenemos el punto más cercano al area
    // el punto donde podemos agarrarnos
    Vector vecCliff;
    pArea->GetClosestPointOnArea( pPlayer->GetAbsOrigin(), &vecCliff );

    if ( !vecCliff.IsValid() )
        return;

    // Cuadro rojo: Punto más cercano
    NDebugOverlay::Box( vecCliff, Vector( -5, -5, -5 ), Vector( 5, 5, 5 ), 255, 0, 0, 150, 10.0f );

    //
    Vector vecDiff = GetAbsOrigin() - vecCliff;
    QAngle angCliff, angToPlayer;
    VectorAngles( vecDiff, angCliff );

    // Obtenemos la dirección hacia la caida
    AngleVectors( angCliff, &vecForward, NULL, &vecUp );
    VectorAngles( -vecForward, angToPlayer );

    // Distancia de separación
    float distance = 1.0f;

    // Entramos en bucle hasta encontrar la distancia de caida
    while ( true ) {
        trace_t hullTrace;
        vecOrigin = vecCliff + distance * vecForward;
        UTIL_TraceHull( vecOrigin, vecOrigin + -vecUp * sv_player_check_fall_tolerance.GetFloat(), NAI_Hull::Mins( GetPlayer()->GetHullType() ), NAI_Hull::Maxs( GetPlayer()->GetHullType() ), MASK_PLAYERSOLID, pPlayer, COLLISION_GROUP_NONE, &hullTrace );

        if ( hullTrace.fraction == 1.0f )
            break;

        distance += 1.0f;
    }

    vecOrigin.z = pArea->GetZ( vecCliff );
    vecOrigin.z -= (HumanHeight * 2);

    //DevMsg( "vecOrigin.z: %.2f - Player.z: %.2f\n", vecOrigin.z, pPlayer->GetAbsOrigin().z );

    // Ubicamos al Jugador justo en la posición de agarre
    NDebugOverlay::Box( vecOrigin, Vector( -5, -5, -5 ), Vector( 5, 5, 5 ), 0, 255, 0, 150, 10.0f );

    pPlayer->Teleport( &vecOrigin, &angToPlayer, NULL );

    //DevMsg("vecOrigin.z: %.2f - Player.z: %.2f\n", vecOrigin.z, pPlayer->GetAbsOrigin().z);

    // Agarrandonos
    pPlayer->SetPlayerStatus( PLAYER_STATUS_CLIMBING );
}