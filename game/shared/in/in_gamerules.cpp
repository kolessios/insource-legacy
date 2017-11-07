//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#include "cbase.h"
#include "in_gamerules.h"

#include "ammodef.h"
#include "KeyValues.h"
#include "in_buttons.h"

#ifndef CLIENT_DLL
    #include "in_player.h"
    #include "bots\squad.h"
    #include "team.h"
    #include "globalstate.h"
    #include "sound_manager.h"
    #include "world.h"
#else
    #include "c_in_player.h"
    #include "takedamageinfo.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CInGameRules *TheGameRules = NULL;

//====================================================================
// Callbacks
//====================================================================


//====================================================================
// Comandos
//====================================================================

#ifndef CLIENT_DLL
void SetVoiceProximity( IConVar *var, const char *pOldValue, float flOldValue )
{
    ConVarRef sv_voice_proximity("sv_voice_proximity");
    // Si es mayor a 0, se activara el chat de voz por distancia
    GetVoiceGameMgr()->SetProximityDistance( sv_voice_proximity.GetInt() );
}

ConVar sv_voice_proximity("sv_voice_proximity", "-1", FCVAR_SERVER, "", SetVoiceProximity );
#endif

// Modo de Juego
DECLARE_COMMAND( sv_gamemode, "0", "", FCVAR_NOT_CONNECTED | FCVAR_SERVER );

// Seed para eventos al azar
DECLARE_COMMAND( sv_gameseed, "", "", FCVAR_NOT_CONNECTED | FCVAR_SERVER );

// Dificultad del Juego
DECLARE_COMMAND( sv_difficulty, "2", "", FCVAR_SERVER | FCVAR_ARCHIVE | FCVAR_ARCHIVE_XBOX );

// Define si el chat por voz de los jugadores muertos solo podrán escucharlo los muertos
DECLARE_REPLICATED_COMMAND( sv_voice_death_to_death, "1", "" );

// Define si los bots estan permitidos a dañar a un aliado humano
DECLARE_NOTIFY_COMMAND( sv_bot_friendlyfire, "0", "" );

// Define si los Jugadores deben respawnear de inmediato
DECLARE_NOTIFY_COMMAND( sv_respawn_immediately, "0", "" );

// Cantidad de segundos para hacer respawn
DECLARE_NOTIFY_COMMAND( sv_respawn_wait, "10", "" );

// Escala de daño para personajes importantes
DECLARE_REPLICATED_COMMAND( sk_ally_scale_damage_taken, "0.8", "" )
DECLARE_REPLICATED_COMMAND( sk_vital_ally_scale_damage_taken, "0.5", "" )

// Escudo
DECLARE_NOTIFY_COMMAND( sk_player_shield_pause, "5.0", "Tiempo en segundos que la regeneracion de escudo es pausada al tomar dano." )

extern ConVar servercfgfile;
extern ConVar lservercfgfile;
extern ConVar aimcrosshair;
extern ConVar footsteps;
extern ConVar flashlight;
extern ConVar allowNPCs;
//extern ConVar sv_alltalk;
extern ConVar friendlyfire;

#ifndef CLIENT_DLL
extern ConVar sk_player_head;
extern ConVar sk_player_chest;
extern ConVar sk_player_stomach;
extern ConVar sk_player_arm;
extern ConVar sk_player_leg;

//====================================================================
//====================================================================
bool CVoiceGameMgrHelper::CanPlayerHearPlayer( CBasePlayer *pListener, CBasePlayer *pTalker, bool &bProximity )
{
    ConVarRef sv_alltalk("sv_alltalk");

    // El chat no es global y no son del mismo equipo
    if ( !sv_alltalk.GetBool() && !pTalker->InSameTeam(pListener) )
        return false;

    // El hablante esta muerto
    if ( !pTalker->IsAlive() ) {
        // Solo podemos hablar a los muertos y el escucha esta vivo
        if ( sv_voice_death_to_death.GetBool() && pListener->IsAlive() )
            return false;
    }

    return true;
}

// Ayudante de voz
CVoiceGameMgrHelper g_VoiceGameMgrHelper;
IVoiceGameMgrHelper *g_pVoiceGameMgrHelper = &g_VoiceGameMgrHelper;
#endif

//====================================================================
// Información y Red
//====================================================================

LINK_ENTITY_TO_CLASS( in_gamerules, CInGameRulesProxy );
IMPLEMENT_NETWORKCLASS_ALIASED( InGameRulesProxy, DT_InGameRulesProxy );

REGISTER_GAMERULES_CLASS( CInGameRules );

BEGIN_NETWORK_TABLE_NOBASE( CInGameRulesProxy, DT_InGameRules )
END_NETWORK_TABLE()

//====================================================================
// Proxy
//====================================================================

#ifndef CLIENT_DLL
void *SendProxy_InGameRules( const SendProp *pProp, const void *pStructBase, const void *pData, CSendProxyRecipients *pRecipients, int objectID )
{
    CInGameRules *pRules = TheGameRules;
    pRecipients->SetAllRecipients();

    return pRules;
}

BEGIN_SEND_TABLE( CInGameRulesProxy, DT_InGameRulesProxy )
    SendPropDataTable( "in_gamerules_data", 0, &REFERENCE_SEND_TABLE(DT_InGameRules), SendProxy_InGameRules )
END_SEND_TABLE()
#else
void RecvProxy_InGameRules( const RecvProp *pProp, void **pOut, void *pData, int objectID )
{
    CInGameRules *pRules = TheGameRules;
    *pOut = pRules;
}

BEGIN_RECV_TABLE( CInGameRulesProxy, DT_InGameRulesProxy )
    RecvPropDataTable( "in_gamerules_data", 0, 0, &REFERENCE_RECV_TABLE( DT_InGameRules ), RecvProxy_InGameRules )
END_RECV_TABLE()
#endif

//====================================================================
//====================================================================
void InitBodyQue()
{
}

//====================================================================
// Constructor
//====================================================================
CInGameRules::CInGameRules()
{
    TheGameRules = this;

#ifndef CLIENT_DLL
    StartServer();
#endif
}

//====================================================================
// Destructor
//====================================================================
CInGameRules::~CInGameRules()
{
    // Somos nosotros
    TheGameRules = NULL;

#ifndef CLIENT_DLL
    g_Teams.Purge();
#endif
}

