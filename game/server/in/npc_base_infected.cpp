//==== Woots 2017. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "npc_base_infected.h"

#include "func_break.h"
#include "doors.h"
#include "BasePropDoor.h"
#include "func_breakablesurf.h"

#include "director.h"
#include "in_utils.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
// Información y Red
//================================================================================

LINK_ENTITY_TO_CLASS( npc_base_infected, CBaseInfected );

BEGIN_DATADESC( CBaseInfected )
END_DATADESC()

//================================================================================
//================================================================================
void CBaseInfected::Spawn()
{
	BaseClass::Spawn();

	m_bFalling = false;
	m_nFakeEnemy = NULL;
	m_nObstructionEntity = NULL;
	m_flObstructionYaw = -1.0f;
}

//================================================================================
// Pensamiento
//================================================================================
void CBaseInfected::NPCThink() 
{
	BaseClass::NPCThink();

	if ( IsAlive() )
	{
		// Animacion de caída
        // FIXME: A veces los infectados se quedan atorados en las paredes
		//UpdateFall();

		// ¿Podemos trepar una pared cercana?
		UpdateClimb();

		// ¡No sabemos nadar!
		// TODO: Animación de "ahogo"
        if ( GetWaterLevel() >= WL_Waist )
            Event_Killed( CTakeDamageInfo(this, this, 100.0f, DMG_GENERIC) );
	}
}

//================================================================================
//================================================================================
void CBaseInfected::SetCapabilities() 
{
	CapabilitiesAdd( bits_CAP_MOVE_GROUND );
    //CapabilitiesAdd( bits_CAP_MOVE_JUMP );
    CapabilitiesAdd( bits_CAP_MOVE_CLIMB );
    CapabilitiesAdd( bits_CAP_INNATE_MELEE_ATTACK1 );
}

//================================================================================
//================================================================================
bool CBaseInfected::CreateBehaviors() 
{
	m_nClimbBehavior = new CAI_ClimbBehavior();
	AddBehavior( m_nClimbBehavior );

	return BaseClass::CreateBehaviors();
}

//================================================================================
// Actualiza manualmente la animación al caer
//================================================================================
void CBaseInfected::UpdateFall() 
{
	// Estamos escalando
	if ( m_nClimbBehavior->IsClimbing() )
		return;

	// Cuando un NPC salta para llegar a su destino el código original de la IA cambia su tipo de navegación
	// a MOVETYPE_FLY, aquí verificamos que no sea un salto programado si no un salto por caida natural
	if ( GetMoveType() == MOVETYPE_FLY )
		return;

	Vector vecUp;
	GetVectors( NULL, NULL, &vecUp );

	// Trazamos una línea hacia abajo
	trace_t	tr;
	AI_TraceHull( WorldSpaceCenter(), WorldSpaceCenter() + 80.0f * -vecUp, WorldAlignMins(), WorldAlignMaxs(), MASK_NPCSOLID, this, COLLISION_GROUP_NONE, &tr );
	
	// No estamos en el suelo, ¡estamos cayendo!
	if ( !(GetFlags() & FL_ONGROUND) )
	{
		// No estamos a una caida considerable
		if ( tr.fraction != 1.0 )
			return;

		SetGravity( 0.8 );

		if ( HasActivity(ACT_GLIDE) )
			SetActivity( ACT_GLIDE );

		else if ( HasActivity(ACT_JUMP) )
			SetActivity( ACT_JUMP );

		//GetNavigator()->StopMoving();
		//GetMotor()->SetMoveInterval( 0 );

		m_bFalling = true;
	}

	// Estamos en el suelo
	else
	{
		// Estabamos cayendo, hemos aterrizado
		if ( m_bFalling )
		{
			float flTime = GetGroundChangeTime();
			AddStepDiscontinuity( flTime, GetAbsOrigin(), GetAbsAngles() );

			if ( HasActivity(ACT_LAND) )
				SetActivity( ACT_LAND );

			GetMotor()->SetMoveInterval( 0 );
			SetGravity( 1.0f );

			m_bFalling = false;
		}
	}
}

