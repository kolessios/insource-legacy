//==== Woots 2017. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "ai_behavior_climb.h"
#include "ai_moveprobe.h"


#include "in_utils.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
// Comandos
//================================================================================

DECLARE_NOTIFY_CMD( ai_climb_behavior_enable, "1", "" )
DECLARE_CHEAT_CMD( ai_climb_behavior_debug, "0", "" )
DECLARE_NOTIFY_CMD( ai_climb_behavior_front_distance, "30", "" )
DECLARE_NOTIFY_CMD( ai_climb_behavior_interval, "1.0", "" )

//================================================================================
// Alturas que podemos escalar
//================================================================================
int g_ClimbHeights[] =
{
	24,
	36,
	48,
	60,
	72,
	84,
	96,
	108,
	120,
	132,
	144,
	156,
	168,

	38,
	50,
	70,
	115,
	130,
	150,
	166,
};

#define CLIMB_ACTIVITY( height ) ACT_TERROR_CLIMB_##height##_FROM_STAND

//================================================================================
// Animaciones
//================================================================================

Activity ACT_TERROR_CLIMB_24_FROM_STAND;
Activity ACT_TERROR_CLIMB_36_FROM_STAND;
Activity ACT_TERROR_CLIMB_48_FROM_STAND;
Activity ACT_TERROR_CLIMB_60_FROM_STAND;
Activity ACT_TERROR_CLIMB_72_FROM_STAND;
Activity ACT_TERROR_CLIMB_84_FROM_STAND;
Activity ACT_TERROR_CLIMB_96_FROM_STAND;
Activity ACT_TERROR_CLIMB_108_FROM_STAND;
Activity ACT_TERROR_CLIMB_120_FROM_STAND;
Activity ACT_TERROR_CLIMB_132_FROM_STAND;
Activity ACT_TERROR_CLIMB_144_FROM_STAND;
Activity ACT_TERROR_CLIMB_156_FROM_STAND;
Activity ACT_TERROR_CLIMB_168_FROM_STAND;

Activity ACT_TERROR_CLIMB_38_FROM_STAND;
Activity ACT_TERROR_CLIMB_50_FROM_STAND;
Activity ACT_TERROR_CLIMB_70_FROM_STAND;
Activity ACT_TERROR_CLIMB_115_FROM_STAND;
Activity ACT_TERROR_CLIMB_130_FROM_STAND;
Activity ACT_TERROR_CLIMB_150_FROM_STAND;
Activity ACT_TERROR_CLIMB_166_FROM_STAND;

//================================================================================
// Constructor
//================================================================================
CAI_ClimbBehavior::CAI_ClimbBehavior()
{
	m_iClimbHeight = -1;
	m_flClimbYaw = -1.0f;
	m_bIsClimbing = false;
}

//================================================================================
// Devuelve la animación adecuada para la altura especificada
//================================================================================
Activity CAI_ClimbBehavior::GetClimbActivity( int height ) 
{
	switch ( height )
	{
		case 24:
			return CLIMB_ACTIVITY( 24 );
		break;

		case 36:
			return CLIMB_ACTIVITY( 36 );
		break;

		case 38:
			return CLIMB_ACTIVITY( 38 );
		break;

		case 48:
			return CLIMB_ACTIVITY( 48 );
		break;

		case 50:
			return CLIMB_ACTIVITY( 50 );
		break;

		case 60:
			return CLIMB_ACTIVITY( 60 );
		break;

		case 70:
			return CLIMB_ACTIVITY( 70 );
		break;

		case 72:
			return CLIMB_ACTIVITY( 72 );
		break;

		case 84:
			return CLIMB_ACTIVITY( 84 );
		break;

		case 96:
			return CLIMB_ACTIVITY( 96 );
		break;

		case 108:
			return CLIMB_ACTIVITY( 108 );
		break;

		case 115:
			return CLIMB_ACTIVITY( 115 );
		break;

		case 120:
			return CLIMB_ACTIVITY( 120 );
		break;

		case 130:
			return CLIMB_ACTIVITY( 130 );
		break;

		case 132:
			return CLIMB_ACTIVITY( 132 );
		break;

		case 144:
			return CLIMB_ACTIVITY( 144 );
		break;

		case 150:
			return CLIMB_ACTIVITY( 150 );
		break;

		case 156:
			return CLIMB_ACTIVITY( 156 );
		break;

		case 166:
			return CLIMB_ACTIVITY( 166 );
		break;

		default:
			return CLIMB_ACTIVITY( 168 );
		break;
	}
}

