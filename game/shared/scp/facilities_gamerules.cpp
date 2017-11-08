//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#include "cbase.h"
#include "scp_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar sv_gamemode;

//================================================================================
// Reglas de juego base para SCP: Source
//================================================================================
class CFacilitiesGameRules : public CSCPGameRules
{
public:
    DECLARE_CLASS( CFacilitiesGameRules, CSCPGameRules );

    CFacilitiesGameRules();

#ifndef CLIENT_DLL
#else
#endif
};

REGISTER_GAMERULES_CLASS( CFacilitiesGameRules );

//================================================================================
// Constructor
//================================================================================
CFacilitiesGameRules::CFacilitiesGameRules()
{
    TheGameRules = this;
}