//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

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
// Definiciones
//================================================================================

#define HANDS_VIEWMODEL_INDEX 1

// Infectados
#define INFECTED_TANK_MODEL "models/infected/hulk.mdl"

#ifdef USE_L4D2_MODELS

// PLAYER
#define SURVIVOR_ABIGAIL_MODEL "models/survivors/survivor_teenangst.mdl"

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

#else

#define SOLDIER_LEADER_MODEL "models/payday2/units/heavy_swat_player.mdl"
#define SOLDIER_SNIPER_MODEL "models/payday2/units/heavy_swat_player.mdl"

#define SOLDIER_BULLDOZER_MODEL "models/mark2580/payday2/pd2_bulldozer_player.mdl" // PLAYER_CLASS_SOLDIER_LEVEL3
#define SOLDIER_BULLDOZER_UH_MODEL "models/mark2580/payday2/pd2_bulldozer_zeal_player.mdl" // PLAYER_CLASS_SOLDIER_LEVEL3

#define SOLDIER_CLOAKER_MODEL "models/mark2580/payday2/pd2_cloaker_player.mdl" // PLAYER_CLASS_SOLDIER_LEVEL2
#define SOLDIER_CLOAKER_UH_MODEL "models/mark2580/payday2/pd2_cloaker_zeal_player.mdl" // PLAYER_CLASS_SOLDIER_LEVEL2

#define SOLDIER_MEDIC_MODEL "models/payday2/units/medic_player.mdl" // PLAYER_CLASS_SOLDIER_MEDIC

#define SURVIVOR_ABIGAIL_MODEL "models/frosty/jacketless/j_alyx.mdl"
#define SURVIVOR_COMMON_MODEL ""

// PLAYER_CLASS_SOLDIER_LEVEL1
static const char *g_SoldierModels[] = {
    "models/payday2/units/heavy_swat_player.mdl"
};
static const char *g_SoldierUltraHardModels[] = {
    "models/mark2580/payday2/pd2_gs_elite_player.mdl"
};

#define SOLDIER_MODEL g_SoldierModels[RandomInt(0, (ARRAYSIZE(g_SoldierModels)-1))]
#define SOLDIER_UH_MODEL g_SoldierUltraHardModels[RandomInt(0, (ARRAYSIZE(g_SoldierUltraHardModels)-1))]
#endif

static const char *g_HandsModels[] = {
#ifdef USE_L4D2_MODELS
    "models/weapons/arms/v_arms_zoey.mdl",
    "models/weapons/arms/v_arms_francis.mdl",
    "models/weapons/arms/v_arms_coach_new.mdl",
    "models/weapons/arms/v_arms_gambler_new.mdl",
    "models/weapons/arms/v_arms_mechanic_new.mdl",
    "models/weapons/arms/v_arms_producer_new.mdl"
#else
    "models/weapons/c_arms_refugee.mdl",
    "models/mark2580/payday2/bulldozer_c_arms.mdl",
    "models/mark2580/payday2/pd2_bulldozer_zeal_c_arms.mdl",
    "models/mark2580/payday2/c_arms_cloaker.mdl",
    "models/mark2580/payday2/pd2_cloaker_zeal_c_arms.mdl",
    "models/payday2/units/medic_arms.mdl",
    "models/payday2/units/heavy_swat_arms.mdl",
    "models/mark2580/payday2/gensec_c_arms.mdl",
#endif
};

//================================================================================
// Comandos
//================================================================================

//================================================================================
// Información y Red
//================================================================================

LINK_ENTITY_TO_CLASS( player, CAP_Player );
PRECACHE_REGISTER( player );

IMPLEMENT_SERVERCLASS_ST( CAP_Player, DT_AP_Player )
END_SEND_TABLE()

