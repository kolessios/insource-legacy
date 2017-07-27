//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "bot.h"

#include "in_utils.h"
#include "bot_manager.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
// Macros
//================================================================================

#define RANDOM_AIM_INTERVAL ( IsAlerted() ) ? RandomInt( 3, 10 ) : RandomInt( 6, 20 )
#define INTERESTING_AIM_INTERVAL ( IsAlerted() ) ? RandomInt( 2, 5 )  : RandomInt( 6, 15 )

//================================================================================
//================================================================================
void CAimComponent::OnSpawn()
{
    // Restauramos información
    m_nRandomAimTimer.Start( 2 );
    m_nIntestingAimTimer.Start( 10 );

    Clear();
}

//================================================================================
//================================================================================
void CAimComponent::OnUpdate( CBotCmd* &cmd )
{
    VPROF_BUDGET( "CAimComponent::OnUpdate", VPROF_BUDGETGROUP_BOTS );

    if ( GetBot()->OptimizeThisFrame() )
        return;

    if ( GetBot()->HasDestination() ) {
        LookNavigation();
    }

    LookAround();
    Update( cmd );
}

//================================================================================
// Establece la prioridad al apuntar
//================================================================================
void CAimComponent::SetPriority( int priority, int isLower )
{
    if ( GetPriority() > isLower )
        return;

    m_iAimPriority = priority;
}

//================================================================================
// Establece [vecAim] como el punto ideal para mirar a la entidad especificada
//================================================================================
void CAimComponent::GetAimCenter( CBaseEntity *pEntity, Vector &vecAim )
{
    if ( !pEntity )
        return;

    // Si es un personaje, intentamos apuntar a un lugar del cuerpo
    if ( pEntity->MyCombatCharacterPointer() ) {
        QAngle angles;
        HitboxType aimType = GetSkill()->GetFavoriteHitbox();

        // Es nuestro enemigo, no deberíamos usar esto.
        // Esto ya es manejado por el componentente de enemigos
        if ( GetBot()->GetEnemy() == pEntity )
            Assert( !"Se ha llamado a LookAt para mirar al enemigo!" );

        if ( Utils::GetHitboxPosition( pEntity, vecAim, aimType ) )
            return;
    }

    vecAim = pEntity->edict()->GetCollideable()->GetCollisionOrigin();

    if ( !vecAim.IsValid() )
        vecAim = pEntity->WorldSpaceCenter();
}

//================================================================================
// Devuelve la velocidad al apuntar
//================================================================================
AimSpeed CAimComponent::GetAimSpeed()
{
    int speed = GetSkill()->GetMinAimSpeed();

    if ( speed == AIM_SPEED_INSTANT )
        return AIM_SPEED_INSTANT;

    if ( GetHost()->IsMoving() )
        ++speed;

    if ( !GetSkill()->IsEasy() ) {
        if ( IsAlerted() || IsCombating() )
            ++speed;

        if ( GetHost()->IsUnderAttack() )
            ++speed;
    }

    // Clamp
    speed = clamp( speed, GetSkill()->GetMinAimSpeed(), GetSkill()->GetMaxAimSpeed() );
    return (AimSpeed)speed;
}

//================================================================================
//================================================================================
float CAimComponent::GetStiffness()
{
    AimSpeed speed = GetAimSpeed();

    switch ( speed ) {
        case AIM_SPEED_VERYLOW:
            return 90.0f;
            break;

        case AIM_SPEED_LOW:
            return 110.0f;
            break;

        case AIM_SPEED_NORMAL:
        default:
            return 150.0f;
            break;

        case AIM_SPEED_FAST:
            return 180.0f;
            break;

        case AIM_SPEED_VERYFAST:
            return 200.0f;
            break;

        case AIM_SPEED_INSTANT:
            return 999.0f;
            break;
    }
}

