//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "director_manager.h"

#include "director.h"
#include "bots\bot.h"

#include "dbhandler.h"

#include "in_shareddefs.h"
#include "in_gamerules.h"
#include "in_player.h"
#include "in_utils.h"
#include "players_system.h"

#include "ai_basenpc.h"
#include "ai_network.h"
#include "ai_node.h"

#include "KeyValues.h"
#include "fmtstr.h"

#include "nav.h"
#include "nav_area.h"
#include "nav_mesh.h"

#include "decals.h"
#include "ilagcompensationmanager.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DirectorManager g_DirectorManager;
DirectorManager *TheDirectorManager = &g_DirectorManager;

//================================================================================
// Comandos
//================================================================================

extern ConVar director_debug;

DECLARE_REPLICATED_COMMAND( director_spawn_mode, "2", "" )
DECLARE_CHEAT_COMMAND( director_manager_use_navmesh, "1", "" )
DECLARE_CHEAT_COMMAND( director_manager_spawn_novisible_spots, "1", "" )
DECLARE_DEBUG_COMMAND( director_manager_check_unreachable, "1", "" );

//================================================================================
// Constructor 
//================================================================================
DirectorManager::DirectorManager()
{
    m_PopulationList.EnsureCapacity( 32 );
}

//================================================================================
//================================================================================
void DirectorManager::Init()
{
    SetPopulation("default");
}

//================================================================================
//================================================================================
void DirectorManager::ResetMap()
{
    Precache();

    FOR_EACH_MINION_TYPE(it)
    {
        m_Minions[it].alive = 0;
        m_Minions[it].tooClose = 0;
        m_Minions[it].created = 0;
        m_Minions[it].lastSpawn = 0;
    }

    FOR_EACH_VEC( m_PopulationList, pt )
    {
        m_PopulationList[pt]->lastSpawn = 0;
        m_PopulationList[pt]->nextSpawn.Start( m_PopulationList[pt]->spawnInterval );
    }

    m_ScanTimer.Start( 1.0f );
}

//================================================================================
//================================================================================
void DirectorManager::ScanSpawnableSpots()
{
    if ( !m_ScanTimer.IsElapsed() )
        return;

    m_CandidateAreas.Purge();

    // ¡Este mapa no tiene nodos!
    // Lamentablemente los NPC's siguen usando la red de navegación por nodos
    /*if ( !g_pBigAINet || g_pBigAINet->NumNodes() <= 0 )
    {
        DevWarning( 2, "Este mapa no tiene nodos de movimiento!!\n");
        m_ScanTimer.Start( 10.0f );
        return;
    }*/

    ScanNodes();
    ScanNavMesh();
    m_ScanTimer.Start( 1.0f );
}

//================================================================================
// Establece el tipo de población que usara el Director
//================================================================================
void DirectorManager::SetPopulation( const char *type )
{
    Q_strncpy( m_nPopulation, type, sizeof(m_nPopulation) );
    LoadPopulation();
}

