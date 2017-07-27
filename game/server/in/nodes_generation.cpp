//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "nodes_generation.h"

#include "in_player.h"
#include "in_shareddefs.h"

#include "nav.h"
#include "nav_mesh.h"
#include "nav_area.h"
#include "nav_ladder.h"
#include "nav_node.h"

#include "ai_initutils.h"
#include "ai_node.h"
#include "ai_network.h"
#include "ai_networkmanager.h"

#include "editor_sendcommand.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CNodesGeneration g_NodesGeneration;
CNodesGeneration *TheNodeGenerator = &g_NodesGeneration;

//================================================================================
// Comandos
//================================================================================

DECLARE_REPLICATED_COMMAND( ai_generate_nodes_underwater, "0", "" )
DECLARE_REPLICATED_COMMAND( ai_generate_nodes_walkable, "1", "" )
DECLARE_REPLICATED_COMMAND( ai_generate_nodes_walkable_distance, "300", "" )
DECLARE_REPLICATED_COMMAND( ai_generate_nodes_climb, "1", "" )
DECLARE_REPLICATED_COMMAND( ai_generate_nodes_hints, "0", "" )


//================================================================================
//================================================================================
void CNodesGeneration::Start()
{
    if ( !engine->IsInEditMode() ) {
        Warning( "=====================================================================\n" );
        Warning( "== Inicie el modo de de edicion antes de comenzar (map_edit)\n" );
        Warning( "=====================================================================\n" );
        return;
    }

    if ( !engine->IsDedicatedServer() ) {
        CPlayer *pHost = ToInPlayer( UTIL_GetListenServerHost() );

        if ( pHost )
            pHost->Spectate();
    }

    engine->ServerCommand( "ai_inhibit_spawners 1\n" );
    engine->ServerCommand( "bot_kick\n" );

    m_iNavAreaCount = TheNavMesh->GetNavAreaCount();

    if ( m_iNavAreaCount == 0 ) {
        engine->ServerCommand( "nav_generate\n" );
        return;
    }

    m_iWalkableNodesCount = 0;
    m_WalkLocations.Purge();

    Msg( "Comenzando a examinar %i areas de navegación... \n", m_iNavAreaCount );

    GenerateHintNodes();
    GenerateWalkableNodes();
    GenerateClimbNodes();

    Msg( "Proceso terminado. Se han generado %i nodos. \n\n", g_pAINetworkManager->GetNetwork()->NumNodes() );
}

//================================================================================
//================================================================================
void CNodesGeneration::GenerateWalkableNodes()
{
    if ( !ai_generate_nodes_walkable.GetBool() )
        return;

    if ( m_iWalkableNodesCount == 0 )
        Msg( "Generando nodos de tipo transitables...\n" );

    int nodesGenerated = m_iWalkableNodesCount;

    FOR_EACH_VEC( TheNavAreas, it )
    {
        CNavArea *pArea = TheNavAreas[it];

        if ( !pArea )
            continue;

        if ( pArea->IsUnderwater() && !ai_generate_nodes_underwater.GetBool() )
            continue;

        if ( pArea->IsBlocked( TEAM_ANY ) || pArea->HasAvoidanceObstacle() )
            continue;

        for ( int e = 0; e <= MAX_NODES_PER_AREA; ++e ) {
            // Obtenemos una posición al azar
            Vector vecPosition = pArea->GetRandomPoint();
            vecPosition.z += 10;

            bool tooClose = false;

            // Ya hemos generado un nodo aquí
            if ( m_WalkLocations.HasElement( vecPosition ) )
                continue;

            // Revisamos todos los nodos que hemos generado
            FOR_EACH_VEC( m_WalkLocations, it )
            {
                Vector vecTmp = m_WalkLocations[it];

                if ( !vecTmp.IsValid() )
                    continue;

                // Esta muy cerca de otro nodo
                if ( vecTmp.DistTo( vecPosition ) <= ai_generate_nodes_walkable_distance.GetFloat() ) {
                    if ( vecPosition.z == vecTmp.z || (vecPosition.z < (vecTmp.z + 15.0f) && vecPosition.z >( vecTmp.z - 15.0f )) ) {
                        tooClose = true;
                        break;
                    }
                }
            }

            // Esta muy cerca de otro nodo
            if ( tooClose )
                continue;

            int status = Editor_CreateNode( "info_node", g_pAINetworkManager->GetEditOps()->m_nNextWCIndex, vecPosition.x, vecPosition.y, vecPosition.z );

            // Creacion exitosa
            if ( status == Editor_OK ) {
                m_WalkLocations.AddToTail( vecPosition );
                ++m_iWalkableNodesCount;
            }
            else {
                Warning( "Ha ocurrido un problema al crear un nodo en %.2f,%.2f,%.2f\n", vecPosition.x, vecPosition.y, vecPosition.z );
                NDebugOverlay::Box( vecPosition, Vector( -5, -5, -5 ), Vector( 5, 5, 5 ), 255, 0, 0, 255, 10.0f );
            }
        }

        int max = MAX_NODES;

        // No se han generado todos los nodos que tenemos disponibles, para generar
        // un movimiento más fluido repetimos el proceso hasta completar o que no haya
        // lugares donde crear más
        if ( m_iWalkableNodesCount < max && nodesGenerated < m_iWalkableNodesCount ) {
            Msg( "%i / %i...\n", m_iWalkableNodesCount, max );
            GenerateWalkableNodes();
            return;
        }
    }

    Msg( "Se han creado %i nodos de tipo transitables... \n\n", m_iWalkableNodesCount );
}