//================================================================================
// Restablece el apuntado
//================================================================================
void CAimComponent::Clear()
{
    m_vecLookAt.Invalidate();
    m_nAimTimer.Invalidate();

    m_nEntityAim = NULL;
    m_iAimPriority = PRIORITY_INVALID;
    m_pAimDesc = NULL;
    m_flAimDuration = 1.0f;
    m_bInAimRange = false;
}

//================================================================================
// Apunta a la entidad especificada
//================================================================================
void CAimComponent::LookAt( const char *pDesc, CBaseEntity *pEntity, int priority, float duration )
{
    if ( !pEntity )
        return;

    // Obtenemos la posición donde apuntar esta entidad
    Vector vecLook;
    GetAimCenter( pEntity, vecLook );

    bool success = (GetPriority() <= priority);
    LookAt( pDesc, vecLook, priority, duration );

    if ( success )
        m_nEntityAim = pEntity;
}

//================================================================================
// Apunta a la ubicación especificada declarando que se trata de una entidad
//================================================================================
void CAimComponent::LookAt( const char *pDesc, CBaseEntity *pEntity, const Vector &vecLook, int priority, float duration )
{
    if ( !pEntity )
        return;

    bool success = (GetPriority() <= priority);
    LookAt( pDesc, vecLook, priority, duration );

    if ( success )
        m_nEntityAim = pEntity;
}

//================================================================================
// Apunta a la ubicación especificada
//================================================================================
void CAimComponent::LookAt( const char *pDesc, const Vector &vecLook, int priority, float duration )
{
    if ( !vecLook.IsValid() )
        return;

    //if ( m_vecLookAt.DistTo( vecLook ) <= 2.0f )
    //return;

    if ( GetPriority() > priority )
        return;

    m_vecLookAt = vecLook;
    m_pAimDesc = pDesc;
    m_nEntityAim = NULL;

    SetPriority( priority );

    if ( !FStrEq( "Aiming Route", pDesc ) && !FStrEq( "Enemy Body Part", pDesc ) && !FStrEq( "Last Enemy Position", pDesc ) )
        GetBot()->DebugAddMessage( "LookAt(%s, %.2f %.2f, %i, %.2f)", m_pAimDesc, m_vecLookAt.x, m_vecLookAt.y, priority, duration );

    // Duración
    m_nAimTimer.Invalidate();
    m_flAimDuration = duration;
}

//================================================================================
// Mira hacia la siguiente posición en la ruta
//================================================================================
void CAimComponent::LookNavigation()
{
    Vector lookAt( GetBot()->GetNextSpot() );
    lookAt.z = GetHost()->EyePosition().z;

    /*lookAt.x += RandomFloat( -5.0f, 5.0f );
    lookAt.y += RandomFloat( -5.0f, 5.0f );
    lookAt.z += RandomFloat( -5.0f, 5.0f );*/

    int priority = PRIORITY_LOW;

    if ( GetBot()->IsFollowingSomeone() )
        priority = PRIORITY_NORMAL;

    LookAt( "Aiming Route", lookAt, priority, 0.5f );
}

//================================================================================
// Bloquea poder mirar a los lados durante la cantidad de segundos
//================================================================================
void CAimComponent::BlockLookAround( float duration )
{
    // Sin bloqueo
    if ( duration <= 0 ) {
        m_nBlockLookAroundTimer.Invalidate();
        return;
    }

    m_nBlockLookAroundTimer.Start( duration );

    GetBot()->DebugAddMessage( "BlockLookAround(%.2f)", duration );
}

