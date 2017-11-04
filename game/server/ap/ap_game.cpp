//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#include "cbase.h"

#include "ap_player.h"
#include "in_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar sv_gamemode;

//================================================================================
// Install the rules of the game of the current mode
//================================================================================
void InstallGameRules()
{
    switch ( sv_gamemode.GetInt() ) {
        case GAME_MODE_SURVIVAL:
        {
            CreateGameRulesObject("CSurvivalGameRules");
            break;
        }

        case GAME_MODE_ASSAULT:
        {
            CreateGameRulesObject("CAssaultGameRules");
            break;
        }

        default:
        {
            CreateGameRulesObject("CInGameRules");
            break;
        }
    }
}