//====================================================================
// Devuelve el nombre del Juego
//====================================================================
const char *CInGameRules::GetGameDescription()
{
    return GAME_NAME;
}

//====================================================================
//====================================================================
bool CInGameRules::ShouldCollide( int collisionGroup0, int collisionGroup1 )
{
    // Don't stand on COLLISION_GROUP_WEAPON
    if ( collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT && collisionGroup1 == COLLISION_GROUP_WEAPON )
        return false;

    // No deben colisionar entre ellos
    if ( collisionGroup0 == COLLISION_GROUP_NOT_BETWEEN_THEM && collisionGroup1 == COLLISION_GROUP_NOT_BETWEEN_THEM )
        return false;

    if ( collisionGroup0 == COLLISION_GROUP_NPC && collisionGroup1 == COLLISION_GROUP_NPC )
        return true;

    return BaseClass::ShouldCollide( collisionGroup0, collisionGroup1 ); 
}

//====================================================================
//====================================================================
const CViewVectors *CInGameRules::GetViewVectors() const
{
    return &g_InViewVectors;
}

//====================================================================
//====================================================================
const CInViewVectors *CInGameRules::GetInViewVectors() const
{
    return &g_InViewVectors;
}

//====================================================================
//====================================================================
int CInGameRules::Damage_GetTimeBased()
{
    return ( DMG_PARALYZE | DMG_NERVEGAS | DMG_POISON | DMG_RADIATION | DMG_DROWNRECOVER | DMG_ACID | DMG_SLOWBURN );
}

//====================================================================
//====================================================================
int    CInGameRules::Damage_GetShouldGibCorpse()
{
    return ( DMG_CRUSH | DMG_BLAST | DMG_SONIC | DMG_CLUB );
}

//====================================================================
//====================================================================
int CInGameRules::Damage_GetShowOnHud()
{
    return ( DMG_POISON | DMG_ACID | DMG_DROWN | DMG_BURN | DMG_SLOWBURN | DMG_NERVEGAS | DMG_RADIATION | DMG_SHOCK );
}

//====================================================================
//====================================================================
int    CInGameRules::Damage_GetNoPhysicsForce()
{
    int iTimeBasedDamage = Damage_GetTimeBased();
    return ( DMG_FALL | DMG_BURN | DMG_PLASMA | DMG_DROWN | iTimeBasedDamage | DMG_CRUSH | DMG_PHYSGUN | DMG_PREVENT_PHYSICS_FORCE );
}

//====================================================================
//====================================================================
int    CInGameRules::Damage_GetShouldNotBleed()
{
    return ( DMG_POISON | DMG_ACID | DMG_GENERIC );
}

//====================================================================
//====================================================================
int CInGameRules::Damage_GetCausesSlowness()
{
    int iTimeBasedDamage = Damage_GetTimeBased();
    return (DMG_SLASH | DMG_BULLET | iTimeBasedDamage );
}

//====================================================================
// Devuelve si [pPushingEntity] puede empujar a [pEntity]  
//====================================================================
bool CInGameRules::CanPushEntity( CBaseEntity *pPushingEntity, CBaseEntity *pEntity )
{
    if ( !pEntity || pEntity->IsWorld() )
        return false;

    CBasePlayer *pPushingPlayer = ToBasePlayer( pPushingEntity );
    CBasePlayer *pPlayer = ToBasePlayer( pEntity );

    if ( pPushingPlayer && pPlayer )
    {
        // Los bots no pueden empujar a los humanos
        if ( pPushingPlayer->IsBot() && !pPlayer->IsBot() )
            return false;

#ifndef CLIENT_DLL
        // No se debe empujar enemigos
        if ( PlayerRelationship(pPushingEntity, pEntity) == GR_ENEMY )
            return false;
#endif
    }

    return true;
}

//====================================================================
//====================================================================
bool CInGameRules::Damage_IsTimeBased( int iDmgType )
{
    // Damage types that are time-based.
    return ( (iDmgType & Damage_GetTimeBased()) != 0 );
}

//====================================================================
//====================================================================
bool CInGameRules::Damage_ShouldGibCorpse( int iDmgType )
{
    // Damage types that are time-based.
    return ( (iDmgType & Damage_GetShouldGibCorpse()) != 0 );
}

//====================================================================
//====================================================================
bool CInGameRules::Damage_ShowOnHUD( int iDmgType )
{
    // Damage types that are time-based.
    return ( (iDmgType & Damage_GetShowOnHud()) != 0 );
}

//====================================================================
//====================================================================
bool CInGameRules::Damage_NoPhysicsForce( int iDmgType )
{
    // Damage types that are time-based.
    return ( (iDmgType & Damage_GetNoPhysicsForce()) != 0 );
}

//====================================================================
//====================================================================
bool CInGameRules::Damage_ShouldNotBleed( int iDmgType )
{
    // Damage types that are time-based.
    return ( (iDmgType & Damage_GetShouldNotBleed()) != 0 );
}

//====================================================================
//====================================================================
bool CInGameRules::Damage_CausesSlowness( const CTakeDamageInfo &info )
{
    return ((info.GetDamageType() & Damage_GetCausesSlowness()) != 0);
}

#ifndef CLIENT_DLL /////////// SERVER ONLY

//====================================================================
//====================================================================
void CInGameRules::RefreshSkillData( bool forceUpdate )
{
    if ( !forceUpdate ) {
        if ( GlobalEntity_IsInTable( "skill.cfg" ) )
            return;
    }

    GlobalEntity_Add( "skill.cfg", STRING( gpGlobals->mapname ), GLOBAL_ON );

    // Ejecutamos el archivo de configuración para la dificultad
    engine->ServerCommand( UTIL_VarArgs( "exec skill%d.cfg\n", GetSkillLevel() ) );
    engine->ServerExecute();
}

//====================================================================
// Se ha cambiado la dificultad del juego
//====================================================================
void CInGameRules::OnSkillLevelChanged( int iNewLevel )
{
    RefreshSkillData( true );
}

//====================================================================
//====================================================================
void CInGameRules::SetSkillLevel( int iLevel )
{
    if ( iLevel < SKILL_EASY || iLevel > SKILL_HARDEST )
        return;

    int oldLevel = g_iDifficultyLevel;
    g_iDifficultyLevel = iLevel;

    if ( g_iDifficultyLevel != oldLevel ) {
        OnSkillLevelChanged( g_iDifficultyLevel );
    }
}