//================================================================================
// Verifica si podemos trepar una pared
//================================================================================
void CBaseInfected::UpdateClimb() 
{
	m_nClimbBehavior->Update();

	// Parece que hay una pared enfrente que puedo trepar
	if ( m_nClimbBehavior->ShouldClimb() )
	{
		if ( !GetPrimaryBehavior() )
		{
			// Cancelamos cualquier acción y pasamos el funcionamiento al CAI_ClimbBehavior
			ClearSchedule( "Starting to Climb" );
			SetPrimaryBehavior( m_nClimbBehavior );
		}
	}
	else
	{
		SetPrimaryBehavior( NULL );
	}
}

//================================================================================
// Crea y lanza una parte del cuerpo con el modelo especificado
//================================================================================
CGib * CBaseInfected::CreateGib( const char *pModelName, const Vector &vecOrigin, const Vector &vecDir, int iNum, int iSkin ) 
{
	CGib *pGib = CREATE_ENTITY( CGib, "gib" );

	pGib->Spawn( pModelName, RandomFloat(10.0f, 35.0f) );
	pGib->m_material	= matFlesh;
	pGib->m_nBody		= iNum;
	pGib->m_nSkin		= iSkin;

	pGib->InitGib( this, RandomInt(100, 200), RandomInt(200, 350) );
	pGib->SetCollisionGroup( COLLISION_GROUP_INTERACTIVE_DEBRIS );

	Vector vecNewOrigin = vecOrigin;
	vecNewOrigin.x += RandomFloat( -3.0, 3.0 );
	vecNewOrigin.y += RandomFloat( -3.0, 3.0 );
	vecNewOrigin.z += RandomFloat( -3.0, 3.0 );

	pGib->SetAbsOrigin( vecNewOrigin );
	return pGib;
}

//================================================================================
//================================================================================
const char * CBaseInfected::GetGender() 
{
	// Modelo femenino
	if ( Q_stristr(STRING(GetModelName()), "female") )
		return "female";
	
	return "male";
}

//================================================================================
// Devuelve si hay una entidad obstruyendo el camino
//================================================================================
bool CBaseInfected::HasObstruction() 
{
	CBaseEntity *pBlockingEnt = GetObstruction();

	// No hay ninguna obstrucción
	if ( !pBlockingEnt )
		return false;

	CBreakableSurface *pSurf = dynamic_cast<CBreakableSurface *>( pBlockingEnt );

	// Es una superficie de cristal y ya esta rota
	if ( pSurf && pSurf->m_bIsBroken )
		return false;

	CBasePropDoor *pPropDoor = dynamic_cast<CBasePropDoor *>( pBlockingEnt );

	// Es una puerta pero ya se ha abierto
	if ( pPropDoor && pPropDoor->IsDoorOpen() )
		return false;
	
	CBaseDoor *pDoor = dynamic_cast<CBaseDoor *>( pBlockingEnt );

	// Es una puerta pero ya se ha abierto
	if ( pDoor && (pDoor->m_toggle_state == TS_AT_TOP || pDoor->m_toggle_state == TS_GOING_UP)  )
		return false;

	return true;
}

//================================================================================
// Procesa la navegación del NPC
// Verifica si hay alguna entidad obstruyendo el camino y la marca para que
// podamos eliminarla.
//================================================================================
bool CBaseInfected::OnCalcBaseMove( AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult ) 
{
	CBaseEntity *pObstruction = pMoveGoal->directTrace.pObstruction;

	// No hay ninguna entidad obstruyendo nuestro camino
	if ( !pObstruction )
		return false;

	// Solo nos interesan objetos
	if ( pObstruction->IsPlayer() || pObstruction->IsNPC() || pObstruction->MyCombatCharacterPointer() )
		return false;
	
	// Algo esta obstruyendo nuestro camino (y podemos romperlo/quitarlo)
	if ( CanDestroyObstruction(pMoveGoal, pObstruction, distClear, pResult) )
	{
		// Mientras no estemos muy cerca, marquemos el camino como "libre"
		if ( distClear < 0.1 )
		{
			*pResult = ( pObstruction->IsWorld() ) ? AIMR_BLOCKED_WORLD : AIMR_BLOCKED_ENTITY;
		}
		else
		{
			pMoveGoal->maxDist = distClear;
			*pResult = AIMR_OK;
		}

		// Estamos muy cerca, hora de romper el bloqueo
		if ( IsMoveBlocked(*pResult) )
		{
			m_nObstructionEntity	= pObstruction;
			m_flObstructionYaw		= -1.0f;

			if ( pMoveGoal->directTrace.vHitNormal.IsValid() )
				m_flObstructionYaw = UTIL_VecToYaw( pMoveGoal->directTrace.vHitNormal * -1 );				
		}

		return true;
	}

	return false;
}

