//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "bot_maker.h"

#include "in_player.h"
#include "in_utils.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
// Información y Red
//================================================================================

BEGIN_DATADESC( CBotSpawn )
    DEFINE_KEYFIELD( m_bDisabled, FIELD_BOOLEAN, "StartDisabled" ),
    DEFINE_KEYFIELD( m_iMaxCount, FIELD_INTEGER, "MaxCount" ),
    DEFINE_KEYFIELD( m_flFrequency, FIELD_FLOAT, "SpawnFrequency" ),
    DEFINE_KEYFIELD( m_iMaxLiveChildren, FIELD_INTEGER, "MaxLiveChildren" ),
    DEFINE_KEYFIELD( m_iHullCheckMode, FIELD_INTEGER, "HullCheckMode" ),

    DEFINE_THINKFUNC( MakerThink ),

    // Outputs
    DEFINE_OUTPUT( m_OnSpawn, "OnSpawn" ),
    DEFINE_OUTPUT( m_OnAllSpawned, "OnAllSpawned" ),
    DEFINE_OUTPUT( m_OnAllSpawnedDead, "OnAllSpawnedDead" ),
    DEFINE_OUTPUT( m_OnAllLiveChildrenDead, "OnAllLiveChildrenDead" ),
END_DATADESC()

//================================================================================
// Creación en el mapa
//================================================================================
void CBotSpawn::Spawn()
{
    // No somos solidos
    SetSolid( SOLID_NONE );

    // Reiniciamos la información
    m_iLiveChildren = 0;

    // Podemos empezar a crear bots!
    if ( !m_bDisabled )
    {
        SetThink( &CBaseBotMaker::MakerThink );
        SetNextThink( gpGlobals->curtime + 0.1f );
    }
    else
    {
        // Esperamos...
        SetThink( &CBaseBotMaker::SUB_DoNothing );
    }
}