//====================================================================
// Devuelve si el director puede controlar la dificultad del juego
//====================================================================
bool CInGameRules::Director_AdaptativeSkill()
{
	ConVarRef director_adaptative_skill("director_adaptative_skill");
	return director_adaptative_skill.GetBool();
}

//====================================================================
// Le permite a las reglas del juego agregar comportamiento
// al Director.
//====================================================================
void CInGameRules::Director_Update()
{
}

#define DIRECTOR_MUSIC_INFO( name, loop ) SOUND_INFO(name, loop, TEAM_HUMANS, NULL, GetWorldEntity(), TARGET_NONE)

#define DIRECTOR_MUSIC( channel, info ) pSound = new CSoundInstance( info ); \
	pSound->SetChannel( channel ); \
	pManager->Add( pSound );

//====================================================================
// Crea la música que podrá reproducir el director
//====================================================================
void CInGameRules::Director_CreateMusic( CSoundManager *pManager )
{
#ifdef APOCALYPSE
    CSoundInstance *pSound;

    DIRECTOR_MUSIC( CHANNEL_1, DIRECTOR_MUSIC_INFO( "Music.Director.Boss", true ) );
    DIRECTOR_MUSIC( CHANNEL_1, DIRECTOR_MUSIC_INFO( "Music.Director.BigBoss", true ) );

    DIRECTOR_MUSIC( CHANNEL_2, DIRECTOR_MUSIC_INFO( "Music.Director.MiniFinale", true ) );
    DIRECTOR_MUSIC( CHANNEL_2, DIRECTOR_MUSIC_INFO( "Music.Director.Finale", true ) );

    DIRECTOR_MUSIC( CHANNEL_3, DIRECTOR_MUSIC_INFO( "Music.Director.Background.LowAngry", false ) );
    DIRECTOR_MUSIC( CHANNEL_3, DIRECTOR_MUSIC_INFO( "Music.Director.Background.MediumAngry", false ) );
    DIRECTOR_MUSIC( CHANNEL_3, DIRECTOR_MUSIC_INFO( "Music.Director.Background.HighAngry", false ) );
    DIRECTOR_MUSIC( CHANNEL_3, DIRECTOR_MUSIC_INFO( "Music.Director.Background.CrazyAngry", false ) );

    DIRECTOR_MUSIC( CHANNEL_ANY, DIRECTOR_MUSIC_INFO( "Music.Director.MobRules", false ) );
    DIRECTOR_MUSIC( CHANNEL_ANY, DIRECTOR_MUSIC_INFO( "Music.Director.FinalNail", false ) );
    DIRECTOR_MUSIC( CHANNEL_ANY, DIRECTOR_MUSIC_INFO( "Music.Director.Preparation", false ) );
    DIRECTOR_MUSIC( CHANNEL_ANY, DIRECTOR_MUSIC_INFO( "Music.Director.Gameover", false ) );
    DIRECTOR_MUSIC( CHANNEL_ANY, DIRECTOR_MUSIC_INFO( "Music.Director.Choir", false ) );

    DIRECTOR_MUSIC( CHANNEL_ANY, DIRECTOR_MUSIC_INFO( "Music.Director.Horde", true ) );
    DIRECTOR_MUSIC( CHANNEL_ANY, DIRECTOR_MUSIC_INFO( "Music.Director.HordeSlayer", true ) );
	
	/*
    for ( int i = 0; i < ARRAYSIZE( g_hordeMusic ); ++i ) {
        DIRECTOR_MUSIC( CHANNEL_ANY, DIRECTOR_MUSIC_INFO( g_hordeMusic[i], false ) );
        m_HordeMusicList[i] = pSound;
    }
	*/
#endif
}

//====================================================================
// Devuelve el nivel de deseo que se tiene para reproducir la
// canción especificada.
//====================================================================
int CInGameRules::Director_MusicDesire( const char *soundname, int channel )
{
	return 0;
}

void CInGameRules::Director_OnMusicPlay( const char * soundname )
{
}

void CInGameRules::Director_OnMusicStop( const char * soundname )
{
}

//====================================================================
// Carga lo necesario en el servidor
//====================================================================
void CInGameRules::StartServer()
{
    LoadServer();

    g_iDifficultyLevel = 0;
    g_Teams.Purge();

    for ( int i = 0; i < ARRAYSIZE( sTeamNames ); ++i ) {
        CTeam *pTeam = dynamic_cast<CTeam *>(CreateEntityByName( "team_manager" ));
        pTeam->Init( sTeamNames[i], i );

        // Lo agregamos a la lista.
        g_Teams.AddToTail( pTeam );
    }
}

//====================================================================
// Carga el archivo de configuración para el servidor
//====================================================================
void CInGameRules::LoadServer()
{
    // Dedicated: server.cfg
    // Player Host: listenserver.cfg
    const char *cfgFile = (engine->IsDedicatedServer()) ? servercfgfile.GetString() : lservercfgfile.GetString();

    if ( cfgFile && cfgFile[0] ) {
        char szCommand[256];
        Msg( "Executing server config file: %s\n", cfgFile );

        Q_snprintf( szCommand, sizeof( szCommand ), "exec %s\n", cfgFile );
        engine->ServerCommand( szCommand );
    }
}

//====================================================================
// Guarda objetos necesarios en caché
//====================================================================
void CInGameRules::Precache()
{
    BaseClass::Precache();
}

//====================================================================
// Pensamiento
//====================================================================
void CInGameRules::Think()
{
    BaseClass::Think();

    SetSkillLevel( sv_difficulty.GetInt() );
}

//====================================================================
// Reincia el mapa
//====================================================================
void CInGameRules::RestartMap()
{
    // Lanzamos un evento para que los clientes hagan los ajustes necesarios
    IGameEvent *pEvent = gameeventmanager->CreateEvent( "game_round_restart" );

    if ( pEvent )
        gameeventmanager->FireEvent( pEvent );
}

//====================================================================
// Se ha conectado un jugador
//====================================================================
bool CInGameRules::ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen )
{
    GetVoiceGameMgr()->ClientConnected( pEntity );
    return true;
}

