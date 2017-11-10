//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#include "cbase.h"
#include "bots\bot.h"

#ifdef INSOURCE_DLL
#include "in_utils.h"
#include "in_gamerules.h"
#else
#include "bots\in_utils.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar bot_primary_attack;

//================================================================================
// Logging System
// Only for the current file, this should never be in a header.
//================================================================================

#define Msg(...) Log_Msg(LOG_BOTS, __VA_ARGS__)
#define Warning(...) Log_Warning(LOG_BOTS, __VA_ARGS__)

//================================================================================
//================================================================================
void CBot::SetPeaceful( bool enabled )
{
    if ( !GetMemory() )
        return;

    if ( enabled )
        GetMemory()->Disable();
    else
        GetMemory()->Enable();
}

//================================================================================
//================================================================================
CBaseEntity *CBot::GetEnemy() const
{
    if ( !GetMemory() )
        return NULL;

    return GetMemory()->GetEnemy();
}

//================================================================================
//================================================================================
CEntityMemory * CBot::GetPrimaryThreat() const
{
    if ( !GetMemory() )
        return NULL;

    return GetMemory()->GetPrimaryThreat();
}

//================================================================================
//================================================================================
void CBot::SetEnemy( CBaseEntity *pEnemy, bool bUpdate )
{
    if ( !GetMemory() )
        return;

    return GetMemory()->SetEnemy( pEnemy, bUpdate );
}
