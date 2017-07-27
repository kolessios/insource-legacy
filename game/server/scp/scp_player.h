//==== Woots 2017. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef SCP_BASEPLAYER_H
#define SCP_BASEPLAYER_H

#ifdef _WIN32
#pragma once
#endif

#include "in_player.h"
#include "sound_instance.h"

class CSurvivorPlayer;
class CSoldierPlayer;
class CMonsterPlayer;

//================================================================================
// Código compartido entre jugadores, soldados y SCP's
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
    virtual void UpdateSpeed();

    // Equipo
    virtual void ChangeTeam( int iTeamNum, bool bAutoTeam, bool bSilent );

    // Tipo de jugador
    virtual const char *GetPlayerModel();
    virtual void PrepareModel();

    // Features
    virtual void CreateFeatures();

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
    SoundInstance *m_nMovementSound;
};

CONVERT_PLAYER_FUNCTION( CSCP_Player, Scp );

#endif // SCP_BASEPLAYER_H