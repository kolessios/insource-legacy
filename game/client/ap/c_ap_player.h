//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef C_AP_PLAYER_H
#define C_AP_PLAYER_H

#ifdef _WIN32
#pragma once
#endif

#include "c_in_player.h"

#include "sound_manager.h"
#include "sound_instance.h"

class C_AP_PlayerSurvivor;
class C_AP_PlayerSoldier;
class C_AP_PlayerInfected;
class C_AP_PlayerTank;

#define CAP_Player C_AP_Player

//================================================================================
// Música
// Definitivamente debemos hacer esto más fácil y flexible
//================================================================================

#define DEATH_MUSIC "Music.Player.Death"
#define DEJECTED_MUSIC "Music.Player.PuddleOfYou"
#define DYING_MUSIC "Music.Player.SoCold"
#define CLINGING1_MUSIC "Music.Player.Clinging1"
#define CLINGING2_MUSIC "Music.Player.Clinging2"
#define CLINGING3_MUSIC "Music.Player.Clinging3"
#define CLINGING4_MUSIC "Music.Player.Clinging4"

//================================================================================
// Un Jugador de Apocalypse
//================================================================================
class C_AP_Player : public C_Player
{
public:
	DECLARE_CLASS( C_AP_Player, C_Player );
	DECLARE_CLIENTCLASS();

    ~C_AP_Player();
	
	// Devoluciones
	virtual bool IsSurvivor() { return (GetTeamNumber() == TEAM_HUMANS); }
	virtual bool IsSoldier() { return (GetTeamNumber() == TEAM_SOLDIERS); }
	virtual bool IsInfected() { return (GetTeamNumber() == TEAM_INFECTED); }
    virtual bool IsTank() { return (IsInfected() && GetPlayerClass() == PLAYER_CLASS_INFECTED_BOSS); }

    // Principales
    virtual bool Simulate();
    virtual void PostDataUpdate( DataUpdateType_t data );

    // Velocidad
    virtual float GetSpeed();

    // Música
    virtual void CreateMusic();
    virtual void DestroyMusic();

    virtual float SoundDesire( const char *soundName, int channel );
    virtual void UpdateMusic();

    virtual void OnSoundPlay( const char *soundName );

    // Animaciones
    virtual void UpdatePoseParams();

    // Linternas
    virtual bool CreateDejectedLight();
    virtual void DestroyDejectedLight();
    virtual void UpdateDejectedLight();

    virtual bool CreateMuzzleLight();
    virtual void DestroyMuzzleLight();

    virtual void ShowMuzzleFlashlight();

    // Camara
    virtual void CalcPlayerView( Vector& eyeOrigin, QAngle& eyeAngles, float& fov );

    // Efectos
    virtual void DoPostProcessingEffects( PostProcessParameters_t &params );

public:
	// Compartido
    virtual void CreateAnimationSystem();
	virtual Activity TranslateActivity( Activity actBase );

protected:
    CSoundManager *m_nMusicManager;
};

#endif // C_AP_PLAYER_H