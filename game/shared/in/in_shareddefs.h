//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef IN_SHAREDDEFS_H
#define IN_SHAREDDEFS_H

#pragma once

#ifndef CLIENT_DLL
#include "nav.h"
#include "recipientfilter.h"
#endif

#ifndef MAX_CONDITIONS
#define    MAX_CONDITIONS 32*8
#endif

typedef CBitVec<MAX_CONDITIONS> CFlagsBits;

//================================================================================
// Nombre del Juego
//================================================================================

#ifdef APOCALYPSE
    #define GAME_NAME "Apocalypse-22"
#elif SCP
    #define GAME_NAME "SCP: Source"
#else
    #define GAME_NAME "InSource"
#endif

#define USE_L4D2_MODELS

//================================================================================
// Preprocesadores
//================================================================================

//FCVAR_SERVER_CAN_EXECUTE

// Player avoidance
#define PUSHAWAY_THINK_INTERVAL        (1.0f / 20.0f)

#define roundup( expression ) (int)ceil(expression)

#define MAX_NAV_AREAS			9999
#define MAX_NODES_PER_AREA		60
#define MAX_SOUND_CHANNELS		8

#define INFINITE -1

#define AE_PLAYER_FOOTSTEP_LEFT 62
#define AE_PLAYER_FOOTSTEP_RIGHT 63

#define PLAYER_SOUND_RADIUS 832.0f
#define GET_COVER_RADIUS 1500.0f

#define DEFAULT_FORGET_TIME 60.0f

//================================================================================
// Funciones de utlidad
//================================================================================

#define FCVAR_SERVER            FCVAR_REPLICATED | FCVAR_DEMO				// Obedece solo el valor del servidor
#define FCVAR_ADMIN_ONLY        FCVAR_SERVER_CAN_EXECUTE | FCVAR_SERVER     // Comando que solo puede cambiarse desde el servidor (srcds console o local server owner)

#undef DECLARE_COMMAND
#define DECLARE_COMMAND( name, value, description, flags )				ConVar name( #name, value, flags, description );
#define DECLARE_ADMIN_COMMAND( name, value, description )				DECLARE_COMMAND( name, value, description, FCVAR_ADMIN_ONLY )
#define DECLARE_REPLICATED_CHEAT_COMMAND( name, value, description )	DECLARE_COMMAND( name, value, description, FCVAR_REPLICATED | FCVAR_CHEAT )
#define DECLARE_REPLICATED_COMMAND( name, value, description )			DECLARE_COMMAND( name, value, description, FCVAR_SERVER )
#define DECLARE_DEBUG_COMMAND( name, value, description )				DECLARE_COMMAND( name, value, description, FCVAR_ADMIN_ONLY | FCVAR_CHEAT | FCVAR_NOTIFY )

#ifdef CLIENT_DLL
    #define DECLARE_CHEAT_COMMAND( name, value, description )			DECLARE_COMMAND( name, value, description, FCVAR_CHEAT )
    #define DECLARE_NOTIFY_COMMAND										DECLARE_REPLICATED_COMMAND
#else
    #define DECLARE_CHEAT_COMMAND			DECLARE_REPLICATED_CHEAT_COMMAND
    #define DECLARE_NOTIFY_COMMAND( name, value, description )			DECLARE_COMMAND( name, value, description, FCVAR_SERVER | FCVAR_NOTIFY )
#endif

//================================================================================
// Alien FX
//================================================================================

#define ALIENFX_SETCOLOR 1

//================================================================================
// Canales y capas de sonidos
//================================================================================

enum
{
	CHANNEL_ANY,
	CHANNEL_1 = 1,
	CHANNEL_2,
	CHANNEL_3,
	CHANNEL_4,
	CHANNEL_5,
	CHANNEL_6,
	CHANNEL_7,
	CHANNEL_8,

	LAST_CHANNEL
};

enum
{
	LAYER_VERYLOW = 0,
	LAYER_LOW,
	LAYER_MEDIUM,
	LAYER_HIGH,
	LAYER_VERYHIGH,
	LAYER_IMPORTANT,
	LAYER_ABSOLUTE = 10
};