//================================================================================
//================================================================================
Class_T CAP_Player::Classify()
{
    // Es un Bot
    if ( IsBot() ) {
        if ( FStrEq( GetPlayerName(), "Adan" ) ) {
            return CLASS_PLAYER_ALLY_VITAL;
        }

        if ( GetSquad() && GetSquad()->IsNamed( "player" ) ) {
            if ( TheGameRules->GetGameMode() == GAME_MODE_ASSAULT )
                return CLASS_PLAYER_ALLY_VITAL;
            else
                return CLASS_PLAYER_ALLY;
        }

        if ( IsSurvivor() ) {
            return CLASS_HUMAN;
        }
    }

    if ( IsSoldier() ) {
        return CLASS_SOLDIER;
    }

    if ( IsTank() ) {
        return CLASS_INFECTED_BOSS;
    }

    if ( IsInfected() ) {
        return CLASS_INFECTED;
    }

    return CLASS_PLAYER;
}

//================================================================================
// Prepara el jugador para ser controlado por la I.A.
//================================================================================
void CAP_Player::SetUpAI()
{
    SetBotController( new CBot(this) );

    /*
    if ( IsSoldier() ) {
        SetAI( new CAP_BotSoldier() );
    }
    else {
        SetAI( new CAP_Bot() );
    }*/
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
    BaseClass::Spawn();
}

//================================================================================
// Pre-pensamiento
// Se llama antes de procesar el movimiento y el pensamiento normal
//================================================================================
void CAP_Player::PreThink()
{
    BaseClass::PreThink();
}

//================================================================================
// Pensamiento normal
// Se llama después de PreThink pero antes de procesar el movimiento
//================================================================================
void CAP_Player::PlayerThink()
{
    BaseClass::PlayerThink();

    if ( IsTank() ) {
        if ( m_nIdleSoundTimer.IsElapsed() ) {
            if ( IsIdle() )
                IdleSound();
            else if ( IsAlerted() )
                AlertSound();

            m_nIdleSoundTimer.Start( RandomInt( 6, 12 ) );
        }
    }

    SetNextThink( gpGlobals->curtime + 0.1f );
}

//================================================================================
// Post-pensamiento
// Se llama después de procesar el movimiento y el pensamiento normal
//================================================================================
void CAP_Player::PostThink()
{
    BaseClass::PostThink();

    if ( IsAlive() && IsActive() ) {
#ifdef USE_L4D2_MODELS
        if ( ShouldFidget() ) {
            DoAnimationEvent( PLAYERANIMEVENT_CUSTOM_GESTURE, ACT_TERROR_FIDGET );
            m_nFidgetTimer.Start( RandomFloat( 10.0f, 25.0f ) );
        }
#endif

        // A nadie le gusta estar bajo el agua
        if ( GetWaterLevel() >= WL_Feet ) {
            AddAttributeModifier( "stress_water" );
        }
    }
}

//================================================================================
//================================================================================
void CAP_Player::Precache() 
{
    BaseClass::Precache();

    PrecacheModel( SOLDIER_BULLDOZER_MODEL );
    PrecacheModel( SOLDIER_BULLDOZER_UH_MODEL );
    PrecacheModel( SOLDIER_CLOAKER_MODEL );
    PrecacheModel( SOLDIER_CLOAKER_UH_MODEL );
    PrecacheModel( SOLDIER_MEDIC_MODEL );

    PrecacheModel( SURVIVOR_ABIGAIL_MODEL );
    PrecacheModel( INFECTED_TANK_MODEL );

    for ( int it = 0; it < ARRAYSIZE( g_SoldierModels ); ++it ) {
        PrecacheModel( g_SoldierModels[it] );
    }

    for ( int it = 0; it < ARRAYSIZE( g_HandsModels ); ++it ) {
        PrecacheModel( g_HandsModels[it] );
    }

#ifndef USE_L4D2_MODELS
    PrecacheModel( SOLDIER_LEADER_MODEL );
    PrecacheModel( SOLDIER_SNIPER_MODEL );

    for ( int it = 0; it < ARRAYSIZE( g_SoldierUltraHardModels ); ++it ) {
        PrecacheModel( g_SoldierUltraHardModels[it] );
    }
#endif

    PrecacheScriptSound( "Tank.Idle" );
    PrecacheScriptSound( "Tank.Alert" );
    PrecacheScriptSound( "Tank.Pain" );
    PrecacheScriptSound( "Tank.Attack" );
    PrecacheScriptSound( "Tank.Death" );
    PrecacheScriptSound( "Tank.Hit" );
    PrecacheScriptSound( "Tank.Punch" );
}

