//==== InfoSmart 2015. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "npc_infected.h"



#include "physics_prop_ragdoll.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
// Actividades personalizadas
//================================================================================

Activity ACT_TERROR_IDLE_NEUTRAL;
Activity ACT_TERROR_IDLE_ALERT;
Activity ACT_TERROR_WALK_NEUTRAL;
Activity ACT_TERROR_RUN_INTENSE;
Activity ACT_TERROR_CROUCH_RUN_INTENSE;
Activity ACT_TERROR_SHAMBLE;
Activity ACT_TERROR_ATTACK;
Activity ACT_TERROR_JUMP;
Activity ACT_TERROR_JUMP_OVER_GAP;
Activity ACT_TERROR_JUMP_LANDING_NEUTRAL;
Activity ACT_TERROR_JUMP_LANDING;
Activity ACT_TERROR_JUMP_LANDING_HARD;
Activity ACT_TERROR_LADDER_DISMOUNT;

Activity ACT_TERROR_FACE_LEFT_NEUTRAL;
Activity ACT_TERROR_FACE_RIGHT_NEUTRAL;
Activity ACT_TERROR_ABOUT_FACE_NEUTRAL;

Activity ACT_TERROR_IDLE_ALERT_AHEAD;
Activity ACT_TERROR_IDLE_ACQUIRE;

Activity ACT_TERROR_SIT_FROM_STAND;
Activity ACT_TERROR_LIE_FROM_STAND;

Activity ACT_TERROR_SIT_TO_STAND;
Activity ACT_TERROR_SIT_TO_STAND_ALERT;
Activity ACT_TERROR_SIT_TO_LIE;
Activity ACT_TERROR_SIT_IDLE;

Activity ACT_TERROR_LIE_TO_STAND;
Activity ACT_TERROR_LIE_TO_STAND_ALERT;
Activity ACT_TERROR_LIE_TO_SIT;
Activity ACT_TERROR_LIE_IDLE;

Activity ACT_TERROR_DIE_BACKWARD_FROM_SHOTGUN;
Activity ACT_TERROR_DIE_LEFTWARD_FROM_SHOTGUN;
Activity ACT_TERROR_DIE_RIGHTWARD_FROM_SHOTGUN;
Activity ACT_TERROR_DIE_FORWARD_FROM_SHOTGUN;

Activity ACT_TERROR_RUN_STUMBLE;

Activity ACT_EXP_IDLE;
Activity ACT_EXP_ANGRY;

//================================================================================
//================================================================================

DECLARE_NPC_HEALTH( CNPC_Infected, sk_infected_health, "10" )
DECLARE_NPC_MELEE_DAMAGE( CNPC_Infected, sk_infected_damage, "2" )
DECLARE_NPC_MELEE_DISTANCE( CNPC_Infected, sk_infected_melee_distance, "70" )

DECLARE_SERVER_CMD( sk_infected_vision_distance, "912", "Max Vision" )
DECLARE_SERVER_CMD( sk_infected_sleep_chance, "0.5", "Chance of Spawn sleeping" )
DECLARE_SERVER_CMD( sk_infected_investigate_sounds, "0", "Investigate rare sounds" )

#define	INFECTED_GIB_MODEL "models/infected/gibs/gibs.mdl"

//================================================================================
// Información y Red
//================================================================================

LINK_ENTITY_TO_CLASS( npc_infected, CNPC_Infected );

BEGIN_DATADESC( CNPC_Infected )
END_DATADESC()

static const char *g_infectedModels[] = 
{
    "models/infected/common_male_baggagehandler_01.mdl",
    "models/infected/common_male_pilot.mdl",
    "models/infected/common_male_rural01.mdl",
    "models/infected/common_male_suit.mdl",
    "models/infected/common_military_male01.mdl",
    "models/infected/common_police_male01.mdl",
    "models/infected/common_tsaagent_male01.mdl",
    "models/infected/common_worker_male01.mdl",

    "models/infected/common_female_rural01.mdl"
};

//================================================================================
// Devuelve el modelo que debe tener el NPC
//================================================================================
const char *CNPC_Infected::GetNPCModel()
{
    return g_infectedModels[ RandomInt(0, 8) ];
}