//================================================================================
// Macros de ayuda para música del Jugador
//================================================================================

#define TAG_PLAYER_SOUND( name ) SOUND_INFO(name, false, TEAM_ANY, NULL, this, TARGET_EXCEPT)
#define PLAYER_SOUND( name, loop ) SOUND_INFO(name, loop, TEAM_ANY, this, this, TARGET_ONLY)

#define PREPARE_SOUND( channel, info ) pSound = new CSoundInstance( info ); \
	pSound->SetChannel( channel ); \
	pSound->SetOrigin( this ); \
	m_nMusicManager->Add( pSound );

#define PREPARE_TAG_SOUND( info ) pTag = new CSoundInstance( info, pSound ); \
	pTag->SetExcept( this ); \
	pTag->SetTeam( GetTeamNumber() );

#define PREPARE_SOUND_WITH_TAG( channel, info, tag_info ) PREPARE_SOUND(channel, info); PREPARE_TAG_SOUND(tag_info);

//================================================================================
// Colisiones
//================================================================================
enum
{
    COLLISION_GROUP_NOT_BETWEEN_THEM = LAST_SHARED_COLLISION_GROUP,
    LAST_SHARED_IN_COLLISION_GROUP
};

//================================================================================
// Navigation Mesh
//================================================================================
#ifndef CLIENT_DLL
enum
{
    NAV_MESH_DONT_SPAWN = NAV_MESH_FIRST_CUSTOM,
    NAV_MESH_PLAYER_START = 0x00020000,
    NAV_MESH_HIDDEN = 0x00040000,

#ifdef APOCALYPSE
    NAV_MESH_SPAWN_AMBIENT = 0x00080000,
    NAV_MESH_SPAWN_FORCED = 0x00100000,
    NAV_MESH_SPAWN_BOSS = 0x00200000,
    NAV_MESH_SPAWN_SOLDIERS = 0x00400000,
    NAV_MESH_RESOURCES = 0x00800000,
#endif

    //NAV_MESH_LAST_CUSTOM
};
#endif

//================================================================================
// Equipos
//================================================================================
static char *sTeamNames[] =
{
    "#Team_Unassigned",
    "#Team_Spectator",
    "#Team_Generic",

    #ifdef APOCALYPSE
        "#Team_Humans",     // Humanos
        "#Team_Soldiers",   // Soldados
        "#Team_Infected"    // Infectados
    #elif SCP
        "#TeamHumans",
        "#TeamSoldiers",
        "#TeamSCP"
    #endif
};

enum
{
    //TEAM_UNASSIGNED,
    //TEAM_SPECTATOR

    TEAM_GENERIC,

    #ifdef APOCALYPSE
        TEAM_HUMANS = LAST_SHARED_TEAM + 1,
        TEAM_SOLDIERS,
        TEAM_INFECTED
    #elif SCP
        TEAM_HUMANS = LAST_SHARED_TEAM + 1,
        TEAM_SOLDIERS,
        TEAM_SCP,
    #endif
};

//================================================================================
// Modos de Juego
//================================================================================
enum
{
    GAME_MODE_NONE = 0,

    #ifdef APOCALYPSE
        GAME_MODE_COOP,
        GAME_MODE_SURVIVAL,
        GAME_MODE_SURVIVAL_TIME,
        GAME_MODE_ASSAULT,
        GAME_MODE_CAPTURE,
    #elif SCP
        GAME_MODE_FACILITIES_SINGLEPLAYER,
        GAME_MODE_FACILITIES_COOP,
        GAME_MODE_FACILITIES_VERSUS,

        GAME_MODE_052,            // http://lafundacionscp.wikidot.com/scp-052
        GAME_MODE_087,            // http://lafundacionscp.wikidot.com/scp-087
        GAME_MODE_450,            // http://lafundacionscp.wikidot.com/scp-450
    #endif

    LAST_GAME_MODE
};

//================================================================================
// Modo de Spawn
//================================================================================
enum
{
    SPAWN_MODE_NONE = 0,