//================================================================================
// Actualiza y declara si podemos escalar una pared delante nuestra
//================================================================================
void CAI_ClimbBehavior::Update() 
{
	// Ya tratamos de escalar
	if ( ShouldClimb() )
		return;

    if ( !(GetOuter()->GetFlags() & FL_ONGROUND) )
    {
        m_iClimbHeight = -1;
        m_flClimbYaw = -1.0f;
        return;
    }

    // Verificamos si una pared/objeto esta enfrente de nosotros a 20 unidades de altura
    trace_t hitting_trace;
    bool hitting = IsHittingWall( ai_climb_behavior_front_distance.GetFloat(), 20, &hitting_trace );

    if ( hitting_trace.m_pEnt )
    {
        // Es un objeto que se mueve, no lo contamos
        if ( Utils::IsMoveableObject( hitting_trace.m_pEnt ) )
            hitting = false;
    }
    
	if ( hitting )
	{
        // Depuración
        if ( ai_climb_behavior_debug.GetBool() )
        {
            if ( hitting_trace.m_pEnt->IsWorld() )
                NDebugOverlay::Box( hitting_trace.endpos, Vector( -5, -5, -5 ), Vector( 5, 5, 5 ), 255, 0, 0, 1.0f, 4.0f );
            else
                NDebugOverlay::Box( hitting_trace.m_pEnt->GetAbsOrigin(), hitting_trace.m_pEnt->WorldAlignMins(), hitting_trace.m_pEnt->WorldAlignMaxs(), 255, 0, 0, 1.0f, 4.0f );
        }

		// Verificamos de que altura es la pared
		for ( int i = 0; i < ARRAYSIZE(g_ClimbHeights); ++i )
		{
			// Probamos con esta altura
			int height	= g_ClimbHeights[i];
			int check	= height + 3;

			//Vector vecSrc = GetAbsOrigin() + Vector( 0, 0, check );
			//NDebugOverlay::Text( vecSrc, UTIL_VarArgs("%i (%i)", height, check), false, 0.5f );

            trace_t check_tr;

			// La pared es más grande que esta altura (o algo esta bloqueandolo)
            if ( IsHittingWall(180.0f, check, &check_tr) )
            {
                // Es un objeto que se mueve, terminamos todo.
                if ( Utils::IsMoveableObject( check_tr.m_pEnt ) )
                {
                    if ( ai_climb_behavior_debug.GetBool() )
                        NDebugOverlay::Text( GetOuter()->EyePosition(), UTIL_VarArgs( "Utils::IsMoveableObject!!!!!" ), false, 3.0f );

                    break;
                }


                continue;
            }

			// Obtenemos la animación que corresponde para escalar esta pared
			Activity iActivity = GetClimbActivity( height );

			// Algunas animaciones son especiales para diferentes tipos
			// de NPC
			if ( GetOuter()->SelectWeightedSequence(iActivity) == ACTIVITY_NOT_AVAILABLE )
				continue;

			// Obtenemos un trace cuya posición termine en una parte de la pared
			// La usaremos para poder obtener los angulos donde debemos mirar para empezar a escalar
			trace_t tr;
			GetTraceWall( height, &tr );

            Vector vecNormal = (tr.plane.normal * -1);

			// Ideal para escalar
			m_iClimbHeight	= height;
			m_flClimbYaw	= UTIL_VecToYaw( vecNormal );

            if ( ai_climb_behavior_debug.GetBool() )
            {
                NDebugOverlay::Line( GetOuter()->EyePosition(), GetOuter()->EyePosition() + 30.0f * vecNormal, 0, 0, 255, true, 4.0f );
            }

			// Paramos de inmediato
			GetNavigator()->StopMoving();

			// Debug!
			if ( ai_climb_behavior_debug.GetBool() )
				NDebugOverlay::Text( GetOuter()->EyePosition(), UTIL_VarArgs("ESCALAMOS: %i - Yaw: %f - *EMPEZANDO*", height, m_flClimbYaw), false, 3.0f );

			return;
		}

        if ( ai_climb_behavior_debug.GetBool() )
            NDebugOverlay::Text( GetOuter()->EyePosition(), UTIL_VarArgs( "NO HE ENCONTRADO UNA ALTURA PARA ESCALAR" ), false, 3.0f );
	}

	m_iClimbHeight = -1;
	m_flClimbYaw = -1.0f;
}