//================================================================================
//================================================================================
void CNodesGeneration::GenerateClimbNodes()
{
    if ( !ai_generate_nodes_climb.GetBool() )
        return;

    Msg("Generando nodos de tipo escalables...\n");

    int nodesGenerated = 0;

    Vector vecTop;
    Vector vecBottom;

    for ( int i = 0; i <= MAX_NAV_AREAS; ++i )
    {
        CNavArea *pArea = TheNavMesh->GetNavAreaByID( i );

        // No existe
        if ( !pArea )
            continue;

        // Sin nodos bajo el agua
        if ( pArea->IsUnderwater() && !ai_generate_nodes_underwater.GetBool() )
            continue;

        // Obtenemos las "escaleras" del area
        const NavLadderConnectVector *pLadders = pArea->GetLadders( CNavLadder::LADDER_UP );

        // No hay escaleras
        if ( pLadders->Count() <= 0 )
            continue;

        for ( int e = 0; e < pLadders->Count(); ++e )
        {
            // Obtenemos la escalera con esta ID
            CNavLadder *pLadder = pLadders->Element(e).ladder;

            // No existe
            if ( !pLadder )
                continue;

            // Ubicación de donde comienza y donde termina
            vecTop        = pLadder->m_top;
            vecBottom    = pLadder->m_bottom;

            // Agregamos un poco de altura
            vecTop.z += 5.0f;
            vecBottom.z += 5.0f;

            int iAngles = ( 90 * (int)( pLadder->GetDir() ) ) + 90;
            QAngle angles( 0, iAngles, 0 );

            Editor_CreateNode( "info_node_climb", g_pAINetworkManager->GetEditOps()->m_nNextWCIndex, vecTop.x, vecTop.y, vecTop.z, false );
            Editor_CreateNode( "info_node_climb", g_pAINetworkManager->GetEditOps()->m_nNextWCIndex, vecBottom.x, vecBottom.y, vecBottom.z, false );

            // Rotamos hacia la dirección correcta
            Editor_RotateEntity( "info_node_climb", vecTop.x, vecTop.y, vecTop.z, angles );
            Editor_RotateEntity( "info_node_climb", vecBottom.x, vecBottom.y, vecBottom.z, angles );

            ++nodesGenerated;
            ++nodesGenerated;
        }
    }

    Msg( "Se han creado %i nodos de tipo escalables... \n\n", nodesGenerated );
}

//================================================================================
//================================================================================
void CNodesGeneration::GenerateHintNodes()
{
    if ( !ai_generate_nodes_hints.GetBool() )
        return;

    Msg("Generando nodos de ayuda...");

    int hidingSpots        = TheHidingSpots.Count();
    int nodesGenerated    = 0;

    if ( hidingSpots == 0 )
    {
        Msg("No se han generado puntos de ayuda. Omitiendo...\n");
        return;
    }

    for ( int i = 0; i <= hidingSpots; ++i )
    {
        // Obtenemos información acerca de este punto.
        HidingSpot *pSpot = TheHidingSpots.Element( i );

        // No existe
        if ( !pSpot )
            continue;

        // No existe
        if ( TheHidingSpots.Find(pSpot) <= -1 )
            continue;

        // Obtenemos la ubicación del punto
        Vector vecPosition = pSpot->GetPosition();
        vecPosition.z += 10.0f;

        // Obtenemos el area que se encuentra en la ubicación
        CNavArea *pArea = TheNavMesh->GetNavArea( vecPosition );

        // No existe
        if ( !pArea )
            continue;

        // Sin nodos bajo el agua
        if ( pArea->IsUnderwater() && !ai_generate_nodes_underwater.GetBool() )
            continue;

        // Buena cobertura
        // Los NPC lo usarán para cubrirse de ataques
        if ( pSpot->HasGoodCover() )
        {
            Editor_CreateNode( "info_node_hint", g_pAINetworkManager->GetEditOps()->m_nNextWCIndex, vecPosition.x, vecPosition.y, vecPosition.z );
            Editor_SetKeyValue( "info_node_hint", vecPosition.x, vecPosition.y, vecPosition.z, "hinttype", "100" );
            ++nodesGenerated;
        }

        // Buena posición francotirador
        // Los NPC lo usarán para mirar de vez en cuando esta posición
        if ( pSpot->IsIdealSniperSpot() || pSpot->IsGoodSniperSpot() )
        {
            Editor_CreateNode( "info_hint", g_pAINetworkManager->GetEditOps()->m_nNextWCIndex, vecPosition.x, vecPosition.y, vecPosition.z );
            Editor_SetKeyValue( "info_hint", vecPosition.x, vecPosition.y, vecPosition.z, "hinttype", "13" );
            ++nodesGenerated;
        }
    }

    Msg( "Se han creado %i nodos de ayuda... \n\n", nodesGenerated );
}

//================================================================================
//================================================================================
void C_GenerateNodes()
{
    TheNodeGenerator->Start();
}

ConCommand ai_generate_nodes("ai_generate_nodes", C_GenerateNodes, "Genera nodos de movimiento a partir de la red de navegacion");