//================================================================================
// Carga la población de hijos
//================================================================================
void DirectorManager::LoadPopulation()
{
    m_PopulationList.PurgeAndDeleteElements();

    dbReadResult *results = TheGameDatabase->ReadMultiple( "SELECT unit,spawn_chance,spawn_interval,type,is_template,max_units FROM director_population WHERE population = '%s'", m_nPopulation );

    FOR_EACH_RESULT( it, results, 6 )
    {
        CMinionInfo *info = new CMinionInfo();
        Q_strncpy( info->unit, COLUMN( results, 0 ).text, sizeof( info->unit ) );
        info->spawnChance = COLUMN( results, 1 ).integer;
        info->spawnInterval = COLUMN( results, 2 ).integer;
        info->type = (MinionType)COLUMN( results, 3 ).integer;
        Q_strncpy( info->population, m_nPopulation, sizeof( info->population ) );
        info->isTemplate = ( COLUMN( results, 4 ).integer == 1 ) ? true : false;
        info->maxUnits = COLUMN( results, 5 ).integer;

        info->alive = 0;
        info->created = 0;
        info->lastSpawn = 0;
        info->nextSpawn.Start( 0 );

        DevMsg( "[DirectorManager] Minion: %s (spawnChance: %i) (spawnInterval: %i) (type: %i)  (population: %s) (maxUnits: %i)\n", info->unit, info->spawnChance, info->spawnInterval, info->type, info->population, info->maxUnits );
        m_PopulationList.AddToTail( info );
    }

    results->Purge();
    delete results;


    /*
    KeyValues *pFile = new KeyValues("Population");
    KeyValues::AutoDelete autoDelete( pFile );

    // Leemos el archivo
    pFile->LoadFromFile( filesystem, DIRECTOR_POPULATION_FILE, NULL );
    KeyValues *pType;

    // Pasamos por cada sección
    for ( pType = pFile->GetFirstTrueSubKey(); pType; pType = pType->GetNextTrueSubKey() )
    {
        // Este tipo de población no es la que usa el mapa actual
        if ( !FStrEq(pType->GetName(), m_nPopulation) )
            continue;

        KeyValues *pChildType;

        // Pasamos por los tipos de hijos
        for ( pChildType = pType->GetFirstSubKey(); pChildType; pChildType = pChildType->GetNextKey() )
        {
            KeyValues *pData;

            // Pasamos por los hijos a crear
            for ( pData = pChildType->GetFirstSubKey(); pData; pData = pData->GetNextKey() )
            {
                FOR_EACH_MINION_TYPE
                {
                    int index = m_ChildsList.Find( it );

                    ChildInfo info;
                    Q_strncpy( info.name, pData->GetName(), sizeof(info.name) );
                    info.percent = pChildType->GetInt(pData->GetName(), 0);

                    // Lo guardamos en la lista.
                    if ( FStrEq(pChildType->GetName(), g_DirectorChildType[it]) )
                        m_ChildsList.Element(index)->AddToTail( info );
                }
            }
        }
    }
    */
}

//================================================================================
// Devuelve el nombre de una unidad para el tipo de minion especificado
//================================================================================
CMinionInfo *DirectorManager::GetMinionInfo( MinionType type )
{
    PopulationList list;
    list.EnsureCapacity( m_PopulationList.Count() );

    // Colocamos en una lista temporal los minions de este tipo
    FOR_EACH_VEC( m_PopulationList, it )
    {
        CMinionInfo *info = m_PopulationList[it];
        Assert( info );

        if ( info->type != type )
            continue;

        if ( !TheDirector->CanSpawnMinion( info ) )
            continue;

        list.AddToTail( info );
    }

    if ( list.Count() == 0 )
        return NULL;

    if ( list.Count() == 1 )
        return list.Element( 0 );

    int attempts = 0;

    // TODO: Hacer algo mejor
    while ( attempts < 6 )
    {
        ++attempts;

        FOR_EACH_VEC( list, et )
        {
            CMinionInfo *info = list[et];
            Assert( info );

            if ( RandomInt( 1, 100 ) <= info->spawnChance )
                return info;
        }
    }

    // Intentos fallidos (poco porcentaje), usamos uno al azar
    return NULL;
    //return list.Element( RandomInt(0, list.Count()-1) );
}

//================================================================================
//================================================================================
bool DirectorManager::GetIdealSpot( MinionType type, Vector &vecPosition ) 
{
    int attempts = 20;

    do
    {
		--attempts;

        // Nav Mesh
        if ( ShouldUseNavMesh() )
        {
            if ( m_CandidateAreas.Count() == 0 )
                return false;

            Assert( m_CandidateAreas.Count() > 0 );

            int random = RandomInt( 0, m_CandidateAreas.Count() - 1 );
            CNavArea *pArea = m_CandidateAreas.Element( random );

            vecPosition = pArea->GetRandomPoint();
        }
        else
        {
            if ( m_CandidateNodes.Count() == 0 )
                return false;

            Assert( m_CandidateNodes.Count() > 0 );

            int random = RandomInt( 0, m_CandidateNodes.Count() - 1 );
            CAI_Node *pNode = m_CandidateNodes.Element( random );

            vecPosition = pNode->GetOrigin();
        }

        if ( ShouldUseSpot(vecPosition) )
			return true;
		
    } while ( attempts > 0 );

	return false;
}

