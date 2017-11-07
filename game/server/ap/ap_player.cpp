//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#include "cbase.h"
#include "ap_player.h"

#include "in_gamerules.h"
#include "in_utils.h"
#include "players_system.h"
#include "director.h"

#include "ap_bot.h"
#include "predicted_viewmodel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
// Declarations
//================================================================================

// ID of the viewmodel representing the hands
#define HANDS_VIEWMODEL_INDEX 1

// Tank Model
#define INFECTED_TANK_MODEL "models/infected/hulk.mdl"

// Main Player Model
#define SURVIVOR_ABIGAIL_MODEL "models/survivors/survivor_teenangst.mdl"

// Soldier models
// PLAYER_CLASS_SOLDIER_LEVEL1
static const char *g_SoldierModels[] = {
    "models/survivors/survivor_coach.mdl",
    "models/survivors/survivor_producer.mdl"
};

#define SOLDIER_MODEL g_SoldierModels[RandomInt(0, (ARRAYSIZE(g_SoldierModels)-1))]

// PLAYER_CLASS_SOLDIER_LEVEL3
#define SOLDIER_BULLDOZER_MODEL "models/survivors/survivor_biker.mdl" 
#define SOLDIER_BULLDOZER_UH_MODEL "models/survivors/survivor_biker.mdl"

// PLAYER_CLASS_SOLDIER_LEVEL2
#define SOLDIER_CLOAKER_MODEL "models/survivors/survivor_gambler.mdl" 
#define SOLDIER_CLOAKER_UH_MODEL "models/survivors/survivor_gambler.mdl" 

// PLAYER_CLASS_SOLDIER_MEDIC
#define SOLDIER_MEDIC_MODEL "models/survivors/survivor_mechanic.mdl"

// List of models of hands to make precache
static const char *g_HandsModels[] = {
    "models/weapons/arms/v_arms_zoey.mdl",
    "models/weapons/arms/v_arms_francis.mdl",
    "models/weapons/arms/v_arms_coach_new.mdl",
    "models/weapons/arms/v_arms_gambler_new.mdl",
    "models/weapons/arms/v_arms_mechanic_new.mdl",
    "models/weapons/arms/v_arms_producer_new.mdl"
};

//================================================================================
// Commands
//================================================================================

//================================================================================
// Information and network
//================================================================================

LINK_ENTITY_TO_CLASS(player, CAP_Player);
PRECACHE_REGISTER(player);

IMPLEMENT_SERVERCLASS_ST(CAP_Player, DT_AP_Player)
END_SEND_TABLE()

//================================================================================
// Classification of the entity
//================================================================================
Class_T CAP_Player::Classify()
{
    if ( IsBot() ) {
        // Brother of Abigail
        if ( FStrEq(GetPlayerName(), "Adan") ) {
            return CLASS_PLAYER_ALLY_VITAL;
        }

        // It is in the player's squad.
        if ( GetSquad() && GetSquad()->IsNamed("player") ) {
            if ( TheGameRules->IsGameMode(GAME_MODE_ASSAULT) ) {
                return CLASS_PLAYER_ALLY_VITAL;
            }
            else {
                return CLASS_PLAYER_ALLY;
            }
        }

        // Common survivor
        if ( IsSurvivor() ) {
            return CLASS_HUMAN;
        }
    }

    // Soldier
    if ( IsSoldier() ) {
        return CLASS_SOLDIER;
    }

    // Infected
    if ( IsTank() ) {
        return CLASS_INFECTED_BOSS;
    }

    if ( IsInfected() ) {
        return CLASS_INFECTED;
    }

    // Main player
    return CLASS_PLAYER;
}

//================================================================================
// Create the artificial intelligence of the bot
//================================================================================
void CAP_Player::SetUpBot()
{
    SetBotController(new CAP_Bot(this));
}

//================================================================================
//================================================================================
void CAP_Player::InitialSpawn()
{
    BaseClass::InitialSpawn();
}

//================================================================================
//================================================================================
void CAP_Player::Spawn()
{
    if ( IsSoldier() && GetPlayerClass() == PLAYER_CLASS_NONE ) {
        SetPlayerClass(PLAYER_CLASS_SOLDIER_LEVEL1);
        return;
    }

    BaseClass::Spawn();
}

//================================================================================
// It is called before processing the movement and normal thinking
//================================================================================
void CAP_Player::PreThink()
{
    BaseClass::PreThink();
}