//================================================================================
// Permite que podamos apuntar a un lugar interesante o al azar.
//================================================================================
void CAimComponent::LookAround()
{
    VPROF_BUDGET( "CAimComponent::LookAround", VPROF_BUDGETGROUP_BOTS );

    if ( GetPriority() > PRIORITY_NORMAL )
        return;

    if ( IsLookAroundBlocked() )
        return;

    // Hemos escuchado un sonido que representa peligro
    if ( HasCondition( BCOND_HEAR_COMBAT ) || HasCondition( BCOND_HEAR_DANGER ) || HasCondition( BCOND_HEAR_ENEMY ) ) {
        if ( GetBot()->ShouldAimDangerSpot() ) {
            LookDanger();
            return;
        }
    }

    // Un lugar interesante:
    // Lugares donde los enemigos puedan cubrirse o revelarse.
    if ( GetBot()->ShouldLookInterestingSpot() ) {
        LookInterestingSpot();
        return;
    }

    // Lugar al azar
    if ( GetBot()->ShouldLookRandomSpot() ) {
        LookRandomSpot();
        return;
    }

    // Estamos en un escuadron, miremos a un amigo :)
    if ( GetBot()->GetSquad() && GetBot()->ShouldLookSquadMember() ) {
        LookSquadMember();
        return;
    }
}

//================================================================================
// Busca un lugar interesante y apuntamos hacia ese lugar
//================================================================================
void CAimComponent::LookInterestingSpot()
{
    m_nIntestingAimTimer.Start( INTERESTING_AIM_INTERVAL );

    CSpotCriteria criteria;
    criteria.SetMaxRange( 1000.0f );
    criteria.OnlyVisible( GetBot()->ShouldAimOnlyVisibleInterestingSpots() );
    criteria.UseRandom( true );
    criteria.SetTacticalMode( GetBot()->GetTacticalMode() );

    Vector vecSpot;

    if ( !Utils::FindIntestingPosition( &vecSpot, GetHost(), criteria ) )
        return;

    // Apuntamos
    LookAt( "Intesting Spot", vecSpot, PRIORITY_NORMAL, 1.0f );
}

//================================================================================
// Apuntamos a un lugar al azar
//================================================================================
void CAimComponent::LookRandomSpot()
{
    m_nRandomAimTimer.Start( RANDOM_AIM_INTERVAL );

    QAngle viewAngles = GetBot()->GetCmd()->viewangles;
    viewAngles.x += RandomInt( -10, 10 );
    viewAngles.y += RandomInt( -40, 40 );

    Vector vecForward;
    AngleVectors( viewAngles, &vecForward );

    Vector vecPosition = GetHost()->EyePosition();

    // Apuntamos
    LookAt( "Random Spot", vecPosition + 30 * vecForward, PRIORITY_VERY_LOW );
}

//================================================================================
// Apuntamos a un miembro de nuestro escuadron
//================================================================================
void CAimComponent::LookSquadMember()
{
    CPlayer *pMember = GetHost()->GetSquad()->GetRandomMember();

    if ( !pMember || pMember == GetHost() )
        return;

    // Aputamos
    LookAt( "Squad Member", pMember, PRIORITY_VERY_LOW, RandomFloat(1.0f, 3.5f) );
}

//================================================================================
// Apuntamos a un lugar donde hemos escuchado peligro/combate
//================================================================================
void CAimComponent::LookDanger()
{
    CSound *pSound = GetHost()->GetBestSound( SOUND_COMBAT | SOUND_PLAYER | SOUND_DANGER );

    if ( !pSound ) {
        return;
    }

    Vector vecLookAt = pSound->GetSoundReactOrigin();
    vecLookAt.z += HalfHumanHeight;

    float distance = GetAbsOrigin().DistTo( vecLookAt );

    if ( distance < 600.0f )
        return;

    // Apuntamos
    LookAt( "Danger Sound", vecLookAt, PRIORITY_LOW, GetSkill()->GetAlertDuration() );
}

//================================================================================
// Apuntamos hacia enfrente
// TODO: Creo que esto no tiene sentido...
//================================================================================
void CAimComponent::LookForward( int priority )
{
    Vector forward;
    GetHost()->EyeVectors( &forward );

    // Apuntamos
    LookAt( "Forward", GetHost()->EyePosition() + 30.0f * forward, priority, -1 );
}