//================================================================================
// Guarda en caché los minions
//================================================================================
void DirectorManager::Precache()
{
    FOR_EACH_VEC( m_PopulationList, it )
    {
        CMinionInfo *info = m_PopulationList[it];
        Assert( info );

        if ( info->isTemplate )
            continue;

        UTIL_PrecacheOther( info->unit );
    }
}

//================================================================================
// Devuelve la cantidad de spawns que faltan para llenar el máximo
//================================================================================
int DirectorManager::GetSpawnsLeft( MinionType type )
{
    int alive = m_Minions[type].alive;
    int max = TheDirector->GetMaxUnits( type );
    return (max - alive);
}

//================================================================================
// Devuelve la cantidad de todos los minions creados
//================================================================================
int DirectorManager::GetMinionsCreated()
{
    int count = 0;

    FOR_EACH_MINION_TYPE( it )
    {
        count += m_Minions[it].created;
    }

    return count;
}

//================================================================================
// Devuelve la cantidad de minions del tipo especificado que hay en la lista
// para poblar
//================================================================================
int DirectorManager::GetPopulation( MinionType type )
{
    int count = 0;

    FOR_EACH_VEC( m_PopulationList, it )
    {
        CMinionInfo *info = m_PopulationList[it];
        Assert( info );

        if ( info->type == type )
            count++;
    }

    return count;
}

//================================================================================
// Intenta crear minions de todos los tipos
//================================================================================
void DirectorManager::Spawn() 
{
    if ( m_PopulationList.Count() == 0 )
        return;

    FOR_EACH_MINION_TYPE(it)
    {
        Spawn( (MinionType)it );
    }
}

//================================================================================
// Intenta crear minions del tipo especificado
//================================================================================
void DirectorManager::Spawn( MinionType type ) 
{
    if ( !TheDirector->CanSpawnMinions(type) )
        return;

    int left = GetSpawnsLeft( type );
    int attempts = 0;
    AssertMsg( left > 0, "Minion Spawns satisfied but Spawn() is called!" );

    while ( left > 0 ) {
        if ( CreateMinion( type ) ) {
            left--;
            TheDirectorManager->Scan();
            continue;
        }

        attempts++;

        if ( attempts > 10 ) {
            left--;
            attempts = 0;
        }
    }
}

//================================================================================
//================================================================================
bool DirectorManager::CreateMinion( MinionType type, Vector *vecPosition ) 
{
    CMinionInfo *minion = GetMinionInfo( type );

    if ( !minion ) {
        if ( type == CHILD_TYPE_COMMON ) {
            Warning( "[Director] Sin minions para hacer spawn de %s!\n", g_MinionTypes[type] );
        }

        return false;
    }

    return CreateMinion( minion, vecPosition );
}

//================================================================================
//================================================================================
bool DirectorManager::CreateMinion( CMinionInfo *minion, Vector *vecPosition )
{
    if ( !minion )
        return false;

    Assert( minion->nextSpawn.IsElapsed() );

    if ( !TheDirector->CanSpawnMinion(minion) )
        return false;

    if ( !vecPosition ) {
        Vector vecIdeal;
        GetIdealSpot( minion->type, vecIdeal );
        vecPosition = &vecIdeal;
    }

    bool result;

    if ( minion->isTemplate ) {
        CBotSpawn *pSpawner = (CBotSpawn *)gEntList.FindEntityByName( NULL, minion->unit );

        if ( !pSpawner ) {
            Assert( !"No se ha encontrado el spawner template para un minion" );
            return false;
        }

        result = SetupMinion( pSpawner, minion, vecPosition );
    }
    else {
        QAngle angles = RandomAngle( 0.0f, 360.0f );
        angles.x = 0.0f;
        angles.z = 0.0f;

        CAI_BaseNPC *pMinion = (CAI_BaseNPC *)CBaseEntity::CreateNoSpawn( minion->unit, *vecPosition, angles );

        if ( !pMinion ) {
            Assert( !"Ha ocurrido un problema al crear un minion NPC" );
            return false;
        }

        result = SetupMinion( pMinion, minion, vecPosition );
    }

    if ( !result )
        return false;

    minion->created++;
    minion->lastSpawn = gpGlobals->curtime;
    minion->nextSpawn.Start( minion->spawnInterval - 1 );

    m_Minions[minion->type].created++;
    m_Minions[minion->type].lastSpawn = gpGlobals->curtime;
    return true;
}