    SPAWN_MODE_RANDOM,
    SPAWN_MODE_UNIQUE,

    LAST_SPAWN_MODE
};

//================================================================================
// ID de arma
//================================================================================
enum WeaponID
{
    WEAPON_ID_NONE = 0,

    WEAPON_ID_SMG,
    WEAPON_ID_PISTOL,
    WEAPON_ID_CROWBAR,
    WEAPON_ID_SHOTGUN,

    LAST_WEAPON_ID
};

//================================================================================
// Clase de arma
//================================================================================
enum WeaponClass
{
    WEAPON_CLASS_NONE = 0,

    WEAPON_CLASS_PRIMARY,
    WEAPON_CLASS_SECONDARY,

    LAST_WEAPON_CLASS
};


//================================================================================
// Condición del jugador
//================================================================================
enum
{
    PLAYER_STATUS_NONE = 0,     // Normal
    PLAYER_STATUS_CLIMBING,     // Trepando por su vida
    PLAYER_STATUS_DEJECTED,     // Incapacitado
    PLAYER_STATUS_FALLING,      // Cayendo

    LAST_PLAYER_STATUS
};

//================================================================================
// Estado del jugador
//================================================================================
enum
{
    PLAYER_STATE_NONE = 0,
    PLAYER_STATE_WELCOME,
    PLAYER_STATE_ACTIVE,
    PLAYER_STATE_PICKING_TEAM,
    PLAYER_STATE_PICKING_CLASS,
    PLAYER_STATE_SPECTATOR,
    PLAYER_STATE_DEAD,

    LAST_PLAYER_STATE
};

static char *g_PlayerStateNames[LAST_PLAYER_STATE] = {
    "NONE",
    "WELCOME",
    "ACTIVE",
    "PICKING_TEAM",
    "PICKING_CLASS",
    "SPECTATOR",
    "DEAD"
};

//================================================================================
// Clases de jugador
//================================================================================
enum
{
    PLAYER_CLASS_NONE = 0,
    PLAYER_CLASS_COMMANDER,

#ifdef APOCALYPSE
    PLAYER_CLASS_DIRECTOR_ASSISTANT = PLAYER_CLASS_COMMANDER,
    PLAYER_CLASS_INFECTED_COMMON,
    PLAYER_CLASS_INFECTED_BOSS,
    PLAYER_CLASS_SOLDIER_LEVEL1,
    PLAYER_CLASS_SOLDIER_LEVEL2,
    PLAYER_CLASS_SOLDIER_LEVEL3,
    PLAYER_CLASS_SOLDIER_MEDIC,
#elif SCP
    PLAYER_CLASS_SCP_173, // 173!
    PLAYER_CLASS_SCP_096, // No lo mires
    PLAYER_CLASS_SCP_035, // Loco de la mascara
    PLAYER_CLASS_SCP_049, // Dr.
    PLAYER_CLASS_SCP_106, // Old man
    PLAYER_CLASS_SCP_4579, // A.
#endif

    LAST_PLAYER_CLASS
};

//================================================================================
// Componentes del jugador
//================================================================================
enum
{
    PLAYER_COMPONENT_INVALID = 0,
    PLAYER_COMPONENT_HEALTH,
    PLAYER_COMPONENT_EFFECTS,
    PLAYER_COMPONENT_DEJECTED,

#ifdef SCP
    PLAYER_COMPONENT_BLIND_MOVEMENT,
#endif

    LAST_PLAYER_COMPONENT
};

//================================================================================
// Atributos
//================================================================================
enum
{
	ATTR_INVALID = 0,
	ATTR_HEALTH,
	ATTR_STAMINA,
	ATTR_SHIELD,

	LAST_PLAYER_ATTRIBUTE
};

//================================================================================
// Mensaje de depuración
//================================================================================
struct DebugMessage
{
    char m_string[1024];
    IntervalTimer m_age;
};

//================================================================================
// Allows you to save various types of information.
//================================================================================
class CMultidata
{
public:
    DECLARE_CLASS_NOBASE( CMultidata );