//====================================================================
// Se ha desconectado un jugador
//====================================================================
void CInGameRules::ClientDisconnected( edict_t *pClient )
{
    if ( !pClient )
        return;

    CPlayer *pPlayer = ToInPlayer( pClient );

    if ( !pPlayer )
        return;

    pPlayer->SetConnected( PlayerDisconnecting );

    ClientDestroy( pPlayer );

    FireTargets( "game_playerleave", pPlayer, pPlayer, USE_TOGGLE, 0 );

    pPlayer->SetConnected( PlayerDisconnected );
}

//====================================================================
// Un jugador ha escrito un comando
//====================================================================
bool CInGameRules::ClientCommand( CBaseEntity *pEdict, const CCommand &args )
{
    if ( BaseClass::ClientCommand(pEdict, args) )
        return true;

    CPlayer *pPlayer = ToInPlayer( pEdict );

    if ( pPlayer && pPlayer->ClientCommand(args) )
        return true;

    return false;
}

//====================================================================
// Realiza todo lo necesario antes de eliminar al jugador
//====================================================================
void CInGameRules::ClientDestroy( CBasePlayer *pPlayer )
{
    // Le quitamos todos sus objetos
    pPlayer->RemoveAllItems( true );

    // Destruimos sus vistas en primera persona
    pPlayer->DestroyViewModels();

    // Cambiamos de equipo
    pPlayer->ChangeTeam( TEAM_UNASSIGNED );
}

//====================================================================
//====================================================================
void CInGameRules::InitDefaultAIRelationships()
{
    int i, j;

    //  Allocate memory for default relationships
    CBaseCombatCharacter::AllocateDefaultRelationships();

    // First initialize table so we can report missing relationships
    for ( i = 0; i < LAST_SHARED_ENTITY_CLASS; i++ )
    {
        for ( j = 0; j < LAST_SHARED_ENTITY_CLASS; j++ )
        {
            // By default all relationships are neutral of priority zero
            CBaseCombatCharacter::SetDefaultRelationship( (Class_T)i, (Class_T)j, D_NU, 0 );
        }
    }

    // ------------------------------------------------------------
    //    > CLASS_PLAYER
    // ------------------------------------------------------------
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_PLAYER, CLASS_PLAYER, D_LI, 0);
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_PLAYER, CLASS_PLAYER_ALLY, D_LI, 0);
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_PLAYER, CLASS_PLAYER_ALLY_VITAL, D_LI, 0 );
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_PLAYER, CLASS_INFECTED, D_HT, 0);
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_PLAYER, CLASS_INFECTED_BOSS, D_HT, 2);
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_PLAYER, CLASS_HUMAN, D_LI, 0);
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_PLAYER, CLASS_SOLDIER, D_HT, 1);

    // ------------------------------------------------------------
    //    > CLASS_PLAYER_ALLY
    // ------------------------------------------------------------
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_PLAYER_ALLY, CLASS_PLAYER_ALLY, D_LI, 0 );
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_PLAYER_ALLY, CLASS_PLAYER_ALLY_VITAL, D_LI, 0 );
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_PLAYER_ALLY, CLASS_PLAYER, D_LI, 0);    
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_PLAYER_ALLY, CLASS_INFECTED, D_HT, 0);
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_PLAYER_ALLY, CLASS_INFECTED_BOSS, D_HT, 2);
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_PLAYER_ALLY, CLASS_HUMAN, D_LI, 0);
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_PLAYER_ALLY, CLASS_SOLDIER, D_HT, 1);

    CBaseCombatCharacter::SetDefaultRelationship( CLASS_PLAYER_ALLY_VITAL, CLASS_PLAYER_ALLY, D_LI, 0 );
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_PLAYER_ALLY_VITAL, CLASS_PLAYER_ALLY_VITAL, D_LI, 0 );
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_PLAYER_ALLY_VITAL, CLASS_PLAYER, D_LI, 0 );
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_PLAYER_ALLY_VITAL, CLASS_INFECTED, D_HT, 0 );
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_PLAYER_ALLY_VITAL, CLASS_INFECTED_BOSS, D_HT, 2 );
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_PLAYER_ALLY_VITAL, CLASS_HUMAN, D_LI, 0 );
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_PLAYER_ALLY_VITAL, CLASS_SOLDIER, D_HT, 1 );

    // ------------------------------------------------------------
    //    > CLASS_INFECTED
    // ------------------------------------------------------------
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_INFECTED, CLASS_INFECTED, D_LI, 0);
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_INFECTED, CLASS_PLAYER, D_HT, 0);    
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_INFECTED, CLASS_PLAYER_ALLY, D_HT, 0);
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_INFECTED, CLASS_PLAYER_ALLY_VITAL, D_HT, 0 );
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_INFECTED, CLASS_HUMAN, D_HT, 0);
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_INFECTED, CLASS_SOLDIER, D_HT, 0);
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_INFECTED, CLASS_FAUNA, D_HT, 0);
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_INFECTED, CLASS_FLORA, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship( CLASS_INFECTED, CLASS_INFECTED_BOSS, D_NU, 0);

    // ------------------------------------------------------------
    //    > CLASS_INFECTED_BOSS
    // ------------------------------------------------------------
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_INFECTED_BOSS, CLASS_INFECTED_BOSS, D_LI, 0);
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_INFECTED_BOSS, CLASS_PLAYER, D_HT, 0);
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_INFECTED_BOSS, CLASS_PLAYER_ALLY, D_HT, 0);
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_INFECTED_BOSS, CLASS_PLAYER_ALLY_VITAL, D_HT, 0 );
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_INFECTED_BOSS, CLASS_HUMAN, D_HT, 0);
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_INFECTED_BOSS, CLASS_SOLDIER, D_HT, 0);
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_INFECTED_BOSS, CLASS_FAUNA, D_HT, 0);
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_INFECTED_BOSS, CLASS_FLORA, D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship( CLASS_INFECTED_BOSS, CLASS_INFECTED, D_NU, 0);

    // ------------------------------------------------------------
    //    > CLASS_HUMAN
    // ------------------------------------------------------------
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_HUMAN, CLASS_HUMAN, D_LI, 0);
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_HUMAN, CLASS_PLAYER, D_LI, 0);
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_HUMAN, CLASS_PLAYER_ALLY, D_LI, 0);
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_HUMAN, CLASS_PLAYER_ALLY_VITAL, D_LI, 0 );
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_HUMAN, CLASS_INFECTED, D_HT, 0);
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_HUMAN, CLASS_INFECTED_BOSS, D_HT, 0);
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_HUMAN, CLASS_SOLDIER, D_HT, 0);

    // ------------------------------------------------------------
    //    > CLASS_SOLDIER
    // ------------------------------------------------------------
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_SOLDIER, CLASS_SOLDIER, D_LI, 0);
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_SOLDIER, CLASS_PLAYER, D_HT, 1);
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_SOLDIER, CLASS_PLAYER_ALLY, D_HT, 1);
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_SOLDIER, CLASS_PLAYER_ALLY_VITAL, D_HT, 1 );
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_SOLDIER, CLASS_INFECTED, D_HT, 0);
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_SOLDIER, CLASS_INFECTED_BOSS, D_HT, 2);
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_SOLDIER, CLASS_HUMAN, D_HT, 1);

    // ------------------------------------------------------------
    //    > CLASS_FAUNA
    // ------------------------------------------------------------
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_FAUNA, CLASS_FAUNA, D_LI, 0);
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_FAUNA, CLASS_PLAYER, D_FR, 0);
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_FAUNA, CLASS_PLAYER_ALLY, D_FR, 0);
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_FAUNA, CLASS_PLAYER_ALLY_VITAL, D_FR, 0 );
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_FAUNA, CLASS_INFECTED, D_FR, 2);
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_FAUNA, CLASS_INFECTED_BOSS, D_FR, 3);
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_FAUNA, CLASS_HUMAN, D_FR, 0);
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_FAUNA, CLASS_SOLDIER, D_FR, 1);
    CBaseCombatCharacter::SetDefaultRelationship( CLASS_FAUNA, CLASS_FLORA, D_LI, 0);
}

