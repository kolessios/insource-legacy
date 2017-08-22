//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "players_system.h"

#include "dbhandler.h"

#include "ai_basenpc.h"
#include "ai_pathfinder.h"

#include "in_player.h"
#include "in_utils.h"
#include "in_shareddefs.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CPlayersSystem g_PlayersSystem;
CPlayersSystem *ThePlayersSystem = &g_PlayersSystem;

// Puntero al acceso de la base de datos players.db
dbHandler *ThePlayersDatabase = NULL;
dbHandler *TheGameDatabase = NULL;

//================================================================================
// Comandos
//================================================================================

//================================================================================
// Inicialización
//================================================================================
bool CPlayersSystem::Init() 
{
    ThePlayersDatabase = new dbHandler();
    ThePlayersDatabase->Initialise("players.db");

    TheGameDatabase = new dbHandler();
    TheGameDatabase->Initialise("game.db");

    return true;
}

//================================================================================
// Apagado, al cerrar el motor
//================================================================================
void CPlayersSystem::Shutdown() 
{
    if ( ThePlayersDatabase )
        delete ThePlayersDatabase;
}

//================================================================================
// Reinicia la información del sistema
//================================================================================
void CPlayersSystem::Restart()
{
    m_iTotal        = 0;
    m_iAlive        = 0;
    m_iConnected    = 0;

    m_iHealth            = 0;
    m_iInCombat            = 0;
    m_iUnderAttack        = 0;
    m_iWithFireWeapons    = 0;

    m_ThinkTimer.Start( 1.0f );

    SetTeam( TEAM_ANY );
}

//================================================================================
//================================================================================
void CPlayersSystem::LevelInitPreEntity()
{
    Restart();
}

//================================================================================
//================================================================================
void CPlayersSystem::FrameUpdatePostEntityThink()
{
    // Aún no
    if ( !m_ThinkTimer.IsElapsed() )
        return;

    Update();
    m_ThinkTimer.Start( 1.0f );
}

//================================================================================
//================================================================================
void CPlayersSystem::Update()
{
    m_iTotal        = 0;
    m_iConnected    = 0;
    m_iInCombat        = 0;
    m_iUnderAttack    = 0;

    FOR_EACH_PLAYER(
    {
        // Conectado
        ++m_iConnected;

        // No es del equipo que queremos
        if ( GetTeam() != TEAM_ANY && pPlayer->GetTeamNumber() != GetTeam() )
            continue;

        // Total del equipo
        ++m_iTotal;

        // No esta vivo
        if ( !pPlayer->IsAlive() || pPlayer->IsObserver() )
            continue;

        // En combate
        if ( pPlayer->IsInCombat() )
            ++m_iInCombat;

        // Bajo ataque
        if ( pPlayer->IsUnderAttack() )
            ++m_iUnderAttack;
    });

    // Basic Stats
    m_iHealth           = GetHealthTotal( GetTeam() );
    m_flStress          = GetStressTotal( GetTeam() );
    m_iAlive            = GetAliveCount( GetTeam() );
    m_iWithFireWeapons  = GetFireWeaponsCount( GetTeam() );
    m_iDejected         = GetDejectedCount( GetTeam() );
    m_WeaponsStats      = GetWeaponStats( GetTeam() );
    m_PlayerStats       = GetPlayersStats();

    // Debug
    /*
    DevMsg("STATS \n");
    DevMsg("    m_iHealth = %i \n", m_iHealth);
    DevMsg("    m_iAlive = %i / %i \n", m_iAlive, m_iTotal);
    DevMsg("    m_iInCombat = %i \n", m_iInCombat);
    DevMsg("    m_iUnderAttack = %i \n", m_iUnderAttack);
    DevMsg("    m_iWithFireWeapons = %i \n", m_iWithFireWeapons);
    DevMsg("    m_iDejected = %i \n", m_iDejected);
    DevMsg("    m_WeaponsStats = %s \n", g_StatsNames[m_WeaponsStats]);
    DevMsg("    m_PlayerStats = %s \n", g_StatsNames[m_PlayerStats]);
    DevMsg("=============================================================\n");
    */
}

