//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef BOT_SOLDIER_H
#define BOT_SOLDIER_H

#ifdef _WIN32
#pragma once
#endif

#include "bots\bot.h"
#include "bot_soldier_schedules.h"

//================================================================================
// Inteligencia Artificial para crear un Bot similar a un soldado en la vida real
//================================================================================
class CBotSoldier : public CBot
{
public:
	DECLARE_CLASS_GAMEROOT( CBotSoldier, CBot );

	// Principales
	virtual void Spawn();
	virtual void Process( CBotCmd* &cmd );

	// Cover
	virtual void UpdateNearestCover();
	virtual bool GetNearestCover( Vector &vecPosition );
	virtual bool GetNearestCover();
	virtual bool IsInCoverPosition();

	// Follow Component
	virtual bool ShouldFollow();

	// I.A.
	virtual void SetUpComponents();
	virtual void SetUpSchedules();

protected:
	Vector m_vecCover;
};

#endif