//================================================================================
// Aparición en el mundo
//================================================================================
void CNPC_Infected::Spawn()
{
    BaseClass::Spawn();

	m_bWithoutCollision = ( RandomInt(0, 5) >= 2 );
	m_iFaceGesture = -1;

    m_iInfectedStatus   = INFECTED_STAND;
    m_iDesiredStatus    = INFECTED_NONE;

    m_nSitTimer.Start( RandomInt(60, 120) );
    m_nLieTimer.Start( RandomInt(120, 180) );
    m_nStandTimer.Invalidate();

    // Empezamos estando dormidos
    if ( RandomFloat(0.0f, 1.0f) <= sk_infected_sleep_chance.GetFloat() )
        GoLie();
}

//================================================================================
// Pensamiento
//================================================================================
void CNPC_Infected::NPCThink()
{
    BaseClass::NPCThink();

	if ( IsAlive() )
	{
		// Gestos
		UpdateGesture();

		// ¿Hay alguién en mi cabeza?
		IsSomeoneOnTop();

		if ( m_NPCState == NPC_STATE_IDLE )
		{
			// Hora de sentarnos
			if ( m_nSitTimer.HasStarted() && m_nSitTimer.IsElapsed() )
				GoSit();
    
			// Hora de acostarnos
			if ( m_nLieTimer.HasStarted() && m_nLieTimer.IsElapsed() )
				GoLie();

			// Hora de pararnos
			if ( m_nStandTimer.HasStarted() && m_nStandTimer.IsElapsed() )
				GoStand();

			if ( m_iInfectedStatus != INFECTED_STAND && IsPanicked() )
				GoStand();
		}

		// Somos uno de los que podemos traspasar a los demás
		if ( m_NPCState == NPC_STATE_COMBAT )
		{
			SetCollisionGroup( COLLISION_GROUP_NOT_BETWEEN_THEM );
		}
		else
		{
			SetCollisionGroup( COLLISION_GROUP_NPC );
		}
	}
}

//================================================================================
// Guardar objetos en caché
//================================================================================
void CNPC_Infected::Precache()
{
    for ( int i = 0; i < ARRAYSIZE(g_infectedModels); ++i )
        PrecacheModel( g_infectedModels[i] );

	PrecacheModel(INFECTED_GIB_MODEL);

    PrecacheScriptSound("Infected.Idle");
    PrecacheScriptSound("Infected.Alert");
    PrecacheScriptSound("Infected.Pain");
    PrecacheScriptSound("Infected.Die");
    PrecacheScriptSound("Infected.BecomeAlert");
    PrecacheScriptSound("Infected.Hit");
    PrecacheScriptSound("Infected.HitMiss");
    PrecacheScriptSound("Infected.Attack");

    BaseClass::Precache();
}

//================================================================================
// Prepara el modelo
//================================================================================
void CNPC_Infected::SetUpModel()
{
    SetBodygroup( 0, RandomInt (0,3) ); // Head
    SetBodygroup( 1, RandomInt (0,5) ); // Upper body

    // Ropa y aspecto al azar
    m_nSkin = RandomInt(0, 31);

    // Color de la ropa
    SetRenderColor( RandomInt(10,255), RandomInt(10,255), RandomInt(10,255) );
}

//================================================================================
// Actualiza el gesto en la cara del infectado
//================================================================================
void CNPC_Infected::UpdateGesture() 
{
	if ( m_iFaceGesture >= 0 )
	{
		CAnimationLayer *pFace = GetAnimOverlay( m_iFaceGesture );

		if ( pFace && !pFace->m_bSequenceFinished )
			return;
	}

	if ( !GetEnemy() )
		m_iFaceGesture = AddGesture( ACT_EXP_IDLE );
	else
		m_iFaceGesture = AddGesture( ACT_EXP_ANGRY );
}

//================================================================================
// Verifica si alguién me ha caido en la cabeza. RIP
//================================================================================
void CNPC_Infected::IsSomeoneOnTop() 
{
	Vector vecUp, vecSrc;
	GetVectors( NULL, NULL, &vecUp );

	vecSrc = EyePosition() + Vector(0,0,10);

	// Trazamos una linea hacia arriba
	trace_t tr;
	AI_TraceLine( vecSrc, vecSrc + 50.0f * vecUp, MASK_NPCSOLID, this, COLLISION_GROUP_NOT_BETWEEN_THEM, &tr );

	// Nuestra cabeza esta bien
	if ( !tr.m_pEnt )
		return;

	// No es un NPC ni un Jugador
	if ( !tr.m_pEnt->IsNPC() && !tr.m_pEnt->IsPlayer() )
		return;

	// ¡Alguien nos ha caido encima!
	GetNavigator()->StopMoving();
	TakeDamage( CTakeDamageInfo(this, this, 300, DMG_GENERIC) );
}

