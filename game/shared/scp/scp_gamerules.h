//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#ifndef SCP_GAMERULES_H
#define SCP_GAMERULES_H

#pragma once

#include "in_gamerules.h"

//================================================================================
// Reglas de juego base para SCP: Source
//================================================================================
class CSCPGameRules : public CInGameRules
{
public:
    DECLARE_CLASS( CSCPGameRules, CInGameRules );

    CSCPGameRules();

    // Definiciones
    virtual bool HasDirector() { return false; }
    virtual bool IsMultiplayer();

    // Game Mode
    virtual int GetGameMode()
    {
        return GAME_MODE_FACILITIES_SINGLEPLAYER;
    }

#ifndef CLIENT_DLL
    virtual void InitDefaultAIRelationships();

    virtual bool FPlayerCanDejected( CBasePlayer *, const CTakeDamageInfo & ) { return false; }

    virtual bool CanPlayDeathAnim( CBaseEntity *pEntity, const CTakeDamageInfo &info );
#else
#endif
};

#endif // SCP_GAMERULES_H