//==== Woots 2017 ===========//

#ifndef IN_PLAYER_COMPONENTS_BASIC_H
#define IN_PLAYER_COMPONENTS_BASIC_H

#ifdef _WIN32
#pragma once
#endif

#include "in_player_component.h"

//====================================================================
// Regeneración y procesamiento de vida
//====================================================================
class CPlayerHealthComponent : public CPlayerComponent
{
public:
    DECLARE_COMPONENT( PLAYER_COMPONENT_HEALTH );

    virtual void Init();
    virtual void Update();

public:
    CountdownTimer m_RegenerationTimer;
};

//====================================================================
// Procesamiento de efectos
//====================================================================
class CPlayerEffectsComponent : public CPlayerComponent
{
public:
    DECLARE_COMPONENT( PLAYER_COMPONENT_EFFECTS );

    virtual void Update();
};

//====================================================================
// Procesamiento de incapacitación
//====================================================================
class CPlayerDejectedComponent : public CPlayerComponent
{
public:
    DECLARE_COMPONENT( PLAYER_COMPONENT_DEJECTED );

    virtual void Update();

    virtual void UpdateDejected();
    virtual void UpdateFall();
};

#endif // IN_PLAYER_COMPONENTS_BASIC_H