//================================================================================
// Devuelve si es posible eliminar la obstrucción
//================================================================================
bool CBaseInfected::CanDestroyObstruction( AILocalMoveGoal_t *pMoveGoal, CBaseEntity *pEntity, float distClear, AIMoveResult_t *pResult ) 
{
	CBasePropDoor *pPropDoor = dynamic_cast<CBasePropDoor *>( pEntity );

	// Podemos romper/abrir la puerta
	if ( pPropDoor && OnUpcomingPropDoor(pMoveGoal, pPropDoor, distClear, pResult) )
		return true;

	// Podemos romper la pared
	if ( Utils::IsBreakableSurf(pEntity) )
		return true;

	return false;
}

//================================================================================
// Hay una puerta obstruyendo nuestro camino
//================================================================================
bool CBaseInfected::OnObstructingDoor( AILocalMoveGoal_t * pMoveGoal, CBaseDoor * pDoor, float distClear, AIMoveResult_t * pResult ) 
{
	if ( BaseClass::OnObstructingDoor(pMoveGoal, pDoor, distClear, pResult) )
	{
		// Establecemos la puerta como nuestro destino a romper
		if ( IsMoveBlocked(*pResult) && pMoveGoal->directTrace.vHitNormal.IsValid() )
		{
			m_nObstructionEntity	= pDoor;
			m_flObstructionYaw		= UTIL_VecToYaw( pMoveGoal->directTrace.vHitNormal * -1 );
		}

		return true;
	}

	return false;
}

//================================================================================
// Devuelve si el NPC esta a punto de chocar con una pared
//================================================================================
bool CBaseInfected::IsHittingWall( float flDistance, int height ) 
{
	// Obtenemos el vector que indica "enfrente de mi"
	Vector vecForward, vecSrc, vecEnd;
	GetVectors( &vecForward, NULL, NULL );

	vecSrc = WorldSpaceCenter() + Vector( 0, 0, height );
	vecEnd = vecSrc + flDistance * vecForward;

	trace_t	tr;
	AI_TraceHull( vecSrc, vecEnd, WorldAlignMins(), WorldAlignMaxs(), MASK_NPCSOLID, this, COLLISION_GROUP_NONE, &tr );

	return ( tr.fraction != 1.0f || tr.startsolid );
}

//================================================================================
//================================================================================
int CBaseInfected::MeleeAttack1Conditions( float flDot, float flDist ) 
{
	m_nFakeEnemy = NULL;

	 if ( !GetEnemy() )
        return COND_NONE;

	 if ( flDot < 0.7 )
        return COND_NONE;

	 if ( flDist <= GetMeleeDistance() )
		 return COND_CAN_MELEE_ATTACK1;

	// Hull
	Vector vecMins	= GetHullMins();
	Vector vecMaxs	= GetHullMaxs();
	vecMins.z		= vecMins.x;
	vecMaxs.z		= vecMaxs.x;

	// Obtenemos el vector que indica "enfrente de mi"
	Vector vecForward;
	GetVectors( &vecForward, NULL, NULL );

	// Trazamos una línea y verificamos si hemos dado con algo
	trace_t	tr;
	AI_TraceHull( WorldSpaceCenter(), WorldSpaceCenter() + GetMeleeDistance() * vecForward, vecMins, vecMaxs, MASK_NPCSOLID, this, COLLISION_GROUP_NONE, &tr );

	// Hemos dado con algo
	if ( tr.fraction != 1.0 && tr.m_pEnt )
	{
		m_nFakeEnemy = tr.m_pEnt;

		// Podemos atacar a esto
		if ( CanAttackEntity(m_nFakeEnemy) )
			return COND_CAN_MELEE_ATTACK1;
	}

	// Siempre devolvemos que esta lejos para que el infectado
	// siga corriendo.
	return COND_TOO_FAR_TO_ATTACK;
}