//================================================================================
// Actualiza manualmente la animación al caer
//================================================================================
void CNPC_Infected::UpdateFall() 
{
	if ( m_iInfectedStatus != INFECTED_STAND )
		return;

	BaseClass::UpdateFall();
}

//================================================================================
// Verifica si podemos trepar una pared
//================================================================================
void CNPC_Infected::UpdateClimb() 
{
	if ( m_iInfectedStatus != INFECTED_STAND )
		return;

	BaseClass::UpdateClimb();
}

//================================================================================
// Reproduce el sonido de dolor
//================================================================================
void CNPC_Infected::PainSound( const CTakeDamageInfo &info )
{
    // We're constantly taking damage when we are on fire. Don't make all those noises!
    if ( IsOnFire() )
        return;

    EmitSound( "Infected.Pain" );
	//CSoundEnt::InsertSound( SOUND_COMBAT, GetAbsOrigin(), 512.0f, 0.1f, this, SOUNDENT_CHANNEL_INJURY );
}

//================================================================================
// Reproduce el sonido de muerte
//================================================================================
void CNPC_Infected::DeathSound( const CTakeDamageInfo &info ) 
{
    EmitSound( "Infected.Die" );
	//CSoundEnt::InsertSound( SOUND_COMBAT, GetAbsOrigin(), 512.0f, 0.1f, this, SOUNDENT_CHANNEL_INJURY );
}

//================================================================================
// Reproduce el sonido de alerta
//================================================================================
void CNPC_Infected::AlertSound()
{
    EmitSound( "Infected.Alert" );
	//CSoundEnt::InsertSound( SOUND_COMBAT, GetAbsOrigin(), 512.0f, 0.3f, this, SOUNDENT_CHANNEL_REPEATING );
}

//================================================================================
// Reproduce el sonido al estar tranquilos
//================================================================================
void CNPC_Infected::IdleSound()
{
    // Estamos acostados
    if ( m_iInfectedStatus == INFECTED_LIE )
        return;

    EmitSound( "Infected.Idle" );
	//CSoundEnt::InsertSound( SOUND_COMBAT, GetAbsOrigin(), 312.0f, 0.3f, this, SOUNDENT_CHANNEL_REPEATING );
}

//================================================================================
// Reproduce el sonido de ataque
//================================================================================
void CNPC_Infected::AttackSound()
{
    EmitSound("Infected.Attack");
	//CSoundEnt::InsertSound( SOUND_COMBAT, GetAbsOrigin(), 512.0f, 0.1f, this, SOUNDENT_CHANNEL_REPEATING );
}

//================================================================================
// He muerto
//================================================================================
void CNPC_Infected::Event_Killed( const CTakeDamageInfo &info ) 
{
	Dismembering( info );
	BaseClass::Event_Killed( info );
}

//================================================================================
// Dependiendo del daño, desmiembra al infectado
//================================================================================
void CNPC_Infected::Dismembering( const CTakeDamageInfo &info ) 
{
	// Solo por daño de bala
	if ( !(info.GetDamageType() & DMG_BULLET) )
		return;

	Vector vecDir = info.GetDamageForce();
	Vector vecSrc;
	QAngle angDir;

	int bloodAttach = -1;
	int hitGroup	= LastHitGroup();

	switch ( hitGroup )
	{
		// 3 - derecha
		// 4 - izquierda
		// 

		// 1 - derecha
		// 2 - izquierda

		// Adios cabeza
		case HITGROUP_HEAD:
			SetBodygroup( FindBodygroupByName("Head"), 4 );
			bloodAttach = GetAttachment( "Head", vecSrc, angDir );
		break;

		// Adios brazo izquierdo
		case HITGROUP_LEFTLEG:
			SetBodygroup( FindBodygroupByName("LowerBody"), 2 );
			bloodAttach = GetAttachment( "severed_LLeg", vecSrc, angDir );
		break;

		// Adios brazo derecho
		case HITGROUP_RIGHTLEG:
			SetBodygroup( FindBodygroupByName("LowerBody"), 1 );
			bloodAttach = GetAttachment( "severed_RLeg", vecSrc, angDir );
		break;
	}

	// Seguimos con nuestro cuerpo :)
	if ( bloodAttach <= -1 )
		return;

	// Soltamos más sangre
	//UTIL_BloodSpray( vecSrc, vecDir, BloodColor(), 18, FX_BLOODSPRAY_GORE );
	UTIL_BloodDrips( vecSrc, vecDir, BloodColor(), 18 );

	// Sonido Gore
	EmitSound("Infected.Hit");

	// Pintamos sangre en las paredes
	for ( int i = 0 ; i < 15; i++ )
	{
		Vector vecTraceDir = vecDir;
		vecTraceDir.x += random->RandomFloat( -5.5f, 5.5f );
		vecTraceDir.y += random->RandomFloat( -5.5f, 5.5f );
		vecTraceDir.z += random->RandomFloat( -5.5f, 5.5f );
 
		trace_t tr;
		UTIL_TraceLine( vecSrc, vecSrc + 230.0f * vecTraceDir, MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );

		if ( tr.fraction != 1.0 )
			UTIL_BloodDecalTrace( &tr, BloodColor() );
	}

	// Soltamos pedazos de carne
	CreateGoreGibs( hitGroup, vecSrc, vecDir );
}