//================================================================================
// Procesa el sistema de apuntado
//================================================================================
void CAimComponent::Update( CBotCmd* &cmd )
{
    VPROF_BUDGET( "CAimComponent::Update", VPROF_BUDGETGROUP_BOTS );

    if ( !IsAiming() )
        return;

    if ( IsAimingTimeExpired() ) {
        Clear();
        return;
    }

    // Velocidad al apuntar
    AimSpeed speed = GetAimSpeed();

    // Transformamos el [Vector] a [QAngle]
    Vector vecLook = m_vecLookAt - GetHost()->EyePosition();
    QAngle anglesLook;
    VectorAngles( vecLook, anglesLook );

    // Donde queremos a puntar
    float lookYaw = anglesLook.y;
    float lookPitch = anglesLook.x;
    QAngle viewAngles = cmd->viewangles;

    // Globales
    float deltaT = TheBots->GetDeltaT();
    float maxAccel = 3000.0f;
    float stiffness = GetStiffness();
    float damping = 25.0f;

    const float onTargetTolerance = 0.5f;
    const float aimTolerance = 3.0f;

    //
    // Yaw
    //
    float angleDiffYaw = AngleNormalize( lookYaw - viewAngles.y );

    if ( angleDiffYaw < onTargetTolerance && angleDiffYaw > -onTargetTolerance || speed == AIM_SPEED_INSTANT ) {
        m_lookYawVel = 0.0f;
        viewAngles.y = lookYaw;
    }
    else {
        // simple angular spring/damper
        float accel = stiffness * angleDiffYaw - damping * m_lookYawVel;

        // limit rate
        if ( accel > maxAccel )
            accel = maxAccel;
        else if ( accel < -maxAccel )
            accel = -maxAccel;

        m_lookYawVel += deltaT * accel;
        viewAngles.y += deltaT * m_lookYawVel;

        // keep track of how long our view remains steady
        /*const float steadyYaw = 1000.0f;
        if (fabs( accel ) > steadyYaw)
        {
        m_viewSteadyTimer.Start();
        }*/
    }

    //
    // Pitch
    //
    float angleDiffPitch = lookPitch - viewAngles.x;
    angleDiffPitch = AngleNormalize( angleDiffPitch );

    if ( angleDiffPitch < onTargetTolerance && angleDiffPitch > -onTargetTolerance || speed == AIM_SPEED_INSTANT ) {
        m_lookPitchVel = 0.0f;
        viewAngles.x = lookPitch;
    }
    else {
        // simple angular spring/damper
        // double the stiffness since pitch is only +/- 90 and yaw is +/- 180
        float accel = 2.0f * stiffness * angleDiffPitch - damping * m_lookPitchVel;

        // limit rate
        if ( accel > maxAccel )
            accel = maxAccel;
        else if ( accel < -maxAccel )
            accel = -maxAccel;

        m_lookPitchVel += deltaT * accel;
        viewAngles.x += deltaT * m_lookPitchVel;

        // keep track of how long our view remains steady
        /*const float steadyPitch = 1000.0f;
        if (fabs( accel ) > steadyPitch)
        {
        m_viewSteadyTimer.Start();
        }*/
    }

    // Ya estamos viendo cerca de la dirección deseada
    // Usamos "cerca" para compensar el movimiento.
    if ( (angleDiffYaw < aimTolerance && angleDiffYaw > -aimTolerance) && (angleDiffPitch < aimTolerance && angleDiffPitch > -aimTolerance) ) {
        m_bInAimRange = true;

        // Empezamos el contador de tiempo
        if ( !m_nAimTimer.HasStarted() && m_flAimDuration > 0 )
            m_nAimTimer.Start( m_flAimDuration );
    }
    else {
        m_bInAimRange = false;
    }

    // Establecemos los viewAngles nuevos
    cmd->viewangles = viewAngles;

    // 
    Utils::DeNormalizeAngle( cmd->viewangles.x );
    Utils::DeNormalizeAngle( cmd->viewangles.y );
}