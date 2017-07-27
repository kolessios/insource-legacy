//==== InfoSmart 2015. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "npc_basein.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

int AE_FOOTSTEP_RIGHT;
int AE_FOOTSTEP_LEFT;
int AE_ATTACK_HIT;

//================================================================================
// Información y Red
//================================================================================

BEGIN_DATADESC( CBaseNPC )
END_DATADESC()

LINK_ENTITY_TO_CLASS( npc_base_in, CBaseNPC );

//================================================================================
// Constructor
//================================================================================
CBaseNPC::CBaseNPC()
{
    m_iMoveXPoseParam = -1;
}

//================================================================================
// Destructor
//================================================================================
CBaseNPC::~CBaseNPC()
{
}

//================================================================================
//================================================================================
void CBaseNPC::Spawn()
{
    // Guardamos objetos en caché
    Precache();

    // Modelo
    SetModel( GetNPCModel() );

    // Base
    BaseClass::Spawn();    

    // Restauramos el HULL
    SetHullType( GetNPCHull() );
    SetHullSizeNormal();
    SetDefaultEyeOffset();

    // Navegación
    SetNavType( NAV_GROUND );

    // Físicas
    SetSolid( SOLID_BBOX );
    AddSolidFlags( FSOLID_NOT_STANDABLE );
    SetMoveType( MOVETYPE_STEP );

    // Salud
    m_iHealth            = m_iMaxHealth = GetNPCHealth();
    m_flFieldOfView      = GetNPCFOV();
    m_NPCState           = NPC_STATE_IDLE;

    // Establecemos las capacidades del NPC
    SetCapabilities();

    // Configuramos los parametros
    SetupGlobalModelData();

    // Init
    NPCInit();

    // Preparamos el modelo
    SetUpModel();
}

//================================================================================
// Establece las capacidades del NPC
//================================================================================
void CBaseNPC::SetCapabilities()
{
    CapabilitiesAdd( bits_CAP_MOVE_GROUND );
    CapabilitiesAdd( bits_CAP_INNATE_MELEE_ATTACK1 );
}

//================================================================================
//================================================================================
void CBaseNPC::SetupGlobalModelData()
{
    // Ya se ha configurado
    if ( m_iMoveXPoseParam != -1 )
        return;

    m_iMoveXPoseParam = LookupPoseParameter("move_x");
    m_iMoveYPoseParam = LookupPoseParameter("move_y");

    m_iLeanYawPoseParam = LookupPoseParameter("lean_yaw");
    m_iLeanPitchPoseParam = LookupPoseParameter("lean_pitch");
}

//================================================================================
//================================================================================
bool CBaseNPC::OverrideMoveFacing( const AILocalMoveGoal_t &move, float flInterval )
{
    // required movement direction
    float flMoveYaw = UTIL_VecToYaw( move.dir );

    // FIXME: move this up to navigator so that path goals can ignore these overrides.
    Vector dir;
    float flInfluence = GetFacingDirection( dir );
    dir = move.facing * (1 - flInfluence) + dir * flInfluence;
    VectorNormalize( dir );

    // ideal facing direction
    float idealYaw = UTIL_AngleMod( UTIL_VecToYaw( dir ) );
        
    // FIXME: facing has important max velocity issues
    GetMotor()->SetIdealYawAndUpdate( idealYaw );    

    // find movement direction to compensate for not being turned far enough
    float flDiff = UTIL_AngleDiff( flMoveYaw, GetLocalAngles().y );

    // Setup the 9-way blend parameters based on our speed and direction.
    Vector2D vCurMovePose( 0, 0 );

    vCurMovePose.x = cos( DEG2RAD( flDiff ) ) * 1.0f; //flPlaybackRate;
    vCurMovePose.y = -sin( DEG2RAD( flDiff ) ) * 1.0f; //flPlaybackRate;

    SetPoseParameter( m_iMoveXPoseParam, vCurMovePose.x );
    SetPoseParameter( m_iMoveYPoseParam, vCurMovePose.y );

    // ==== Update Lean pose parameters
    if ( m_iLeanYawPoseParam >= 0 )
    {
        float targetLean    = GetPoseParameter( m_iMoveYPoseParam ) * 30.0f;
        float curLean        = GetPoseParameter( m_iLeanYawPoseParam );

        if( curLean < targetLean )
            curLean += MIN(fabs(targetLean-curLean), GetAnimTimeInterval()*15.0f);
        else
            curLean -= MIN(fabs(targetLean-curLean), GetAnimTimeInterval()*15.0f);

        SetPoseParameter( m_iLeanYawPoseParam, curLean );
    }

    if( m_iLeanPitchPoseParam >= 0 )
    {
        float targetLean    = GetPoseParameter( m_iMoveXPoseParam ) * -30.0f;
        float curLean        = GetPoseParameter( m_iLeanPitchPoseParam );

        if( curLean < targetLean )
            curLean += MIN(fabs(targetLean-curLean), GetAnimTimeInterval()*15.0f);
        else
            curLean -= MIN(fabs(targetLean-curLean), GetAnimTimeInterval()*15.0f);

        SetPoseParameter( m_iLeanPitchPoseParam, curLean );
    }

    return true;
}

