//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "scp_player.h"

#include "scp_player_features.h"

#include "in_gamerules.h"
#include "gameinterface.h"
#include "in_buttons.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
// Comandos
//================================================================================

//================================================================================
// Información y Red
//================================================================================

LINK_ENTITY_TO_CLASS( player, CSCP_Player );
PRECACHE_REGISTER( player );

IMPLEMENT_SERVERCLASS_ST( CSCP_Player, DT_SCP_Player )
END_SEND_TABLE()

//================================================================================
// Constructor
//================================================================================
CSCP_Player::CSCP_Player()
{
    
}

//================================================================================
// Clasificación del jugador
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
// Creación inicial, cuando el jugador se conecta
//================================================================================
void CSCP_Player::InitialSpawn()
{
    BaseClass::InitialSpawn();

    // Debemos seleccionar un equipo
    SetPlayerState( PLAYER_STATE_PICKING_TEAM );
}

//================================================================================
// 
//================================================================================
void CSCP_Player::Precache()
{
    BaseClass::Precache();

    PrecacheModel( "models/bloocobalt/player/l4d/riot_01.mdl" );
    PrecacheModel( "models/humans/orange1/player/male_02.mdl" );
    PrecacheModel( "models/vinrax/scp173/scp173.mdl" );
    PrecacheModel( "models/shaklin/scp/096/scp_096.mdl" );
    PrecacheModel( "models/vinrax/player/035_player.mdl" );
    PrecacheModel( "models/vinrax/player/scp049_player.mdl" );
    PrecacheModel( "models/vinrax/player/scp106_player.mdl" );

    PrecacheScriptSound( "SCP173.Attack" );
    PrecacheScriptSound( "SCP173.Rattle" );
}

//================================================================================
// Pensamiento del jugador
//================================================================================
void CSCP_Player::PlayerThink()
{
    SetNextThink( gpGlobals->curtime + 0.05f );

    // Es un SCP
    if ( IsMonster() )
    {
        if ( IsButtonPressed( IN_ATTACK2 ) )
            PrimaryAttack();
    }   
}

//================================================================================
// Ataque primario para los SCP
// Quizá en un futuro debamos mover esto al código de un arma
//================================================================================
void CSCP_Player::PrimaryAttack()
{
    float flDistance = 40.0f;

    // SCP-173
    if ( GetPlayerClass() == PLAYER_CLASS_SCP_173 )
    {
        // Realizamos el ataque y obtenemos nuestra victima
        CBaseEntity *pVictim = CheckTraceHullAttack( flDistance, -Vector( 16, 16, 12 ), Vector( 16, 16, 12 ), 200.0f, DMG_SLASH, 0.5f );

        // No le hemos dado a nada
        if ( !pVictim )
            return;

        EmitSound("SCP173.Attack");
    }
}

//================================================================================
//================================================================================
void CSCP_Player::EnterPlayerState( int state )
{
    BaseClass::EnterPlayerState( state );
}

//================================================================================
// Hemos cambiado de clase
//================================================================================
void CSCP_Player::OnPlayerClass( int playerClass )
{
    if ( playerClass == PLAYER_CLASS_SCP_173 )
    {
        // Sonido de movimiento para SCP-173
        m_nMovementSound = new SoundInstance( "SCP173.Movement", this, this );

        // SCP-173 no puede recibir daño
        m_takedamage = DAMAGE_NO;
    }
    else
    {
        if ( m_nMovementSound )
        {
            m_nMovementSound->Destroy();
            m_nMovementSound = NULL;
        }
    }
}

//================================================================================
// Coloca al jugador en el equipo especificado
//================================================================================
void CSCP_Player::ChangeTeam( int iTeamNum, bool bAutoTeam, bool bSilent )
{
    BaseClass::ChangeTeam( iTeamNum, bAutoTeam, bSilent );

    // Algo ha ocurrido que no hemos cambiado de equipo
    if ( GetTeamNumber() != iTeamNum )
        return;

    switch ( iTeamNum )
    {
        // Humanos y soldados
        case TEAM_HUMANS:
        case TEAM_SOLDIERS:
        {
            // Linterna disponible
            SetFlashlightEnabled( true );

            // Podemos saltar y agacharnos
            EnableButtons( IN_JUMP | IN_DUCK );

            // Los humanos si pueden recibir daño
            m_takedamage = DAMAGE_YES;

            // Entramos al juego
            SetPlayerClass( PLAYER_CLASS_NONE );
            EnterToGame();
            break;
        }

        // SCP
        case TEAM_SCP:
        {
            // Los SCP no tienen linterna
            SetFlashlightEnabled( false );

            // Los SCP no necesitan saltar ni agacharse
            DisableButtons( IN_JUMP | IN_DUCK );

            // Selecciona que clase de SCP serás
            SetPlayerState( PLAYER_STATE_PICKING_CLASS );
            break;
        }
    }
}

//================================================================================
// Devuelve el modelo del jugador
//================================================================================
const char *CSCP_Player::GetPlayerModel()
{
    // Soldados
    if ( IsSoldier() )
        return "models/bloocobalt/player/l4d/riot_01.mdl";

    // Humanos
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
// Prepara el modelo del jugador
//================================================================================
void CSCP_Player::PrepareModel()
{
    // Soldados
    if ( IsSoldier() )
    {
        SetBodygroup( 1, 1 );
        SetBodygroup( 2, 1 );
    }

    if ( IsSurvivor() )
    {
        SetSkin( RandomInt( 0, 3 ) );
    }
}

//================================================================================
// Crea las características de este jugador
//================================================================================
void CSCP_Player::CreateFeatures()
{
    // SCP-173
    if ( GetPlayerClass() == PLAYER_CLASS_SCP_173 )
    {
        AddFeature( new C173Behavior );
    }
}

//================================================================================
// Devuelve el nombre de la entidad que servirá para crear este jugador
//================================================================================
const char *CSCP_Player::GetSpawnEntityName()
{
    // Soldados
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
bool CSCP_Player::ClientCommand( const CCommand & args )
{
    // Unirse a otro equipo
    if ( FStrEq( args[0], "select_class" ) )
    {
        // Solamente los SCP pueden seleccionar una clase
        if ( !IsMonster() )
            return false;

        if ( args.ArgC() < 2 )
        {
            Warning( "Player sent bad select_class syntax \n" );
            return false;
        }

        int playerClass = atoi(args[1]);        

        // Establecemos la clase y entramos al juego
        SetPlayerClass( playerClass );
        EnterToGame();

        return true;
    }

    return BaseClass::ClientCommand( args );
}