//================================================================================
// It is called after PreThink but before processing the movement
//================================================================================
void CAP_Player::PlayerThink()
{
    BaseClass::PlayerThink();

    // Tank Idle & Alert sounds
    if ( IsTank() ) {
        if ( m_nIdleSoundTimer.IsElapsed() ) {
            if ( IsIdle() ) {
                IdleSound();
            }
            else if ( IsAlerted() ) {
                AlertSound();
            }

            m_nIdleSoundTimer.Start(RandomInt(6, 12));
        }
    }

    SetNextThink(gpGlobals->curtime + 0.1f);
}

//================================================================================
// It is called after processing the movement and normal thinking
//================================================================================
void CAP_Player::PostThink()
{
    BaseClass::PostThink();

    if ( IsAlive() && IsActive() ) {
        // Fidget animation
        if ( ShouldFidget() ) {
            DoAnimationEvent(PLAYERANIMEVENT_CUSTOM_GESTURE, ACT_TERROR_FIDGET);
            m_nFidgetTimer.Start(RandomFloat(10.0f, 25.0f));
        }

        // Nobody likes to be under water
        if ( GetWaterLevel() >= WL_Feet ) {
            AddAttributeModifier("stress_water");
        }
    }
}

//================================================================================
//================================================================================
void CAP_Player::Precache()
{
    BaseClass::Precache();

    PrecacheModel(SOLDIER_BULLDOZER_MODEL);
    PrecacheModel(SOLDIER_BULLDOZER_UH_MODEL);
    PrecacheModel(SOLDIER_CLOAKER_MODEL);
    PrecacheModel(SOLDIER_CLOAKER_UH_MODEL);
    PrecacheModel(SOLDIER_MEDIC_MODEL);

    PrecacheModel(SURVIVOR_ABIGAIL_MODEL);
    PrecacheModel(INFECTED_TANK_MODEL);

    for ( int it = 0; it < ARRAYSIZE(g_SoldierModels); ++it ) {
        PrecacheModel(g_SoldierModels[it]);
    }

    for ( int it = 0; it < ARRAYSIZE(g_HandsModels); ++it ) {
        PrecacheModel(g_HandsModels[it]);
    }

    PrecacheScriptSound("Tank.Idle");
    PrecacheScriptSound("Tank.Alert");
    PrecacheScriptSound("Tank.Pain");
    PrecacheScriptSound("Tank.Attack");
    PrecacheScriptSound("Tank.Death");
    PrecacheScriptSound("Tank.Hit");
    PrecacheScriptSound("Tank.Punch");
}

//================================================================================
// Player components
//================================================================================
void CAP_Player::CreateComponents()
{
    AddComponent(PLAYER_COMPONENT_HEALTH);
    AddComponent(PLAYER_COMPONENT_EFFECTS);
    //AddComponent( PLAYER_COMPONENT_DEJECTED );
}

//================================================================================
// Player attribute's
//================================================================================
void CAP_Player::CreateAttributes()
{
    AddAttribute("health");
    AddAttribute("stamina");
    AddAttribute("stress");

    if ( TheGameRules->GetGameMode() == GAME_MODE_ASSAULT ) {
        AddAttribute("shield");
    }
}

//================================================================================
// The player has entered a new state
//================================================================================
void CAP_Player::EnterPlayerState(int status)
{
    BaseClass::EnterPlayerState(status);

    switch ( status ) {
        // You have entered the server, now you must choose your team.
        case PLAYER_STATE_WELCOME:
        {
            SetPlayerState(PLAYER_STATE_PICKING_TEAM);
            break;
        }

        // Welcome to the game
        case PLAYER_STATE_ACTIVE:
        {
            // All the soldiers are given a pistol at least
            if ( IsSoldier() ) {
                GiveNamedItem("weapon_pistol");
            }

            if ( TheGameRules->GetGameMode() == GAME_MODE_ASSAULT ) {
                if ( IsSurvivor() ) {
                    SetSquad("player");
                }
            }

            break;
        }
    }
}

//================================================================================
// Set a random team for the player.
//================================================================================
void CAP_Player::SetRandomTeam()
{
    ChangeTeam(RandomInt(TEAM_HUMANS, TEAM_SOLDIERS));
    // TODO: TEAM_INFECTED = Buggy
}

//================================================================================
// The player has selected a class
//================================================================================
void CAP_Player::OnPlayerClass(int playerClass)
{
    BaseClass::OnPlayerClass(playerClass);

    // Ready for the game
    Spawn();
}