//================================================================================
// Ejecuta un comando y lo envía a todos los jugadores
//================================================================================
void CPlayersSystem::ExecuteCommand( const char *command, int team )
{
    FOR_EACH_PLAYER_TEAM(
    {
        pPlayer->ExecuteCommand( command );
    }, team);
}

//================================================================================
// Ejecuta un comando y lo envía a todos los jugadores
//================================================================================
CPlayer *CPlayersSystem::GetRandom( int team )
{
    // Lista de Jugadores posibles
    CUtlVector<int> players;

    FOR_EACH_PLAYER_TEAM(
    {
        // No esta vivo
        if ( !pPlayer->IsAlive() )
            continue;

        // Lo agregamos a la lista
        players.AddToTail( it );
    }, team);

    // Ningún Jugador
    if ( players.Count() == 0 )
        return NULL;

    // Uno al azar
    int random = RandomInt( 0, players.Count()-1 );

    CPlayer *pPlayer = ToInPlayer(UTIL_PlayerByIndex( players.Element(random) ));
    return pPlayer;
}

//================================================================================
// Hace Spawn a todos los jugadores
//================================================================================
void CPlayersSystem::RespawnAll( int team )
{
    FOR_EACH_PLAYER_TEAM(
    {
        // No esta vivo
        if ( !pPlayer->IsAlive() )
            continue;

        // Spawn!
        pPlayer->Spawn();
    }, team);
}

//================================================================================
// Devuelve la cantidad de jugadores incapacitados
//================================================================================
int CPlayersSystem::GetDejectedCount( int team )
{
    int count = 0;

    FOR_EACH_PLAYER_TEAM(
    {
        // No esta vivo
        if ( !pPlayer->IsAlive() )
            continue;
        
        // Incapacitado
        if ( pPlayer->IsDejected() )
            ++count;
    }, team);

    return count;
}

//================================================================================
// Devuelve la cantidad de jugadores
//================================================================================
int CPlayersSystem::GetTotalCount( int team )
{
    int count = 0;
    FOR_EACH_PLAYER_TEAM( ++count;, team );

    return count;
}

//================================================================================
// Devuelve la cantidad de jugadores vivos
//================================================================================
int CPlayersSystem::GetAliveCount( int team )
{
    int count = 0;

    FOR_EACH_PLAYER_TEAM(
    {
        // No esta vivo
        if ( !pPlayer->IsAlive() )
            continue;

        ++count;
    }, team);

    return count;
}

//================================================================================
// Devuelve la cantidad de jugadores que tienen armas de fuego
//================================================================================
int CPlayersSystem::GetFireWeaponsCount( int team )
{
    int count = 0;

    FOR_EACH_PLAYER_TEAM(
    {
        // No esta vivo
        if ( !pPlayer->IsAlive() )
            continue;

        CBaseWeapon *pWeapon = pPlayer->GetActiveBaseWeapon();

        // No tiene arma
        if ( !pWeapon )
            continue;

        // Es de cuerpo a cuerpo
        if ( pWeapon->IsMeleeWeapon() )
            continue;

        ++count;
    }, team);

    return count;
}

//================================================================================
// Devuelve el promedio de la salud de todos los jugadores
//================================================================================
int CPlayersSystem::GetHealthTotal( int team )
{
    int health = 0;
    int players = 0;

    FOR_EACH_PLAYER_TEAM(
    {
        // No esta vivo
        if ( !pPlayer->IsAlive() )
            continue;

        // Los jugadores incapacitados tienen una penalización
        if ( pPlayer->IsDejected() )
            health += (int)round( (float)pPlayer->GetHealth() / 2.0f );
        else
            health += pPlayer->GetHealth();
        
        ++players;
    }, team);

    // Dividimos la salud entre todos
    if ( health > 0 )
        health = (int)round( (float)health / (float)players );

    return health;
}