//================================================================================
//================================================================================
float CBaseNPC::GetIdealSpeed() const
{
    // Ensure navigator will move
    // TODO: Could limit it to move sequences only.
    float speed = BaseClass::GetIdealSpeed();
    if( speed <= 0 ) speed = 1.0f;
    return speed;
}

//================================================================================
//================================================================================
float CBaseNPC::GetSequenceGroundSpeed( CStudioHdr *pStudioHdr, int iSequence )
{
    // Ensure navigator will move
    // TODO: Could limit it to move sequences only.
    float speed = BaseClass::GetSequenceGroundSpeed( pStudioHdr, iSequence );
    if( speed <= 0 /*&& GetSequenceActivity( iSequence) == ACT_TERROR_RUN_INTENSE*/ ) speed = 1.0f;
    return speed;
}

//================================================================================
//================================================================================
float CBaseNPC::GetEnemyDistance()
{
    if ( !GetEnemy() )
        return -1;

    return GetEnemy()->GetAbsOrigin().DistTo( GetAbsOrigin() );
}

//================================================================================
//================================================================================
bool CBaseNPC::MeleeAttack()
{
    float flDistance    = GetMeleeDistance();
    float flDamage        = GetMeleeDamage();

    // Realizamos el ataque y obtenemos nuestra victima
    CBaseEntity *pVictim = CheckTraceHullAttack( flDistance, -Vector(16,16,32), Vector(16,16,32), flDamage, DMG_SLASH, 5.0f );

    // No le hemos dado a nada :(
    if ( !pVictim )
    {
        //EmitSound("Infected.HitMiss");
        return false;
    }

    // Obtenemos la dirección del ataque
    Vector vecForceDir = ( pVictim->WorldSpaceCenter() - WorldSpaceCenter() );

    // Hemos golpeado a un Jugador
    if ( pVictim->IsPlayer() )
    {
        // QAngle( 20.0f, 0.0f, -12.0f ), Vector( -250.0f, 1.0f, 1.0f )
        CBasePlayer *pPlayer = ToBasePlayer( pVictim );

        Vector vecDir = pVictim->GetAbsOrigin() - GetAbsOrigin();
        VectorNormalize( vecDir );

        Vector vecShove( -250.0f, 1.0f, 1.0f );

        QAngle angles;
        VectorAngles( vecDir, angles );

        Vector vecForward, vecRight;
        AngleVectors( angles, &vecDir, &vecRight, NULL );

        // Golpeamos la camara del Jugador
        pPlayer->ViewPunch( QAngle(5.0f, 0.0f, -8.0f) );
        //pPlayer->ApplyAbsVelocityImpulse( -vecRight * vecShove[1] - vecForward * vecShove[0] );
    }

    EmitSound("Infected.Hit");
	return true;
}

//================================================================================
//================================================================================
void CBaseNPC::HandleAnimEvent( animevent_t *pEvent )
{
    int iEvent = pEvent->Event();

    // Pasos
    if ( iEvent == AE_FOOTSTEP_LEFT || iEvent == AE_FOOTSTEP_RIGHT )
        return;

    // Ataque
    if ( iEvent == AE_ATTACK_HIT )
    {
        MeleeAttack();
        return;
    }

    BaseClass::HandleAnimEvent( pEvent );
}

//================================================================================
// Comienza a ejecutar una tarea
//================================================================================
void CBaseNPC::StartTask( const Task_t *pTask )
{
    switch( pTask->iTask )
    {
        // Ataque continuo
        case TASK_CONTINUOS_MELEE_ATTACK1:
		{
			// Última vez que he atacado
            SetLastAttackTime( gpGlobals->curtime );
            m_iAttackGesture = AddGesture( NPC_TranslateActivity(ACT_MELEE_ATTACK1) );
			break;
		}

        default:
            BaseClass::StartTask( pTask );
		break;
    }
}

