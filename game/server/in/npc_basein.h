//==== InfoSmart 2015. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef NPC_BASEIN_H
#define NPC_BASEIN_H

#ifdef _WIN32
#pragma once
#endif

#include "ai_baseactor.h"
#include "ai_basenpc.h"

//================================================================================
// Macros
//================================================================================

#define DECLARE_NPC_HEALTH( classname, name, value ) DECLARE_REPLICATED_COMMAND( name, value, "##classname Health" ); \
    int classname::GetNPCHealth() { return name.GetInt(); }

#define DECLARE_NPC_MELEE_DAMAGE( classname, name, value ) DECLARE_REPLICATED_COMMAND( name, value, "##classname Damage per Attack" ); \
    float classname::GetMeleeDamage() { return name.GetFloat(); }

#define DECLARE_NPC_MELEE_DISTANCE( classname, name, value ) DECLARE_REPLICATED_COMMAND( name, value, "##classname Melee Attack Distance" ); \
    float classname::GetMeleeDistance() { return name.GetFloat(); }

//================================================================================
// Clase base para crear NPC's
//================================================================================
class CBaseNPC : public CAI_BaseNPC
{
public:
    DECLARE_CLASS( CBaseNPC, CAI_BaseNPC );
    DECLARE_DATADESC();
    DEFINE_CUSTOM_AI;

    /*
    enum
    {
        COND_ADDON_LOST_HOST = NEXT_CONDITION,
        NEXT_CONDITION,
    };
    */

    enum
    {
        TASK_CONTINUOS_MELEE_ATTACK1 = NEXT_TASK,
        NEXT_TASK,
    };

    enum 
    {
        SCHED_CONTINUOUS_MELEE_ATTACK1 = NEXT_SCHEDULE,
        NEXT_SCHEDULE
    };

    CBaseNPC();
    ~CBaseNPC();

    // Información
    virtual int GetNPCHealth() { return 100; }
    virtual const char *GetNPCModel() { return ""; }
    virtual float GetNPCFOV() { return 0.5f; }
    virtual Hull_t GetNPCHull() { return HULL_HUMAN; }

    virtual float GetMeleeDistance() { return 200.0f; }
    virtual float GetMeleeDamage() { return 1.0f; }

	virtual bool IsRunning() { return ( m_flGroundSpeed >= 200 ); }

	virtual bool HasActivity( Activity iAct )
	{
		return ( SelectWeightedSequence( NPC_TranslateActivity(iAct) ) == ACTIVITY_NOT_AVAILABLE ) ? false : true;
	}

    // Principales
    virtual void Spawn();
    virtual void SetCapabilities();
    virtual void SetUpModel() { }

    // Movimiento y Navegación
    void SetupGlobalModelData();
    virtual bool OverrideMoveFacing( const AILocalMoveGoal_t &move, float flInterval );

    virtual float GetIdealSpeed() const;
    virtual float GetSequenceGroundSpeed( CStudioHdr *, int );

    // Enemigos
    virtual float GetEnemyDistance();

    // Ataque
    virtual bool MeleeAttack();

    // Animaciones
    virtual void HandleAnimEvent( animevent_t * );

    // I.A.
    virtual void StartTask( const Task_t * );
    virtual void RunTask( const Task_t * );

    virtual void RunAttackTask( int );


	// Salud/Muerte
	virtual bool ShouldPlayDeathAnimation();

	virtual int SelectDeadSchedule();
    virtual void RunDieTask();


protected:
    int m_iMoveXPoseParam;
    int m_iMoveYPoseParam;

    int m_iLeanYawPoseParam;
    int m_iLeanPitchPoseParam;

    int m_iAttackGesture;
};

#endif // NPC_BASEIN_H