    CMultidata()
    {
        Reset();
    }

    CMultidata( int value )
    {
        Reset();
        SetInt( value );
    }

    CMultidata( Vector value )
    {
        Reset();
        SetVector( value );
    }

    CMultidata( float value )
    {
        Reset();
        SetFloat( value );
    }

    CMultidata( const char *value )
    {
        Reset();
#ifndef CLIENT_DLL
        SetString( value );
#endif
    }

    CMultidata( CBaseEntity *value )
    {
        Reset();
        SetEntity( value );
    }

    virtual void Reset() {
        vecValue.Invalidate();
        flValue = 0;
        iValue = 0;
        iszValue = NULL_STRING;
        pszValue = NULL;
        Purge();
    }

    void SetInt( int value ) {
        iValue = value;
        flValue = (float)value;
        OnSet();
    }

    void SetFloat( float value ) {
        iValue = (int)value;
        flValue = value;
        OnSet();
    }

    void SetVector( const Vector &value ) {
        vecValue = value;
        OnSet();
    }

#ifndef CLIENT_DLL
    void SetString( const char *value ) {
        iszValue = AllocPooledString( value );
        OnSet();
    }
#endif

    void SetEntity( CBaseEntity *value ) {
        pszValue = value;
        OnSet();
    }

    virtual void OnSet() { }

    const Vector &GetVector() const {
        return vecValue;
    }

    float GetFloat() const {
        return flValue;
    }

    int GetInt() const {
        return iValue;
    }

#ifndef CLIENT_DLL
    const char *GetString() const {
        return STRING( iszValue );
    }
#endif

    CBaseEntity *GetEntity() const {
        return pszValue.Get();
    }

    int Add( CMultidata *data ) {
        int index = list.AddToTail( data );
        OnSet();
        return index;
    }

    bool Remove( CMultidata *data ) {
        return list.FindAndRemove( data );
    }

    void Purge() {
        list.PurgeAndDeleteElements();
    }

    Vector vecValue;
    float flValue;
    int iValue;
    string_t iszValue;
    EHANDLE pszValue;
    CUtlVector<CMultidata *> list;
};

//================================================================================
//================================================================================
#ifndef CLIENT_DLL
class CBulletsTraceFilter : public CTraceFilterSimpleList
{
public:
    CBulletsTraceFilter( int collisionGroup ) : CTraceFilterSimpleList( collisionGroup ) {}

    bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
    {
        if ( m_PassEntities.Count() )
        {
            CBaseEntity *pEntity = EntityFromEntityHandle( pHandleEntity );
            CBaseEntity *pPassEntity = EntityFromEntityHandle( m_PassEntities[0] );
            if ( pEntity && pPassEntity && pEntity->GetOwnerEntity() == pPassEntity && 
                pPassEntity->IsSolidFlagSet(FSOLID_NOT_SOLID) && pPassEntity->IsSolidFlagSet( FSOLID_CUSTOMBOXTEST ) && 
                pPassEntity->IsSolidFlagSet( FSOLID_CUSTOMRAYTEST ) )
            {
                // It's a bone follower of the entity to ignore (toml 8/3/2007)
                return false;
            }
        }
        return CTraceFilterSimpleList::ShouldHitEntity( pHandleEntity, contentsMask );
    }

};
#else
typedef CTraceFilterSimpleList CBulletsTraceFilter;
#endif

//================================================================================
// Filtro para usuarios fuera del limite auditivo de un sonido
//================================================================================
#ifndef CLIENT_DLL
class CPASOutAttenuationFilter : public CPASFilter
{
public:
    CPASOutAttenuationFilter( CBaseEntity *entity, soundlevel_t soundlevel ) :
        CPASFilter( static_cast<const Vector&>(entity->GetSoundEmissionOrigin()) )
    {
        Filter( entity->GetSoundEmissionOrigin(), SNDLVL_TO_ATTN( soundlevel ) );
    }

