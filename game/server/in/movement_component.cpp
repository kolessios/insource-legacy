//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "bot.h"
#include "bot_manager.h"

#include "in_utils.h"

#include "nav.h"
#include "nav_mesh.h"

#include "in_buttons.h"
#include "basetypes.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
// Comandos
//================================================================================

DECLARE_REPLICATED_COMMAND( bot_navigation_teleport, "1", "Indica si los Bots pueden teletransportarse al quedar atascados." )
DECLARE_REPLICATED_COMMAND( bot_navigation_hiddden_teleport, "1", "Indica si los Bots pueden teletransportarse solo si ningun Jugador los esta mirando" )
DECLARE_REPLICATED_COMMAND( bot_navigation_goal_tolerance, "60", "" )
DECLARE_REPLICATED_COMMAND( bot_navigation_wiggle, "1", "" )

extern ConVar bot_debug;
extern ConVar bot_debug_navigation;

//================================================================================
// El Bot ha aparecido en el mundo
//================================================================================
void CNavigationComponent::OnSpawn()
{
    m_bTraversingLadder = false;
    m_bJumping = false;
    m_bDisabled = false;

    m_vecDestination.Invalidate();
    m_iDestinationPriority = PRIORITY_INVALID;

    m_WiggleDirection = FORWARD;
    m_WiggleTimer.Invalidate();

    m_Path.Invalidate();

    m_Navigation.Reset();
    m_Navigation.SetPath( &m_Path );
    m_Navigation.SetImprov( this );
    m_Navigation.SetFollowPathExactly( false );
}

//================================================================================
// Pensamiento
//================================================================================
void CNavigationComponent::OnUpdate( CBotCmd* &cmd )
{
    VPROF_BUDGET( "CNavigationComponent::OnUpdate", VPROF_BUDGETGROUP_BOTS );

    if ( IsJumping() && GetHost()->IsInGround() )
        m_bJumping = false;

    if ( !HasDestination() )
        return;

    if ( !GetBot()->ShouldUpdateNavigation() )
        return;

    CheckPath();
    ComputePath();

    if ( !HasPath() )
        return;

    if ( GetBot()->ShouldShowDebug() )
        m_Navigation.Debug( bot_debug_navigation.GetBool() );
    else
        m_Navigation.Debug( false );

    m_Navigation.Update( TheBots->GetDeltaT() );

    if ( m_Navigation.IsStuck() && m_Navigation.GetStuckDuration() > 1.0f ) {
        if ( GetBot()->ShouldWiggle() )
            Wiggle();
    }

    float flDistance = GetDistanceLeft();
    float flTolerance = GetDistanceTolerance();

    // Hemos llegado al destino
    if ( !IsTraversingLadder() && !IsJumping() && flDistance <= flTolerance )
        OnMoveToSuccess( GetDestination() );
}

//================================================================================
// Devuelve si será necesario volver a calcular la mejor ruta al destino
//================================================================================
bool CNavigationComponent::ShouldRepath( const Vector &vecGoal )
{
    const float repathRange = 100.0f;

    // El destino ha cambiado, invalidamos la ruta para
    // volver a calcularla
    if ( m_Path.GetEndpoint().DistTo( vecGoal ) > repathRange )
        return true;

    return false;
}

//================================================================================
// Devuelve a que distancia debemos estar del destino para considerlo como "completo"
//================================================================================
float CNavigationComponent::GetDistanceTolerance()
{  
    float flTolerance = bot_navigation_goal_tolerance.GetFloat();

    // Estamos siguiendo a un jugador
    // agregamos más distancia para asegurarnos de no estar muy cerca
    if ( GetBot()->IsFollowingSomeone() && !GetBot()->IsFollowingPaused() ) {
        if ( IsIdle() )
            flTolerance += 150.0f;
        else
            flTolerance += 180.0f;
    } 
    else if ( GetBot()->GetActiveSchedule() ) {
        return 60.0f;
    }

    return flTolerance;
}

//================================================================================
// Verifica si la ruta sigue siendo válida
//================================================================================
void CNavigationComponent::CheckPath()
{
    // No tenemos una ruta aún
    if ( !HasPath() )
        return;

    // Aún no
    if ( !ShouldRepath( GetDestination() ) )
        return;

    m_Path.Invalidate();
}