//================================================================================
// Crea restos de la parte del cuerpo indicado.
//================================================================================
void CNPC_Infected::CreateGoreGibs( int bodyPart, const Vector & vecOrigin, const Vector & vecDir ) 
{
	CGib *pGib;

	// TODO: Manos
	switch ( bodyPart )
	{
		// Cabeza
		case HITGROUP_HEAD:
		{
			pGib = CreateGib( INFECTED_GIB_MODEL, vecOrigin, vecDir );
			pGib->SetBodygroup( pGib->FindBodygroupByName("gibs"), 3 );

			pGib = CreateGib( INFECTED_GIB_MODEL, vecOrigin, vecDir, 1 );
			pGib->SetBodygroup( pGib->FindBodygroupByName("gibs"), 4 );

			break;
		}

		// Pie izquierdo
		case HITGROUP_LEFTLEG:
		{
			pGib = CreateGib( INFECTED_GIB_MODEL, vecOrigin, vecDir );
			pGib->SetBodygroup( pGib->FindBodygroupByName("gibs"), 2 );

			break;
		}

		// Pie derecho
		case HITGROUP_RIGHTLEG:
		{
			pGib = CreateGib( INFECTED_GIB_MODEL, vecOrigin, vecDir );
			pGib->SetBodygroup( pGib->FindBodygroupByName("gibs"), 2 );

			break;
		}
	}

	// Creamos de 3 a 8 pedacitos de carne
	int iRand = RandomInt(3, 8);

	for ( int i = 2; i < iRand; ++i )
	{
		pGib = CreateGib( INFECTED_GIB_MODEL, vecOrigin, vecDir, i );
		pGib->SetBodygroup( pGib->FindBodygroupByName("gibs"), RandomInt(5, 9) );
	}
}

//================================================================================
// Traduce una animación a otra
//================================================================================
Activity CNPC_Infected::NPC_TranslateActivity( Activity baseAct )
{
    switch( baseAct )
    {
        case ACT_WALK:
            return ACT_TERROR_WALK_NEUTRAL;

        case ACT_RUN:
            return ACT_TERROR_RUN_INTENSE;

		case ACT_JUMP:
			return ACT_TERROR_JUMP;

		case ACT_LAND:
		{
			if ( m_NPCState == NPC_STATE_IDLE )
				return ACT_TERROR_JUMP_LANDING_NEUTRAL;
			else
				return ACT_TERROR_JUMP_LANDING;

			break;
		}

		case ACT_GLIDE:
			return ACT_TERROR_FALL;

        case ACT_IDLE:
		{
			// Estamos en llamas
			if ( IsOnFire() )
				return ACT_TERROR_STADING_ON_FIRE;

			if ( m_NPCState == NPC_STATE_ALERT || m_NPCState == NPC_STATE_COMBAT )
				return ACT_TERROR_IDLE_ALERT;

            return ACT_TERROR_IDLE_NEUTRAL;
		}

        case ACT_MELEE_ATTACK1:
            return ACT_TERROR_ATTACK;

		case ACT_CLIMB_DISMOUNT:
			return ACT_TERROR_LADDER_DISMOUNT;

		case ACT_CROUCH:
			return ACT_TERROR_CROUCH_RUN_INTENSE;

        case ACT_DIESIMPLE:
        case ACT_DIE_HEADSHOT:
        case ACT_DIE_GUTSHOT:
        case ACT_DIEFORWARD:
        case ACT_DIEBACKWARD:
		{
			if ( IsRunning() )
				return ACT_TERROR_DIE_WHILE_RUNNING;
			
            return ACT_TERROR_DIE_FROM_STAND;
		}
    }

    return baseAct;
}

