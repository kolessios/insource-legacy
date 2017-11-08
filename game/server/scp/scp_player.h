//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#ifndef SCP_PLAYER_H
#define SCP_PLAYER_H

#ifdef _WIN32
#pragma once
#endif

#include "in_player.h"
#include "sound_instance.h"

//================================================================================
// An SCP player
//================================================================================
class CSCP_Player : public CPlayer
{
public:
    DECLARE_CLASS( CSCP_Player, CPlayer );
    DECLARE_SERVERCLASS();

    // Devoluciones
	virtual bool IsSurvivor() { return (GetTeamNumber() == TEAM_HUMANS); }
	virtual bool IsSoldier() { return (GetTeamNumber() == TEAM_SOLDIERS); }
	virtual bool IsMonster() { return (GetTeamNumber() == TEAM_SCP); }

    CSCP_Player();

    // 
    virtual Class_T Classify();

    // Principales
    virtual void InitialSpawn();
    virtual void Precache();

    virtual void PlayerThink();

    // Ataque
    virtual void PrimaryAttack();

    // Estado
    virtual void EnterPlayerState( int status );

    // Clase
    virtual void OnPlayerClass( int playerClass );

    // Velocidad
    virtual float GetSpeed();

    // Equipo
    virtual void ChangeTeam( int iTeamNum, bool bAutoTeam, bool bSilent );

    // Tipo de jugador
    virtual const char *GetPlayerModel();
    virtual void PrepareModel();

    // Features
    virtual void CreateComponents();

    // Utilidades
    virtual const char *GetSpawnEntityName();
    virtual bool ClientCommand( const CCommand &args );

public:
    // Compartido
    virtual void CreateAnimationSystem();
	virtual Activity TranslateActivity( Activity actBase );

    // Sonido de pasos
    virtual void UpdateStepSound( surfacedata_t *psurface, const Vector &vecOrigin, const Vector &vecVelocity );

protected:
    CSoundInstance *m_pMovementSound;
};

CONVERT_PLAYER_FUNCTION( CSCP_Player, Scp );

#endif // SCP_PLAYER_H