//================================================================================
// Calcula la ruta más segura/rápida para llegar al destino
//================================================================================
void CNavigationComponent::ComputePath()
{
    VPROF_BUDGET( "CNavigationComponent::ComputePath", VPROF_BUDGETGROUP_BOTS );

    // Ya tenemos una ruta
    if ( HasPath() )
        return;

    Vector from = GetAbsOrigin();
    Vector to = GetDestination();

    BotPathCost pathCost( GetHost() );

    m_Path.Compute( from, to, pathCost );
    m_Navigation.Reset();

    //m_bTraversingLadder = false;

    // Debug
    //GetBot()->DebugAddMessage("Computing Path!");
}

//================================================================================
// Establece el destino hacia un punto especifico del mapa
//================================================================================
void CNavigationComponent::SetDestination( const Vector &vecGoal, int priority )
{
    if ( IsDisabled() )
        return;

    if ( !vecGoal.IsValid() )
        return;

    if ( GetPriority() > priority )
        return;

    float flDistance = GetAbsOrigin().DistTo( vecGoal );
    float flTolerance = GetDistanceTolerance();

    if ( flDistance <= flTolerance )
        return;

    m_vecDestination = vecGoal;
    m_iDestinationPriority = priority;

    // Debug
    //GetBot()->DebugAddMessage("SetDestination(%.2f %.2f %.2f, %i) - %.2f", vecGoal.x, vecGoal.y, vecGoal.z, priority, GetDistanceLeft());
}

//================================================================================
// Establece el destino hacia una entidad
//================================================================================
void CNavigationComponent::SetDestination( CBaseEntity *pEntity, int iPriority )
{
    SetDestination( pEntity->GetAbsOrigin(), iPriority );
}

//================================================================================
// Establece el destino hacia un punto al azar del area
//================================================================================
void CNavigationComponent::SetDestination( CNavArea *pArea, int iPriority )
{
    SetDestination( pArea->GetRandomPoint(), iPriority );
}

//================================================================================
// Para el sistema de navegación
//================================================================================
void CNavigationComponent::Stop()
{
    m_bTraversingLadder = false;

    m_Path.Invalidate();
    m_Navigation.Reset();

    m_vecDestination.Invalidate();
    m_vecNextSpot.Invalidate();
    m_iDestinationPriority = PRIORITY_INVALID;

    // Debug
    GetBot()->DebugAddMessage( "Navigation Stopped" );
}

//================================================================================
// Mueve el Bot en direcciones al azar para desatorarse
//================================================================================
void CNavigationComponent::Wiggle()
{
    if ( !m_WiggleTimer.HasStarted() || m_WiggleTimer.IsElapsed() ) {
        m_WiggleDirection = (NavRelativeDirType)RandomInt( 0, 3 );
        m_WiggleTimer.Start( RandomFloat( 0.5f, 1.5f ) );
    }

    Vector vecForward, vecRight;
    GetHost()->EyeVectors( &vecForward, &vecRight );

    const float lookRange = 15.0f;
    Vector vecPos;
    float flGround;

    switch ( m_WiggleDirection ) {
        case LEFT:
            vecPos = GetAbsOrigin() - (lookRange * vecRight);
            break;

        case RIGHT:
            vecPos = GetAbsOrigin() + (lookRange * vecRight);
            break;

        case FORWARD:
        default:
            vecPos = GetAbsOrigin() + (lookRange * vecForward);
            break;

        case BACKWARD:
            vecPos = GetAbsOrigin() - (lookRange * vecForward);
            break;
    }

    // No nos caeremos al vacio, nos movemos!
    if ( GetSimpleGroundHeightWithFloor( vecPos, &flGround ) ) {
        GetBot()->InjectMovement( m_WiggleDirection );
    }

    // Saltamos
    if ( m_Navigation.GetStuckDuration() > 3.5f && RandomInt( 0, 100 ) > 80 )
        Jump();
}

//================================================================================
//================================================================================
const Vector &CNavigationComponent::GetCentroid() const
{
    static Vector centroid;

    const Vector &mins = m_pParent->WorldAlignMins();
    const Vector &maxs = m_pParent->WorldAlignMaxs();

    centroid = GetFeet();
    centroid.z += (maxs.z - mins.z) / 2.0f;

    return centroid;
}

//================================================================================
//================================================================================
const Vector &CNavigationComponent::GetFeet() const
{
    static Vector feet;

    feet = m_pParent->GetAbsOrigin();
    return feet;
}

//================================================================================
//================================================================================
const Vector &CNavigationComponent::GetEyes() const
{
    static Vector eyes;

    eyes = m_pParent->EyePosition();
    return eyes;
}

//================================================================================
//================================================================================
float CNavigationComponent::GetMoveAngle() const
{
    return m_pParent->GetAbsAngles().y;
}

