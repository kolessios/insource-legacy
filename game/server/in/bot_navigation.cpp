//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"

#include "bot.h"
#include "in_utils.h"

#include "players_system.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar bot_navigation_wiggle;
extern ConVar bot_debug_jump;

//================================================================================
//================================================================================
bool CBot::HasDestination()
{
	if ( !Navigation() )
		return false;

	return Navigation()->HasDestination();
}

bool CBot::IsNavigationDisabled() 
{
	if ( !Navigation() )
		return true;

	return Navigation()->IsDisabled();
}

void CBot::SetNavigationDisabled( bool value ) 
{
	if ( !Navigation() )
		return;

	Navigation()->SetDisabled( value );
}

const Vector &CBot::GetDestination()
{
	if ( !Navigation() )
		return vec3_invalid;

	return Navigation()->GetDestination();
}

const Vector & CBot::GetNextSpot() 
{
	if ( !Navigation() )
		return vec3_invalid;

	return Navigation()->GetNextSpot();
}

float CBot::GetDestinationDistanceLeft() 
{
	if ( !Navigation() )
		return 0.0f;

	return Navigation()->GetDistanceLeft();
}

float CBot::GetDestinationDistanceTolerance() 
{
	if ( !Navigation() )
		return 0.0f;

	return Navigation()->GetDistanceTolerance();
}

int CBot::GetNavigationPriority() 
{
	if ( !Navigation() )
		return PRIORITY_INVALID;

	return Navigation()->GetPriority();
}

//================================================================================
// Devuelve si es necesario actualizar todo el sistema de navegación
//================================================================================
bool CBot::ShouldUpdateNavigation()
{
    if ( HasCondition(BCOND_DEJECTED) )
        return false;

    if ( IsNavigationDisabled() )
        return false;

    if ( GetHost()->IsMovementDisabled() )
        return false;

    return true;
}

extern ConVar bot_navigation_teleport;
extern ConVar bot_navigation_hiddden_teleport;

//================================================================================
// Devuelve si el Bot puede teletransportarse al quedar atascado
//================================================================================
bool CBot::ShouldTeleport( const Vector &vecGoal )
{
    // Solamente si nadie nos esta viendo
    if ( bot_navigation_hiddden_teleport.GetBool() ) {
        if ( ThePlayersSystem->IsVisible( GetHost() ) )
            return false;

        if ( ThePlayersSystem->IsVisible( vecGoal ) )
            return false;
    }

    Vector	vUpBit = GetAbsOrigin();
    vUpBit.z += 1;

    trace_t tr;
    UTIL_TraceHull( vecGoal, vUpBit, GetHost()->WorldAlignMins(), GetHost()->WorldAlignMaxs(),
                  MASK_SOLID, GetHost(), COLLISION_GROUP_NONE, &tr );

    if ( tr.startsolid || (tr.fraction < 1.0) )
        return false;

    return bot_navigation_teleport.GetBool();
}

//================================================================================
// Devuelve si podemos movernos a los lados para desatorarnos
//================================================================================
bool CBot::ShouldWiggle()
{
	if ( !Navigation() )
		return true;

    if ( Navigation()->IsUsingLadder() || Navigation()->IsTraversingLadder() )
        return false;

    if ( !bot_navigation_wiggle.GetBool() )
        return false;

    return true;
}

//================================================================================
//================================================================================
void CBot::SetDestination( const Vector &vecDestination, int priority ) 
{
	if ( !Navigation() )
		return;

	Navigation()->SetDestination( vecDestination, priority );
}

//================================================================================
//================================================================================
void CBot::SetDestination( CBaseEntity *pEntity, int priority ) 
{
	if ( !Navigation() )
		return;

	Navigation()->SetDestination( pEntity, priority );
}

//================================================================================
//================================================================================
void CBot::SetDestination( CNavArea *pArea, int priority ) 
{
	if ( !Navigation() )
		return;

	Navigation()->SetDestination( pArea, priority );
}

//================================================================================
//================================================================================
void CBot::StopNavigation() 
{
	if ( !Navigation() )
		return;

	Navigation()->Stop();
}

//================================================================================
// Devuelve si es necesario correr
//================================================================================
bool CBot::ShouldRun()
{
    if ( !GetHost()->IsInGround() || Navigation()->IsJumping() )
        return false;

    if ( Navigation()->IsUsingLadder() )
        return false;

    if ( m_bNeedRun )
        return true;

    if ( GetActiveScheduleID() == SCHEDULE_NONE ) {
        if ( Navigation() && Navigation()->HasDestination() ) {
            const float range = 130.0f;
            float flDistance = Navigation()->GetDistanceLeft();

            // Estamos demasiado lejos del destino
            if ( flDistance > range )
                return true;
        }
    }

    return false;
}

