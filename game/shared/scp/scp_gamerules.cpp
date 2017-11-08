//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#include "cbase.h"
#include "scp_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

REGISTER_GAMERULES_CLASS( CSCPGameRules );

CSCPGameRules::CSCPGameRules()
{

}

//================================================================================
// ¿Modo multijugador?
//================================================================================
bool CSCPGameRules::IsMultiplayer()
{
    if ( IsGameMode( GAME_MODE_FACILITIES_COOP ) || IsGameMode( GAME_MODE_FACILITIES_VERSUS ) )
        return true;

    return false;
}

#ifndef CLIENT_DLL

//================================================================================
// Establece la relación entre distintas clases
//================================================================================
void CSCPGameRules::InitDefaultAIRelationships()
{
    BaseClass::InitDefaultAIRelationships();

    // ------------------------------------------------------------
    //    > CLASS_HUMAN
    // ------------------------------------------------------------
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_HUMAN, CLASS_MONSTER, D_HT, 0 );

    // ------------------------------------------------------------
    //    > CLASS_SOLDIER
    // ------------------------------------------------------------
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_SOLDIER, CLASS_MONSTER, D_HT, 0 );

    // ------------------------------------------------------------
    //    > CLASS_MONSTER
    // ------------------------------------------------------------
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_MONSTER, CLASS_MONSTER, D_LI, 0 );
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_MONSTER, CLASS_PLAYER, D_HT, 0 );
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_MONSTER, CLASS_PLAYER_ALLY, D_HT, 0 );
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_MONSTER, CLASS_PLAYER_ALLY_VITAL, D_HT, 0 );
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_MONSTER, CLASS_HUMAN, D_HT, 0 );
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_MONSTER, CLASS_SOLDIER, D_HT, 0 );
}

//================================================================================
// Devuelve si la entidad puede reproducir la animación de muerte
// por el daño especificado.
//================================================================================
bool CSCPGameRules::CanPlayDeathAnim( CBaseEntity *pEntity, const CTakeDamageInfo &info )
{
    return false;
}

#endif