    CPASOutAttenuationFilter( CBaseEntity *entity, float attenuation = ATTN_NORM ) :
        CPASFilter( static_cast<const Vector&>(entity->GetSoundEmissionOrigin()) )
    {
        Filter( entity->GetSoundEmissionOrigin(), attenuation );
    }

    CPASOutAttenuationFilter( const Vector& origin, soundlevel_t soundlevel ) :
        CPASFilter( origin )
    {
        Filter( origin, SNDLVL_TO_ATTN( soundlevel ) );
    }

    CPASOutAttenuationFilter( const Vector& origin, float attenuation = ATTN_NORM ) :
        CPASFilter( origin )
    {
        Filter( origin, attenuation );
    }

    CPASOutAttenuationFilter( CBaseEntity *entity, const char *lookupSound ) :
        CPASFilter( static_cast<const Vector&>(entity->GetSoundEmissionOrigin()) )
    {
        soundlevel_t level = CBaseEntity::LookupSoundLevel( lookupSound );
        float attenuation = SNDLVL_TO_ATTN( level );
        Filter( entity->GetSoundEmissionOrigin(), attenuation );
    }

    CPASOutAttenuationFilter( const Vector& origin, const char *lookupSound ) :
        CPASFilter( origin )
    {
        soundlevel_t level = CBaseEntity::LookupSoundLevel( lookupSound );
        float attenuation = SNDLVL_TO_ATTN( level );
        Filter( origin, attenuation );
    }

    CPASOutAttenuationFilter( CBaseEntity *entity, const char *lookupSound, HSOUNDSCRIPTHANDLE& handle ) :
        CPASFilter( static_cast<const Vector&>(entity->GetSoundEmissionOrigin()) )
    {
        soundlevel_t level = CBaseEntity::LookupSoundLevel( lookupSound, handle );
        float attenuation = SNDLVL_TO_ATTN( level );
        Filter( entity->GetSoundEmissionOrigin(), attenuation );
    }

    CPASOutAttenuationFilter( const Vector& origin, const char *lookupSound, HSOUNDSCRIPTHANDLE& handle ) :
        CPASFilter( origin )
    {
        soundlevel_t level = CBaseEntity::LookupSoundLevel( lookupSound, handle );
        float attenuation = SNDLVL_TO_ATTN( level );
        Filter( origin, attenuation );
    }

public:
    void Filter( const Vector& origin, float attenuation = ATTN_NORM )
    {
        // Don't crop for attenuation in single player
    if ( gpGlobals->maxClients == 1 ) 
    {
        RemoveAllRecipients();
        return;
    }

    // CPASFilter adds them by pure PVS in constructor
    if ( attenuation <= 0 ) 
    {
        RemoveAllRecipients();
        return;
    }

    AddAllPlayers();

    // Now remove recipients that are outside sound radius
    float minAudible = ( 2 * SOUND_NORMAL_CLIP_DIST ) / attenuation;
    minAudible -= 1100.0f; // FIXED

    int c = GetRecipientCount();
    
    for ( int i = c - 1; i >= 0; i-- ) {
        int index = GetRecipientIndex( i );

        CBaseEntity *ent = CBaseEntity::Instance( index );

        if ( !ent || !ent->IsPlayer() ) {
            continue;
        }

        CBasePlayer *player = ToBasePlayer( ent );

        if ( !player ) {
            continue;
        }

#ifndef _XBOX
        // never remove the HLTV or Replay bot
        if ( player->IsHLTV() || player->IsReplay() )
            continue;
#endif

        float flDistance = player->EarPosition().DistTo(origin);

        if ( flDistance > minAudible )
            continue;

        if ( player->GetSplitScreenPlayers().Count() )
        {
            CUtlVector< CHandle< CBasePlayer > > &list = player->GetSplitScreenPlayers();
            bool bSend = false;
            for ( int k = 0; k < list.Count(); k++ )
            {
                if ( list[k]->EarPosition().DistTo(origin) > minAudible )
                {
                    bSend = true;
                    break;
                }
            }
            if ( bSend )
                continue;
        }

        RemoveRecipient( player );
    }
    }
};
#endif


#endif // IN_SHAREDDEFS_H