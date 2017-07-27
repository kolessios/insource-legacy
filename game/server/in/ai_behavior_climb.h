//==== Woots 2017. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef AI_BEHAVIOR_CLIMB_H
#define AI_BEHAVIOR_CLIMB_H

#ifdef _WIN32
#pragma once
#endif

#include "ai_behavior.h"

//================================================================================
// Permite escalar muros y paredes
//================================================================================
class CAI_ClimbBehavior : public CAI_SimpleBehavior
{
public:
	DECLARE_CLASS( CAI_ClimbBehavior, CAI_SimpleBehavior );
	//DECLARE_DATADESC();
	DEFINE_CUSTOM_SCHEDULE_PROVIDER;

	enum
	{
		COND_CLIMB_START = BaseClass::NEXT_CONDITION,
		NEXT_CONDITION,

		TASK_CLIMB_START = BaseClass::NEXT_TASK,
		TASK_FACE_CLIMB,
		NEXT_TASK,

		SCHED_CLIMB_START = BaseClass::NEXT_SCHEDULE,
		NEXT_SCHEDULE,
	};

	CAI_ClimbBehavior();

	virtual const char *GetName() {	return "Climb Walls"; }
	virtual bool IsClimbing() { return m_bIsClimbing; }

	// Principales
	virtual Activity GetClimbActivity( int height );

	// Detección
	virtual void Update();
	virtual bool ShouldClimb();

	virtual bool IsHittingWall( float flDistance, int iHeight, trace_t *tr );
	virtual void GetTraceWall( int height, trace_t *tr );
	
	// Escalamos!
	virtual void ClimbStart();
	virtual void ClimbRun();
	virtual void ClimbStop();

	// Inteligencia Artificial
	virtual void GatherConditions();
	virtual int SelectSchedule();

	virtual void StartTask( const Task_t *pTask );
    virtual void RunTask( const Task_t *pTask );

protected:
	int m_iClimbHeight;
	float m_flClimbYaw;
	bool m_bIsClimbing;
};

#endif // AI_BEHAVIOR_CLIMB_H