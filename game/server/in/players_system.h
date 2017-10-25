//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef PLAYERS_MANAGER_H
#define PLAYERS_MANAGER_H

#ifdef _WIN32
#pragma once
#endif

#include "ai_navtype.h"

class CPlayer;
class dbHandler;

//================================================================================
// Macros
//================================================================================

#define FOR_EACH_PLAYER( code ) for ( int it = 0; it <= gpGlobals->maxClients; ++it )  {\
    CPlayer *pPlayer = ToInPlayer(UTIL_PlayerByIndex( it ));\
    if ( !pPlayer ) continue;\
    code\
}

#define FOR_EACH_PLAYER_TEAM( code, team ) for ( int it = 0; it <= gpGlobals->maxClients; ++it )  {\
    CPlayer *pPlayer = ToInPlayer(UTIL_PlayerByIndex( it ));\
    if ( !pPlayer ) continue;\
    if ( team != TEAM_ANY && pPlayer->GetTeamNumber() != team ) continue;\
    code\
}

//================================================================================
// Estados
//================================================================================
enum StatType
{
    STATS_MEDIOCRE = 0,
    STATS_POOR,
    STATS_NORMAL,
    STATS_GOOD,
    STATS_EXCELENT,

    LAST_STATS
};

static const char *g_StatsNames[LAST_STATS] =
{
    "MEDIOCRE",
    "POOR",
    "NORMAL",
    "GOOD",
    "EXCELENT"
};

//================================================================================
// Clase para administrar y escanear el estado de los Jugadores
//================================================================================
class CPlayersSystem : public CAutoGameSystemPerFrame
{
public:
    CPlayersSystem() : CAutoGameSystemPerFrame("PlayersManager")
    {
    }

    virtual bool Init();
    virtual void Shutdown();

    virtual void Restart();
    virtual void LevelInitPreEntity();
    virtual void FrameUpdatePostEntityThink();

public:
    virtual int GetTeam() { return m_iTeam; }
    virtual void SetTeam( int team ) { m_iTeam = team; }

    virtual void Update();

    virtual int GetTotal() { return m_iTotal; }
    virtual int GetConnected() { return m_iConnected; }
    virtual int GetAlive() { return m_iAlive; }
    virtual int GetDejected() { return m_iDejected; }
    virtual int GetHealth() { return m_iHealth; }
    virtual float GetStress() { return m_flStress; }

    virtual int GetWithFireWeapons() { return m_iWithFireWeapons; }
    virtual bool HasFireWeapons() { return (GetWithFireWeapons() > 0); }

    virtual int GetInCombat() { return m_iInCombat; }
    virtual bool IsOnCombat() { return (GetInCombat() > 0); }

    virtual int GetUnderAttack() { return m_iUnderAttack; }
    virtual bool IsUnderAttack() { return (GetUnderAttack() > 0); }

    virtual StatType GetStats() { return m_PlayerStats; }

public:
    virtual void ExecuteCommand( const char *command, int team = TEAM_ANY );
    virtual CPlayer *GetRandom( int team = TEAM_ANY );

    virtual void RespawnAll( int team = TEAM_ANY );

    virtual int GetDejectedCount( int team = TEAM_ANY );
    virtual int GetTotalCount( int team = TEAM_ANY );
    virtual int GetAliveCount( int team = TEAM_ANY );
    virtual int GetFireWeaponsCount( int team = TEAM_ANY );
    virtual int GetHealthTotal( int team = TEAM_ANY );
    virtual float GetStressTotal( int team = TEAM_ANY );

    virtual bool IsAnyPlayerInCombat( int team = TEAM_ANY );
    virtual bool IsAnyPlayerUnderAttack( int team = TEAM_ANY );

    virtual StatType GetWeaponStats( int team = TEAM_ANY );
    virtual StatType GetAmmoStats( int team = TEAM_ANY );
    virtual StatType GetPlayersStats();

    virtual CPlayer *GetNear( const Vector &vecPosition, float &distance, CBasePlayer *pExcept = NULL, int team = TEAM_ANY, bool ignoreBots = false );
    virtual CPlayer *GetNear( const Vector &vecPosition, CBasePlayer *pExcept = NULL, int team = TEAM_ANY, bool ignoreBots = false );

    virtual bool HasRouteToPlayer( CPlayer *pPlayer, CAI_BaseNPC *pNPC, float tolerance = 100, Navigation_t type = NAV_GROUND );
    virtual bool HasRouteToAnyPlayer( CAI_BaseNPC *pNPC, float tolerance = 100, int team = TEAM_ANY, Navigation_t type = NAV_GROUND );

    virtual bool IsAbleToSee( CBaseEntity *pEntity, const CRecipientFilter &filter, CBaseCombatCharacter::FieldOfViewCheckType checkFOV = CBaseCombatCharacter::USE_FOV );
    virtual bool IsAbleToSee( const Vector &vecPosition, const CRecipientFilter &filter, CBaseCombatCharacter::FieldOfViewCheckType checkFOV = CBaseCombatCharacter::USE_FOV );

    virtual bool IsVisible( CBaseEntity *pEntity, bool ignoreBots = true, int team = TEAM_ANY );
    virtual bool IsVisible( const Vector &vecPosition, bool ignoreBots = true, int team = TEAM_ANY );

    virtual bool IsEyesVisible( CBaseEntity *pEntity, bool ignoreBots = true, int team = TEAM_ANY );
    virtual bool IsEyesVisible( const Vector &vecPosition, bool ignoreBots = true, int team = TEAM_ANY );

    virtual bool IsInViewcone( CBaseEntity *pEntity, bool ignoreBots = true, int team = TEAM_ANY );
    virtual bool IsInViewcone( const Vector &vecPosition, bool ignoreBots = true, int team = TEAM_ANY );

    virtual void SendLesson( const char *pLesson, CPlayer *pPlayer, bool once = false, CBaseEntity *pSubject = NULL );
    virtual void SendLesson2All( const char *pLesson, int team = TEAM_ANY, bool once = false, CBaseEntity *pSubject = NULL );

protected:
    int m_iTotal;
    int m_iAlive;
    int m_iConnected;
    int m_iDejected;

    int m_iHealth;
    float m_flStress;
    int m_iInCombat;
    int m_iUnderAttack;
    int m_iWithFireWeapons;

    int m_iTeam;

    StatType m_WeaponsStats;
    StatType m_PlayerStats;

    CountdownTimer m_ThinkTimer;
    //CUtlVector<CPlayer *> m_DejectedPlayers;

    friend class Director;
};

extern CPlayersSystem *ThePlayersSystem;

extern dbHandler *ThePlayersDatabase;
extern dbHandler *TheGameDatabase;

#endif // PLAYERS_MANAGER_H