//================================================================================
// Devuelve si podemos escalar una pared
//================================================================================
bool CAI_ClimbBehavior::ShouldClimb() 
{
	if ( !ai_climb_behavior_enable.GetBool() )
		return false;

	return ( m_iClimbHeight >= 24 );
}

//================================================================================
// Devuelve si el NPC esta a punto de chocar con una pared
//================================================================================
bool CAI_ClimbBehavior::IsHittingWall( float flDistance, int height, trace_t *tr )
{
	// Obtenemos el vector que indica "enfrente de mi"
	Vector vecForward, vecSrc, vecEnd;
	GetOuter()->GetVectors( &vecForward, NULL, NULL );

    // Lugar donde empezamos
	vecSrc = GetAbsOrigin() + Vector( 0, 0, height );

    // Lugar donde termina
	vecEnd = vecSrc + flDistance * vecForward;

	AI_TraceHull( vecSrc, vecEnd, GetOuter()->GetHullMins(), GetOuter()->GetHullMaxs(), MASK_NPCSOLID, GetOuter(), COLLISION_GROUP_NONE, tr );
	return ( !tr->startsolid && tr->fraction < 1.0f );
}

//================================================================================
//================================================================================
void CAI_ClimbBehavior::GetTraceWall( int height, trace_t *tr ) 
{
	// Obtenemos el vector que indica "enfrente de mi"
	Vector vecForward, vecSrc, vecEnd;
	GetOuter()->GetVectors( &vecForward, NULL, NULL );

	vecSrc = GetAbsOrigin() + Vector( 0, 0, (height - 10.0f) );
	vecEnd = vecSrc + 50.0f * vecForward;

	AI_TraceHull( vecSrc, vecEnd, GetOuter()->GetHullMins(), GetOuter()->GetHullMaxs(), MASK_NPCSOLID, GetOuter(), COLLISION_GROUP_NONE, tr );
}

//================================================================================
//================================================================================
void CAI_ClimbBehavior::ClimbStart() 
{
	// Nos movemos un poco más adelante
	/*Vector vecForward;
	GetOuter()->GetVectors( &vecForward, NULL, NULL );
	GetOuter()->SetAbsOrigin( GetAbsOrigin() + 8.0f * vecForward );*/

	// No debemos tener gravedad
	GetOuter()->AddFlag( FL_FLY );
	//SetSolid( SOLID_BBOX );
	//SetGravity( 0.0 );
	SetGroundEntity( NULL );

	// Establecemos la animación adecuada
	GetOuter()->SetIdealActivity( GetClimbActivity(m_iClimbHeight) );
	m_bIsClimbing = true;
}

//================================================================================
//================================================================================
void CAI_ClimbBehavior::ClimbRun() 
{
	GetOuter()->AutoMovement();
}

//================================================================================
//================================================================================
void CAI_ClimbBehavior::ClimbStop() 
{
	// Restauramos la gravedad
	GetOuter()->RemoveFlag( FL_FLY );
	SetGravity( 1.0 );

	m_bIsClimbing	= false;
	m_iClimbHeight	= -1;
	m_flClimbYaw	= -1.0f;

	// Debug!
	if ( ai_climb_behavior_debug.GetBool() )
		NDebugOverlay::Text( GetOuter()->EyePosition(), UTIL_VarArgs("*TERMINADO*"), false, 3.0f );
}

//================================================================================
//================================================================================
void CAI_ClimbBehavior::GatherConditions() 
{
	// Podemos escalar, empezamos
	if ( ShouldClimb() )
	{
		SetCondition( COND_CLIMB_START );
	}
	else
	{
		ClearCondition( COND_CLIMB_START );
	}

	BaseClass::GatherConditions();
}