//================================================================================
//================================================================================
CNavArea *CNavigationComponent::GetLastKnownArea() const
{
    return m_pParent->GetLastKnownArea();
}

//================================================================================
//  Find "simple" ground height, treating current nav area as part of the floo
//================================================================================
bool CNavigationComponent::GetSimpleGroundHeightWithFloor( const Vector &pos, float *height, Vector *normal )
{
    if ( TheNavMesh->GetSimpleGroundHeight( pos, height, normal ) ) {
        // our current nav area also serves as a ground polygon
        if ( GetLastKnownArea() && GetLastKnownArea()->IsOverlapping( pos ) ) {
            *height = MAX( (*height), GetLastKnownArea()->GetZ( pos ) );
        }

        return true;
    }

    return false;
}

//================================================================================
//================================================================================
void CNavigationComponent::Crouch()
{
    GetBot()->Crouch();
}

//================================================================================
//================================================================================
void CNavigationComponent::StandUp()
{
    GetBot()->StandUp();
}

//================================================================================
//================================================================================
bool CNavigationComponent::IsCrouching() const
{
    return GetBot()->IsCrouching();
}

//================================================================================
//================================================================================
void CNavigationComponent::Jump()
{
    InjectButton( IN_JUMP );
    m_bJumping = true;
}

//================================================================================
//================================================================================
bool CNavigationComponent::IsJumping() const
{
    return m_bJumping;
}

//================================================================================
//================================================================================
void CNavigationComponent::Run()
{
    GetBot()->Run();
}

//================================================================================
//================================================================================
void CNavigationComponent::Walk()
{
    GetBot()->NormalWalk();
}

//================================================================================
//================================================================================
bool CNavigationComponent::IsRunning() const
{
    return GetBot()->IsRunning();
}

//================================================================================
//================================================================================
void CNavigationComponent::StartLadder( const CNavLadder *ladder, NavTraverseType how, const Vector &approachPos, const Vector &departPos )
{
    // TODO?
    m_bTraversingLadder = true;
}

//================================================================================
//================================================================================
bool CNavigationComponent::TraverseLadder( const CNavLadder *ladder, NavTraverseType how, const Vector &approachPos, const Vector &departPos, float deltaT )
{
    Vector vecStart;
    Vector vecEnd;
    Vector vecTraverseLook;

    //m_Navigation.ResetStuck();
    m_Navigation.m_stuckMonitor.Update( this );

    // Debug
    if ( bot_debug.GetBool() )
        ladder->DrawLadder();

    // Debemos ir hacia arriba
    if ( how == GO_LADDER_UP ) {
        vecStart = ladder->m_bottom;
        vecEnd = ladder->m_top;

        if ( ladder->m_topForwardArea )
            vecTraverseLook = ladder->m_topForwardArea->GetCenter();
        else if ( ladder->m_topLeftArea )
            vecTraverseLook = ladder->m_topLeftArea->GetCenter();
        else if ( ladder->m_topRightArea )
            vecTraverseLook = ladder->m_topRightArea->GetCenter();
        else
            Assert( false );

        vecEnd.z += JumpHeight;
    }

    // Hacia abajo
    else if ( how == GO_LADDER_DOWN ) {
        Assert( ladder->m_bottomArea );

        vecStart = ladder->m_top;
        vecEnd = ladder->m_bottom;
        vecTraverseLook = ladder->m_bottomArea->GetCenter();
    }
    else {
        Assert( !"Invalid how in Traversing Ladder" );
        return true;
    }

    if ( m_Navigation.IsStuck() && m_Navigation.GetStuckDuration() > 4.0f )
        OnMoveToFailure( vecEnd, FAIL_STUCK );

    // Debemos apuntar justo delante de donde termina la escalera
    vecTraverseLook.x = vecEnd.x;
    vecTraverseLook.z += JumpCrouchHeight;
    //NDebugOverlay::Text( vecTraverseLook, "Aim Here!", false, 0.1f );

    // No estamos en la escalera
    if ( !IsUsingLadder() ) {
        if ( departPos.IsValid() ) {
            // Detectamos si hemos llegado al final
            if ( how == GO_LADDER_UP ) {
                if ( GetAbsOrigin().z >= departPos.z ) {
                    m_bTraversingLadder = false;
                    return true;
                }
            }
            else {
                if ( GetAbsOrigin().z <= departPos.z ) {
                    m_bTraversingLadder = false;
                    return true;
                }
            }
        }

        // Nos movemos hasta el lugar donde comienza la escalera
        // TODO: Detectar si nos hemos atorado
        TrackPath( vecStart, deltaT );
        GetBot()->DebugAddMessage( UTIL_VarArgs( "Going to ladder... %.2f \n", GetAbsOrigin().DistTo( vecStart ) ) );
    }
    else {
        // Miramos hacia dirección deseada (arriba/abajo)
        if ( GetBot()->Aim() )
            GetBot()->Aim()->LookAt( "Look Ladder End", vecTraverseLook, PRIORITY_CRITICAL );

        // Distancia faltante
        float flDistance = GetAbsOrigin().DistTo( vecEnd );

        // Nos movemos hacia delante para cruzar la escalera
        GetBot()->InjectMovement( FORWARD );
        GetBot()->DebugAddMessage( UTIL_VarArgs( "Traversing ladder... %.2f \n", flDistance ) );

        // Estamos bajando y casi llegamos al destino
        // Saltamos para soltarnos!
        if ( flDistance <= 30.0f && how == GO_LADDER_DOWN )
            Jump();
    }

    m_bTraversingLadder = true;

    /*
    {
    NDebugOverlay::Text( approachPos, "ApproachPos", false, 0.1f );
    NDebugOverlay::Text( departPos, "DepartPos", false, 0.1f );

    NDebugOverlay::Text( ladder->m_top, "Top!", false, 0.1f );
    NDebugOverlay::Text( ladder->m_bottom, "Bottom!", false, 0.1f );
    }
    */

    return false;
}