//================================================================================
//================================================================================
bool DirectorManager::SetupMinion( CAI_BaseNPC * pMinion, CMinionInfo * minion, Vector * vecPosition )
{
    const char *pName = UTIL_VarArgs( "director_%s", g_MinionTypes[minion->type] );

    string_t name = AllocPooledString( pName );
    pMinion->SetName( name );

    pMinion->AddSpawnFlags( SF_NPC_FADE_CORPSE );
    pMinion->AddSpawnFlags( SF_NPC_FALL_TO_GROUND );

    DispatchSpawn( pMinion );
    pMinion->Activate();

    if ( director_debug.GetInt() >= 3 )
        NDebugOverlay::VertArrow( *vecPosition + Vector( 0, 0, 25.0f ), *vecPosition, 25.0f / 4.0f, 255, 0, 255, 0, true, 1.0f );

    lagcompensation->AddAdditionalEntity( pMinion );
    return true;
}

//================================================================================
//================================================================================
bool DirectorManager::SetupMinion( CBotSpawn * pSpawner, CMinionInfo * minion, Vector * vecPosition )
{
    pSpawner->SpawnBot();

    if ( !pSpawner->GetPlayer() ) {
        Assert( !"Ha ocurrido un problema al hacer que un info_bot_spawn haga Spawn!" );
        return false;
    }

    QAngle angles = RandomAngle( 0.0f, 360.0f );
    angles.x = 0.0f;
    angles.z = 0.0f;

    CPlayer *pPlayer = pSpawner->GetPlayer();

    const char *pName = UTIL_VarArgs( "director_%s", g_MinionTypes[minion->type] );

    string_t name = AllocPooledString(pName);
    pPlayer->SetName( name );

    //pPlayer->GetAI()->m_vecSpawnSpot = *vecPosition;

    pPlayer->Teleport( vecPosition, &angles, NULL );
    return true;
}

//================================================================================
// Devuelve si el Director puede usar el Nav Mesh para crear hijos
//================================================================================
bool DirectorManager::ShouldUseNavMesh()
{
    return director_manager_use_navmesh.GetBool();
}

//================================================================================
// Devuelve si el area se puede usar para hacer spawn
//================================================================================
bool DirectorManager::CanUseNavArea( CNavArea *pArea )
{
    if ( !pArea )
        return false;

    if ( pArea->HasAttributes( NAV_MESH_DONT_SPAWN | NAV_MESH_PLAYER_START ) )
        return false;

    if ( pArea->IsUnderwater() )
        return false;

    if ( pArea->IsBlocked( TEAM_ANY ) || pArea->HasAvoidanceObstacle() )
        return false;

    if ( pArea->GetDanger( TEAM_ANY ) > 10.0f )
        return false;

    if ( m_CandidateAreas.HasElement( pArea ) )
        return false;

    return true;
}

//================================================================================
// Devuelve si el nodo se puede usar para hacer spawn
//================================================================================
bool DirectorManager::CanUseNode( CAI_Node *pNode )
{
    if ( !pNode )
        return false;

    if ( pNode->GetType() != NODE_GROUND )
        return false;

    Vector vecPosition = pNode->GetPosition( HULL_HUMAN );

    if ( director_manager_spawn_novisible_spots.GetBool() ) {
        if ( ThePlayersSystem->IsInViewcone( vecPosition ) || ThePlayersSystem->IsVisible( vecPosition ) ) {
            return false;
        }
    }

    return true;
}