//================================================================================
// Devuelve el promedio del estrés de todos los jugadores
//================================================================================
float CPlayersSystem::GetStressTotal( int team )
{
    float stress = 0;
    int players = 0;

    FOR_EACH_PLAYER_TEAM(
    {
        stress += pPlayer->GetStress();
        ++players;
    }, team);

    // Dividimos entre todos
    if ( stress > 0.0f )
        stress = stress / (float)players;

    return stress;
}

//================================================================================
// Devuelve si algún jugador se encuentra en combate
//================================================================================
bool CPlayersSystem::IsAnyPlayerInCombat( int team )
{
    FOR_EACH_PLAYER_TEAM(
    {
        // No esta vivo
        if ( !pPlayer->IsAlive() )
            continue;

        if ( pPlayer->IsInCombat() )
            return true;
    }, team);

    return false;
}

//================================================================================
// Devuelve si algún jugador se encuentra bajo ataque
//================================================================================
bool CPlayersSystem::IsAnyPlayerUnderAttack( int team )
{
    FOR_EACH_PLAYER_TEAM(
    {
        // No esta vivo
        if ( !pPlayer->IsAlive() )
            continue;

        if ( pPlayer->IsUnderAttack() )
            return true;
    }, team);

    return false;
}

//================================================================================
// Devuelve el estado actual de las armas de los jugadores
//================================================================================
StatType CPlayersSystem::GetWeaponStats( int team )
{
    // STATS_MEDIOCRE
    int type = STATS_MEDIOCRE;

    // Stats
    int total        = GetTotalCount( team );
    int count        = GetFireWeaponsCount( team );
    StatType ammo    = GetAmmoStats( team );

    // La mitad de jugadores tiene un arma - STATS_POOR
    if ( count >= roundup(total*0.5) )
        ++type;

    // Todos tienen un arma - STATS_NORMAL
    if ( count == total )
        ++type;

    // Buen estado de munición - STATS_GOOD
    if ( ammo >= STATS_NORMAL )
        ++type;

    // Excelente estado de munición - STATS_EXCELENT
    if ( ammo >= STATS_EXCELENT )
        ++type;

    // clamp
    type = clamp( type, STATS_MEDIOCRE, STATS_EXCELENT );

    return (StatType)type;
}

//================================================================================
// Devuelve el estado actual de la munición de los jugadores
// TODO: Algo mejor
//================================================================================
StatType CPlayersSystem::GetAmmoStats( int team )
{
    // STATS_MEDIOCRE
    int total        = GetTotalCount( team );
    int type        = STATS_MEDIOCRE;

    int mediocreCount = 0;
    int poorCount = 0;
    int normalCount = 0;
    int goodCount = 0;
    int excelentCount = 0;

    FOR_EACH_PLAYER_TEAM(
    {
        // No esta vivo
        if ( !pPlayer->IsAlive() )
            continue;

        CBaseWeapon *pWeapon = pPlayer->GetActiveBaseWeapon();

        // No tiene arma
        if ( !pWeapon )
            continue;

        // Es de cuerpo a cuerpo
        if ( pWeapon->IsMeleeWeapon() )
            continue;

        // Munición actual
        int ammo        = pPlayer->GetAmmoCount( pWeapon->GetPrimaryAmmoType() );
        int max            = pWeapon->GetMaxClip1();
        int individual    = STATS_MEDIOCRE;

        // Más de 0.2 - STATS_POOR
        if ( ammo >= roundup(max*0.2) )
            ++individual;

        // Más de la mitad - STATS_NORMAL
        if ( ammo >= roundup(max*0.4) )
            ++individual;

        // Más de la mitad - STATS_GOOD
        if ( ammo >= roundup(max*0.6) )
            ++individual;

        // Mucha - STATS_EXCELENT
        if ( ammo >= roundup(max*0.8) )
            ++individual;
        
        switch( individual )
        {
            case STATS_MEDIOCRE:
                ++mediocreCount;
            break;

            case STATS_POOR:
                ++poorCount;
            break;

            case STATS_NORMAL:
                ++normalCount;
            break;

            case STATS_GOOD:
                ++goodCount;
            break;

            case STATS_EXCELENT:
                ++excelentCount;
            break;
        }
    }, team);

    // STATS_POOR
    if ( mediocreCount <= roundup(total*0.3) )
        ++type;

    // STATS_NORMAL
    if ( poorCount <= roundup(total*0.3) )
        ++type;

    // STATS_GOOD
    if ( normalCount >= roundup(total*0.3) )
        ++type;

    // STATS_EXCELENT
    if ( goodCount >= roundup(total*0.3) )
        ++type;

    // clamp
    type = clamp( type, STATS_MEDIOCRE, STATS_EXCELENT );
    return (StatType)type;
}



