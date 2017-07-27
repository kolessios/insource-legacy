//========= Copyright © 1996-2001, Valve LLC, All rights reserved. ============

#include "cbase.h"
#include "player_command.h"
#include "igamemovement.h"
#include "in_buttons.h"
#include "ipredictionsystem.h"

#include "in_player.h"
#include "in_movedata.h"

// Enlace para el acceso global a [CInMoveData]
static CInMoveData g_MoveData;
CMoveData *g_pMoveData = &g_MoveData;

IPredictionSystem *IPredictionSystem::g_pPredictionSystems = NULL;

extern IGameMovement *g_pGameMovement;
extern ConVar sv_noclipduringpause;

//================================================================================
//================================================================================
class CInPlayerMove : public CPlayerMove
{
    DECLARE_CLASS( CInPlayerMove, CPlayerMove );

public:
    virtual void SetupMove( CBasePlayer *, CUserCmd *, IMoveHelper *, CMoveData * );
    virtual void RunCommand( CBasePlayer *, CUserCmd *, IMoveHelper * );
};

// PlayerMove Interface
static CInPlayerMove g_PlayerMove;

//================================================================================
// Acceso global a [CInPlayerMove]
//================================================================================
CPlayerMove *PlayerMove()
{
    return &g_PlayerMove;
}

//================================================================================
//================================================================================
void CInPlayerMove::SetupMove( CBasePlayer *pPlayer, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move )
{
    //pPlayer->AvoidPhysicsProps( ucmd );

    BaseClass::SetupMove( pPlayer, ucmd, pHelper, move );

    // setup trace optimization
    //g_pGameMovement->SetupMovementBounds( move );
}

//================================================================================
//================================================================================
void CInPlayerMove::RunCommand( CBasePlayer *pPlayer, CUserCmd *ucmd, IMoveHelper *moveHelper )
{
    BaseClass::RunCommand( pPlayer, ucmd, moveHelper );
}