//================================================================================
//================================================================================
bool CNavigationComponent::IsUsingLadder() const
{
    return (GetHost()->GetMoveType() == MOVETYPE_LADDER);
}

//================================================================================
//================================================================================
void CNavigationComponent::TrackPath( const Vector &pathGoal, float deltaT )
{
    Vector myOrigin = GetCentroid();

    // Próximo punto de destino
    m_vecNextSpot = pathGoal;

    // compute our current forward and lateral vectors
    float flAngle = GetHost()->EyeAngles().y;

    Vector2D dir( Utils::BotCOS( flAngle ), Utils::BotSIN( flAngle ) );
    Vector2D lat( -dir.y, dir.x );

    // compute unit vector to goal position
    Vector2D to( pathGoal.x - myOrigin.x, pathGoal.y - myOrigin.y );
    to.NormalizeInPlace();

    // move towards the position independant of our view direction
    float toProj = to.x * dir.x + to.y * dir.y;
    float latProj = to.x * lat.x + to.y * lat.y;

    const float c = 0.25f;    // 0.5

    if ( toProj > c )
        GetBot()->InjectMovement( FORWARD );
    else if ( toProj < -c )
        GetBot()->InjectMovement( BACKWARD );

    if ( latProj >= c )
        GetBot()->InjectMovement( LEFT );
    else if ( latProj <= -c )
        GetBot()->InjectMovement( RIGHT );
}

//================================================================================
// [Evento] Hemos llegado a nuestro destino
//================================================================================
void CNavigationComponent::OnMoveToSuccess( const Vector &goal )
{
    Stop();
    GetBot()->DebugAddMessage( "OnMoveToSuccess" );
}

//================================================================================
// [Evento] Hemos fallado al intentar llegar a un lugar
//================================================================================
void CNavigationComponent::OnMoveToFailure( const Vector &goal, MoveToFailureType reason )
{
    if ( GetBot()->ShouldTeleport( goal ) ) {
        Vector theGoal = goal;
        theGoal.z += 20;

        // Teletransportamos y paramos
        GetHost()->Teleport( &theGoal, &GetAbsAngles(), NULL );
        Stop();

        // Debug
        if ( reason == FAIL_STUCK )
            GetBot()->DebugAddMessage( "Im Stuck! Teleporting..." );
        if ( reason == FAIL_INVALID_PATH )
            GetBot()->DebugAddMessage( "Invalid Path! Teleporting..." );
        if ( reason == FAIL_FELL_OFF )
            GetBot()->DebugAddMessage( "Very HIGH! Teleporting..." );
    }
    else {
        m_Path.Invalidate();

        // Debug
        if ( reason == FAIL_STUCK )
            GetBot()->DebugAddMessage( "Im Stuck! I cant teleport!" );
        if ( reason == FAIL_INVALID_PATH )
            GetBot()->DebugAddMessage( "Invalid Path! I cant teleport!" );
        if ( reason == FAIL_FELL_OFF )
            GetBot()->DebugAddMessage( "Very HIGH! I cant teleport!" );
    }
}