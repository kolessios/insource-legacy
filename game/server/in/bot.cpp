//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "bot.h"

#include "in_player.h"
#include "in_shareddefs.h"
#include "in_utils.h"
#include "in_gamerules.h"
#include "players_system.h"

#include "bot_manager.h"

#include "nav.h"
#include "nav_mesh.h"
#include "nav_area.h"

#include "fmtstr.h"
#include "in_buttons.h"

#include "ai_hint.h"
#include "movehelper_server.h"
#include "squad_manager.h"

#include "datacache/imdlcache.h"

//#include "weapon_base.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
// Comandos
//================================================================================

DECLARE_CHEAT_COMMAND( bot_frozen, "0", "" )
DECLARE_CHEAT_COMMAND( bot_crouch, "0", "" )
DECLARE_REPLICATED_COMMAND( bot_flashlight, "0", "" )
DECLARE_CHEAT_COMMAND( bot_mimic, "0", "" )
DECLARE_CHEAT_COMMAND( bot_aim_player, "0", "" )
DECLARE_CHEAT_COMMAND( bot_primary_attack, "0", "" )

DECLARE_CHEAT_COMMAND( bot_notarget, "0", "" )
DECLARE_CHEAT_COMMAND( bot_god, "0", "" )
DECLARE_CHEAT_COMMAND( bot_buddha, "0", "" )
//DECLARE_CHEAT_COMMAND( bot_dont_attack, "0", "" )

DECLARE_CHEAT_COMMAND( bot_debug, "0", "" )
DECLARE_CHEAT_COMMAND( bot_debug_navigation, "0", "" )
DECLARE_CHEAT_COMMAND( bot_debug_jump, "0", "" )
DECLARE_CHEAT_COMMAND( bot_debug_memory, "0", "" )

DECLARE_CHEAT_COMMAND( bot_debug_cmd, "0", "" )
DECLARE_CHEAT_COMMAND( bot_debug_conditions, "0", "" )
DECLARE_CHEAT_COMMAND( bot_debug_desires, "0", "" )
DECLARE_CHEAT_COMMAND( bot_debug_max_msgs, "10", "" )
DECLARE_CHEAT_COMMAND( bot_optimize, "0", "" );

DECLARE_REPLICATED_COMMAND( bot_far_distance, "2500", "" )

//================================================================================
// Macros
//================================================================================

// Define si el estado actual del bot ha terminado
// Para ponerse en modo Relajado
#define STATE_FINISHED m_iStateTimer.HasStarted() && m_iStateTimer.IsElapsed()

#define NEW_ENEMY_INTERVAL gpGlobals->curtime + 0.3f;

//================================================================================
// Crea un nuevo Bot con el nombre de jugador y posición especificados
//================================================================================
CPlayer *CreateBot( const char *pPlayername, const Vector *vecPosition, const QAngle *angles )
{
    MDLCACHE_CRITICAL_SECTION();

    // Seleccionamos un nombre al azar
    if ( !pPlayername ) {
        pPlayername = m_botNames[RandomInt( 0, ARRAYSIZE( m_botNames ) - 1 )];
        pPlayername = UTIL_VarArgs( "%s Bot", pPlayername );
    }

    bool allowPrecache = CBaseEntity::IsPrecacheAllowed();
    CBaseEntity::SetAllowPrecache( true );

    edict_t *pSoul = engine->CreateFakeClient( pPlayername );

    if ( !pSoul ) {
        Warning( "[CreateBot] Ha ocurrido un problema al crear un Bot." );
        return NULL;
    }

    CPlayer *pPlayer = (CPlayer *)CBaseEntity::Instance( pSoul );
    Assert( pPlayer );

    pPlayer->ClearFlags();
    pPlayer->AddFlag( FL_CLIENT | FL_FAKECLIENT );

    if ( vecPosition ) {
        pPlayer->Teleport( vecPosition, angles, NULL );
    }

    CBaseEntity::SetAllowPrecache( allowPrecache );

    ++g_botID;
    return pPlayer;
}

//================================================================================
// Constructor
//================================================================================
CBot::CBot()
{
    SetDefLessFunc( m_nComponents );
    SetDefLessFunc( m_nSchedules );

    m_Skill = NULL;
    m_iPerformance = BOT_PERFORMANCE_AWAKE;
}

//================================================================================
// Establece que [CBasePlayer] que controlara la I.A.
//================================================================================
void CBot::SetParent( CBasePlayer *parent )
{
    m_pParent = parent;

    // Componentes
    m_nComponents.Purge();
    SetupComponents();
}