//================================================================================
//================================================================================
bool CNPC_Infected::IsPanicked() 
{
    return ( HasCondition(COND_NEW_ENEMY) || HasCondition(COND_SEE_FEAR) || HasCondition(COND_LIGHT_DAMAGE) || HasCondition(COND_HEAVY_DAMAGE) || HasCondition(COND_PROVOKED) || HasCondition(COND_GIVE_WAY) || HasCondition(COND_HEAR_PLAYER) || HasCondition(COND_HEAR_DANGER) || HasCondition(COND_HEAR_COMBAT) || HasCondition(COND_HEAR_BULLET_IMPACT) || HasCondition(COND_IDLE_INTERRUPT) );
}

//================================================================================
// Queremos pararnos
//================================================================================
void CNPC_Infected::GoStand()
{
    // Ya estamos parados
    if ( m_iInfectedStatus == INFECTED_STAND )
        return;

    m_iDesiredStatus = INFECTED_STAND;

    m_nSitTimer.Start( RandomInt(60, 120) );
    m_nLieTimer.Start( RandomInt(120, 180) );
    m_nStandTimer.Invalidate();
}

//================================================================================
// Queremos sentarnos
//================================================================================
void CNPC_Infected::GoSit()
{
    // Ya estamos sentados
    if ( m_iInfectedStatus == INFECTED_SIT )
        return;

    m_iDesiredStatus = INFECTED_SIT;

    m_nSitTimer.Invalidate();
    m_nStandTimer.Start( RandomInt(20, 60) );
}

//================================================================================
// Queremos acostarnos
//================================================================================
void CNPC_Infected::GoLie()
{
    // Ya estamos acostados
    if ( m_iInfectedStatus == INFECTED_LIE )
        return;

    m_iDesiredStatus = INFECTED_LIE;

    m_nLieTimer.Invalidate();
    m_nStandTimer.Start( RandomInt(20, 60) );
}

//================================================================================
// Agrega un poco de Zig-Zag al movimiento del infectado
//================================================================================
bool CNPC_Infected::MovementCost( int moveType, const Vector &vecStart, const Vector &vecEnd, float *pCost ) 
{
	//NDebugOverlay::VertArrow( vecStart + Vector(0, 0, 25.0f), vecStart, 25.0f/4.0f, 255, 0, 0, 255, true, 0.15f );
	//NDebugOverlay::VertArrow( vecEnd + Vector(0, 0, 25.0f), vecEnd, 25.0f/4.0f, 0, 255, 0, 255, true, 0.15f );

	if ( GetEnemy() == NULL )
		return true;

	/*
	float	multiplier = 1.0f;

	Vector	moveDir = ( vecEnd - vecStart );
	VectorNormalize( moveDir );

	Vector	enemyDir = ( GetEnemy()->GetAbsOrigin() - vecStart );
	VectorNormalize( enemyDir );

	// If we're moving towards our enemy, then the cost is much higher than normal
	if ( DotProduct( enemyDir, moveDir ) > 0.5f )
	{
		multiplier = 5.0f;
	}

	*pCost *= multiplier;

	return ( multiplier != 1 );
	*/

	float multiplier = RandomFloat(1.0f, 7.0f);
	*pCost *= multiplier;

	return ( multiplier != 1 );
}