//================================================================================
// Devuelve el estado de todos los jugadores
//================================================================================
StatType CPlayersSystem::GetPlayersStats()
{
    // STATS_EXCELENT
    int type = STATS_EXCELENT;

    // Stats
    float total     = (float)( GetTotal() );
    int alive       = GetAlive();
    int dejected    = GetDejected();
    int health      = GetHealth();
    float stress    = GetStress();
    int weapons     = m_WeaponsStats;
    //int combat      = GetInCombat();
    //int attacked    = GetUnderAttack();

    // La mayoría de jugadores estan muertos
    if ( alive < roundup(total * 0.8f) )
        --type;

    // Baja salud
    if ( health <= 60 )
        --type;

    // Baja salud
    if ( health <= 30 )
        --type;

    // Buen estado de armas y munición
    if ( weapons < STATS_NORMAL )
        --type;

    // Jugadores incapacitados!
    if ( dejected >= roundup(total * 0.3f) )
        --type;

    // Jugadores incapacitados!
    if ( dejected >= roundup(total * 0.6f) )
        --type;

    // Alto estrés
    if ( stress >= 50 )
        --type;

    // clamp
    type = clamp( type, STATS_MEDIOCRE, STATS_EXCELENT );
    return (StatType)type;
}

//================================================================================
// Devuelve el Jugador más cercano a la posición indicada
//================================================================================
CPlayer *CPlayersSystem::GetNear( const Vector &vecPosition, float &distance, CBasePlayer *pExcept, int team, bool ignoreBots )
{
    CPlayer *pNear = NULL;
    distance = 999999.0f;

    FOR_EACH_PLAYER_TEAM(
    {
        // No esta vivo
        if ( !pPlayer->IsAlive() )
            continue;

        // No queremos este
        if ( pExcept && pPlayer == pExcept )
            continue;

        if ( ignoreBots && pPlayer->IsBot() )
            continue;

        float flDistance = pPlayer->GetAbsOrigin().DistTo( vecPosition );

        // Esta más lejos
        if ( flDistance > distance )
            continue;

        pNear        = pPlayer;
        distance    = flDistance;
    }, team);

    return pNear;
}

//================================================================================
// Devuelve el Jugador más cercano a la posición indicada
//================================================================================
CPlayer *CPlayersSystem::GetNear( const Vector &vecPosition, CBasePlayer *pExcept, int team, bool ignoreBots )
{
    float distance;
    return GetNear( vecPosition, distance, pExcept, team, ignoreBots );
}

//================================================================================
// Devuelve si el NPC tiene una ruta segura hacia el Jugador
//================================================================================
bool CPlayersSystem::HasRouteToPlayer( CPlayer *pPlayer, CAI_BaseNPC *pNPC, float tolerance, Navigation_t type )
{
    AI_Waypoint_t *pRoute = pNPC->GetPathfinder()->BuildRoute( pNPC->GetAbsOrigin(), pPlayer->GetAbsOrigin(), pPlayer, tolerance, type );

    if ( !pRoute )
        return false;

    return true;
}