//====================================================================
// Devuelve la relación entre dos entidades
//====================================================================
int CInGameRules::PlayerRelationship( CBaseEntity *pCharacter, CBaseEntity *pTarget )
{
    VPROF_BUDGET("CInGameRules::PlayerRelationship", VPROF_BUDGETGROUP_OTHER_UNACCOUNTED);

    CPlayer *pPlayer = ToInPlayer( pCharacter );
    Assert( pPlayer );

    {
        VPROF_BUDGET("GetEnemy", VPROF_BUDGETGROUP_OTHER_UNACCOUNTED);
        if ( pPlayer->GetEnemy() == pTarget )
            return GR_ENEMY;
    }

    if ( !pTarget->MyCombatCharacterPointer() )
        return GR_NEUTRAL;

    if ( IsTeamplay() ) {
        if ( pCharacter->InSameTeam( pTarget ) )
            return GR_ALLY;
    }
    
    // SLOW
    /*{
        VPROF_BUDGET("GetSquad", VPROF_BUDGETGROUP_OTHER_UNACCOUNTED);
        CPlayer *pTargetPlayer = ToInPlayer(pTarget);

        // El objetivo también es un jugador y estamos en un escuadron
        if ( pTargetPlayer && pPlayer->GetSquad() ) {
            // En el mismo escuadron
            if ( pPlayer->GetSquad()->IsMember(pTargetPlayer) )
                return GR_ALLY;
        }
    }*/

    // Aliados
    if ( pPlayer->IRelationType( pTarget ) == D_LI )
        return GR_ALLY;

    // Enemigo
    if ( pPlayer->IRelationType( pTarget ) == D_HT || pPlayer->IRelationType( pTarget ) == D_FR )
        return GR_ENEMY;

    return GR_NEUTRAL;
}

//====================================================================
// Devuelve si el jugador indicado puede cambiar al arma indicada
//====================================================================
bool CInGameRules::FShouldSwitchWeapon( CBasePlayer *pPlayer, CBaseCombatWeapon *pWeapon )
{
	// Excepción no controlada en 0xFFFFFFFF en SWARM.EXE: 0xC0000005: Infracción de acceso al leer la ubicación 0xFFFFFFFF. ???
	try
	{
		if ( !pPlayer || !pWeapon )
			return false;

		// Can't switch weapons for some reason.
		if ( !pPlayer->Weapon_CanSwitchTo(pWeapon) )
			return false;

		// Player doesn't have an active item, might as well switch.
		if ( !pPlayer->GetActiveWeapon() )
			return true;

		// The given weapon should not be auto switched to from another weapon.
		if ( !pWeapon->AllowsAutoSwitchTo() )
			return false;

		// The active weapon does not allow autoswitching away from it.
		if ( !pPlayer->GetActiveWeapon()->AllowsAutoSwitchFrom() )
			return false;

		// Es un arma mucho mejor
		if ( pWeapon->GetWeight() > pPlayer->GetActiveWeapon()->GetWeight() )
			return true;

		return false;
	}
	catch( ... ) {
		return false;
	}
}

//====================================================================
// Devuelve si la victima puede recibir daño
//====================================================================
bool CInGameRules::AllowDamage( CBaseEntity *pVictim, const CTakeDamageInfo &info )
{
    return true;
}

//====================================================================
// Devuelve si se puede hacer fuego amigo
//====================================================================
bool CInGameRules::CanFriendlyfire( CBasePlayer *pVictim, CBasePlayer *pAttacker )
{
    if ( pAttacker->IsBot() ) {
        return CanBotsFriendlyfire( pVictim, pAttacker );
    }

    return CanFriendlyfire();
}

//====================================================================
// Devuelve si se puede hacer fuego amigo
//====================================================================
bool CInGameRules::CanFriendlyfire()
{
    return friendlyfire.GetBool();
}

//====================================================================
// Devuelve si los Bots pueden hacer fuego amigo
//====================================================================
bool CInGameRules::CanBotsFriendlyfire( CBasePlayer *pVictim, CBasePlayer *pAttacker )
{
    return CanBotsFriendlyfire();
}

//====================================================================
//====================================================================
bool CInGameRules::CanBotsFriendlyfire()
{
    return sv_bot_friendlyfire.GetBool();
}