//================================================================================
// Aparición en el mundo
//================================================================================
void CBot::Spawn()
{
    // Conjuntos de tareas
    m_nSchedules.Purge();
    SetupSchedules();

    // Llamamos "OnSpawn" para cada componente
    FOR_EACH_COMPONENT( OnSpawn() );

    m_cmd = NULL;
    m_nLastCmd = NULL;
    m_nBetterWeapon = NULL;
    m_nDejectedFriend = NULL;

    m_bNeedCrouch = false;
    m_bNeedRun = false;
    m_bNeedWalk = false;

    m_iState = STATE_IDLE;
    m_iTacticalMode = TACTICAL_MODE_NONE;
    m_iStateTimer.Invalidate();
    m_occludedEnemyTimer.Invalidate();

    m_nActiveSchedule = NULL;
    m_nConditions.ClearAll();
    m_vecSpawnSpot = GetAbsOrigin();

    m_iRepeatedDamageTimes = 0;
    m_flDamageAccumulated = 0.0f;

    if ( !GetSkill() )
        SetSkill( 0 );
}

//================================================================================
// Pensamiento de la I.A.
//================================================================================
void CBot::Think()
{
    VPROF_BUDGET( "Think", VPROF_BUDGETGROUP_BOTS );

    if ( STATE_FINISHED )
        CleanState();

    ApplyDebugCommands();

    // @TODO: FIXME
    if ( bot_mimic.GetInt() > 0 ) {
        MimicThink( bot_mimic.GetInt() );
        return;
    }

    m_cmd = new CBotCmd();
    m_cmd->viewangles = GetHost()->pl.v_angle;

    if ( ShouldProcess() ) {
        Process( m_cmd );
    }

	DebugDisplay();
	PlayerMove( m_cmd );
}

//================================================================================
// Ejecuta los comandos del Bot
//================================================================================
void CBot::PlayerMove( CBotCmd *cmd )
{
    VPROF_BUDGET( "PlayerMove", VPROF_BUDGETGROUP_BOTS );

	m_nLastCmd = m_cmd;

    if ( !GetHost()->IsBot() )
        return;

    PostClientMessagesSent();
    RunPlayerMove( cmd );
}

//================================================================================
// Aplica estados de depuración para el Bot
//================================================================================
void CBot::ApplyDebugCommands()
{
    // Modo Dios
    if ( bot_god.GetBool() ) GetHost()->AddFlag( FL_GODMODE );
    else GetHost()->RemoveFlag( FL_GODMODE );

    // Los enemigos no nos ven
    if ( bot_notarget.GetBool() ) GetHost()->AddFlag( FL_NOTARGET );
    else GetHost()->RemoveFlag( FL_NOTARGET );

    // Buddha
    if ( bot_buddha.GetBool() ) GetHost()->m_debugOverlays = GetHost()->m_debugOverlays | OVERLAY_BUDDHA_MODE;
    else GetHost()->m_debugOverlays = GetHost()->m_debugOverlays & ~OVERLAY_BUDDHA_MODE;

    // Agachados
    if ( bot_crouch.GetBool() )
        InjectButton( IN_DUCK );
	
	// Disparar siempre
    if ( bot_primary_attack.GetBool() )
        InjectButton( IN_ATTACK );

    // Linterna
    if ( bot_flashlight.GetBool() ) GetHost()->FlashlightTurnOn();
    else GetHost()->FlashlightTurnOff();

    if ( Aim() ) {
        // Apuntamos al Jugador local
        if ( bot_aim_player.GetBool() ) {
            CBasePlayer *pPlayer = UTIL_GetListenServerHost();

            if ( pPlayer )
                LookAt( "bot_aim_player", pPlayer->EyePosition(), PRIORITY_UNINTERRUPTABLE, 1.0f );
        }
    }

    // bot_dont_attack
}

//================================================================================
// Devuelve si se debe optimizar la I.A.
//================================================================================
bool CBot::ShouldOptimize()
{
    return false;

    if ( bot_optimize.GetBool() )
        return true;

    if ( GetPerformance() == BOT_PERFORMANCE_VISIBILITY || GetPerformance() == BOT_PERFORMANCE_PVS_AND_VISIBILITY ) {
        if ( !ThePlayersSystem->IsVisible(GetHost()) )
            return true;
    }

    if ( GetPerformance() == BOT_PERFORMANCE_PVS || GetPerformance() == BOT_PERFORMANCE_PVS_AND_VISIBILITY ) {
        CHumanPVSFilter filter(GetAbsOrigin());

        if ( filter.GetRecipientCount() == 0 )
            return true;
    }

    return false;
}

//================================================================================
// Devuelve si debemos optimizar el frame actual, al devolver true algunos
// sistemas no se procesaran.
//================================================================================
bool CBot::OptimizeThisFrame()
{
    if ( !ShouldOptimize() )
        return false;

    if ( !(gpGlobals->tickcount % 2) )
        return true;

    return false;
}