//================================================================================
// Devuelve si es posible usar el punto especificado para hacer spawn
//================================================================================
bool DirectorManager::ShouldUseSpot( const Vector vecPosition )
{
    if ( !vecPosition.IsValid() )
        return false;

    if ( director_manager_spawn_novisible_spots.GetBool() ) {
        if ( ThePlayersSystem->IsInViewcone( vecPosition ) || ThePlayersSystem->IsVisible( vecPosition ) ) {
            return false;
        }
    }

    Vector vecFloor = vecPosition;
    vecFloor.z -= 300.0f;

    // Hacemos un trace hacia el suelo
    trace_t tr;
    UTIL_TraceLine( vecPosition, vecFloor, MASK_SOLID, NULL, COLLISION_GROUP_NONE, &tr );

    // Es una textura de utilidad
    if ( strstr( tr.surface.name, "TOOLS/" ) )
        return false;

    // Un modelo?
    if ( strstr( tr.surface.name, "studio" ) )
        return false;

    return true;
}

//================================================================================
// Escanea el Nav Mesh en busca de lugares para hacer spawns
//================================================================================
void DirectorManager::ScanNavMesh()
{
    // Todas las areas generadas
    int count = TheNavMesh->GetNavAreaCount();

    if ( count == 0 )
        return;

    FOR_EACH_VEC( TheNavAreas, it )
    {
        CNavArea *pArea = TheNavAreas[it];

        if ( !CanUseNavArea( pArea ) )
            continue;

        Vector vecPosition = pArea->GetCenter();
        float distance = 0;

        // Obtenemos al jugador más cercano
        CPlayer *pPlayer = ThePlayersSystem->GetNear( vecPosition, distance );

        if ( !pPlayer )
            continue;

        // Esta muy lejos o muy cerca
        if ( distance > TheDirector->GetMaxDistance() || distance < TheDirector->GetMinDistance() )
            continue;

        Vector vecTemporal = vecPosition;
        vecTemporal.z += HalfHumanHeight;

        // No podemos usar este punto
        if ( !ShouldUseSpot( vecTemporal ) && !pArea->HasAttributes( NAV_MESH_HIDDEN ) )
            continue;

        // Marcamos el punto afortunado
        if ( director_debug.GetInt() >= 2 )
            pArea->DrawFilled( 0, 0, 255, 100, 0.4f ); // Azul

        // Area disponible!
        m_CandidateAreas.AddToTail( pArea );
    }
}

//================================================================================
// Escanea los nodos de navegación en busca de lugares para hacer spawns
//================================================================================
void DirectorManager::ScanNodes()
{
    // Todos los nodos
    int count = g_pBigAINet->NumNodes();
    //Assert( count > 0 );

    if ( count == 0 )
        return;

    for ( int i = 0; i < count; ++i ) {
        // Obtenemos el nodo
        CAI_Node *pNode = g_pBigAINet->GetNode( i );

        // Inválido
        if ( !CanUseNode( pNode ) )
            continue;

        Vector vecPosition = pNode->GetPosition( HULL_HUMAN );
        CNavArea *pArea = TheNavMesh->GetNearestNavArea( vecPosition );

        // Inválido
        if ( !CanUseNavArea( pArea ) )
            continue;

        float distance = 0;

        // Obtenemos al jugador más cercano
        CPlayer *pPlayer = ThePlayersSystem->GetNear( vecPosition, distance );

        if ( !pPlayer )
            continue;

        // Esta muy lejos o muy cerca
        if ( distance > TheDirector->GetMaxDistance() || distance < TheDirector->GetMinDistance() )
            continue;

        Vector vecTemporal = vecPosition;
        vecTemporal.z += HalfHumanHeight;

        // No podemos usar este punto
        if ( !ShouldUseSpot( vecTemporal ) && !pArea->HasAttributes( NAV_MESH_HIDDEN ) )
            continue;

        // Marcamos el punto afortunado
        if ( director_debug.GetInt() >= 2 )
            NDebugOverlay::Box( vecTemporal, -Vector( 5, 5, 5 ), Vector( 5, 5, 5 ), 0, 0, 255, 255, 0.4f );

        // Area disponible!
        m_CandidateNodes.AddToTail( pNode );
    }
}

