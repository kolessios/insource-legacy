//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#ifndef AP_PLAYER_H
#define AP_PLAYER_H

#ifdef _WIN32
#pragma once
#endif

#include "in_player.h"

//================================================================================
// A player of Apocalypse
//================================================================================
class CAP_Player : public CPlayer
{
public:
    DECLARE_CLASS(CAP_Player, CPlayer);
    DECLARE_SERVERCLASS();

    virtual Class_T Classify();

    virtual bool IsSurvivor()
    {
        return (GetTeamNumber() == TEAM_HUMANS);
    }
    virtual bool IsSoldier()
    {
        return (GetTeamNumber() == TEAM_SOLDIERS);
    }
    virtual bool IsInfected()
    {
        return (GetTeamNumber() == TEAM_INFECTED);
    }
    virtual bool IsTank()
    {
        return (IsInfected() && GetPlayerClass() == PLAYER_CLASS_INFECTED_BOSS);
    }

    // Main
    virtual void SetUpBot();

    virtual void InitialSpawn();
    virtual void Spawn();
    virtual void Precache();

    virtual void PreThink();
    virtual void PlayerThink();
    virtual void PostThink();

    // Features
    virtual void CreateComponents();

    // Attributes
    virtual void CreateAttributes();

    // Speed
    virtual float GetSpeed();

    //
    virtual void EnterPlayerState(int status);

    // Team
    virtual void SetRandomTeam();

    // Class
    virtual void OnPlayerClass(int playerClass);
    virtual void SetRandomPlayerClass();

    // Equipos
    virtual void ChangeTeam(int iTeamNum);

    // Player Type
    virtual const char *GetPlayerModel();
    virtual void SetUpModel();
    virtual const char *GetPlayerType();
    virtual gender_t GetPlayerGender();

    // Sounds
    virtual void IdleSound();
    virtual void AlertSound();
    virtual void PainSound(const CTakeDamageInfo &info);
    virtual void DeathSound(const CTakeDamageInfo &info);

    // Viewmodel
    virtual const char *GetHandsModel(int viewmodelindex);
    virtual void SetUpHands();

    // Status
    virtual void OnPlayerStatus(int oldStatus, int status);
    //virtual void Event_Killed( const CTakeDamageInfo &info );

    // Squad
    virtual void OnNewLeader(CPlayer *pMember);

    // Effects
    virtual bool ShouldFidget();

    // Animations
    virtual void HandleAnimEvent(animevent_t *event);

    // Utils
    //virtual void SetConnected( PlayerConnectedState iConnected );
    virtual bool ClientCommand(const CCommand &args);

public:
    // Shared
    virtual void CreateAnimationSystem();
    virtual Activity TranslateActivity(Activity actBase);

protected:
    CountdownTimer m_nFidgetTimer;
    CountdownTimer m_nIdleSoundTimer;
};

CONVERT_PLAYER_FUNCTION(CAP_Player, Ap)

#endif // AP_PLAYER_H