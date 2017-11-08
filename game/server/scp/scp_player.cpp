//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#include "cbase.h"
#include "scp_player.h"

#include "scp_player_features.h"

#include "in_gamerules.h"
#include "gameinterface.h"
#include "in_buttons.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
// Commands
//================================================================================

//================================================================================
// Data and network
//================================================================================

LINK_ENTITY_TO_CLASS(player, CSCP_Player);
PRECACHE_REGISTER(player);

IMPLEMENT_SERVERCLASS_ST(CSCP_Player, DT_SCP_Player)
END_SEND_TABLE()

//================================================================================
// Constructor
//================================================================================
CSCP_Player::CSCP_Player()
{

}

//================================================================================
// Classification of the entity
//================================================================================
Class_T CSCP_Player::Classify()
{
    if ( IsSurvivor() )
        return CLASS_HUMAN;

    if ( IsSoldier() )
        return CLASS_SOLDIER;

    if ( IsMonster() )
        return CLASS_MONSTER;

    return CLASS_PLAYER;
}

//================================================================================
// Initial Spawn, is called once when accessing the server.
//================================================================================
void CSCP_Player::InitialSpawn()
{
    BaseClass::InitialSpawn();
}

//================================================================================
// Precache important assets
//================================================================================
void CSCP_Player::Precache()
{
    BaseClass::Precache();

    PrecacheModel("models/bloocobalt/player/l4d/riot_01.mdl");
    PrecacheModel("models/humans/orange1/player/male_02.mdl");
    PrecacheModel("models/vinrax/scp173/scp173.mdl");
    PrecacheModel("models/shaklin/scp/096/scp_096.mdl");
    PrecacheModel("models/vinrax/player/035_player.mdl");
    PrecacheModel("models/vinrax/player/scp049_player.mdl");
    PrecacheModel("models/vinrax/player/scp106_player.mdl");

    PrecacheScriptSound("SCP173.Attack");
    PrecacheScriptSound("SCP173.Rattle");
}

//================================================================================
// Pensamiento del jugador
//================================================================================
void CSCP_Player::PlayerThink()
{
    SetNextThink(gpGlobals->curtime + 0.05f);

    // SCP
    if ( IsMonster() ) {
        if ( IsButtonPressed(IN_ATTACK2) ) {
            PrimaryAttack();
        }
    }
}

//================================================================================
// Primary attack for SCPs
// TODO: Would this be better on a weapon?
//================================================================================
void CSCP_Player::PrimaryAttack()
{
    Assert(IsMonster());
    const float flDistance = 40.0f;

    // SCP-173
    if ( GetPlayerClass() == PLAYER_CLASS_SCP_173 ) {
        // We make an attack and we get a victim
        CBaseEntity *pVictim = CheckTraceHullAttack(flDistance, -Vector(16, 16, 12), Vector(16, 16, 12), 200.0f, DMG_SLASH, 0.5f);

        if ( !pVictim )
            return;

        EmitSound("SCP173.Attack");
    }
}

//================================================================================
//================================================================================
void CSCP_Player::EnterPlayerState(int state)
{
    BaseClass::EnterPlayerState(state);
}

//================================================================================
// Hemos cambiado de clase
//================================================================================
void CSCP_Player::OnPlayerClass(int playerClass)
{
    if ( playerClass == PLAYER_CLASS_SCP_173 ) {
        // SCP-173 can not take damage
        m_takedamage = DAMAGE_NO;

        // Sonido de movimiento para SCP-173
        m_pMovementSound = new CSoundInstance("SCP173.Movement", this, this);
    }
    else {
        if ( m_pMovementSound ) {
            delete m_pMovementSound;
            m_pMovementSound = NULL;
        }
    }

    // Ready for the game
    Spawn();
}