//================================================================================
// Devuelve si el NPC tiene una ruta segura hacia cualquier Jugador
//================================================================================
bool CPlayersSystem::HasRouteToAnyPlayer( CAI_BaseNPC *pNPC, float tolerance, int team, Navigation_t type )
{
    FOR_EACH_PLAYER_TEAM(
    {
        // No esta vivo
        if ( !pPlayer->IsAlive() )
            continue;

        // Tiene una ruta a este jugador!
        if ( HasRouteToPlayer(pPlayer, pNPC, tolerance, type) )
            return true;
    }, team);

    return false;
}

bool CPlayersSystem::IsAbleToSee( CBaseEntity *pEntity, const CRecipientFilter &filter, CBaseCombatCharacter::FieldOfViewCheckType checkFOV )
{
    for ( int i = 0; i < filter.GetRecipientCount(); ++i ) {
        CPlayer *pPlayer = ToInPlayer( filter.GetRecipientIndex( i ) );

        if ( !pPlayer ) 
            continue;

        if ( !pPlayer->IsAlive() )
            continue;

        if ( pPlayer->IsAbleToSee( pEntity, checkFOV ) )
            return true;
    }

    return false;
}

bool CPlayersSystem::IsAbleToSee( const Vector &vecPosition, const CRecipientFilter &filter, CBaseCombatCharacter::FieldOfViewCheckType checkFOV )
{
    for ( int i = 0; i < filter.GetRecipientCount(); ++i ) {
        CPlayer *pPlayer = ToInPlayer( filter.GetRecipientIndex( i ) );

        if ( !pPlayer ) 
            continue;

        if ( !pPlayer->IsAlive() )
            continue;

        if ( pPlayer->IsAbleToSee( vecPosition, checkFOV ) )
            return true;
    }

    return false;
}

//================================================================================
// Devuelve si algún jugador tiene visibilidad hacia la entidad
// NOTA: No es visibilidad real, no tiene en cuenta el FOV
//================================================================================
bool CPlayersSystem::IsVisible( CBaseEntity *pEntity, bool ignoreBots, int team )
{
    if ( !pEntity )
        return false;

    FOR_EACH_PLAYER(
    {
        // No esta vivo
        if ( !pPlayer->IsAlive() )
            continue;

        // No es del equipo que queremos
        if ( team != TEAM_ANY && pPlayer->GetTeamNumber() != team )
            continue;

        // Los Bots no cuentan
        if ( ignoreBots && pPlayer->IsBot() )
            continue;

        // Es visible!
        if ( pPlayer->FVisible(pEntity, MASK_BLOCKLOS) )
            return true;
    });

    return false;
}

//================================================================================
// Devuelve si algún jugador tiene visibilidad hacia la posición
// NOTA: No es visibilidad real, no tiene en cuenta el FOV
//================================================================================
bool CPlayersSystem::IsVisible( const Vector &vecPosition, bool ignoreBots, int team )
{
    FOR_EACH_PLAYER(
    {
        // No esta vivo
        if ( !pPlayer->IsAlive() )
            continue;

        // No es del equipo que queremos
        if ( team != TEAM_ANY && pPlayer->GetTeamNumber() != team )
            continue;

        // Los Bots no cuentan
        if ( ignoreBots && pPlayer->IsBot() )
            continue;

        // Es visible!
        if ( pPlayer->FVisible(vecPosition, MASK_BLOCKLOS) )
            return true;
    });

    return false;
}

