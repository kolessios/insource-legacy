//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef AP_PLAYER_H
#define AP_PLAYER_H

#ifdef _WIN32
#pragma once
#endif

#include "in_player.h"

class CAP_PlayerSurvivor;
class CAP_PlayerSoldier;
class CAP_PlayerInfected;
class CAP_PlayerTank;

//================================================================================
// Un Jugador de Apocalypse
//================================================================================
class CAP_Player : public CPlayer
{
public:
	DECLARE_CLASS( CAP_Player, CPlayer );
	DECLARE_SERVERCLASS();

	// Definiciones según el tipo de Jugador
	virtual Class_T Classify();
	
	// Devoluciones
	virtual bool IsSurvivor() { return (GetTeamNumber() == TEAM_HUMANS); }
	virtual bool IsSoldier() { return (GetTeamNumber() == TEAM_SOLDIERS); }
	virtual bool IsInfected() { return (GetTeamNumber() == TEAM_INFECTED); }
    virtual bool IsTank() { return (IsInfected() && GetPlayerClass() == PLAYER_CLASS_INFECTED_BOSS); }

	// Principales
	virtual void SetUpAI();

    virtual void InitialSpawn();
    virtual void Spawn();
    virtual void Precache();

    virtual void PreThink();
    virtual void PlayerThink();
    virtual void PostThink();

    // Features
    virtual void CreateComponents();

    // Atributos
    virtual void CreateAttributes();

    // Soldados


    // Velocidad
    virtual float GetSpeed();

    //
    virtual void EnterPlayerState( int status );

	// Team
	virtual void SetRandomTeam();

    // Clase
    virtual void OnPlayerClass( int playerClass );
	virtual void SetRandomPlayerClass();

    // Equipos
    virtual void ChangeTeam( int iTeamNum );

    // Tipo de Jugador
    virtual const char *GetPlayerModel();
    virtual void SetUpModel();
    virtual const char *GetPlayerType();
    virtual gender_t GetPlayerGender();

    // Sonidos
    virtual void IdleSound();
    virtual void AlertSound();
    virtual void PainSound( const CTakeDamageInfo &info );
    virtual void DeathSound( const CTakeDamageInfo &info );

    // Viewmodel
    virtual const char *GetHandsModel( int viewmodelindex );
    virtual void SetUpHands();

    // Incapacitación/Estado
    virtual void OnPlayerStatus( int oldStatus, int status );
    //virtual void Event_Killed( const CTakeDamageInfo &info );

    // Escuadron
    virtual void OnNewLeader( CPlayer *pMember );

    // Efectos
    virtual bool ShouldFidget();

    // Animaciones
    virtual void HandleAnimEvent( animevent_t *event );

    // Utilidades
    //virtual void SetConnected( PlayerConnectedState iConnected );
    virtual bool ClientCommand( const CCommand &args );

public:
	// Compartido
    virtual void CreateAnimationSystem();
	virtual Activity TranslateActivity( Activity actBase );

protected:
    CountdownTimer m_nFidgetTimer;
    CountdownTimer m_nIdleSoundTimer;
};

CONVERT_PLAYER_FUNCTION( CAP_Player, Ap )

#endif // AP_PLAYER_H