//================================================================================
//================================================================================
void CAP_Player::CreateComponents()
{
    AddComponent( PLAYER_COMPONENT_HEALTH );
    AddComponent( PLAYER_COMPONENT_EFFECTS );
    //AddComponent( PLAYER_COMPONENT_DEJECTED );
}

//================================================================================
//================================================================================
void CAP_Player::CreateAttributes()
{
    AddAttribute( "health" );
    AddAttribute( "stamina" );
    AddAttribute( "stress" );
    AddAttribute( "shield" );
}

//================================================================================
//================================================================================
void CAP_Player::EnterPlayerState( int status )
{
    BaseClass::EnterPlayerState( status );

    switch ( status ) {
        case PLAYER_STATE_WELCOME:
        {
            SetPlayerState( PLAYER_STATE_PICKING_TEAM );
            break;
        }

        case PLAYER_STATE_ACTIVE:
        {
            if ( IsSoldier() ) {
                GiveNamedItem( "weapon_pistol" );
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
	ChangeTeam( RandomInt(TEAM_HUMANS, TEAM_SOLDIERS) );
	// TODO: TEAM_INFECTED = Buggy
}

//================================================================================
//================================================================================
void CAP_Player::OnPlayerClass( int playerClass )
{
    BaseClass::OnPlayerClass( playerClass );
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
			SetPlayerClass( PLAYER_CLASS_NONE );
			break;
		}

		case TEAM_SOLDIERS:
		{
			SetPlayerClass( RandomInt( PLAYER_CLASS_SOLDIER_LEVEL1, PLAYER_CLASS_SOLDIER_MEDIC ) );
			break;
		}

		case TEAM_INFECTED:
		{
			SetPlayerClass( RandomInt( PLAYER_CLASS_INFECTED_COMMON, PLAYER_CLASS_INFECTED_BOSS ) );
			break;
		}

		default:
		{
			Assert( !"An attempt has been made to establish a random class without a selected team." );
			SetPlayerClass( PLAYER_CLASS_NONE );
			break;
		}
	}
}

//================================================================================
//================================================================================
void CAP_Player::ChangeTeam( int iTeamNum )
{
    BaseClass::ChangeTeam( iTeamNum );

    if ( GetTeamNumber() != iTeamNum )
        return;

    switch ( iTeamNum ) {
        case TEAM_HUMANS:
        case TEAM_SOLDIERS:
        {
            SetFlashlightEnabled( true );

            m_takedamage = DAMAGE_YES;
            m_nFidgetTimer.Start( 5.0f );

            if ( iTeamNum == TEAM_HUMANS ) {
                SetPlayerClass( PLAYER_CLASS_NONE );
            }

            if ( iTeamNum == TEAM_SOLDIERS ) {
                SetPlayerState( PLAYER_STATE_PICKING_CLASS );
            }

            break;
        }

        case TEAM_INFECTED:
        {
            SetFlashlightEnabled( false );
            SetPlayerClass( PLAYER_CLASS_NONE );
            break;
        }
    }
}

//================================================================================
//================================================================================
const char *CAP_Player::GetPlayerModel()
{
    if ( IsSoldier() ) {
        if ( GetPlayerClass() == PLAYER_CLASS_SOLDIER_LEVEL1 || GetPlayerClass() == PLAYER_CLASS_NONE ) {
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

    if ( IsSurvivor() ) {
        return SURVIVOR_ABIGAIL_MODEL;
    }

    if ( IsTank() ) {
        return INFECTED_TANK_MODEL;
    }

    return BaseClass::GetPlayerModel();
}

//================================================================================
//================================================================================
void CAP_Player::SetUpModel()
{
    if ( IsSoldier() ) {
#ifndef USE_L4D2_MODELS
        // Cloaker
        if ( GetPlayerClass() == PLAYER_CLASS_SOLDIER_LEVEL2 ) {
            if ( GetDifficultyLevel() < SKILL_ULTRA_HARD ) {
                SetSkin( 1 );
            }
        }

        // Bulldozer
        if ( GetPlayerClass() == PLAYER_CLASS_SOLDIER_LEVEL3 ) {
            if ( GetDifficultyLevel() >= SKILL_ULTRA_HARD ) {
                SetSkin( RandomInt(1, 2) );
            }
            else {
                SetBodygroup( 2, 0 ); // Cubierta trasera
                SetBodygroup( 3, 0 ); // Cubierta delantera

                switch ( GetDifficultyLevel() ) {
                    case SKILL_EASY:
                        SetSkin( 0 ); // Original
                        SetBodygroup( 1, RandomInt( 1, 2 ) ); // Casco: Protección transparente
                        SetBodygroup( 2, 1 ); // Sin cubierta trasera
                        SetBodygroup( 3, 1 ); // Sin cubierta delantera
                        break;

                    case SKILL_MEDIUM:
                        SetSkin( 0 ); // Original
                        SetBodygroup( 1, 0 ); // Casco: Cubierto
                        break;

                    case SKILL_HARD:
                        SetSkin( 1 ); // Negro
                        SetBodygroup( 1, RandomInt( 0, 1 ) );
                        break;

                    case SKILL_VERY_HARD:
                        SetSkin( RandomInt( 2, 4 ) ); // Blanco/Azul - Calabera/Calabera gatito - Blanco
                        SetBodygroup( 1, 0 );
                        break;
                }
            }
        }
#endif
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
        EmitSound( "Tank.Idle" );
    }
}

//================================================================================
//================================================================================
void CAP_Player::AlertSound()
{
    if ( IsTank() ) {
        EmitSound( "Tank.Alert" );
    }
}

//================================================================================
//================================================================================
void CAP_Player::PainSound( const CTakeDamageInfo &info )
{
    if ( IsTank() ) {
        EmitSound( "Tank.Pain" );
    }
    else {
        BaseClass::PainSound( info );
    }
}

//================================================================================
//================================================================================
void CAP_Player::DeathSound( const CTakeDamageInfo &info )
{
    // Este tipo de muerte no emite sonido
    if ( !TheGameRules->FCanPlayDeathSound( info ) ) return;

    if ( IsTank() ) {
        EmitSound( "Tank.Death" );
    }
    else {
        BaseClass::DeathSound( info );
    }
}

//================================================================================
//================================================================================
const char * CAP_Player::GetHandsModel( int viewmodelindex )
{
    if ( GetTeamNumber() == TEAM_UNASSIGNED )
        return NULL;

#ifdef USE_L4D2_MODELS
    const char *pModel = STRING( GetModelName() );

    if ( FStrEq( pModel, SOLDIER_BULLDOZER_MODEL ) )
        return "models/weapons/arms/v_arms_francis.mdl";

    if ( FStrEq( pModel, SOLDIER_CLOAKER_MODEL ) )
        return "models/weapons/arms/v_arms_gambler_new.mdl";

    if ( FStrEq( pModel, SOLDIER_MEDIC_MODEL ) )
        return "models/weapons/arms/v_arms_mechanic_new.mdl";

    if ( FStrEq( pModel, "models/survivors/survivor_coach.mdl" ) )
        return "models/weapons/arms/v_arms_coach_new.mdl";

    if ( FStrEq( pModel, "models/survivors/survivor_producer.mdl" ) )
        return "models/weapons/arms/v_arms_producer_new.mdl";

    return "models/weapons/arms/v_arms_zoey.mdl";
#else
    const char *pModel = STRING( GetModelName() );

    if ( FStrEq( pModel, SOLDIER_BULLDOZER_MODEL ) )
        return "models/mark2580/payday2/bulldozer_c_arms.mdl";

    if ( FStrEq( pModel, SOLDIER_BULLDOZER_UH_MODEL ) )
        return "models/mark2580/payday2/pd2_bulldozer_zeal_c_arms.mdl";

    if ( FStrEq( pModel, SOLDIER_CLOAKER_MODEL ) )
        return "models/mark2580/payday2/c_arms_cloaker.mdl";

    if ( FStrEq( pModel, SOLDIER_CLOAKER_UH_MODEL ) )
        return "models/mark2580/payday2/pd2_cloaker_zeal_player.mdl";

    if ( FStrEq( pModel, SOLDIER_MEDIC_MODEL ) )
        return "models/payday2/units/medic_arms.mdl";

    if ( FStrEq( pModel, "models/payday2/units/heavy_swat_player.mdl" ) )
        return "models/payday2/units/heavy_swat_arms.mdl";

    if ( FStrEq( pModel, "models/mark2580/payday2/pd2_gs_elite_player.mdl" ) )
        return "models/mark2580/payday2/gensec_c_arms.mdl";

    return "models/weapons/c_arms_refugee.mdl";
#endif
}

//================================================================================
// El modelo ha sido establecido, ya podemos crear nuestras manos
//================================================================================
void CAP_Player::SetUpHands()
{
    CreateHands( HANDS_VIEWMODEL_INDEX );
}

//================================================================================
// Establece el estado del jugador
//================================================================================
void CAP_Player::OnPlayerStatus( int oldStatus, int status )
{
    BaseClass::OnPlayerStatus( oldStatus, status );

    switch ( status ) {
        case PLAYER_STATUS_NONE:
            break;

        case PLAYER_STATUS_DEJECTED:
            DoAnimationEvent( PLAYERANIMEVENT_CUSTOM, ACT_DIESIMPLE );
            SetNextAttack( gpGlobals->curtime + SequenceDuration() );
            break;

        case PLAYER_STATUS_CLIMBING:
            DoAnimationEvent( PLAYERANIMEVENT_CUSTOM, ACT_TERROR_FALL_GRAB_LEDGE );
            break;

        case PLAYER_STATUS_FALLING:   
            break;
    }
}

//================================================================================
//================================================================================
void CAP_Player::OnNewLeader( CPlayer * pMember )
{
    // ¡Somos nosotros!
    if ( pMember == this ) {
        if ( IsSoldier() ) {
            //SetModel( SOLDIER_LEADER_MODEL );
        }
    }
}

//================================================================================
//================================================================================
bool CAP_Player::ShouldFidget()
{
    // Solo si estamos normal
    if ( GetPlayerStatus() != PLAYER_STATUS_NONE )
        return false;

    // Esperamos a estar relajados
    if ( IsOnCombat() || IsUnderAttack() )
        return false;

    return m_nFidgetTimer.IsElapsed();
}

//================================================================================
//================================================================================
void CAP_Player::HandleAnimEvent( animevent_t * event )
{
    if ( IsTank() ) {
        // En cada paso hacemos temblar el suelo, ignorandonos a nosotros mismos
        if ( event->Event() == AE_PLAYER_FOOTSTEP_LEFT || event->Event() == AE_PLAYER_FOOTSTEP_RIGHT ) {
            CUtlVector<CBasePlayer *> *ignoreList = new CUtlVector<CBasePlayer *>();
            ignoreList->AddToTail( this );
            UTIL_ScreenShake( GetAbsOrigin(), 3.0f, 1.0f, 1.0f, 700.0f, SHAKE_START, false, ignoreList );
        }
    }

    BaseClass::HandleAnimEvent( event );
}

//================================================================================
//================================================================================
bool CAP_Player::ClientCommand( const CCommand & args )
{
    // Convertirse en espectador
    if ( FStrEq( args[0], "taunt_salute" ) )
    {
        DoAnimationEvent( PLAYERANIMEVENT_CUSTOM_GESTURE, ACT_GMOD_TAUNT_SALUTE );
        return true;
    }

    // Convertirse en espectador
    if ( FStrEq( args[0], "taunt_forward" ) )
    {
        DoAnimationEvent( PLAYERANIMEVENT_CUSTOM_GESTURE, ACT_GMOD_SIGNAL_FORWARD );
        return true;
    }

    // Convertirse en espectador
    if ( FStrEq( args[0], "taunt_group" ) )
    {
        DoAnimationEvent( PLAYERANIMEVENT_CUSTOM_GESTURE, ACT_GMOD_SIGNAL_GROUP );
        return true;
    }

    if ( FStrEq( args[0], "iktest" ) ) {
        DoAnimationEvent( PLAYERANIMEVENT_CUSTOM_SEQUENCE, LookupSequence("Idle_subtle") );
        return true;
    }

    return BaseClass::ClientCommand( args );
}