//================================================================================
// Change the team to where the player belongs
//================================================================================
void CSCP_Player::ChangeTeam(int iTeamNum, bool bAutoTeam, bool bSilent)
{
    BaseClass::ChangeTeam(iTeamNum, bAutoTeam, bSilent);

    if ( GetTeamNumber() != iTeamNum )
        return;

    switch ( iTeamNum ) {
        case TEAM_HUMANS:
        case TEAM_SOLDIERS:
        {
            // We have flashlight
            SetFlashlightEnabled(true);

            // We can jump and crouch
            EnableButtons(IN_JUMP | IN_DUCK);

            // I am mortal!
            m_takedamage = DAMAGE_YES;

            // For now there are no more classes...
            SetPlayerClass(PLAYER_CLASS_NONE);
            break;
        }

        case TEAM_SCP:
        {
            // I am a supreme thing of the unknown, but I can not jump or crouch ...
            SetFlashlightEnabled(false);
            DisableButtons(IN_JUMP | IN_DUCK);

            // Select what kind of SCP you will be
            SetPlayerState(PLAYER_STATE_PICKING_CLASS);
            break;
        }
    }
}

//================================================================================
// Returns the model the player will use
//================================================================================
const char *CSCP_Player::GetPlayerModel()
{
    if ( IsSoldier() )
        return "models/bloocobalt/player/l4d/riot_01.mdl";

    if ( IsSurvivor() )
        return "models/humans/orange1/player/male_02.mdl";

    // SCP-173
    if ( GetPlayerClass() == PLAYER_CLASS_SCP_173 )
        return "models/vinrax/scp173/scp173.mdl";

    // SCP-096
    if ( GetPlayerClass() == PLAYER_CLASS_SCP_096 )
        return "models/shaklin/scp/096/scp_096.mdl";

    // SCP-035
    if ( GetPlayerClass() == PLAYER_CLASS_SCP_035 )
        return "models/vinrax/player/035_player.mdl";

    // SCP-035
    if ( GetPlayerClass() == PLAYER_CLASS_SCP_049 )
        return "models/vinrax/player/scp049_player.mdl";

    // SCP-106
    if ( GetPlayerClass() == PLAYER_CLASS_SCP_106 )
        return "models/vinrax/player/scp106_player.mdl";

    return BaseClass::GetPlayerModel();
}

//================================================================================
// Prepare the model with a visual style
//================================================================================
void CSCP_Player::PrepareModel()
{
    if ( IsSoldier() ) {
        SetBodygroup(1, 1);
        SetBodygroup(2, 1);
    }

    if ( IsSurvivor() ) {
        SetSkin(RandomInt(0, 3));
    }
}

//================================================================================
// Create player components
//================================================================================
void CSCP_Player::CreateComponents()
{
    // SCP-173
    if ( GetPlayerClass() == PLAYER_CLASS_SCP_173 ) {
        AddComponent(new C173BehaviorComponent);
    }
}

//================================================================================
// Returns the name of the entity that will be used to create this player
//================================================================================
const char *CSCP_Player::GetSpawnEntityName()
{
    if ( IsSoldier() )
        return "info_player_soldier";

    // SCP-173
    if ( GetPlayerClass() == PLAYER_CLASS_SCP_173 )
        return "info_player_scp173";

    // SCP-096
    if ( GetPlayerClass() == PLAYER_CLASS_SCP_096 )
        return "info_player_scp096";

    // SCP-035
    if ( GetPlayerClass() == PLAYER_CLASS_SCP_035 )
        return "info_player_scp035";

    // SCP-035
    if ( GetPlayerClass() == PLAYER_CLASS_SCP_035 )
        return "info_player_scp049";

    // SCP-106
    if ( GetPlayerClass() == PLAYER_CLASS_SCP_106 )
        return "info_player_scp106";

    return "info_player_start";
}

//================================================================================
// Procesa un comando enviado directamente desde el cliente
//================================================================================
bool CSCP_Player::ClientCommand(const CCommand & args)
{
    // Unirse a otro equipo
    if ( FStrEq(args[0], "select_class") ) {
        // Solamente los SCP pueden seleccionar una clase
        if ( !IsMonster() )
            return false;

        if ( args.ArgC() < 2 ) {
            Warning("Player sent bad select_class syntax \n");
            return false;
        }

        int playerClass = atoi(args[1]);

        // Establecemos la clase y entramos al juego
        SetPlayerClass(playerClass);
        return true;
    }

    return BaseClass::ClientCommand(args);
}