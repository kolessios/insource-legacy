//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#ifndef AP_BOT_SCHEDULES_H
#define AP_BOT_SCHEDULES_H

#ifdef _WIN32
#pragma once
#endif

class CAI_Hint;

//================================================================================
// Search resources across the map
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
	CSearchResourcesSchedule(IBot *bot);

	virtual float GetDesire() const;
	virtual void Finish();

	virtual void GetWander();
	virtual void GetResources();

	virtual void TaskStart();
	virtual void TaskRun();

protected:
	CountdownTimer m_nTimer;
	CUtlVector<CNavArea *>m_CandidateAreas;
};

//================================================================================
// Clean a building (Look for enemies inside)
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

    CCleanBuildingSchedule(IBot *bot);

public:
    virtual float GetDesire() const;

    virtual void Start();
    virtual void Finish();

    virtual void Update();
    virtual void LookAround();

    virtual void TaskStart();
    virtual void TaskRun();
};

//================================================================================
// We maintain a cover position
//================================================================================
class CMaintainCoverSchedule : public IBotSchedule
{
public:
    DECLARE_CLASS_GAMEROOT(CMaintainCoverSchedule, IBotSchedule);
    DECLARE_SCHEDULE(SCHEDULE_MAINTAIN_COVER);

    enum
    {
        BTASK_MAINTAIN_COVER = BCUSTOM_TASK
    };

    CMaintainCoverSchedule(IBot *bot) : BaseClass(bot)
    {

    }

    virtual bool ItsImportant() const
    {
        return false;
    }

public:
    virtual float GetDesire() const;

    virtual int GetSoldiersInCover() const;
    virtual bool ShouldCrouch() const;

    virtual void TaskStart();
    virtual void TaskRun();

protected:
    bool m_bDisabled;
};

#endif // AP_BOT_SCHEDULES_H