//================================================================================
//================================================================================
bool CBaseInfected::CanAttackEntity( CBaseEntity *pEntity ) 
{
	// Es mi enemigo
	if ( pEntity == GetEnemy() )
		return true;

	if ( pEntity->MyCombatCharacterPointer() )
		return false;

	// Es algo que se puede romper
	if ( Utils::IsBreakable(pEntity) )
		return true;

	// Es algo que se puede mover (Pero no necesariamente algo ligero)
	if ( Utils::IsMoveableObject(pEntity) )
		return true;

	return false;
}

//================================================================================
//================================================================================
void CBaseInfected::GatherConditions() 
{
	BaseClass::GatherConditions();

	// Limpiamos las condiciones
	static int conditionsToClear[] = 
	{
		COND_BLOCKED_BY_OBSTRUCTION,
		COND_OBSTRUCTION_FREE,
		COND_ENEMY_REACHABLE
	};

	ClearConditions( conditionsToClear, ARRAYSIZE(conditionsToClear) );

	// No tenemos ninguna obstrucción
	if ( !HasObstruction() )
	{
		SetCondition( COND_OBSTRUCTION_FREE );

		// Limpiamos
		if ( GetObstruction() )
			m_nObstructionEntity = NULL;
	}
	else
	{
		SetCondition( COND_BLOCKED_BY_OBSTRUCTION );
	}

	if ( GetEnemy() && !IsUnreachable(GetEnemy()) )
		SetCondition( COND_ENEMY_REACHABLE );
}

//================================================================================
// Hemos fallado en nuestra tarea ¿que debemos hacer?
//================================================================================
int CBaseInfected::SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode ) 
{
	// Nuestro camino ha sido obstruido por algo
	if ( HasCondition(COND_BLOCKED_BY_OBSTRUCTION) && GetObstruction() && failedSchedule != SCHED_ATTACK_OBSTRUCTION )
	{
		ClearCondition( COND_BLOCKED_BY_OBSTRUCTION );
		return SCHED_ATTACK_OBSTRUCTION;
	}

	return BaseClass::SelectFailSchedule( failedSchedule, failedTask, taskFailCode );
}

//================================================================================
// Comienza la ejecución de una tarea
//================================================================================
void CBaseInfected::StartTask( const Task_t *pTask ) 
{
	switch( pTask->iTask )
    {
		// Giramos hacia la obstrucción
		case TASK_YAW_TO_OBSTRUCTION:
		{
			if ( GetObstruction() && m_flObstructionYaw > -1 )
			{
				GetMotor()->SetIdealYaw( m_flObstructionYaw );
			}

			TaskComplete();
			break;
		}

		// Atacar la obstrucción
		case TASK_ATTACK_OBSTRUCTION:
		{
			if ( Utils::IsDoor(GetObstruction()) )
			{
				SetIdealActivity( ACT_TERROR_ATTACK_DOOR_CONTINUOUSLY );
			}
			else
			{
				SetIdealActivity( ACT_TERROR_ATTACK_CONTINUOUSLY );
			}

			break;
		}

		// Buscar la mejor ruta para perseguir al enemigo
		/*
		case TASK_GET_CHASE_PATH_TO_ENEMY:
		{
			CBaseEntity *pEnemy = GetEnemy();

			// No tenemos ningún enemigo
			if ( !pEnemy )
			{
				TaskFail( FAIL_NO_ROUTE );
				return;
			}

			sharedtasks_e iTask = TASK_GET_PATH_TO_ENEMY;

			// No estamos en el Climax, seremos realistas
			if ( !TheDirector->IsStatus(STATUS_FINALE) )
			{
				// Si estamos viendo a nuestro enemigo vayamos directamente hacia donde esta, 
				// de otra forma solo hacia el último lugar donde lo vimos

				if ( IsInFieldOfView(pEnemy) && FVisible(pEnemy) )
					iTask = TASK_GET_PATH_TO_ENEMY;
				else
					iTask = TASK_GET_PATH_TO_ENEMY_LKP;
			}

			ChainStartTask( iTask );

			// No hemos podido ir
			if ( !TaskIsComplete() && !HasCondition(COND_TASK_FAILED) )
				TaskFail( FAIL_NO_ROUTE );

			break;
		}
		*/

		default:
		{
			BaseClass::StartTask( pTask );
			break;
		}
	}

}