//====================================================================
// Ajusta el daño recibido por un jugador
//====================================================================
void CInGameRules::AdjustPlayerDamageTaken( CPlayer * pVictim, CTakeDamageInfo & info )
{
    AdjustPlayerDamageHitGroup( pVictim, info, pVictim->LastHitGroup() );

    if ( info.GetDamage() <= 0 )
        return;

    if ( pVictim->Classify() == CLASS_PLAYER_ALLY_VITAL ) {
        info.ScaleDamage( sk_vital_ally_scale_damage_taken.GetFloat() );
    }
    else if ( pVictim->Classify() == CLASS_PLAYER_ALLY ) {
        info.ScaleDamage( sk_ally_scale_damage_taken.GetFloat() );
    }

    float handledByShield = FPlayerGetDamageHandledByShield( pVictim, info );

    if ( handledByShield > 0 ) {
        CAttribute *pShield = pVictim->GetAttribute( "shield" );
        Assert( pShield );

        pShield->SubtractValue( handledByShield );
        pShield->PauseRegeneration( PlayerShieldPause( pVictim ) );

        info.SubtractDamage( handledByShield );

        if ( info.GetAttacker() ) {
            if ( info.GetAttacker()->IsPlayer() )
                pVictim->DebugAddMessage( "Handled %.2f damage (shield) from %s\n", handledByShield, info.GetAttacker()->GetPlayerName() );
            else
                pVictim->DebugAddMessage( "Handled %.2f damage (shield) from %s\n", handledByShield, info.GetAttacker()->GetClassname() );
        }
    }
}

//====================================================================
// Ajusta el daño recibido en un lugar especifico del cuerpo
//====================================================================
void CInGameRules::AdjustPlayerDamageHitGroup( CPlayer * pVictim, CTakeDamageInfo & info, int hitGroup )
{
    switch ( hitGroup ) {
        case HITGROUP_GEAR:
            info.SetDamage( 0.0f );
            break;
        case HITGROUP_HEAD:
            info.ScaleDamage( sk_player_head.GetFloat() );
            break;
        case HITGROUP_CHEST:
            info.ScaleDamage( sk_player_chest.GetFloat() );
            break;
        case HITGROUP_STOMACH:
        default:
            info.ScaleDamage( sk_player_stomach.GetFloat() );
            break;
        case HITGROUP_LEFTARM:
        case HITGROUP_RIGHTARM:
            info.ScaleDamage( sk_player_arm.GetFloat() );
            break;
        case HITGROUP_LEFTLEG:
        case HITGROUP_RIGHTLEG:
            info.ScaleDamage( sk_player_leg.GetFloat() );
            break;
    }
}

//====================================================================
// Ajusta el daño antes de aplicarlo
//====================================================================
void CInGameRules::AdjustDamage( CBaseEntity *pVictim, CTakeDamageInfo &info )
{

}

//====================================================================
// Devuelve el daño sufrido por caída
//====================================================================
float CInGameRules::FlPlayerFallDamage( CBasePlayer *pPlayer )
{
#ifdef APOCALYPSE
    // Los infectados no tienen daño por caida
    if ( pPlayer->Classify() == CLASS_INFECTED )
        return 0.0f;
#endif

    pPlayer->m_Local.m_flFallVelocity -= PLAYER_MAX_SAFE_FALL_SPEED;
    return pPlayer->m_Local.m_flFallVelocity * DAMAGE_FOR_FALL_SPEED;
}

//====================================================================
// Devuelve si el jugador puede recibir daño del atacante
//====================================================================
bool CInGameRules::FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker, const CTakeDamageInfo &info )
{
    return FPlayerCanTakeDamage( pPlayer, pAttacker ); // TODO &info
}

//====================================================================
// Devuelve si el jugador puede recibir daño del atacante
//====================================================================
bool CInGameRules::FPlayerCanTakeDamage( CBasePlayer * pPlayer, CBaseEntity * pAttacker )
{
    if ( pAttacker && pAttacker != pPlayer ) {
        // El atacante es un jugador
        if ( pAttacker->IsPlayer() ) {
            CPlayer *pAttackerPlayer = ToInPlayer( pAttacker );

            // ¡Fuego amigo!
            if ( PlayerRelationship( pPlayer, pAttacker ) == GR_ALLY ) {
                if ( !CanFriendlyfire( pPlayer, pAttackerPlayer ) )
                    return false;
            }
        }
    }

    return true;
}

//====================================================================
// Devuelve si el jugador y el daño pueden minimizarse con el escudo
//====================================================================
bool CInGameRules::FPlayerCanShieldHandleDamage( CPlayer * pPlayer, const CTakeDamageInfo & info )
{
    // En principio el escudo solo es para el daño por bala
    if ( (info.GetDamageType() & DMG_BULLET) )
        return true;
    
    return false;
}

//====================================================================
// Devuelve la cantidad de daño absorbido por el escudo
//====================================================================
float CInGameRules::FPlayerGetDamageHandledByShield( CPlayer * pPlayer, const CTakeDamageInfo &info )
{
    if ( !FPlayerCanShieldHandleDamage( pPlayer, info ) )
        return 0.0f;

    CAttribute *pAttribute = pPlayer->GetAttribute( "shield" );

    if ( !pAttribute )
        return 0.0f;

    float shield = pAttribute->GetValue();
    float damage = info.GetDamage();

    if ( shield == 0.0f )
        return 0.0f;

    if ( shield >= damage ) {
        return info.GetDamage();
    }

    return shield;
}

//====================================================================
// Devuelve el tiempo que la regeneración del escudo estara en pausa
// después de recibir daño.
//====================================================================
float CInGameRules::PlayerShieldPause( CPlayer * pPlayer )
{
    return sk_player_shield_pause.GetFloat();
}

//====================================================================
// Devuelve si el jugador indicado puede incapacitarse por el daño sufrido
//====================================================================
bool CInGameRules::FPlayerCanDejected( CBasePlayer *pPlayer, const CTakeDamageInfo &info )
{
#ifdef APOCALYPSE
    // Los infectados no pueden incapacitarse
    if ( pPlayer->GetTeamNumber() == TEAM_INFECTED )
        return false;
#endif

    // Estos tipos de daño no causan incapacitación
    if ( (info.GetDamageType() & (DMG_CRUSH | DMG_FALL | DMG_DISSOLVE | DMG_DROWN | DMG_BULLET)) != 0 )
        return false;

    CPlayer *pInPlayer = ToInPlayer( pPlayer );

    if ( pInPlayer ) {
        // Ya estamos incapacitados
        if ( pInPlayer->GetPlayerStatus() >= PLAYER_STATUS_DEJECTED )
            return false;

        // Se ha superado el número de veces permitido
        if ( pInPlayer->GetDejectedTimes() >= 2 )
            return false;

        // Esta en modo Buddha
        if ( pInPlayer->IsOnBuddhaMode() )
            return false;
    }

    return true;
}

