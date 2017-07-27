//==== InfoSmart 2015. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef NPC_INFECTED_H
#define NPC_INFECTED_H

#ifdef _WIN32
#pragma once
#endif

#include "npc_base_infected.h"

//================================================================================
// Representa un Infectado en Apocalypse
//================================================================================
class CNPC_Infected : public CBaseInfected
{
public:
    DECLARE_CLASS( CNPC_Infected, CBaseInfected );
    DECLARE_DATADESC();
    DEFINE_CUSTOM_AI;

    enum
    {
        TASK_INFECTED_WANDER = NEXT_TASK,
        TASK_INFECTED_TURN,
        TASK_INFECTED_WAKE_ALERT,
            
        TASK_INFECTED_GOTO_STATUS,
        TASK_INFECTED_KEEP_STATUS,

        NEXT_TASK,
    };

    enum 
    {
        SCHED_INFECTED_IDLE = NEXT_SCHEDULE,
        SCHED_INFECTED_WAKE_ANGRY,

        SCHED_INFECTED_GOTO_STATUS,
        SCHED_INFECTED_KEEP_STATUS,

        NEXT_SCHEDULE
    };

    enum
    {
        INFECTED_NONE,
        INFECTED_STAND,
        INFECTED_SIT,
        INFECTED_LIE
    };

    // Información
    virtual int GetNPCHealth();
    virtual const char *GetNPCModel();

    virtual float GetMeleeDistance();
    virtual float GetMeleeDamage();

    // Principales
    virtual void Spawn();
    virtual void NPCThink();

    virtual void Precache();
    virtual void SetUpModel();

	virtual void UpdateGesture();
	virtual void IsSomeoneOnTop();

	virtual void UpdateFall();
	virtual void UpdateClimb();

    // Sonidos
    virtual void PainSound( const CTakeDamageInfo & );
    virtual void DeathSound( const CTakeDamageInfo & );
    virtual void AlertSound();
    virtual void IdleSound();
    virtual void AttackSound();

    // Ataque
	virtual void Event_Killed( const CTakeDamageInfo &info );

	virtual void Dismembering( const CTakeDamageInfo &info );
	virtual void CreateGoreGibs( int bodyPart, const Vector &vecOrigin, const Vector &vecDir );

    // Animaciones
    virtual Activity NPC_TranslateActivity( Activity );    

    // Estado
    virtual bool IsPanicked();

    virtual void GoStand();
    virtual void GoSit();
    virtual void GoLie();

    // I.A.
	virtual bool MovementCost( int moveType, const Vector &vecStart, const Vector &vecEnd, float *pCost );

    virtual void StartTask( const Task_t *pTask );
    virtual void RunTask( const Task_t *pTask );

    virtual int TranslateSchedule( int scheduleType );

protected:
    int m_iInfectedStatus;
    int m_iDesiredStatus;
	bool m_bWithoutCollision;

	int m_iFaceGesture;

    CountdownTimer m_nSitTimer;
    CountdownTimer m_nStandTimer;
    CountdownTimer m_nLieTimer;
};

#endif // NPC_INFECTED_H