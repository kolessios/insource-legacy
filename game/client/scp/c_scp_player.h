//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef C_SCP_BASEPLAYER_H
#define C_SCP_BASEPLAYER_H

#ifdef _WIN32
#pragma once
#endif

#include "c_in_player.h"
#include "sound_instance.h"

class C_SurvivorPlayer;
class C_SoldierPlayer;
class C_MonsterPlayer;

//================================================================================
// Un Jugador de SCP
//================================================================================
class C_SCP_Player : public C_Player
{
public:
	DECLARE_CLASS( C_SCP_Player, C_Player );
	DECLARE_CLIENTCLASS();
    DECLARE_PREDICTABLE();
	
	// Devoluciones
	virtual bool IsSurvivor() { return (GetTeamNumber() == TEAM_HUMANS); }
	virtual bool IsSoldier() { return (GetTeamNumber() == TEAM_SOLDIERS); }
	virtual bool IsMonster() { return (GetTeamNumber() == TEAM_SCP); }

    C_SCP_Player();

	// Principales

    // Posición y Render
    virtual const QAngle &GetRenderAngles();

public:
    // Compartido
    virtual void CreateAnimationSystem();
	virtual Activity TranslateActivity( Activity actBase );

    // Sonido de pasos
    virtual void UpdateStepSound( surfacedata_t *psurface, const Vector &vecOrigin, const Vector &vecVelocity );

    // Velocidad
    virtual void UpdateSpeed();

protected:
    SoundInstance *m_nMovementSound;
};

#endif // C_SCP_BASEPLAYER_H