//================================================================================
// Comienza la ejecución de una tarea
//================================================================================
void CNPC_Infected::StartTask( const Task_t *pTask )
{
    switch( pTask->iTask )
    {
        // Ataque continuo
        case TASK_CONTINUOS_MELEE_ATTACK1:
        {
            BaseClass::StartTask( pTask );
            AttackSound();
            break;
        }

        // Vagar
        case TASK_INFECTED_WANDER:
        {
            SetIdealActivity( ACT_TERROR_WALK_NEUTRAL );
            break;
        }
            
        // Voltearse
        case TASK_INFECTED_TURN:
        {
            if ( RandomInt(0, 3) == 0 )
                TaskComplete();
            else
                SetIdealActivity( ACT_TERROR_ABOUT_FACE_NEUTRAL );

            break;
        }

        // Un nuevo enemigo!!! Argggg
        case TASK_INFECTED_WAKE_ALERT:
        {
            float flDistance = GetEnemyDistance();

            if ( RandomInt(0, 10) > 2 || flDistance < 1000.0f )
            {
                TaskComplete();
            }
            else
            {
                if ( RandomInt(0, 1) == 0 )
                    SetIdealActivity( ACT_TERROR_IDLE_ALERT_AHEAD );
                else
                    SetIdealActivity( ACT_TERROR_IDLE_ACQUIRE );
            }

            break;
        }

        // Sonido de ataque
        case TASK_ANNOUNCE_ATTACK:
        {
            TaskComplete();
            break;
        }

        // Queremos cambiar nuestro estado
        case TASK_INFECTED_GOTO_STATUS:
        {
            // Como llegamos hasta aquí?!
            if ( m_iDesiredStatus == INFECTED_NONE )
            {
                TaskFail("m_iDesiredStatus == INFECTED_NONE");
                return;
            }

            // Queremos pararnos
            if ( m_iDesiredStatus == INFECTED_STAND )
            {
                if ( m_iInfectedStatus == INFECTED_SIT )
                {
                    if ( IsPanicked() )
                        SetIdealActivity( ACT_TERROR_SIT_TO_STAND_ALERT );
                    else
                        SetIdealActivity( ACT_TERROR_SIT_TO_STAND );
                }
                else
                {
                    if ( IsPanicked() )
                        SetIdealActivity( ACT_TERROR_LIE_TO_STAND_ALERT );
                    else
                        SetIdealActivity( ACT_TERROR_LIE_TO_STAND );
                }
            }

            // Queremos sentarnos
            if ( m_iDesiredStatus == INFECTED_SIT )
            {
                if ( m_iInfectedStatus == INFECTED_STAND )
                {
                    SetIdealActivity( ACT_TERROR_SIT_FROM_STAND );
                }
                else
                {
                    SetIdealActivity( ACT_TERROR_LIE_TO_SIT );
                }
            }

            // Queremos acostarnos
            if ( m_iDesiredStatus == INFECTED_LIE )
            {
                if ( m_iInfectedStatus == INFECTED_STAND )
                {
                    SetIdealActivity( ACT_TERROR_LIE_FROM_STAND );
                }
                else
                {
                    SetIdealActivity( ACT_TERROR_SIT_TO_LIE );
                }
            }

            break;
        }

        // Mantener la animación de estado actual
        case TASK_INFECTED_KEEP_STATUS:
        {
            // Como llegamos hasta aquí?!
            if ( m_iInfectedStatus == INFECTED_STAND )
            {
                TaskFail("m_iInfectedStatus == INFECTED_STAND");
                return;
            }

            if ( m_iInfectedStatus == INFECTED_SIT )
            {
                SetIdealActivity( ACT_TERROR_SIT_IDLE );
            }
            else
            {
                SetIdealActivity( ACT_TERROR_LIE_IDLE );
            }

            break;
        }

        default:
            BaseClass::StartTask( pTask );
    }
}

//================================================================================
// Esperando a que una tarea termine
//================================================================================
void CNPC_Infected::RunTask( const Task_t *pTask )
{
    switch( pTask->iTask )
    {
        // Vagar
        case TASK_INFECTED_WANDER:
        {
            AutoMovement();
            
            // Obtenemos el vector que indica nuestro frente
            Vector vecForward;
            trace_t tr;
            AngleVectors( GetAbsAngles(), &vecForward, NULL, NULL );

            // Trazamos una línea frente a nosotros para verificar si chocamos con algo
            AI_TraceHull( WorldSpaceCenter(), WorldSpaceCenter() + 80.0f * vecForward, GetHullMins(), GetHullMaxs(), MASK_SOLID|CONTENTS_HITBOX, this, COLLISION_GROUP_NONE, &tr );

            // Hemos chocado contra alguna pared o algo
            if ( tr.fraction != 1.0f || tr.startsolid )
            {
                TaskComplete();
                return;
            }

            // La animación ha terminado
            if ( IsActivityFinished() )
                TaskComplete();

            break;
        }
        
        // Voltearse
        case TASK_INFECTED_TURN:
        {
            AutoMovement();

            // La animación ha terminado
            if ( IsActivityFinished() )
                TaskComplete();

            break;
        }

        // Un nuevo enemigo!!! Argggg
        case TASK_INFECTED_WAKE_ALERT:
        {
            AutoMovement();

            // La animación ha terminado
            if ( IsActivityFinished() )
                TaskComplete();

            break;
        }

        // Queremos cambiar nuestro estado
        case TASK_INFECTED_GOTO_STATUS:
        {
            AutoMovement();

            // La animación ha terminado
            if ( IsActivityFinished() )
            {
                m_iInfectedStatus   = m_iDesiredStatus;
                m_iDesiredStatus    = INFECTED_NONE;

                TaskComplete();
            }

            break;
        }

        // Mantener la animación de estado actual
        case TASK_INFECTED_KEEP_STATUS:
        {
            // Mantenemos la animación
            MaintainActivity();

            // Queremos cambiar de estado nuevamente...
            if ( m_iDesiredStatus != INFECTED_NONE )
                TaskComplete();

            break;
        }

        default:
            BaseClass::RunTask( pTask );
    }
}

