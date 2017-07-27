//==== Woots 2017. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef NPC_BASE_INFECTED_H
#define NPC_BASE_INFECTED_H

#ifdef _WIN32
#pragma once
#endif

#include "npc_basein.h"
#include "ai_behavior_climb.h"
#include "gib.h"

//CAI_BlendedMotor;
//CAI_BlendingHost;
//CAI_BlendedNPC;

typedef CAI_BehaviorHost<CBaseNPC> CBaseNPCEnhanced;

class CBaseInfected : public CBaseNPCEnhanced
{
public:
	DECLARE_CLASS( CBaseInfected, CBaseNPCEnhanced );
	DECLARE_DATADESC();
	DEFINE_CUSTOM_AI;

	enum
	{
		TASK_YAW_TO_OBSTRUCTION = NEXT_TASK,
		TASK_ATTACK_OBSTRUCTION,
		NEXT_TASK
	};

	enum
	{
		SCHED_ATTACK_OBSTRUCTION = NEXT_SCHEDULE,
		SCHED_UNABLE_TO_REACH,
		NEXT_SCHEDULE
	};

	enum
	{
		COND_BLOCKED_BY_OBSTRUCTION = NEXT_CONDITION,
		COND_OBSTRUCTION_FREE,
		COND_ENEMY_REACHABLE,
		NEXT_CONDITION
	};

	// Información
	virtual	bool AllowedToIgnite() { return true; }
	Class_T Classify() { return CLASS_INFECTED; }

	// Principales
	virtual void Spawn();
	virtual void NPCThink();
	virtual void SetCapabilities();

	virtual bool CreateBehaviors();

	virtual void UpdateFall();
	virtual void UpdateClimb();

	virtual CGib *CreateGib( const char *pModelName, const Vector &vecOrigin, const Vector &vecDir, int iNum = 0, int iSkin = 0 );
	virtual const char *GetGender();

	// Obstrucción
	virtual CBaseEntity *GetObstruction() { return m_nObstructionEntity.Get(); }
	virtual bool HasObstruction();

	virtual bool OnCalcBaseMove( AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult );

	virtual bool CanDestroyObstruction( AILocalMoveGoal_t *pMoveGoal, CBaseEntity *pEntity, float distClear, AIMoveResult_t *pResult );
	virtual bool OnObstructingDoor( AILocalMoveGoal_t *pMoveGoal, CBaseDoor *pDoor, float distClear, AIMoveResult_t *pResult );
	//virtual bool OnUpcomingPropDoor( AILocalMoveGoal_t *pMoveGoal, CBasePropDoor *pDoor, float distClear, AIMoveResult_t *pResult );

	virtual bool IsHittingWall( float flDistance, int iHeight );

	// Ataque
	virtual int MeleeAttack1Conditions( float flDot, float flDist );

	virtual bool CanAttackEntity( CBaseEntity *pEntity );

	// Inteligencia Artificial
	virtual void GatherConditions();
	virtual int SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode );

	virtual void StartTask( const Task_t *pTask );
    virtual void RunTask( const Task_t *pTask );

	virtual int TranslateSchedule( int scheduleType );


protected:
	bool m_bFalling;

	EHANDLE m_nFakeEnemy;
	CAI_ClimbBehavior *m_nClimbBehavior;

	EHANDLE m_nObstructionEntity;
	float m_flObstructionYaw;
	
};

#endif // NPC_BASE_INFECTED_H