//================================================================================
//================================================================================
int CAI_ClimbBehavior::SelectSchedule() 
{
	if ( HasCondition(COND_CLIMB_START) )
		return SCHED_CLIMB_START;

	return BaseClass::SelectSchedule();
}

//================================================================================
//================================================================================
void CAI_ClimbBehavior::StartTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
		// Empezamos a escalar
		case TASK_CLIMB_START:
		{
			ClimbStart();
			break;
		}

        // Miramos hacia la pared
		case TASK_FACE_CLIMB:
		{
			GetMotor()->SetIdealYaw( m_flClimbYaw );
			GetOuter()->SetTurnActivity();
			break;
		}

		default:
		{
			BaseClass::StartTask( pTask );
			break;
		}
	}
}

//================================================================================
//================================================================================
void CAI_ClimbBehavior::RunTask( const Task_t *pTask ) 
{
	switch ( pTask->iTask )
	{
		// Empezamos a escalar
		case TASK_CLIMB_START:
		{
			ClimbRun();

			// La animación ha terminado
			if ( GetOuter()->IsActivityFinished() )
			{
				ClimbStop();
				TaskComplete();
			}

			break;
		}

		case TASK_FACE_CLIMB:
		{
			// If the yaw is locked, this function will not act correctly
			Assert( GetMotor()->IsYawLocked() == false );

			GetMotor()->SetIdealYaw( m_flClimbYaw );
			GetMotor()->UpdateYaw();

			if ( GetOuter()->FacingIdeal( 5.0f ) )
			{
				TaskComplete();
			}

			break;
		}

		default:
		{
			BaseClass::StartTask( pTask );
			break;
		}
	}
}

AI_BEGIN_CUSTOM_SCHEDULE_PROVIDER( CAI_ClimbBehavior )
	DECLARE_CONDITION( COND_CLIMB_START )

	DECLARE_TASK( TASK_CLIMB_START )
	DECLARE_TASK( TASK_FACE_CLIMB )

	DECLARE_ACTIVITY( ACT_TERROR_CLIMB_24_FROM_STAND )
	DECLARE_ACTIVITY( ACT_TERROR_CLIMB_36_FROM_STAND )
	DECLARE_ACTIVITY( ACT_TERROR_CLIMB_48_FROM_STAND )
	DECLARE_ACTIVITY( ACT_TERROR_CLIMB_60_FROM_STAND )
	DECLARE_ACTIVITY( ACT_TERROR_CLIMB_72_FROM_STAND )
	DECLARE_ACTIVITY( ACT_TERROR_CLIMB_84_FROM_STAND )
	DECLARE_ACTIVITY( ACT_TERROR_CLIMB_96_FROM_STAND )
	DECLARE_ACTIVITY( ACT_TERROR_CLIMB_108_FROM_STAND )
	DECLARE_ACTIVITY( ACT_TERROR_CLIMB_120_FROM_STAND )
	DECLARE_ACTIVITY( ACT_TERROR_CLIMB_132_FROM_STAND )
	DECLARE_ACTIVITY( ACT_TERROR_CLIMB_144_FROM_STAND )
	DECLARE_ACTIVITY( ACT_TERROR_CLIMB_156_FROM_STAND )
	DECLARE_ACTIVITY( ACT_TERROR_CLIMB_168_FROM_STAND )

	DECLARE_ACTIVITY( ACT_TERROR_CLIMB_38_FROM_STAND )
	DECLARE_ACTIVITY( ACT_TERROR_CLIMB_50_FROM_STAND )
	DECLARE_ACTIVITY( ACT_TERROR_CLIMB_70_FROM_STAND )
	DECLARE_ACTIVITY( ACT_TERROR_CLIMB_115_FROM_STAND )
	DECLARE_ACTIVITY( ACT_TERROR_CLIMB_130_FROM_STAND )
	DECLARE_ACTIVITY( ACT_TERROR_CLIMB_150_FROM_STAND )
	DECLARE_ACTIVITY( ACT_TERROR_CLIMB_166_FROM_STAND )

	DEFINE_SCHEDULE
	(
		SCHED_CLIMB_START,

		"	Tasks"
		"		 TASK_FACE_CLIMB	  0"
		"		 TASK_CLIMB_START	  0"
		""
		"	Interrupts"
	)
AI_END_CUSTOM_SCHEDULE_PROVIDER()