//================================================================================
// Devuelve si el Bot puede procesar su I.A.
//================================================================================
bool CBot::ShouldProcess()
{
    if ( !GetHost()->IsAlive() )
        return false;

    if ( IsPanicked() )
        return false;

    if ( Navigation() && TheNavMesh->GetNavAreaCount() == 0 )
        return false;

    if ( bot_frozen.GetBool() )
        return false;

    if ( IsFollowingPlayer() ) {
        CPlayer *pLeader = ToInPlayer(GetFollowing());

        if ( pLeader && pLeader->GetAI() ) {
            return pLeader->GetAI()->ShouldProcess();
        }
    }

    if ( GetPerformance() == BOT_PERFORMANCE_SLEEP_PVS ) {
        CHumanPVSFilter filter( GetAbsOrigin() );

        if ( filter.GetRecipientCount() == 0 )
            return false;
    }

    return true;
}

//================================================================================
// Procesa la I.A. del Bot
//================================================================================
void CBot::Process( CBotCmd* &cmd )
{
    if ( GetHost()->GetSenses() && !OptimizeThisFrame() )
        GetHost()->GetSenses()->PerformSensing();

    SelectConditions();

    FOR_EACH_COMPONENT( OnUpdate( cmd ) );

    UpdateSchedule();

    if ( ShouldWalk() )
        InjectButton( IN_WALK );
    else if ( ShouldRun() )
        InjectButton( IN_SPEED );
    if ( ShouldCrouch() )
        InjectButton( IN_DUCK );

    if ( ShouldJump() ) {
        if ( Navigation() )
            Navigation()->Jump();
        else
            InjectButton( IN_JUMP );
    }
}

//================================================================================
// Pensamiento al imitar a un Jugador
//================================================================================
void CBot::MimicThink( int playerIndex )
{
    CBasePlayer *pPlayer = UTIL_PlayerByIndex( playerIndex );

    // El Jugador no existe
    if ( !pPlayer )
        return;

    // Sin input
    if ( !pPlayer->GetLastUserCommand() )
        return;

    m_flDebugYPosition = 0.34f;

    DebugDisplay();

    const CUserCmd *playercmd = pPlayer->GetLastUserCommand();
    m_cmd = new CBotCmd();

    m_cmd->command_number = playercmd->command_number;
    m_cmd->tick_count = playercmd->tick_count;
    m_cmd->viewangles = playercmd->viewangles;
    m_cmd->forwardmove = playercmd->forwardmove;
    m_cmd->sidemove = playercmd->sidemove;
    m_cmd->upmove = playercmd->upmove;
    m_cmd->buttons = playercmd->buttons;
    m_cmd->impulse = playercmd->impulse;
    m_cmd->weaponselect = playercmd->weaponselect;
    m_cmd->weaponsubtype = playercmd->weaponsubtype;
    m_cmd->random_seed = playercmd->random_seed;
    m_cmd->mousedx = playercmd->mousedx;
    m_cmd->mousedy = playercmd->mousedy;

    CFmtStr msg;
    DebugScreenText( msg.sprintf("command_number: %i", GetCmd()->command_number) );
    DebugScreenText( msg.sprintf("tick_count: %i", GetCmd()->tick_count) );
    DebugScreenText( msg.sprintf("viewangles: %.2f, %.2f", GetCmd()->viewangles.x, GetCmd()->viewangles.y) );
    DebugScreenText( msg.sprintf("forwardmove: %.2f", GetCmd()->forwardmove) );
    DebugScreenText( msg.sprintf("sidemove: %.2f", GetCmd()->sidemove) );
    DebugScreenText( msg.sprintf("upmove: %.2f", GetCmd()->upmove) );
    DebugScreenText( msg.sprintf("buttons:%i", (int)GetCmd()->buttons) );
    DebugScreenText( msg.sprintf("impulse: %i", (int)GetCmd()->impulse) );
    DebugScreenText( msg.sprintf("weaponselect: %i", GetCmd()->weaponselect) );
    DebugScreenText( msg.sprintf("weaponsubtype: %i", GetCmd()->weaponsubtype) );
    DebugScreenText( msg.sprintf("random_seed: %i", GetCmd()->random_seed) );
    DebugScreenText( msg.sprintf("mousedx: %i", (int)GetCmd()->mousedx) );
    DebugScreenText( msg.sprintf("mousedy: %i", (int)GetCmd()->mousedy) );
    DebugScreenText( msg.sprintf("hasbeenpredicted: %i", (int)GetCmd()->hasbeenpredicted) );

    //RunPlayerMove( m_cmd );
}

