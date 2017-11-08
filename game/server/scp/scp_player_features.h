//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#ifndef SCP_PLAYER_FEATURES_H
#define SCP_PLAYER_FEATURES_H

#ifdef _WIN32
#pragma once
#endif

#include "in_player_component.h"

//====================================================================
// Motion system for SCP-173
//====================================================================
class C173BehaviorComponent : public CPlayerComponent
{
public:
    DECLARE_CLASS_GAMEROOT(C173BehaviorComponent, CPlayerComponent);
    DECLARE_PLAYER_COMPONENT(PLAYER_COMPONENT_BLIND_MOVEMENT);

	virtual void Init();
	virtual void Update();

    virtual void CheckDanger();
    virtual void ApplyDanger();

public:
    bool m_bDangerZone;
    bool m_bLocked;
};

#endif // SCP_PLAYER_FEATURES_H