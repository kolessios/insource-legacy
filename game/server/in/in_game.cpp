//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//

#include "cbase.h"

#include "in_player.h"
#include "in_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
// Debemos colocar un jugador en el servidor
//================================================================================
void ClientPutInServer( edict_t *pEdict, const char *pPlayername )
{
    CBasePlayer *pPlayer = CBasePlayer::CreatePlayer( "player", pEdict );
    pPlayer->SetPlayerName( pPlayername );
}

//================================================================================
// El jugador acaba de conectarse al servidor
// NOTA: Algunas funciones (como detectar si es un bot) aún no funcionan aquí
//================================================================================
void ClientActive( edict_t *pEdict, bool bLoadGame )
{
    CPlayer *pPlayer = ToInPlayer( pEdict );
    Assert( pPlayer );

    if ( !pPlayer )
        return;

    pPlayer->InitialSpawn();
    Warning( "[%s] Client Active \n", pPlayer->GetPlayerName() );
}

//================================================================================
// El jugador se ha conectado por completo en el servidor
// NOTA: Los Bots no llaman a esta función
//================================================================================
void ClientFullyConnect( edict_t *pEdict )
{
    CPlayer *pPlayer = ToInPlayer( pEdict );
    Assert( pPlayer );

    if ( !pPlayer )
        return;

    pPlayer->Spawn();

    FOR_EACH_VEC( pPlayer->GetSplitScreenPlayers(), it )
    {
        CPlayer *pPartner = ToInPlayer( pPlayer->GetSplitScreenPlayers()[it] );
        if ( !pPartner ) continue;
        pPartner->Spawn();
    }

    Warning( "[%s] Client Fully Connect \n", pPlayer->GetPlayerName() );
}

//================================================================================
//================================================================================
const char *GetGameDescription()
{
    if ( TheGameRules )
        return TheGameRules->GetGameDescription();
    else
        return "InSource";
}

//================================================================================
//================================================================================
PRECACHE_REGISTER_BEGIN( GLOBAL, ClientGamePrecache )
    PRECACHE( MODEL, "models/player.mdl");
    PRECACHE( KV_DEP_FILE, "resource/ParticleEmitters.txt" )
PRECACHE_REGISTER_END()

//================================================================================
//================================================================================
void ClientGamePrecache()
{
    Warning("[ClientGamePrecache] \n");
}

extern void BotThink();

//================================================================================
//================================================================================
void GameStartFrame()
{
    VPROF("GameStartFrame()");
    BotThink();
}