//==== Woots 2017. http://creativecommons.org/licenses/by/2.5/mx/ ===========//
//
// Conjunto de tareas para los Bots de Apocalypse.
//
//=============================================================================//

/*
#ifndef AP_BOT_SCHEDULES_H
#define AP_BOT_SCHEDULES_H

#ifdef _WIN32
#pragma once
#endif

class CAI_Hint;

//================================================================================
// Busca recursos por el mapa
//================================================================================
class CSearchResourcesSchedule : public IBotSchedule
{
	DECLARE_CLASS_GAMEROOT( CSearchResourcesSchedule, IBotSchedule );
    DECLARE_SCHEDULE( SCHEDULE_SEARCH_RESOURCES );

	enum
	{
		BTASK_GET_RESOURCES_SPOT = BCUSTOM_TASK,
	};

public:
	CSearchResourcesSchedule();

	virtual bool ItsImportant() { return true; }

	virtual float GetDesire();
	virtual void OnEnd();

	virtual void GetWander();
	virtual void GetResources();

	virtual void TaskStart();
	virtual void TaskRun();

protected:
	CountdownTimer m_nTimer;
	CUtlVector<CNavArea *>m_CandidateAreas;
};

//================================================================================
// Limpia un edificio (Busca enemigos dentro de el)
//================================================================================
class CCleanBuildingSchedule : public IBotSchedule
{
public:
    DECLARE_CLASS_GAMEROOT( CCleanBuildingSchedule, IBotSchedule );
    DECLARE_SCHEDULE( SCHEDULE_CLEAN_BUILDING );

    enum
    {
        BTASK_CLEAN_AREA = BCUSTOM_TASK,
        BTASK_TACTICAL_LOOK_AROUND,
        BTASK_TACTICAL_COVER,
        BTASK_WAIT_LEADER
    };

    CCleanBuildingSchedule();

    virtual bool ItsImportant()
    {
        return true;
    }

    virtual float GetDesire();

    virtual void Start();
    virtual void OnEnd();

    virtual void Think();
    virtual void LookAround();

    virtual void TaskStart();
    virtual void TaskRun();

protected:
    CAI_Hint *m_pHint;
    Vector m_vecSpot;
};

#endif // AP_BOT_SCHEDULES_H
*/