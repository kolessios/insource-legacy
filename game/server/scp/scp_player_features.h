//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef SCP_PLAYER_FEATURES_H
#define SCP_PLAYER_FEATURES_H

#ifdef _WIN32
#pragma once
#endif

#include "in_player_features.h"

//====================================================================
// Sistema de movimiento para SCP-173
//====================================================================
class C173Behavior : public CPlayerBaseFeature
{
public:
	virtual int GetFeatureID() { return PLAYER_FEATURE_BLIND_MOVEMENT; }

	virtual void Init();
	virtual void OnUpdate();

    virtual void CheckDanger();
    virtual void ApplyDanger();

public:
    bool m_bDangerZone;
    bool m_bLocked;
};

#endif // SCP_PLAYER_FEATURES_H