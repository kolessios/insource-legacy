//==== Woots 2017. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "ap_bot.h"

#include "ap_bot_schedules.h"
#include "in_gamerules.h"
#include "ai_hint.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
// Crea los componentes que tendrá el Bot
//================================================================================
void CAP_Bot::SetupSchedules()
{
	ADD_SCHEDULE( CInvestigateSoundSchedule );
    ADD_SCHEDULE( CInvestigateLocationSchedule );
    ADD_SCHEDULE( CHuntEnemySchedule );
    ADD_SCHEDULE( CReloadSchedule );
    ADD_SCHEDULE( CCoverSchedule );
    ADD_SCHEDULE( CHideSchedule );
    ADD_SCHEDULE( CChangeWeaponSchedule );
    ADD_SCHEDULE( CHideAndHealSchedule );
    ADD_SCHEDULE( CHideAndReloadSchedule );
    ADD_SCHEDULE( CHelpDejectedFriendSchedule );
    ADD_SCHEDULE( CMoveAsideSchedule );
    //ADD_SCHEDULE( CCallBackupSchedule );
    ADD_SCHEDULE( CDefendSpawnSchedule );

	// Survival: Debemos buscar recursos
    if ( TheGameRules->IsGameMode( GAME_MODE_SURVIVAL ) ) {
        ADD_SCHEDULE( CSearchResourcesSchedule );

        // Soldados
        // TODO
        if ( GetHost()->GetTeamNumber() == TEAM_SOLDIERS )
            ADD_SCHEDULE( CCleanBuildingSchedule );
    }
}

//================================================================================
//================================================================================
bool CAP_Bot::ShouldAimOnlyVisibleInterestingSpots() 
{
    // Quitamos un poco la desventaja de poder ver lugares interesantes sin tener visión
    return ( TheGameRules->IsGameMode(GAME_MODE_SURVIVAL) || TheGameRules->IsGameMode(GAME_MODE_ASSAULT) );
}

//================================================================================
//================================================================================
bool CAP_Bot::ShouldLookSquadMember() 
{
    // En survival a veces es extraño que miren a miembros del escuadron aunque esten lejos
    if ( TheGameRules->IsGameMode(GAME_MODE_SURVIVAL) )
        return false;

    return BaseClass::ShouldLookSquadMember();
}

//----------------------------------------------------------------------------------------

//================================================================================
// Constructor
//================================================================================
CAP_BotSoldier::CAP_BotSoldier()
{
    m_BuildingList.EnsureCapacity( 32 );
}

//================================================================================
// Creación en el mundo
//================================================================================
void CAP_BotSoldier::Spawn()
{
    BaseClass::Spawn();

    m_BuildingList.PurgeAndDeleteElements();
    m_pCleaningBuilding = NULL;
    m_pScanningArea = NULL;
}

//================================================================================
// Devuelve si el soldado esta limpiando un edificio.
//================================================================================
bool CAP_BotSoldier::IsCleaningBuilding()
{
    if ( m_pCleaningBuilding )
        return true;

    return IsMinionCleaningBuilding();
}

//================================================================================
// Devuelve si el soldado es miembro de un escuadron cuyo líder indico
// que deben limpiar un edificio.
//================================================================================
bool CAP_BotSoldier::IsMinionCleaningBuilding()
{
    CPlayer *pLeader = GetSquadLeader();

    if ( !pLeader )
        return false;

    if ( pLeader == GetHost() )
        return false;

    if ( !pLeader->IsBot() )
        return false;

    return (pLeader->GetAI()->GetActiveScheduleID() == SCHEDULE_CLEAN_BUILDING);
}

//================================================================================
// Indica que el soldado empezará a limpiar de enemigos un edificio.
//================================================================================
void CAP_BotSoldier::StartBuildingClean( CAI_Hint *pHint )
{
    if ( HasBuildingCleaned( STRING( pHint->GetGroup() ) ) ) {
        Assert( 0 );
        return;
    }

    CBuildingInfo *info = GetBuildingInfo( STRING( pHint->GetGroup() ) );

    if ( m_pCleaningBuilding && m_pCleaningBuilding != info ) {
        Assert( 0 );
        FinishBuildingClean();
    }    

    if ( !info ) {
        m_pCleaningBuilding = new CBuildingInfo( pHint );
        m_BuildingList.AddToTail( m_pCleaningBuilding );
    }
    else {
        DebugAddMessage( "Resuming Building Clean..." );
        m_pCleaningBuilding = info;
    }
}

