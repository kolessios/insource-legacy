//==== Woots 2017. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef ASSAULT_GAMERULES_H
#define ASSAULT_GAMERULES_H

#pragma once

#include "in_gamerules.h"

//================================================================================
// Base de las reglas del juego.
//================================================================================
class CAssaultGameRules : public CInGameRules
{
    DECLARE_CLASS( CAssaultGameRules, CInGameRules );

public:
    // Definiciones
    virtual bool HasDirector() { return true; }
    virtual bool IsMultiplayer() { return true; }
    virtual bool IsTeamplay() { return false; }
    virtual int GetGameMode() { return GAME_MODE_ASSAULT; }

    CAssaultGameRules();

#ifndef CLIENT_DLL
    virtual void PlayerThink( CBasePlayer *pPlayer );
    virtual void AdjustPlayerDamageTaken( CPlayer *pVictim, CTakeDamageInfo &info );
    virtual void AdjustPlayerDamageHitGroup( CPlayer *pVictim, CTakeDamageInfo &info, int hitGroup );

    virtual bool FPlayerCanDejected( CBasePlayer *, const CTakeDamageInfo & ) { return false; }
    virtual float PlayerShieldPause( CPlayer * pPlayer );
#endif
};

#endif