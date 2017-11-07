//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#ifndef AP_BOT_H
#define AP_BOT_H

#ifdef _WIN32
#pragma once
#endif

#include "bots\bot.h"

#include "ai_hint.h"

#include "ap_bot_schedules.h"
#include "ap_bot_components.h"
#include "ap_player.h"

// We will remember a building as "clean" during this time.
#define BUILDING_MEMORY_TIME (5 * 60.0f)

//================================================================================
// Information about a building
//================================================================================
class CBuildingInfo
{
public:
    CBuildingInfo()
    {
        entrance = NULL;
        enemies = 0;
        expired.Invalidate();
    }

    CBuildingInfo(CAI_Hint *pHint)
    {
        Q_strncpy(name, STRING(pHint->GetGroup()), sizeof(name));
        entrance = pHint;
        enemies = 0;
        expired.Invalidate();
        Scan();
    }

    void Scan();

    char name[100];
    CAI_Hint *entrance;
    int enemies;
    CountdownTimer expired;
    NavAreaVector areas;
};

typedef CUtlVector<CBuildingInfo *> BuildingList;

//================================================================================
// Artificial intelligence for Bots of Apocalypse
//================================================================================
class CAP_Bot : public CBot
{
public:
    DECLARE_CLASS_GAMEROOT(CAP_Bot, CBot);

    CAP_Bot(CBasePlayer *parent) : BaseClass(parent)
    {
        m_BuildingList.EnsureCapacity(32);
    }

    virtual CAP_Player *GetPlayer()
    {
        return ToApPlayer(m_pParent);
    }
    virtual CAP_Player *GetPlayer() const
    {
        return ToApPlayer(m_pParent);
    }

public:
    virtual bool IsSurvivor()
    {
        return GetPlayer()->IsSurvivor();
    }

    virtual bool IsSoldier()
    {
        return GetPlayer()->IsSoldier();
    }

    virtual bool IsInfected()
    {
        return GetPlayer()->IsInfected();
    }

    virtual bool IsTank()
    {
        return GetPlayer()->IsTank();
    }

public:
    virtual void SetUpComponents();
    virtual void SetUpSchedules();

    virtual void Spawn();

    virtual void RunCustomAI();

    virtual void FindBuilding();

    // Building Scan
    virtual bool IsCleaningBuilding();
    virtual bool IsMinionCleaningBuilding();

    virtual void StartBuildingClean(CAI_Hint *pHint);
    virtual void FinishBuildingClean();

    virtual CBuildingInfo *GetBuildingInfo(const char *pName);
    virtual CBuildingInfo *GetBuildingInfo();

    virtual bool HasBuildingCleaned(const char *pName);

    virtual void SetScanningArea(CNavArea *pArea)
    {
        m_pScanningArea = pArea;
    }

    virtual CNavArea *GetScanningArea()
    {
        return m_pScanningArea;
    }

protected:
    BuildingList m_BuildingList;
    CBuildingInfo *m_pCleaningBuilding;
    CNavArea *m_pScanningArea;
};

#endif // AP_BOT_H