//================================================================================
// Set a random class for the player.
//================================================================================
void CAP_Player::SetRandomPlayerClass()
{
    switch ( GetTeamNumber() ) {
        case TEAM_HUMANS:
        {
            SetPlayerClass(PLAYER_CLASS_NONE);
            break;
        }

        case TEAM_SOLDIERS:
        {
            SetPlayerClass(RandomInt(PLAYER_CLASS_SOLDIER_LEVEL1, PLAYER_CLASS_SOLDIER_MEDIC));
            break;
        }

        case TEAM_INFECTED:
        {
            SetPlayerClass(RandomInt(PLAYER_CLASS_INFECTED_COMMON, PLAYER_CLASS_INFECTED_BOSS));
            break;
        }

        default:
        {
            Assert(!"An attempt has been made to establish a random class without a selected team.");
            SetPlayerClass(PLAYER_CLASS_NONE);
            break;
        }
    }
}

//================================================================================
//================================================================================
void CAP_Player::ChangeTeam(int iTeamNum)
{
    BaseClass::ChangeTeam(iTeamNum);

    if ( GetTeamNumber() != iTeamNum )
        return;

    switch ( iTeamNum ) {
        case TEAM_HUMANS:
        case TEAM_SOLDIERS:
        {
            // We have flashlight
            SetFlashlightEnabled(true);
            m_nFidgetTimer.Start(5.0f);

            // The survivors do not have classes for now.
            if ( iTeamNum == TEAM_HUMANS ) {
                SetPlayerClass(PLAYER_CLASS_NONE);
            }

            // Select what kind of soldier you want to be
            if ( iTeamNum == TEAM_SOLDIERS ) {
                SetPlayerState(PLAYER_STATE_PICKING_CLASS);
            }

            break;
        }

        case TEAM_INFECTED:
        {
            // Infected people do not have a flashlight
            SetFlashlightEnabled(false);
            SetPlayerClass(PLAYER_CLASS_NONE);
            break;
        }
    }
}

//================================================================================
// Returns the model the player will use
//================================================================================
const char *CAP_Player::GetPlayerModel()
{
    if ( IsSoldier() ) {
        if ( GetPlayerClass() == PLAYER_CLASS_SOLDIER_LEVEL1 ) {
            return SOLDIER_MODEL;
        }

        if ( GetPlayerClass() == PLAYER_CLASS_SOLDIER_LEVEL2 ) {
            if ( GetDifficultyLevel() >= SKILL_ULTRA_HARD )
                return SOLDIER_CLOAKER_UH_MODEL;
            else
                return SOLDIER_CLOAKER_MODEL;
        }

        if ( GetPlayerClass() == PLAYER_CLASS_SOLDIER_LEVEL3 ) {
            if ( GetDifficultyLevel() >= SKILL_ULTRA_HARD )
                return SOLDIER_BULLDOZER_UH_MODEL;
            else
                return SOLDIER_BULLDOZER_MODEL;
        }

        if ( GetPlayerClass() == PLAYER_CLASS_SOLDIER_MEDIC ) {
            return SOLDIER_MEDIC_MODEL;
        }
    }

    // TODO: 
    if ( IsSurvivor() ) {
        return SURVIVOR_ABIGAIL_MODEL;
    }

    // 
    if ( IsTank() ) {
        return INFECTED_TANK_MODEL;
    }

    return BaseClass::GetPlayerModel();
}

//================================================================================
// Prepare the model with a visual style
//================================================================================
void CAP_Player::SetUpModel()
{
    if ( IsSurvivor() ) {
        SetSkin(RandomInt(0, 1));
        SetBodygroup(1, RandomInt(0, 6)); // Hair
        SetBodygroup(2, RandomInt(0, 6)); // Upper body
        SetBodygroup(3, RandomInt(0, 6)); // Lower body
    }
}

//================================================================================
//================================================================================
const char *CAP_Player::GetPlayerType()
{
    if ( IsSoldier() )
        return "Soldier";

    if ( IsSurvivor() )
        return "Abigail";

    if ( IsTank() )
        return "Tank";

    return BaseClass::GetPlayerType();
}

//================================================================================
//================================================================================
gender_t CAP_Player::GetPlayerGender()
{
    if ( IsSoldier() )
        return GENDER_MALE;

    if ( IsSurvivor() )
        return GENDER_FEMALE;

    return GENDER_MALE;
}

//================================================================================
//================================================================================
void CAP_Player::IdleSound()
{
    if ( IsTank() ) {
        EmitSound("Tank.Idle");
    }
}

//================================================================================
//================================================================================
void CAP_Player::AlertSound()
{
    if ( IsTank() ) {
        EmitSound("Tank.Alert");
    }
}

//================================================================================
//================================================================================
void CAP_Player::PainSound(const CTakeDamageInfo &info)
{
    if ( IsTank() ) {
        EmitSound("Tank.Pain");
    }
    else {
        BaseClass::PainSound(info);
    }
}