//====================================================================
// Devuelve si el jugador puede reproducir un sonido de muerte
//====================================================================
bool CInGameRules::FCanPlayDeathSound( const CTakeDamageInfo &info )
{
    // Estos tipos de daño son muertes instantaneas
    if ( (info.GetDamageType() & (DMG_CRUSH|DMG_FALL|DMG_DISSOLVE)) != 0 )
        return false;

    return true;
}

//====================================================================
// Devuelve si el Jugador puede hacer respawn
//====================================================================
bool CInGameRules::FPlayerCanRespawn( CBasePlayer *pPlayer )
{
    return true;
}

//====================================================================
// Devuelve si el Jugador puede hacer respawn ahora mismo
//====================================================================
bool CInGameRules::FPlayerCanRespawnNow( CPlayer *pPlayer )
{
    // Debemos repaarecer al instante
    if ( sv_respawn_immediately.GetBool() )
        return true;

    bool bPressing = ( pPlayer->GetButtons() & ~IN_SCORE ) ? true : false;

	// Debemos presionar una tecla
	if ( !bPressing )
		return false;

	// Ha pasado el tiempo suficiente
    if ( gpGlobals->curtime >= FlPlayerSpawnTime(pPlayer) )
        return true;

    // Partida multijugador
    /*if ( IsMultiplayer() )
    {
        // Ha pasado el tiempo suficiente
        if ( gpGlobals->curtime >= FlPlayerSpawnTime(pPlayer) && bPressing )
            return true;
    }
    else
    {
        // Hemos presionado cualquier tecla
        if ( bPressing )
            return true;
    }*/

    return false;
}

//====================================================================
// Devuelve si el jugador puede cambiar al modo espectador al morir
// de forma automática.
//====================================================================
bool CInGameRules::FPlayerCanGoSpectate( CBasePlayer *pPlayer ) 
{
	// Solo en multijugador
    if ( !IsMultiplayer() )
        return false;

	// Deben pasar 5segs
    if ( gpGlobals->curtime < (pPlayer->GetDeathAnimTime() + 5.0f) )
        return false;

    return true;
}

//================================================================================
// Tiempo en el que el jugador puede hacer respawn
//================================================================================
float CInGameRules::FlPlayerSpawnTime( CBasePlayer *pPlayer )
{
    return ( pPlayer->GetDeathTime() + sv_respawn_wait.GetFloat() );
}

//====================================================================
// Pensamiento del jugador
//====================================================================
void CInGameRules::PlayerThink( CBasePlayer *pPlayer )
{
}

//====================================================================
// Lo que sucede cuando jugador aparece en el mundo
//====================================================================
void CInGameRules::PlayerSpawn( CBasePlayer *pPlayer )
{
    // El traje de protección proporciona el HUD
    pPlayer->EquipSuit();

    CBaseEntity *pEquip = NULL;

    // Proporcionamos todos los "game_player_equip" al Jugador
    do
    {
        pEquip = gEntList.FindEntityByClassname( pEquip, "game_player_equip" );

        if ( !pEquip )
            continue;

        pEquip->Touch( pPlayer );
    } while ( pEquip );
}

//====================================================================
// Lo que sucede cuando un jugador muere
//====================================================================
void CInGameRules::PlayerKilled( CBasePlayer *pVictim, const CTakeDamageInfo &info )
{
    // Notificación de muerte
    DeathNotice( pVictim, info );

    // Definimos el inflictor y el atacante
    //CBaseEntity *pInflictor        = info.GetInflictor();
    CBaseEntity *pKiller        = info.GetAttacker();
    CPlayer *pScorer        = ToInPlayer( pKiller );

    // Has muerto
    pVictim->IncrementDeathCount( 1 );

    // Lanzamos evento
    FireTargets( "game_playerdie", pVictim, pVictim, USE_TOGGLE, 0 );

    // Notificamos al Director
    //if ( HasDirector() )
        //Director->OnPlayerKilled( pVictim );

    // Nos has matado
    pKiller->Event_KilledOther( pVictim, info );

    if ( pScorer )
    {
        // if a player dies in a deathmatch game and the killer is a client, award the killer some points
        pScorer->IncrementFragCount( IPointsForKill(pScorer, pVictim) );

        // Allow the scorer to immediately paint a decal
        pScorer->AllowImmediateDecalPainting();

        FireTargets( "game_playerkill", pScorer, pScorer, USE_TOGGLE, 0 );
    }
    else
    {  
        // Players lose a frag for letting the world kill them            
        pVictim->IncrementFragCount( -1 );                    
    }
}

//====================================================================
//====================================================================
void CInGameRules::InitHUD( CBasePlayer *pPlayer )
{
}

//====================================================================
// Devuelve si es posible usar el autoaim
//====================================================================
bool CInGameRules::AllowAutoTargetCrosshair()
{
    return aimcrosshair.GetBool();
}

//====================================================================
// Devuelve el modo de autoaim a utilizar
// 0 = Sin autoaim
// 1 = Auto-apuntado para PC
// 2 = Auto-apuntado para consola (más asistencia, más fácil)
//====================================================================
int CInGameRules::GetAutoAimMode() 
{
	return sk_autoaim_mode.GetInt();
}

//====================================================================
// Devuelve la cantidad de puntos por matar a alguién
//====================================================================
int CInGameRules::IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled )
{
    return 1;
}

//====================================================================
// Devuelve si la entidad puede reproducir la animación de muerte
// por el daño especificado.
//====================================================================
bool CInGameRules::CanPlayDeathAnim( CBaseEntity *pEntity, const CTakeDamageInfo &info )
{
    if ( !(pEntity->GetFlags() & FL_ONGROUND) )
        return false;

    if ( (pEntity->GetFlags() & FL_DUCKING) )
        return false;

    if ( (info.GetDamageType() & (DMG_SLASH)) != 0 ) {
        if ( pEntity->IsPlayer() ) {
            return (RandomInt( 1, 10 ) > 5);
        }

        return true;
    }

    return false;
}