//================================================================================
//================================================================================
int CNPC_Infected::TranslateSchedule( int iSchedule )
{
    switch( iSchedule )
    {
        // Sin hacer nada
        case SCHED_IDLE_STAND:
        {
            // Queremos cambiar nuestro estado actual
            if ( m_iDesiredStatus != INFECTED_NONE && m_iInfectedStatus != m_iDesiredStatus )
                return SCHED_INFECTED_GOTO_STATUS;

            // No estamos parados, debemos mantenernos sentados o acostados (zzzZz)
            if ( m_iInfectedStatus != INFECTED_STAND )
                return SCHED_INFECTED_KEEP_STATUS;
                
            // Vagamos por el mundo...
            return SCHED_INFECTED_IDLE;
        }

        // Hemos encontrado un enemigo
        case SCHED_WAKE_ANGRY:
        {
            return SCHED_INFECTED_WAKE_ANGRY;
        }
    }

    //SCHED_IDLE_STAND;
    //SCHED_IDLE_WALK;
    //SCHED_IDLE_WANDER;
    // TASK_WAIT_PVS
    // ACT_TERROR_FACE_LEFT_NEUTRAL

    return BaseClass::TranslateSchedule( iSchedule );
}

//================================================================================
// Inteligencia Artificial
//================================================================================
AI_BEGIN_CUSTOM_NPC( npc_infected, CNPC_Infected )
    DECLARE_ACTIVITY( ACT_TERROR_IDLE_NEUTRAL )
    DECLARE_ACTIVITY( ACT_TERROR_IDLE_ALERT )
    DECLARE_ACTIVITY( ACT_TERROR_WALK_NEUTRAL )
    DECLARE_ACTIVITY( ACT_TERROR_RUN_INTENSE )
	DECLARE_ACTIVITY( ACT_TERROR_CROUCH_RUN_INTENSE )
    DECLARE_ACTIVITY( ACT_TERROR_SHAMBLE )
    DECLARE_ACTIVITY( ACT_TERROR_ATTACK )
    DECLARE_ACTIVITY( ACT_TERROR_JUMP )
    DECLARE_ACTIVITY( ACT_TERROR_JUMP_OVER_GAP )
    DECLARE_ACTIVITY( ACT_TERROR_JUMP_LANDING_NEUTRAL )
    DECLARE_ACTIVITY( ACT_TERROR_JUMP_LANDING )
    DECLARE_ACTIVITY( ACT_TERROR_JUMP_LANDING_HARD )
	DECLARE_ACTIVITY( ACT_TERROR_LADDER_DISMOUNT )
        
    DECLARE_ACTIVITY( ACT_TERROR_ABOUT_FACE_NEUTRAL )
    DECLARE_ACTIVITY( ACT_TERROR_FACE_LEFT_NEUTRAL )
    DECLARE_ACTIVITY( ACT_TERROR_FACE_RIGHT_NEUTRAL )

    DECLARE_ACTIVITY( ACT_TERROR_IDLE_ALERT_AHEAD )
    DECLARE_ACTIVITY( ACT_TERROR_IDLE_ACQUIRE )

    DECLARE_ACTIVITY( ACT_TERROR_SIT_FROM_STAND )
    DECLARE_ACTIVITY( ACT_TERROR_LIE_FROM_STAND )

    DECLARE_ACTIVITY( ACT_TERROR_SIT_TO_STAND )
    DECLARE_ACTIVITY( ACT_TERROR_SIT_TO_STAND_ALERT )
    DECLARE_ACTIVITY( ACT_TERROR_SIT_TO_LIE )
    DECLARE_ACTIVITY( ACT_TERROR_SIT_IDLE )

    DECLARE_ACTIVITY( ACT_TERROR_LIE_TO_STAND )
    DECLARE_ACTIVITY( ACT_TERROR_LIE_TO_STAND_ALERT )
    DECLARE_ACTIVITY( ACT_TERROR_LIE_TO_SIT )
    DECLARE_ACTIVITY( ACT_TERROR_LIE_IDLE )

    DECLARE_ACTIVITY( ACT_TERROR_DIE_BACKWARD_FROM_SHOTGUN )
    DECLARE_ACTIVITY( ACT_TERROR_DIE_LEFTWARD_FROM_SHOTGUN )
    DECLARE_ACTIVITY( ACT_TERROR_DIE_RIGHTWARD_FROM_SHOTGUN )
    DECLARE_ACTIVITY( ACT_TERROR_DIE_FORWARD_FROM_SHOTGUN )

	DECLARE_ACTIVITY( ACT_TERROR_RUN_STUMBLE );

	DECLARE_ACTIVITY( ACT_EXP_IDLE );
	DECLARE_ACTIVITY( ACT_EXP_ANGRY );

    DECLARE_TASK( TASK_INFECTED_WANDER )
    DECLARE_TASK( TASK_INFECTED_TURN )
    DECLARE_TASK( TASK_INFECTED_WAKE_ALERT )
    DECLARE_TASK( TASK_INFECTED_GOTO_STATUS )
    DECLARE_TASK( TASK_INFECTED_KEEP_STATUS )

    // Vagar por el mundo
    DEFINE_SCHEDULE
    (
        SCHED_INFECTED_IDLE,

        "    Tasks"
        "        TASK_STOP_MOVING        1"
        "        TASK_INFECTED_TURN      0"
        "        TASK_INFECTED_WANDER    0"
        "        TASK_PLAY_SEQUENCE      ACTIVITY:ACT_IDLE"
        ""
        "    Interrupts"
        "        COND_NEW_ENEMY"
        "        COND_SEE_FEAR"
        "        COND_LIGHT_DAMAGE"
        "        COND_HEAVY_DAMAGE"
        "        COND_SMELL"
        "        COND_PROVOKED"
        "        COND_GIVE_WAY"
        "        COND_HEAR_PLAYER"
        "        COND_HEAR_DANGER"
        "        COND_HEAR_COMBAT"
        "        COND_HEAR_BULLET_IMPACT"
        "        COND_IDLE_INTERRUPT"
    );

    // Un enemigo!!! Cerebro!!
    DEFINE_SCHEDULE
    (
        SCHED_INFECTED_WAKE_ANGRY,

        "    Tasks"
        "        TASK_STOP_MOVING            1"
        "        TASK_SOUND_WAKE                0"
        "        TASK_FACE_ENEMY                0"
        "        TASK_INFECTED_WAKE_ALERT    0"
        ""
        "    Interrupts"
        "        COND_LIGHT_DAMAGE"
        "        COND_HEAVY_DAMAGE"
        "        COND_LOST_ENEMY"
    );

    // Queremos acostarnos, pararnos o sentarnos...
    DEFINE_SCHEDULE
    (
        SCHED_INFECTED_GOTO_STATUS,

        "    Tasks"
        "        TASK_STOP_MOVING            1"
        "        TASK_INFECTED_GOTO_STATUS    0"
        ""
        "    Interrupts"
        "        COND_NEW_ENEMY"
        "        COND_SEE_FEAR"
        "        COND_LIGHT_DAMAGE"
        "        COND_HEAVY_DAMAGE"
        "        COND_SMELL"
        "        COND_PROVOKED"
        "        COND_GIVE_WAY"
        "        COND_HEAR_PLAYER"
        "        COND_HEAR_DANGER"
        "        COND_HEAR_COMBAT"
        "        COND_HEAR_BULLET_IMPACT"
        "        COND_IDLE_INTERRUPT"

    );

    // Mantenernos dormidos o sentados
    DEFINE_SCHEDULE
    (
        SCHED_INFECTED_KEEP_STATUS,

        "    Tasks"
        "        TASK_INFECTED_KEEP_STATUS    0"
        ""
        "    Interrupts"

    );
AI_END_CUSTOM_NPC()