//================================================================================
// Escanea los minions vivos en el mapa
//================================================================================
void DirectorManager::Scan()
{
    ScanSpawnableSpots();

    CBaseEntity *pMinion = NULL;

    // Reiniciamos variables
    FOR_EACH_MINION_TYPE(it)
    {
        m_Minions[it].alive = 0;
        m_Minions[it].tooClose = 0;
    }

    FOR_EACH_VEC( m_PopulationList, pt )
    {
        m_PopulationList[pt]->alive = 0;
    }

    do {
        pMinion = gEntList.FindEntityByName( pMinion, "director_*" );

        if ( !pMinion )
            continue;

        if ( !pMinion->IsAlive() )
            continue;

        ScanMinion( pMinion );
    }
    while ( pMinion );
}

//================================================================================
// Escanea un minion
//================================================================================
void DirectorManager::ScanMinion( CBaseEntity *pMinion )
{
    if ( ShouldKill( pMinion ) ) {
        Kill( pMinion, "ShouldKill()" );
        return;
    }

    FOR_EACH_MINION_TYPE( it )
    {
        const char *pName = UTIL_VarArgs( "director_%s", g_MinionTypes[it] );

        if ( !pMinion->NameMatches( pName ) )
            continue;

        m_Minions[it].alive++;

        if ( IsTooClose( pMinion ) )
            m_Minions[it].tooClose++;

        // Necesitas un nuevo enemigo
        if ( ShouldReportEnemy( pMinion ) )
            ReportEnemy( pMinion );
    }

    FOR_EACH_VEC( m_PopulationList, pt )
    {
        if ( m_PopulationList[pt]->isTemplate ) {
            if ( pMinion->GetOwnerEntity() ) {
                if ( pMinion->GetOwnerEntity()->NameMatches( m_PopulationList[pt]->unit ) ) {
                    m_PopulationList[pt]->alive++;
                }
            }
        }
        else {
            if ( pMinion->ClassMatches( m_PopulationList[pt]->unit ) ) {
                m_PopulationList[pt]->alive++;
            }
        }
    }
}

//================================================================================
// Devuelve si el minion debe ser eliminado
//================================================================================
bool DirectorManager::ShouldKill( CBaseEntity *pEntity )
{
    if ( ThePlayersSystem->IsEyesVisible(pEntity) )
        return false;

    if ( IsTooFar(pEntity) || IsUnreachable(pEntity) )
        return true;

    return false;
}

//================================================================================
// Elimina un minion
//================================================================================
void DirectorManager::Kill( CBaseEntity *pEntity, const char *reason )
{
    if ( director_debug.GetInt() >= 3 )
        NDebugOverlay::Text( pEntity->EyePosition(), reason, false, 3.5f );

    if ( pEntity->IsPlayer() ) {
        CPlayer *pPlayer = ToInPlayer( pEntity );

        if ( pPlayer->IsBot() ) {
            pPlayer->Kick();
        }
        else {
            // TODO
        }
    }
    else {
        UTIL_Remove( pEntity );
    }
}

//================================================================================
// Devuelve si el director puede reportar a este minion la ubicación de un enemigo
//================================================================================
bool DirectorManager::ShouldReportEnemy( CBaseEntity *pEntity )
{
    if ( !TheDirector->IsStatus( STATUS_PANIC ) && !TheDirector->IsStatus( STATUS_FINALE ) )
        return false;

    if ( pEntity->IsNPC() )
        return true;

    if ( pEntity->IsPlayer() ) {
        CPlayer *pPlayer = ToInPlayer( pEntity );
        return pPlayer->IsBot();
    }

    return false;
}

