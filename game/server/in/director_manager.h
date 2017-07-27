//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef DIRECTOR_MANAGER_H
#define DIRECTOR_MANAGER_H

#ifdef _WIN32
#pragma once
#endif

#include "directordefs.h"
#include "bot_maker.h"

//================================================================================
// El ayudante del Director
//================================================================================
class DirectorManager
{
public:
    DirectorManager();

    // CAutoGameSystemPerFrame
    virtual void Init();
    virtual void ResetMap();
    virtual void ScanSpawnableSpots();

    // Principales
    virtual void SetPopulation( const char *type );
    virtual void LoadPopulation();

    virtual CMinionInfo *GetMinionInfo( MinionType type );
    virtual bool GetIdealSpot( MinionType type, Vector &vecPosition );

    virtual void Precache();

    virtual int GetSpawnsLeft( MinionType type );
    virtual int GetMinionsCreated();
    virtual int GetPopulation( MinionType type );

    virtual void Spawn();
    virtual void Spawn( MinionType type );

    virtual bool CreateMinion( MinionType type, Vector *vecPosition = NULL );
    virtual bool CreateMinion( CMinionInfo *minion, Vector *vecPosition = NULL );

    virtual bool SetupMinion( CAI_BaseNPC *pNPC, CMinionInfo *minion, Vector *vecPosition );
    virtual bool SetupMinion( CBotSpawn *pSpawner, CMinionInfo *minion, Vector *vecPosition );

    // Escaneo de posible zonas
    virtual bool ShouldUseNavMesh();

    virtual bool CanUseNavArea( CNavArea *pArea );
    virtual bool CanUseNode( CAI_Node *pNode );

    virtual bool ShouldUseSpot( const Vector vecPosition );

    virtual void ScanNodes();
    virtual void ScanNavMesh();

    // Escaneo de hijos
    virtual void Scan();
    virtual void ScanMinion( CBaseEntity *pEntity );

    virtual bool ShouldKill( CBaseEntity *pEntity );
    virtual void Kill( CBaseEntity *pEntity, const char *reason = "Unknown" );

    virtual bool ShouldReportEnemy( CBaseEntity *pEntity );
    virtual void ReportEnemy( CBaseEntity *pEntity );

    virtual bool IsTooFar( CBaseEntity *pEntity );
    virtual bool IsTooClose( CBaseEntity *pEntity );

    virtual bool IsUnreachable( CBaseEntity *pEntity );

public:
    PopulationList m_PopulationList;
    Minions_t m_Minions[LAST_CHILD_TYPE];

protected:
    char m_nPopulation[32];

    CUtlVector<CNavArea *>m_CandidateAreas;
    CUtlVector<CAI_Node *>m_CandidateNodes;

    CountdownTimer m_ScanTimer;
    friend class Director;
};

extern DirectorManager *TheDirectorManager;

#endif // DIRECTOR_MANAGER_H