//================================================================================
// Esperando a que una tarea termine
//================================================================================
void CBaseInfected::RunTask( const Task_t *pTask ) 
{
	switch( pTask->iTask )
    {
		// Atacar la obstrucción
		case TASK_ATTACK_OBSTRUCTION:
		{
			if ( IsActivityFinished() )
				TaskComplete();

			break;
		}

		default:
		{
			BaseClass::RunTask( pTask );
			break;
		}
	}
}

//================================================================================
// Traduce una tarea registrada a otra
//================================================================================
int CBaseInfected::TranslateSchedule( int scheduleType ) 
{
	switch( scheduleType )
	{
		// Atacamos
        case SCHED_MELEE_ATTACK1:
        {
            return SCHED_CONTINUOUS_MELEE_ATTACK1;
        }

		// No hemos podido llegar a nuestro enemigo
		case SCHED_CHASE_ENEMY_FAILED:
		{
			// Si el enemigo esta muy cerca solo nos quedamos esperando ¡¿que hacemos?!
			if ( GetEnemy() )
			{
				if ( GetEnemyDistance() < 400 )
					return SCHED_STANDOFF;
			}

			// Animación de "no puedo alcanzarte, GRRR!"
			return SCHED_UNABLE_TO_REACH;
		}
	}

	return BaseClass::TranslateSchedule( scheduleType );
}

//================================================================================
// Inteligencia Artificial
//================================================================================

AI_BEGIN_CUSTOM_NPC( npc_base_infected, CBaseInfected )
	DECLARE_TASK( TASK_YAW_TO_OBSTRUCTION )
	DECLARE_TASK( TASK_ATTACK_OBSTRUCTION )

	DECLARE_CONDITION( COND_BLOCKED_BY_OBSTRUCTION )
	DECLARE_CONDITION( COND_OBSTRUCTION_FREE )
	DECLARE_CONDITION( COND_ENEMY_REACHABLE )

	DEFINE_SCHEDULE
	(
		SCHED_ATTACK_OBSTRUCTION,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_TAKE_COVER_FROM_ENEMY"
		"		TASK_YAW_TO_OBSTRUCTION			0"
		"		TASK_FACE_IDEAL					0"
		"		TASK_ATTACK_OBSTRUCTION			0"
		"		TASK_SET_SCHEDULE				SCHEDULE:SCHED_ATTACK_OBSTRUCTION"
		"	"
		"	Interrupts"
		"		COND_OBSTRUCTION_FREE"
		"		COND_HEAVY_DAMAGE"
		"		COND_NEW_ENEMY"
	)

	DEFINE_SCHEDULE
	(
		SCHED_UNABLE_TO_REACH,

		"	Tasks"
		"		TASK_STOP_MOVING				0"
		"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_STANDOFF"
		"		TASK_PLAY_SEQUENCE				ACTIVITY:ACT_TERROR_UNABLE_TO_REACH_TARGET"
		"	"
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_ENEMY_DEAD"
		"		COND_CAN_RANGE_ATTACK1"
		"		COND_CAN_MELEE_ATTACK1"
		"		COND_CAN_RANGE_ATTACK2"
		"		COND_CAN_MELEE_ATTACK2"
		"		COND_HEAR_DANGER"
		"		COND_BETTER_WEAPON_AVAILABLE"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"       COND_ENEMY_REACHABLE"
	)
AI_END_CUSTOM_NPC()