//================================================================================
// Reporta la ubicación de un enemigo al minion
//================================================================================
void DirectorManager::ReportEnemy( CBaseEntity *pMinion )
{
    CPlayer *pEnemy = ThePlayersSystem->GetRandom( TEAM_HUMANS );

    if ( pEnemy ) {
        if ( (pEnemy->GetFlags() & FL_NOTARGET) )
            return;
    }

    if ( pMinion->IsNPC() ) {
        CAI_BaseNPC *pNPC = pMinion->MyNPCPointer();

        if ( pNPC->GetEnemy() && pNPC->GetEnemy()->IsPlayer() ) {
            pNPC->UpdateEnemyMemory( pNPC->GetEnemy(), pNPC->GetEnemy()->GetAbsOrigin() );
            return;
        }

        if ( pEnemy ) {
            pNPC->UpdateEnemyMemory( pEnemy, pEnemy->GetAbsOrigin() );
            pNPC->SetEnemy( pEnemy );
            return;
        }
    }

    if ( pMinion->IsPlayer() ) {
        CPlayer *pPlayer = ToInPlayer( pMinion );

        if ( !pPlayer->IsBot() )
            return;

        IBot *pBot = pPlayer->GetBotController();

        if ( !pBot->GetMemory() )
            return;

        if ( pBot->GetEnemy() && pBot->GetEnemy()->IsPlayer() ) {
            pBot->GetMemory()->UpdateEntityMemory( pBot->GetEnemy(), pBot->GetEnemy()->GetAbsOrigin() );
            return;
        }

        if ( pEnemy ) {
            pBot->GetMemory()->UpdateEntityMemory( pEnemy, pEnemy->GetAbsOrigin() );
            pBot->SetEnemy( pEnemy );
            return;
        }
    }
}

//================================================================================
// Devuelve si el hijo esta muy lejos de los jugadores
//================================================================================
bool DirectorManager::IsTooFar( CBaseEntity *pEntity )
{
    float maxDistance = TheDirector->GetMaxDistance();

    FOR_EACH_PLAYER(
    {
        if ( !pPlayer->IsAlive() )
            continue;

        float distance = pPlayer->GetAbsOrigin().DistTo( pEntity->GetAbsOrigin() );

        if ( distance < maxDistance )
            return false;
    });

    return true;
}

//================================================================================
// Devuelve si el hijo esta muy cerca de los jugadores
//================================================================================
bool DirectorManager::IsTooClose( CBaseEntity *pEntity )
{
    float minDistance = TheDirector->GetMinDistance();

    FOR_EACH_PLAYER(
    {
        // Esta muerto
        if ( !pPlayer->IsAlive() )
            continue;

        if ( pPlayer->IsBot() )
            continue;

        float distance = pPlayer->GetAbsOrigin().DistTo( pEntity->GetAbsOrigin() );

        // Este jugador lo tiene cerca!
        if ( distance <= minDistance )
            return true;
    });

    return false;
}

//================================================================================
// Devuelve si el hijo no puede llegar a los jugadores
//================================================================================
bool DirectorManager::IsUnreachable( CBaseEntity *pEntity )
{
    if ( !pEntity )
        return false;

    // No debemos hacer esta verificación
    if ( !director_manager_check_unreachable.GetBool() )
        return false;    

    // Estas muy cerca de algún jugador, evitamos que
    // de repente desaparezcas
    if ( IsTooClose(pEntity) )
        return false;

    CAI_BaseNPC *pNPC = pEntity->MyNPCPointer();

    if ( pNPC ) {
        // No tiene un enemigo aún
        if ( !pNPC->GetEnemy() )
            return false;

        // Solo verificamos si el enemigo es un Jugador
        if ( !pNPC->GetEnemy()->IsPlayer() )
            return false;

        // No puedes alcanzar a tu objetivo
        return pNPC->IsUnreachable(pNPC->GetEnemy());
    }

    if ( pEntity->IsPlayer() ) {
        CPlayer *pPlayer = ToInPlayer(pEntity);

        if ( pPlayer->IsBot() && pPlayer->GetBotController() ) {
            IBot *pBot = pPlayer->GetBotController();

            if ( !pBot->GetEnemy() )
                return false;

            if ( !pBot->GetEnemy()->IsPlayer() )
                return false;

            if ( pBot->GetLocomotion()->IsStuck() && pBot->GetLocomotion()->GetStuckDuration() >= 3.6f )
                return true;

            return pBot->HasCondition(BCOND_GOAL_UNREACHABLE);
        }
    }

    return true;
}