//================================================================================
//================================================================================
void CAP_Player::DeathSound(const CTakeDamageInfo &info)
{
    if ( !TheGameRules->FCanPlayDeathSound(info) ) {
        return;
    }

    if ( IsTank() ) {
        EmitSound("Tank.Death");
    }
    else {
        BaseClass::DeathSound(info);
    }
}

//================================================================================
// Returns the model of the hands for the player
//================================================================================
const char *CAP_Player::GetHandsModel(int viewmodelindex)
{
    Assert(GetTeamNumber() != TEAM_UNASSIGNED);

    if ( GetTeamNumber() == TEAM_UNASSIGNED )
        return NULL;

    const char *pModel = STRING(GetModelName());

    if ( FStrEq(pModel, SOLDIER_BULLDOZER_MODEL) )
        return "models/weapons/arms/v_arms_francis.mdl";

    if ( FStrEq(pModel, SOLDIER_CLOAKER_MODEL) )
        return "models/weapons/arms/v_arms_gambler_new.mdl";

    if ( FStrEq(pModel, SOLDIER_MEDIC_MODEL) )
        return "models/weapons/arms/v_arms_mechanic_new.mdl";

    if ( FStrEq(pModel, "models/survivors/survivor_coach.mdl") )
        return "models/weapons/arms/v_arms_coach_new.mdl";

    if ( FStrEq(pModel, "models/survivors/survivor_producer.mdl") )
        return "models/weapons/arms/v_arms_producer_new.mdl";

    return "models/weapons/arms/v_arms_zoey.mdl";
}

//================================================================================
// Create the model of the hands or other extensions for the viewmodel.
//================================================================================
void CAP_Player::SetUpHands()
{
    CreateHands(HANDS_VIEWMODEL_INDEX);
}

//================================================================================
// The player has entered a new condition
//================================================================================
void CAP_Player::OnPlayerStatus(int oldStatus, int status)
{
    BaseClass::OnPlayerStatus(oldStatus, status);

    switch ( status ) {
        case PLAYER_STATUS_DEJECTED:
        {
            DoAnimationEvent(PLAYERANIMEVENT_CUSTOM, ACT_DIESIMPLE);
            SetNextAttack(gpGlobals->curtime + SequenceDuration());
            break;
        }

        case PLAYER_STATUS_CLIMBING:
        {
            DoAnimationEvent(PLAYERANIMEVENT_CUSTOM, ACT_TERROR_FALL_GRAB_LEDGE);
            SetNextAttack(gpGlobals->curtime + SequenceDuration());
            break;
        }
    }
}

//================================================================================
//================================================================================
void CAP_Player::OnNewLeader(CPlayer * pMember)
{
}

//================================================================================
// Returns if we can reproduce the animation
//================================================================================
bool CAP_Player::ShouldFidget()
{
    if ( GetPlayerStatus() != PLAYER_STATUS_NONE )
        return false;

    if ( IsOnCombat() || IsUnderAttack() )
        return false;

    return m_nFidgetTimer.IsElapsed();
}

//================================================================================
//================================================================================
void CAP_Player::HandleAnimEvent(animevent_t * event)
{
    if ( IsTank() ) {
        // In each step we make the ground shake
        if ( event->Event() == AE_PLAYER_FOOTSTEP_LEFT || event->Event() == AE_PLAYER_FOOTSTEP_RIGHT ) {
            CUtlVector<CBasePlayer *> *ignoreList = new CUtlVector<CBasePlayer *>();
            ignoreList->AddToTail(this);
            UTIL_ScreenShake(GetAbsOrigin(), 3.0f, 1.0f, 1.0f, 700.0f, SHAKE_START, false, ignoreList);
        }
    }

    BaseClass::HandleAnimEvent(event);
}

//================================================================================
//================================================================================
bool CAP_Player::ClientCommand(const CCommand & args)
{
    if ( FStrEq(args[0], "taunt_salute") ) {
        DoAnimationEvent(PLAYERANIMEVENT_CUSTOM_GESTURE, ACT_GMOD_TAUNT_SALUTE);
        return true;
    }

    if ( FStrEq(args[0], "taunt_forward") ) {
        DoAnimationEvent(PLAYERANIMEVENT_CUSTOM_GESTURE, ACT_GMOD_SIGNAL_FORWARD);
        return true;
    }

    if ( FStrEq(args[0], "taunt_group") ) {
        DoAnimationEvent(PLAYERANIMEVENT_CUSTOM_GESTURE, ACT_GMOD_SIGNAL_GROUP);
        return true;
    }

    if ( FStrEq(args[0], "iktest") ) {
        DoAnimationEvent(PLAYERANIMEVENT_CUSTOM_SEQUENCE, LookupSequence("Idle_subtle"));
        return true;
    }

    return BaseClass::ClientCommand(args);
}