//================================================================================
// Mueve el Bot a la dirección deseada durante el frame actual
// Simulamos también InjectButton para que todo funcione correctamente
//================================================================================
void CBot::InjectMovement( NavRelativeDirType direction )
{
    if ( !GetCmd() )
        return;

    switch ( direction ) {
        case FORWARD:
        default:
            GetCmd()->forwardmove = 450.0f;
            InjectButton( IN_FORWARD );
            break;

        case UP:
            GetCmd()->upmove = 450.0f;
            break;

        case DOWN:
            GetCmd()->upmove = -450.0f;
            break;

        case BACKWARD:
            GetCmd()->forwardmove = -450.0f;
            InjectButton( IN_BACK );
            break;

        case LEFT:
            GetCmd()->sidemove = -450.0f;
            InjectButton( IN_LEFT );
            break;

        case RIGHT:
            GetCmd()->sidemove = 450.0f;
            InjectButton( IN_RIGHT );
            break;
    }
}

//================================================================================
// Fuerza al Bot a presionar un botón
//================================================================================
void CBot::InjectButton( int btn )
{
    if ( !GetCmd() )
        return;

    GetCmd()->buttons |= btn;
}

//================================================================================
// Hace que el Jugador indicado controle el Bot
//================================================================================
void CBot::Possess( CPlayer *pOther )
{
    GetHost()->Possess(pOther);
}

//================================================================================
// Devuelve si el jugador host nos esta mirando en modo espectador
//================================================================================
bool CBot::IsLocalPlayerWatchingMe()
{
    return GetHost()->IsLocalPlayerWatchingMe();
}

//================================================================================

CON_COMMAND_F( bot_add, "Agrega un(os) bot(s)", FCVAR_SERVER )
{
    // Look at -count.
    int count = args.FindArgInt( "-count", 1 );
    count = clamp( count, 1, 16 );

    // Ok, spawn all the bots.
    while ( --count >= 0 ) {
        CreateBot( NULL, NULL, NULL );
    }
}

CON_COMMAND_F( bot_kick, "Elimina todos los Bots en el mapa", FCVAR_SERVER )
{
    FOR_EACH_BOT({
        if ( pBot->GetHost()->IsBot() )
            pBot->GetHost()->Kick();
    });
}

CON_COMMAND_F( bot_navigation_follow, "Todos los Bots siguen al jugador", FCVAR_SERVER )
{
    CPlayer *pOwner = ToInPlayer(CBasePlayer::Instance( UTIL_GetCommandClientIndex() ));

    if ( !pOwner )
        return;

    FOR_EACH_BOT({
        pBot->Follow()->Start( pOwner );
    });
}

CON_COMMAND_F( bot_navigation_stop_follow, "Todos los Bots paran de seguir un objetivo", FCVAR_SERVER )
{
    FOR_EACH_BOT({
        pBot->Follow()->Stop();
    });
}

CON_COMMAND_F( bot_navigation_random, "Todos los bots se dirigen a destinos distintos.", FCVAR_SERVER )
{
    FOR_EACH_BOT({
        CNavArea *pArea = NULL;
        CNavArea *pStartArea = TheNavMesh->GetNearestNavArea( pBot->GetAbsOrigin() );

        while ( !pArea ) {
            pArea = TheNavAreas[ RandomInt(0, TheNavAreas.Count()-1) ];

            if ( !pArea || !pStartArea )
                continue;

            if ( pArea->IsUnderwater() ) {
                pArea = NULL;
                continue;
            }

            pBot->Navigation()->SetDestination( pArea );
        }
    });
}

CON_COMMAND_F( bot_navigation_go, "Todos los Bots van al area donde se encuentra el Jugador", FCVAR_SERVER )
{
    CPlayer *pOwner = ToInPlayer(CBasePlayer::Instance(UTIL_GetCommandClientIndex()));

    if ( !pOwner )
        return;

    CNavArea *pArea = TheNavMesh->GetNearestNavArea( pOwner->GetAbsOrigin() );

    if ( !pArea )
        return;

    FOR_EACH_BOT({
        pBot->Navigation()->SetDestination( pArea );
    });
}

CON_COMMAND_F( bot_possess, "", FCVAR_SERVER )
{
    CPlayer *pOwner = ToInPlayer(CBasePlayer::Instance(UTIL_GetCommandClientIndex()));

    if ( !pOwner )
        return;

    if ( args.ArgC() != 2 )
    {
        Warning( "bot_possess <client index>\n" );
        return;
    }

    int iBotClient = atoi( args[1] );
    int iBotEnt = iBotClient + 1;

    if ( iBotClient < 0 || 
        iBotClient >= gpGlobals->maxClients || 
        pOwner->entindex() == iBotEnt )
    {
        Warning( "bot_possess <client index>\n" );
        return;
    }

    CPlayer *pBot = ToInPlayer(CBasePlayer::Instance( iBotEnt ));

    if ( !pBot )
        return;

    if ( !pBot->GetAI() )
        return;

    DevWarning("Possesing %s!! \n", pBot->GetPlayerName());
    pBot->GetAI()->Possess( pOwner );
}