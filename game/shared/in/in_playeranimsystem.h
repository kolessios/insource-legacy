//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef IN_PLAYERANIMSTATE_H
#define IN_PLAYERANIMSTATE_H

#pragma once

#include "multiplayer_animstate.h"


#ifdef CLIENT_DLL
    class C_Player;

    #define CPlayer C_Player
    #define CPlayerAnimationSystem C_PlayerAnimationSystem
#else
    class CPlayer;
#endif

//================================================================================
// Sistema de procesamiento de animaciones
//================================================================================
class CPlayerAnimationSystem : public CMultiPlayerAnimState
{
    DECLARE_CLASS( CPlayerAnimationSystem, CMultiPlayerAnimState );

public:
    // Devolución
    CPlayer *GetInPlayer() { return m_pInPlayer; }

    // Principales
    CPlayerAnimationSystem( CPlayer *, MultiPlayerMovementData_t & );

    virtual bool ShouldComputeDirection();
    virtual bool ShouldComputeYaw();

    virtual Activity TranslateActivity( Activity );

    virtual void Update();
    virtual bool SetupPoseParameters( CStudioHdr * );

    virtual void ComputePoseParam_AimYaw( CStudioHdr * );

    virtual Activity CalcMainActivity();
    virtual void HandleStatus( Activity &nActivity, Activity nNormal, Activity nInjured, Activity nCalm );

    virtual bool HandleSwimming( Activity & );
    virtual bool HandleMoving( Activity & );
    virtual bool HandleDucking( Activity & );

    virtual void DoAnimationEvent( PlayerAnimEvent_t, int = 0 );

protected:
    CPlayer *m_pInPlayer;
};

CPlayerAnimationSystem *CreatePlayerAnimationSystem( CPlayer *, MultiPlayerMovementData_t & );

#endif // IN_PLAYERANIMSTATE_H