//====================================================================
// Devuelve si el jugador puede  equiparse con el arma indicada
//====================================================================
bool CInGameRules::CanHavePlayerItem( CBasePlayer *pPlayer, CBaseCombatWeapon *pItem )
{
#ifdef APOCALYPSE
    // Los infectados no pueden usar armas
    if ( pPlayer->GetTeamNumber() == TEAM_INFECTED )
        return false;
#elif SCP
    // Los SCP no pueden usar armas
    // TODO: Y los supervivientes?
    if ( pPlayer->GetTeamNumber() == TEAM_SCP )
        return false;
 #endif

    for ( int i = 0 ; i < pPlayer->WeaponCount() ; i++ )
    {
        // Ya tienes esta arma
        if ( pPlayer->GetWeapon(i) == pItem )
            return false;
    }

    return BaseClass::CanHavePlayerItem( pPlayer, pItem );
}

//====================================================================
// Devuelve si el jugador puede equiparse con el objeto indicado
//====================================================================
bool CInGameRules::CanHaveItem( CBasePlayer *pPlayer, CItem *pItem )
{
     #ifdef APOCALYPSE
    // Los infectados no pueden tomar objetos
    if ( pPlayer->GetTeamNumber() == TEAM_INFECTED )
        return false;
    #elif SCP
    if ( pPlayer->GetTeamNumber() == TEAM_SCP )
        return false;
    #endif

    return true;
}

//====================================================================
//====================================================================
CBaseCombatWeapon *CInGameRules::GetNextBestWeapon( CBaseCombatCharacter *pPlayer, CBaseCombatWeapon *pCurrentWeapon )
{
    CBaseCombatWeapon *pCheck = NULL;
    CBaseCombatWeapon *pBest = NULL;// this will be used in the event that we don't find a weapon in the same category.

    int iCurrentWeight = -1;
    int iBestWeight = -1;// no weapon lower than -1 can be autoswitched to

    // If I have a weapon, make sure I'm allowed to holster it
    if ( pCurrentWeapon )
    {
        if ( !pCurrentWeapon->AllowsAutoSwitchFrom() || !pCurrentWeapon->CanHolster() )
        {
            // Either this weapon doesn't allow autoswitching away from it or I
            // can't put this weapon away right now, so I can't switch.
            return NULL;
        }

        iCurrentWeight = pCurrentWeapon->GetWeight();
    }

    for ( int i = 0 ; i < pPlayer->WeaponCount(); ++i )
    {
        pCheck = pPlayer->GetWeapon( i );

        if ( !pCheck )
            continue;

		if ( pCheck == pCurrentWeapon )
			continue;

        // If we have an active weapon and this weapon doesn't allow autoswitching away
        // from another weapon, skip it.
        if ( pCurrentWeapon && !pCheck->AllowsAutoSwitchTo() )
            continue;

        if ( pCheck->GetWeight() > -1 && pCheck->GetWeight() == iCurrentWeight )
        {
			if ( pCurrentWeapon  && pCurrentWeapon->HasAnyAmmo() )
				return NULL;

            // this weapon is from the same category. 
            if ( pCheck->HasAnyAmmo() )
            {
                if ( pPlayer->Weapon_CanSwitchTo( pCheck ) )
                {
                    return pCheck;
                }
            }
        }
        else if ( pCheck->GetWeight() > iBestWeight )
        {
            //Msg( "Considering %s\n", STRING( pCheck->GetClassname() );
            // we keep updating the 'best' weapon just in case we can't find a weapon of the same weight
            // that the player was using. This will end up leaving the player with his heaviest-weighted 
            // weapon. 
            if ( pCheck->HasAnyAmmo() )
            {
                // if this weapon is useable, flag it as the best
                iBestWeight = pCheck->GetWeight();
                pBest = pCheck;
            }
        }
    }

    // if we make it here, we've checked all the weapons and found no useable 
    // weapon in the same catagory as the current weapon. 

    // if pBest is null, we didn't find ANYTHING. Shouldn't be possible- should always 
    // at least get the crowbar, but ya never know.
    return pBest;
}

//====================================================================
// Devuelve el info_player_spawn ideal para el jugador
//====================================================================
CBaseEntity *CInGameRules::GetPlayerSpawnSpot( CBasePlayer *pPlayer )
{
    return BaseClass::GetPlayerSpawnSpot( pPlayer );
}

//====================================================================
// Devuelve si [pListener] puede recibir el mensaje de chat
// de [pSpeaker]
//====================================================================
bool CInGameRules::PlayerCanHearChat( CBasePlayer *pListener, CBasePlayer *pSpeaker )
{
    bool bProximity = false;
    return g_pVoiceGameMgrHelper->CanPlayerHearPlayer( pListener, pSpeaker, bProximity );
}

//====================================================================
// Devuelve si se puede reproducir los sonidos de pasos
//====================================================================
bool CInGameRules::PlayFootstepSounds( CBasePlayer *pPlayer )
{
    return footsteps.GetBool();
}

//====================================================================
// Devuelve si se puede usar la linterna
//====================================================================
bool CInGameRules::FAllowFlashlight()
{
    return flashlight.GetBool();
}

//====================================================================
// Devuelve si puede haber NPC's
//====================================================================
bool CInGameRules::FAllowNPCs()
{
    return allowNPCs.GetBool();
}

//====================================================================
// Devuelve si [pEntity] esta libre como punto de aparición
//====================================================================
bool CInGameRules::CanUseSpawnPoint( CBaseEntity *pEntity )
{
    int i = m_nSpawnSlots.Find( pEntity->entindex() );
    return ( i == -1 );
}

//====================================================================
// Devuelve si [pEntity] puede usarse como punto de aparición
//====================================================================
void CInGameRules::UseSpawnPoint( CBaseEntity *pEntity )
{
    if ( !CanUseSpawnPoint(pEntity) )
        return;

    m_nSpawnSlots.AddToTail( pEntity->entindex() );
}

//====================================================================
// Libera [pEntity] para poder usarse como punto de aparición
//====================================================================
void CInGameRules::FreeSpawnPoint( CBaseEntity *pEntity )
{
    if ( CanUseSpawnPoint(pEntity) )
        return;

    m_nSpawnSlots.FindAndRemove( pEntity->entindex() );
}
#else
//================================================================================
// Devuelve la ruta del vídeo que debe reproducirse en el fondo de la interfaz
//================================================================================
const char *CInGameRules::GetBackgroundMovie() 
{
	return "media/l4d2_background04.avi";
}
#endif