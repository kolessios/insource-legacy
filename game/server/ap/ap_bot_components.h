//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#ifndef AP_BOT_COMPONENTS_H
#define AP_BOT_COMPONENTS_H

#ifdef _WIN32
#pragma once
#endif

#include "bots\components\bot_components.h"

//================================================================================
// Decision component
// Everything related to the decisions that the bot must take.
//================================================================================
class CAP_BotDecision : public CBotDecision
{
public:
    DECLARE_CLASS_GAMEROOT(CAP_BotDecision, CBotDecision);

    CAP_BotDecision(IBot *bot) : BaseClass(bot)
    {
    }

public:
    virtual bool CanLookNoVisibleSpots() const;
    virtual bool ShouldLookSquadMember() const;

    virtual bool ShouldOnlyFeelPlayers() const;

    virtual float GetUpdateCoverRate() const;
    virtual void GetCoverCriteria(CSpotCriteria &criteria);
};

#endif //AP_BOT_COMPONENTS_H