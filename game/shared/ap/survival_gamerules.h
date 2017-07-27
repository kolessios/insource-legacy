//==== Woots 2017. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef SURVIVAL_GAMERULES_H
#define SURVIVAL_GAMERULES_H

#pragma once

#include "in_gamerules.h"

//================================================================================
// Base de las reglas del juego.
//================================================================================
class CSurvivalGameRules : public CInGameRules
{
	DECLARE_CLASS( CSurvivalGameRules, CInGameRules );

public:
	// Definiciones
	virtual bool HasDirector() { return true; }
	virtual bool IsMultiplayer() { return true; }
	virtual bool IsTeamplay() { return false; }

	virtual int GetGameMode() { return GAME_MODE_SURVIVAL; }

	CSurvivalGameRules();
};

#endif