//================================================================================
// Devuelve si algún jugador tiene visibilidad bajo su FOV hacia la entidad
//================================================================================
bool CPlayersSystem::IsEyesVisible( CBaseEntity *pEntity, bool ignoreBots, int team )
{
    if ( !pEntity )
        return false;

    FOR_EACH_PLAYER(
    {
        // No esta vivo
        if ( !pPlayer->IsAlive() )
            continue;

        // Tiene NOTARGET activo, lo ignoramos
        if ( (pPlayer->GetFlags() & FL_NOTARGET) )
            continue;

        // No es del equipo que queremos
        if ( team != TEAM_ANY && pPlayer->GetTeamNumber() != team )
            continue;

        // Los Bots no cuentan
        if ( ignoreBots && pPlayer->IsBot() )
            continue;        

        // Es visible!
        if ( pPlayer->FEyesVisible(pEntity, MASK_BLOCKLOS) )
            return true;
    });

    return false;
}

//================================================================================
// Devuelve si algún jugador tiene visibilidad bajo su FOV hacia la posición
//================================================================================
bool CPlayersSystem::IsEyesVisible( const Vector &vecPosition, bool ignoreBots, int team )
{
    FOR_EACH_PLAYER(
        // No esta vivo
        if ( !pPlayer->IsAlive() )
            continue;

        // No es del equipo que queremos
        if ( team != TEAM_ANY && pPlayer->GetTeamNumber() != team )
            continue;

        // Los Bots no cuentan
        if ( ignoreBots && pPlayer->IsBot() )
            continue;

        // Es visible!
        if ( pPlayer->FEyesVisible(vecPosition, MASK_VISIBLE) )
            return true;
    );

    return false;
}

//================================================================================
// Devuelve si algún jugador tiene en su FOV la entidad
//================================================================================
bool CPlayersSystem::IsInViewcone( CBaseEntity *pEntity, bool ignoreBots, int team )
{
    if ( !pEntity )
        return false;

    FOR_EACH_PLAYER(
    {
        // No esta vivo
        if ( !pPlayer->IsAlive() )
            continue;

        // No es del equipo que queremos
        if ( team != TEAM_ANY && pPlayer->GetTeamNumber() != team )
            continue;

        // Los Bots no cuentan
        if ( ignoreBots && pPlayer->IsBot() )
            continue;

        // Es visible!
        if ( pPlayer->FInViewCone(pEntity) )
            return true;
    });

    return false;
}

//================================================================================
// Devuelve si algún jugador tiene en su FOV hacia la posición
//================================================================================
bool CPlayersSystem::IsInViewcone( const Vector &vecPosition, bool ignoreBots, int team )
{
    FOR_EACH_PLAYER(
    {
        // No esta vivo
        if ( !pPlayer->IsAlive() )
            continue;

        // No es del equipo que queremos
        if ( team != TEAM_ANY && pPlayer->GetTeamNumber() != team )
            continue;

        // Los Bots no cuentan
        if ( ignoreBots && pPlayer->IsBot() )
            continue;

        // Es visible!
        if ( pPlayer->FInViewCone(vecPosition) )
            return true;
    });

    return false;
}

//================================================================================
// Envía una lección del Instructor al jugador especificado.
//================================================================================
void CPlayersSystem::SendLesson( const char *pLesson, CPlayer *pPlayer, bool once, CBaseEntity *pSubject )
{
    // TODO: once

    // Creamos la lección
    IGameEvent *event = Utils::CreateLesson( pLesson, pSubject );

    if ( !event )
        return;

    // Configuramos y lanzamos
    event->SetInt( "userid", pPlayer->GetUserID() );
    gameeventmanager->FireEvent( event );
}

//================================================================================
// Envía una lección del Instructor a todos los jugadores
//================================================================================
void CPlayersSystem::SendLesson2All( const char *pLesson, int team, bool once, CBaseEntity *pSubject )
{
    FOR_EACH_PLAYER_TEAM(
    {
        // No esta vivo
        if ( !pPlayer->IsAlive() )
            continue;

        // Enviamos
        SendLesson( pLesson, pPlayer, once, pSubject );
    }, team);
}