//================================================================================
// Indica que se ha terminado de limpiar un edificio.
//================================================================================
void CAP_BotSoldier::FinishBuildingClean()
{
    Assert( m_pCleaningBuilding );

    if ( !m_pCleaningBuilding )
        return;

    SetScanningArea( NULL );

    m_pCleaningBuilding->expired.Start( BUILDING_MEMORY_TIME );
    m_pCleaningBuilding = NULL;
}

//================================================================================
// Devuelve la información del edificio en la memoria del enemigo
//================================================================================
CBuildingInfo *CAP_BotSoldier::GetBuildingInfo( const char * pName )
{
    if ( m_BuildingList.Count() == 0 )
        return NULL;

    FOR_EACH_VEC( m_BuildingList, it )
    {
        CBuildingInfo *info = m_BuildingList[it];
        Assert( info );

        if ( FStrEq( info->name, pName ) )
            return info;
    }

    return NULL;
}

//================================================================================
//================================================================================
CBuildingInfo * CAP_BotSoldier::GetBuildingInfo()
{
    if ( IsMinionCleaningBuilding() ) {
        CAP_BotSoldier *pAI = dynamic_cast<CAP_BotSoldier *>( GetSquadLeaderAI() );
        return pAI->GetBuildingInfo();
    }

    return m_pCleaningBuilding;
}

//================================================================================
// Devuelve si el edificio indicado ha sido limpiado
//================================================================================
bool CAP_BotSoldier::HasBuildingCleaned( const char * pName )
{
    CBuildingInfo *info = GetBuildingInfo( pName );

    if ( !info )
        return false;

    if ( info == m_pCleaningBuilding )
        return false;

    if ( !info->expired.HasStarted() )
        return false;

    return true;
}

//================================================================================
// Escanea las [Nav Areas] en el radio del edificio.
//================================================================================
void CBuildingInfo::Scan()
{
    CHintCriteria hintCriteria;
    hintCriteria.SetHintType( HINT_TACTICAL_AREA );
    hintCriteria.SetGroup( entrance->GetGroup() );

    CAI_Hint *pHint = CAI_HintManager::FindHint( entrance->GetAbsOrigin(), hintCriteria );
    Assert( pHint );

    if ( !pHint )
        return;

    float radius = pHint->GetRadius();

    NavAreaCollector collector;
    collector.m_area.EnsureCapacity( 1000 );
    TheNavMesh->ForAllAreasInRadius( collector, pHint->GetAbsOrigin(), radius );

    FOR_EACH_VEC( collector.m_area, it )
    {
        CNavArea *pArea = collector.m_area[it];

        if ( pArea->IsUnderwater() )
            continue;

        if ( pArea->HasAttributes( NAV_MESH_RESOURCES ) ) {
            areas.AddToTail( pArea );
            continue;
        }

        if ( pArea->IsBlocked( TEAM_ANY ) || pArea->HasAvoidanceObstacle() )
            continue;

        
        if ( pArea->GetSizeX() < 60.0f || pArea->GetSizeY() < 60.0f )
            continue;

        Vector vecFloor = pArea->GetCenter();
        vecFloor.z += 10.0f;
        vecFloor.z -= 300.0f;

        trace_t tr;
        UTIL_TraceLine( pArea->GetCenter(), vecFloor, MASK_SOLID, NULL, COLLISION_GROUP_NONE, &tr );

        // Es una textura de utilidad
        if ( strstr( tr.surface.name, "TOOLS/" ) )
            continue;

        // Un modelo?
        if ( strstr( tr.surface.name, "studio" ) )
            continue;

        areas.AddToTail( pArea );
    }

    Assert( areas.Count() > 0 );
}
