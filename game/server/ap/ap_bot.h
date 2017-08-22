//==== Woots 2017. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef AP_BOT_H
#define AP_BOT_H

#ifdef _WIN32
#pragma once
#endif

#include "bots\bot.h"
#include "bot_soldier.h"

#include "ai_hint.h"

#include "ap_bot_schedules.h"
#include "ap_player.h"

// Tiempo que dura la memoria al limpiar un edificio
#define BUILDING_MEMORY_TIME (5 * 60.0f)

//================================================================================
// Información acerca de la limpieza de enemigos de un edificio
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

    CBuildingInfo( CAI_Hint *pHint )
    {
        Q_strncpy( name, STRING( pHint->GetGroup() ), sizeof( name ) );
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
// Inteligencia artificial para crear un jugador controlado por el ordenador
//================================================================================
class CAP_Bot : public CBot
{
    DECLARE_CLASS_GAMEROOT( CAP_Bot, CBot );

public:

    CAP_Bot( CBasePlayer *parent ) : BaseClass( parent )
    {

    }

	virtual CAP_Player *GetPlayer() { return ToApPlayer(m_pParent); }
    virtual CAP_Player *GetPlayer() const { return ToApPlayer(m_pParent); }

    // Principales
	virtual void SetUpSchedules();

    // Aim Component
    virtual bool ShouldAimOnlyVisibleInterestingSpots();
    virtual bool ShouldLookSquadMember();
};

//================================================================================
// Inteligencia artificial para crear un jugador controlado por el ordenador
// con un modo de juego similar a un soldado.
//================================================================================
class CAP_BotSoldier : public CAP_Bot
{
public:
    DECLARE_CLASS_GAMEROOT( CAP_BotSoldier, CAP_Bot );

    CAP_BotSoldier( CBasePlayer *parent );

    virtual void Spawn();

    // Building Scan
    virtual bool IsCleaningBuilding();
    virtual bool IsMinionCleaningBuilding();

    virtual void StartBuildingClean( CAI_Hint *pHint );
    virtual void FinishBuildingClean();

    virtual CBuildingInfo *GetBuildingInfo( const char *pName );
    virtual CBuildingInfo *GetBuildingInfo();

    virtual bool HasBuildingCleaned( const char *pName );

    virtual void SetScanningArea( CNavArea *pArea )
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