//================================================================================
// Esperando a que una tarea termine
//================================================================================
void CBaseNPC::RunTask( const Task_t *pTask )
{
    switch( pTask->iTask )
    {
        case TASK_CONTINUOS_MELEE_ATTACK1:
		{
			RunAttackTask( pTask->iTask );
			break;
		}        

        default:
            BaseClass::RunTask( pTask );
    }
}

//================================================================================
//================================================================================
void CBaseNPC::RunAttackTask( int iTask )
{
    // Este código solo funciona para los golpes continuos
    if ( iTask != TASK_CONTINUOS_MELEE_ATTACK1 )
    {
        BaseClass::RunAttackTask( iTask );
        return;
    }

	if ( !GetEnemy() )
	{
		TaskComplete();
		return;
	}

	if ( GetEnemyDistance() >= 45 )
	{
		TaskComplete();
		return;
	}

	CAnimationLayer *pLayer = GetAnimOverlay( m_iAttackGesture );

	if ( pLayer->m_bSequenceFinished )
        TaskComplete();

	/*
    AutoMovement();
    Vector vecEnemyLKP = GetEnemyLKP();

    // If our enemy was killed, but I'm not done animating, the last known position comes
    // back as the origin and makes the me face the world origin if my attack schedule
    // doesn't break when my enemy dies. (sjb)
    if( vecEnemyLKP != vec3_origin )
    {
        if ( ( iTask == TASK_RANGE_ATTACK1 || iTask == TASK_RELOAD ) && 
             ( CapabilitiesGet() & bits_CAP_AIM_GUN ) && 
             FInAimCone( vecEnemyLKP ) )
        {
            // Arms will aim, so leave body yaw as is
            GetMotor()->SetIdealYawAndUpdate( GetMotor()->GetIdealYaw(), AI_KEEP_YAW_SPEED );
        }
        else
        {
            GetMotor()->SetIdealYawToTargetAndUpdate( vecEnemyLKP, AI_KEEP_YAW_SPEED );
        }
    }

    CAnimationLayer *pLayer = GetAnimOverlay( m_iAttackGesture );

    // Hemos completado la animación de golpe
    if ( pLayer->m_bSequenceFinished )
        TaskComplete();
	*/
}

//================================================================================
//================================================================================
bool CBaseNPC::ShouldPlayDeathAnimation() 
{
	// Ya estamos muertos!
	if ( m_lifeState == LIFE_DEAD )
		return false;

	// Estabamos subiendo/bajando una escalera
	if ( ( GetFlags() & FL_FLY ) )
		return false;

	return true;
}

//================================================================================
//================================================================================
int CBaseNPC::SelectDeadSchedule()
{
    // Sin animación de muerte
    if ( !ShouldPlayDeathAnimation() )
	{
		BecomeRagdollOnClient( vec3_origin );
		CleanupOnDeath();
		return SCHED_DIE_RAGDOLL;
	}

	CleanupOnDeath();
    return SCHED_DIE;
}

//================================================================================
//================================================================================
void CBaseNPC::RunDieTask()
{
    AutoMovement();
    m_lifeState = LIFE_DYING;

    // La animación ha terminado
    if ( IsActivityFinished() )
    {
		MaintainActivity();
        m_lifeState = LIFE_DEAD;

		SetAbsVelocity( vec3_origin );

		CleanupOnDeath();
		BecomeRagdollOnClient(vec3_origin);
		

        // Invisibles
        //AddEffects( EF_NODRAW );
    }
}

//================================================================================
// Inteligencia Artificial
//================================================================================
AI_BEGIN_CUSTOM_NPC( npc_base_in, CBaseNPC )
    DECLARE_ANIMEVENT( AE_FOOTSTEP_RIGHT )
    DECLARE_ANIMEVENT( AE_FOOTSTEP_LEFT )
    DECLARE_ANIMEVENT( AE_ATTACK_HIT )

    DECLARE_TASK( TASK_CONTINUOS_MELEE_ATTACK1 )

    DEFINE_SCHEDULE
    (
        SCHED_CONTINUOUS_MELEE_ATTACK1,

        "    Tasks"
		"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_CHASE_ENEMY"
        "        TASK_FACE_ENEMY                0"
        "        TASK_ANNOUNCE_ATTACK           0"
        "        TASK_CONTINUOS_MELEE_ATTACK1   0"
        ""
        "    Interrupts"
        "        COND_NEW_ENEMY"
        "        COND_ENEMY_DEAD"
        "        COND_LIGHT_DAMAGE"
        "        COND_HEAVY_DAMAGE"
        "        COND_ENEMY_OCCLUDED"
    )
AI_END_CUSTOM_NPC()