//================================================================================
//================================================================================
bool CBot::ShouldWalk()
{
    return m_bNeedWalk;
}

//================================================================================
// Devuelve si es necesario agacharse
//================================================================================
bool CBot::ShouldCrouch()
{
    // Si lo combinamos con el salto tenemos un CrouchJump
    if ( Navigation()->IsJumping() )
        return true;

    // Nos obligan
    if ( m_bNeedCrouch )
        return true;

    return false;
}

//================================================================================
// Devuelve si es necesario saltar
//================================================================================
bool CBot::ShouldJump()
{
    VPROF_BUDGET( "ShouldJump", VPROF_BUDGETGROUP_BOTS );

    if ( !Navigation() )
        return false;

    // Ya estamos saltando
    if ( !GetHost()->IsInGround() || Navigation()->IsJumping() )
        return false;

    // Estamos usando una escalera
    if ( Navigation()->IsUsingLadder() )
        return false;

    if ( Navigation()->HasDestination() ) {
        // Altura para verificar que algo nos bloquea
        Vector vecFeetBlocked = Navigation()->GetFeet();
        vecFeetBlocked.z += StepHeight;

        // Altura para verificar que es posible hacer un salto
        Vector vecFeetClear = Navigation()->GetFeet();
        vecFeetClear.z += JumpCrouchHeight + 5.0f;

        // Colocamos temporalmente vecGoal con Z en los pies del jugador
        // de esta forma evitamos que el trace apunte hacia arriba/abajo
        Vector vecNextSpot = Navigation()->GetNextSpot();
        vecNextSpot.z = Navigation()->GetFeet().z;

        // Sacamos el angulo hacia el destino
        Vector vec2Angles = vecNextSpot - Navigation()->GetFeet();
        QAngle angles;
        VectorAngles( vec2Angles, angles );

        // Sacamos el vector que indica "enfrente"
        Vector vecForward;
        AngleVectors( angles, &vecForward );

        // Trazamos dos líneas para verificar que podemos hacer el salto
        trace_t blocked;
        UTIL_TraceLine( vecFeetBlocked, vecFeetBlocked + 30.0f * vecForward, MASK_SOLID, GetHost(), COLLISION_GROUP_NONE, &blocked );

        trace_t clear;
        UTIL_TraceLine( vecFeetClear, vecFeetClear + 30.0f * vecForward, MASK_SOLID, GetHost(), COLLISION_GROUP_NONE, &clear );
        
        if ( bot_debug_jump.GetBool() && ShouldShowDebug() ) {
            NDebugOverlay::Line( vecFeetBlocked, vecFeetBlocked + 30.0f * vecForward, 0, 0, 255, true, 0.1f );
            NDebugOverlay::Line( vecFeetClear, vecFeetClear + 30.0f * vecForward, 0, 0, 255, true, 0.1f );
        }

        // Algo nos esta bloqueando
        if ( blocked.fraction < 1.0 && clear.fraction == 1.0 ) {
            if ( blocked.m_pEnt ) {
                if ( blocked.m_pEnt->IsPlayer() ) {
                    CPlayer *pPlayer = ToInPlayer( blocked.m_pEnt );
                    Assert( pPlayer );

                    // Si esta incapacitada pasamos por encima de el (like a boss)
                    if ( !pPlayer->IsDejected() )
                        return false;
                }
                else if ( blocked.m_pEnt->MyCombatCharacterPointer() ) {
                    return false;
                }
            }

            if ( bot_debug_jump.GetBool() && ShouldShowDebug() ) {
                NDebugOverlay::EntityBounds( GetHost(), 0, 255, 0, 5.0f, 0.1f );
            }

            return true;
        }
    }

    return false;
}

//================================================================================
//================================================================================
bool CBot::IsJumping() const 
{
	if ( !Navigation() )
		return false;

	return Navigation()->IsJumping();
}

//================================================================================
//================================================================================
void CBot::Crouch()
{
    m_bNeedCrouch = true;
}

//================================================================================
//================================================================================
void CBot::StandUp()
{
    m_bNeedCrouch = false;
}

//================================================================================
//================================================================================
void CBot::Run()
{
    m_bNeedRun = true;
    m_bNeedWalk = false;
}

//================================================================================
//================================================================================
void CBot::Walk()
{
    m_bNeedWalk = true;
    m_bNeedRun = false;
}

//================================================================================
//================================================================================
void CBot::NormalWalk()
{
    m_bNeedRun = false;
    m_bNeedWalk = false;
}