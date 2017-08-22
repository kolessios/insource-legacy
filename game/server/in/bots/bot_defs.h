//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#ifndef BOT_CONDITIONS_H
#define BOT_CONDITIONS_H

#ifdef _WIN32
#pragma once
#endif

#include "nav.h"
#include "nav_path.h"
#include "nav_pathfind.h"
#include "nav_area.h"
#include "nav_mesh.h"
#include "bots\interfaces\improv_locomotor.h"
#include "in_gamerules.h"

class CBot;

//================================================================================
//================================================================================

#define MEMORY_BLOCK_LOOK_AROUND "BlockLookAround"
#define MEMORY_SPAWN_POSITION "SpawnPosition"
#define MEMORY_BEST_WEAPON "BestWeapon"
#define MEMORY_DEJECTED_FRIEND "DejectedFriend"

//================================================================================
// Prioridades
//================================================================================
enum
{
    PRIORITY_INVALID = 0,
    PRIORITY_VERY_LOW,
    PRIORITY_LOW,
    PRIORITY_FOLLOWING,
    PRIORITY_NORMAL,
    PRIORITY_HIGH,
    PRIORITY_VERY_HIGH,
    PRIORITY_CRITICAL,
    PRIORITY_UNINTERRUPTABLE,

    LAST_PRIORITY
};

static const char *g_PriorityNames[LAST_PRIORITY] =
{
    "INVALID",
    "VERY LOW",
    "LOW",
    "FOLLOWING",
    "NORMAL",
    "HIGH",
    "VERY HIGH",
    "CRITICAL",
    "UNINTERRUPTABLE"
};

//================================================================================
// Velocidades para apuntar
//================================================================================
enum 
{
    AIM_SPEED_VERYLOW = 0,
    AIM_SPEED_LOW,
    AIM_SPEED_NORMAL,
    AIM_SPEED_FAST,
    AIM_SPEED_VERYFAST,
    AIM_SPEED_INSTANT,

    LAST_AIM_SPEED
};

//================================================================================
// Componentes
//================================================================================
enum
{
    BOT_COMPONENT_VISION = 0,
    BOT_COMPONENT_LOCOMOTION,
    BOT_COMPONENT_FOLLOW,
    BOT_COMPONENT_MEMORY,
    BOT_COMPONENT_ATTACK,
    BOT_COMPONENT_DECISION,
    LAST_COMPONENT,
};

//================================================================================
// Desire
//================================================================================

#define BOT_DESIRE_NONE		0.0f
#define BOT_DESIRE_VERYLOW	0.1f
#define BOT_DESIRE_LOW		0.2f
#define BOT_DESIRE_MODERATE 0.5f
#define BOT_DESIRE_HIGH		0.7f
#define BOT_DESIRE_VERYHIGH 0.9f
#define BOT_DESIRE_ABSOLUTE 1.0f

//================================================================================
// Schedules
//================================================================================
enum
{
    SCHEDULE_NONE = 0,
    SCHEDULE_INVESTIGATE_LOCATION,
    SCHEDULE_INVESTIGATE_SOUND,
    SCHEDULE_HUNT_ENEMY,

    SCHEDULE_ATTACK,
    SCHEDULE_RELOAD,
    SCHEDULE_COVER,
    SCHEDULE_HIDE,

    SCHEDULE_CHANGE_WEAPON,
    SCHEDULE_HIDE_AND_HEAL,
    SCHEDULE_HIDE_AND_RELOAD,

    SCHEDULE_HELP_DEJECTED_FRIEND,
    SCHEDULE_MOVE_ASIDE,
    SCHEDULE_CALL_FOR_BACKUP,
    SCHEDULE_DEFEND_SPAWN,

	SCHEDULE_SOLDIER_COVER,

	#ifdef APOCALYPSE
	SCHEDULE_SEARCH_RESOURCES,
	SCHEDULE_WANDER,
    SCHEDULE_CLEAN_BUILDING,
	#endif

    LAST_BOT_SCHEDULE
};

static const char *g_BotSchedules[LAST_BOT_SCHEDULE] =
{
    "NONE",
    "INVESTIGATE_LOCATION",
    "INVESTIGATE_SOUND",
    "HUNT_ENEMY",
    
    "BOT_COMPONENT_ATTACK",
    "RELOAD",
    "COVER",
    "HIDE",

    "CHANGE_WEAPON",
    "HIDE_AND_HEAL",
    "HIDE_AND_RELOAD",

    "HELP_DEJECTED_FRIEND",
    "MOVE_ASIDE",
    "CALL_FOR_BACKUP",
    "DEFEND_SPAWN",
	"SOLDIER_COVER",

	#ifdef APOCALYPSE
	"SEARCH_RESOURCES",
	"WANDER",
    "CLEAN_BUILDING"
	#endif
};

//================================================================================
// Tasks
//================================================================================

enum
{
	BTASK_INVALID = 0,
	BTASK_WAIT,                         // Esperar una determinada cantidad de segundos
    BTASK_SET_TOLERANCE,                // Establece la tolerancia en distancia para las tareas de moverse a un destino.

    BTASK_PLAY_ANIMATION,
    BTASK_PLAY_GESTURE,
    BTASK_PLAY_SEQUENCE,

	BTASK_SAVE_POSITION,                // Guardar nuestra posición actual
	BTASK_RESTORE_POSITION,             // Restaurar la posición guardada

	BTASK_MOVE_DESTINATION,             // Movernos al destino especificado
    BTASK_MOVE_SPAWN,                   // Movernos al lugar donde hemos aparecido
	BTASK_MOVE_LIVE_DESTINATION,        // Seguir un destino, aunque se mueva

	BTASK_HUNT_ENEMY,                   // Perseguir al enemigo
	BTASK_GET_SPOT_ASIDE,               // Encontrar y guardar un lugar cerca donde movernos

	BTASK_GET_COVER,                    // Encontrar y guardar un lugar de cobertura
	BTASK_GET_FAR_COVER,                // Encontrar y guardar un lugar lejano de cobertura

	BTASK_AIM,                          // Apuntar a un destino especifico

	BTASK_USE,                          // Usar
	BTASK_JUMP,                         // Saltar
	BTASK_CROUCH,                       // Agacharse
	BTASK_STANDUP,                      // Levantarse
	BTASK_RUN,                          // Correr
    BTASK_SNEAK,                         // Caminar (lento) No funciona si esta corriendo
	BTASK_WALK,                  // Dejar de correr y caminar
	BTASK_RELOAD,                       // Recargar y esperar a que termine.
    BTASK_RELOAD_SAFE,                  // Recargar si el enemigo esta lejos
	BTASK_RELOAD_ASYNC,                 // Recargar sin esperar
	BTASK_HEAL,                         // Curarse

	BTASK_CALL_FOR_BACKUP,              // Llamar refuerzos
	BLAST_TASK,

	BCUSTOM_TASK = 999
};

static const char *g_BotTasks[BLAST_TASK] =
{
    "INVALID",
	"WAIT",
    "SET_TOLERANCE",

    "PLAY_ANIMATION",
    "PLAY_GESTURE",
    "PLAY_SEQUENCE",

	"SAVE_LOCATION",
	"RESTORE_LOCATION",

	"MOVE_DESTINATION",
    "MOVE_SPAWN",
	"MOVE_LIVE_DESTINATION",

	"HUNT_ENEMY",
	"GET_SPOT_ASIDE",

	"GET_COVER",
	"GET_FAR_COVER",

    "BOT_COMPONENT_VISION",

    "USE",
    "JUMP",
    "CROUCH",
    "STANDUP",
    "RUN",
    "WALK",
    "WALK_NORMAL",
    "RELOAD",
    "RELOAD_SAFE",
    "RELOAD_ASYNC",
    "HEAL",

    "CALL_FOR_BACKUP"
};

//================================================================================
// Estados
//================================================================================
enum BotState
{
    STATE_IDLE,
    STATE_ALERT,
    STATE_COMBAT,
    STATE_PANIC,

    LAST_STATE
};

//================================================================================
// Estrategia
//================================================================================
enum BotStrategie
{
    ENDURE_UNTIL_DEATH = 0,
    COWARDS,
    LAST_CALL_FOR_BACKUP,

    LAST_STRATEGIE
};

static const char *g_BotStrategie[LAST_STRATEGIE] =
{
    "ENDURE_UNTIL_DEATH",
    "COWARDS",
    "LAST_CALL_FOR_BACKUP"
};

//================================================================================
// Sleep State
//================================================================================
enum BotPerformance
{
    BOT_PERFORMANCE_AWAKE = 0,
    BOT_PERFORMANCE_VISIBILITY,
    BOT_PERFORMANCE_PVS,
    BOT_PERFORMANCE_PVS_AND_VISIBILITY,
    BOT_PERFORMANCE_SLEEP_PVS,

    LAST_PERFORMANCE
};

//================================================================================
// Información de las partes/huesos de una entidad
//================================================================================
struct HitboxBones
{
    int head;
    int chest;
    int leftLeg;
    int rightLeg;
};

typedef int HitboxType;

struct HitboxPositions
{
    void Reset() {
        head.Invalidate();
        chest.Invalidate();
        leftLeg.Invalidate();
        rightLeg.Invalidate();
    }

    Vector head;
    Vector chest;
    Vector leftLeg;
    Vector rightLeg;
};

//================================================================================
// Condiciones
//================================================================================
enum BCOND
{
    BCOND_NONE = 0,

    BCOND_EMPTY_PRIMARY_AMMO,
    BCOND_LOW_PRIMARY_AMMO,

    BCOND_EMPTY_CLIP1_AMMO,
    BCOND_LOW_CLIP1_AMMO,

    BCOND_EMPTY_SECONDARY_AMMO,
    BCOND_LOW_SECONDARY_AMMO,

    BCOND_EMPTY_CLIP2_AMMO,
    BCOND_LOW_CLIP2_AMMO,

	BCOND_HELPLESS,

    BCOND_SEE_HATE,
    BCOND_SEE_FEAR,
    BCOND_SEE_DISLIKE,
    BCOND_SEE_ENEMY,
    BCOND_SEE_DEJECTED_FRIEND,

    BCOND_ENEMY_LOST,
    BCOND_ENEMY_OCCLUDED,
    BCOND_ENEMY_LAST_POSITION_OCCLUDED,
    BCOND_ENEMY_DEAD,
    BCOND_ENEMY_UNREACHABLE, // TODO
    BCOND_ENEMY_TOO_NEAR,
    BCOND_ENEMY_NEAR,
    BCOND_ENEMY_FAR,
    BCOND_ENEMY_TOO_FAR,

    BCOND_LIGHT_DAMAGE,
    BCOND_HEAVY_DAMAGE,
    BCOND_REPEATED_DAMAGE,
    BCOND_LOW_HEALTH,
    BCOND_DEJECTED,

    BCOND_CAN_RANGE_ATTACK1,
    BCOND_CAN_RANGE_ATTACK2,
    BCOND_CAN_MELEE_ATTACK1,
    BCOND_CAN_MELEE_ATTACK2,

    BCOND_NEW_ENEMY,

    BCOND_TOO_CLOSE_TO_ATTACK,
    BCOND_TOO_FAR_TO_ATTACK,
    BCOND_NOT_FACING_ATTACK,
    BCOND_BLOCKED_BY_FRIEND,

    BCOND_TASK_FAILED,
    BCOND_TASK_DONE,
    BCOND_SCHEDULE_FAILED,
    BCOND_SCHEDULE_DONE,

    BCOND_BETTER_WEAPON_AVAILABLE,
    BCOND_MOBBED_BY_ENEMIES, // TODO
    BCOND_PLAYER_PUSHING, // TODO

    BCOND_SMELL_MEAT,
    BCOND_SMELL_CARCASS,
    BCOND_SMELL_GARBAGE,

    BCOND_HEAR_COMBAT,
    BCOND_HEAR_WORLD,
    BCOND_HEAR_ENEMY,
    BCOND_HEAR_ENEMY_FOOTSTEP,
    BCOND_HEAR_BULLET_IMPACT,
	BCOND_HEAR_BULLET_IMPACT_SNIPER,
    BCOND_HEAR_DANGER,
    BCOND_HEAR_MOVE_AWAY,
    BCOND_HEAR_SPOOKY,

    LAST_BCONDITION,
};

static const char *g_Conditions[LAST_BCONDITION] = {
    "NONE",

    "EMPTY_PRIMARY_AMMO",
    "LOW_PRIMARY_AMMO",

    "EMPTY_CLIP1_AMMO",
    "LOW_CLIP1_AMMO",

    "EMPTY_SECONDARY_AMMO",
    "LOW_SECONDARY_AMMO",

    "EMPTY_CLIP2_AMMO",
    "LOW_CLIP2_AMMO",

	"INDEFENSE",

    "SEE_HATE",
    "SEE_FEAR",
    "SEE_DISLIKE",
    "SEE_ENEMY",
    "SEE_DEJECTED_FRIEND",

    "ENEMY_LOST",
    "ENEMY_OCCLUDED",
    "ENEMY_LAST_POSITION_OCCLUDED",
    "ENEMY_DEAD",
    "ENEMY_UNREACHABLE",
    "ENEMY_TOO_NEAR",
    "ENEMY_NEAR",
    "ENEMY_FAR",
    "ENEMY_TOO_FAR",

    "LIGHT_DAMAGE",
    "HEAVY_DAMAGE",
    "REPEATED_DAMAGE",
    "LOW_HEALTH",
    "DEJECTED",

    "CAN_RANGE_ATTACK1",
    "CAN_RANGE_ATTACK2",

    "CAN_MELEE_ATTACK1",
    "CAN_MELEE_ATTACK2",
    "NEW_ENEMY",

    "TOO_CLOSE_TO_ATTACK",
    "TOO_FAR_TO_ATTACK",
    "NOT_FACING_ATTACK",
    "BLOCKED_BY_FRIEND",

    "TASK_FAILED",
    "TASK_DONE",
    "SCHEDULE_FAILED",
    "SCHEDULE_DONE",

    "BETTER_WEAPON_AVAILABLE",
    "MOBBED_BY_ENEMIES",
    "PLAYER_PUSHING",

    "SMELL_MEAT",
    "SMELL_CARCASS",
    "SMELL_GARBAGE",

    "HEAR_COMBAT",
    "HEAR_WORLD",
    "HEAR_PLAYER",
    "HEAR_PLAYER_FOOTSTEP",
    "HEAR_BULLET_IMPACT",
	"HEAR_BULLET_IMPACT_SNIPER",
    "HEAR_DANGER",
    "HEAR_MOVE_AWAY",
    "HEAR_SPOOKY",
};

//================================================================================
// Información acerca de una tarea, se conforma de la tarea que se debe ejecutar
// y un valor que puede ser un Vector, un flotante, un string, etc.
//================================================================================
struct BotTaskInfo_t
{
    BotTaskInfo_t( int iTask )
    {
        task = iTask;

        vecValue.Invalidate();
        flValue = 0;
        iszValue = NULL_STRING;
        pszValue = NULL;
    }

    BotTaskInfo_t( int iTask, int value )
    {
        task = iTask;
        iValue = value;
        flValue = (float)value;

        vecValue.Invalidate();
        iszValue = NULL_STRING;
        pszValue = NULL;
    }

    BotTaskInfo_t( int iTask, Vector value )
    {
        task = iTask;
        vecValue = value;

        flValue = 0;
        iszValue = NULL_STRING;
        pszValue = NULL;
    }

    BotTaskInfo_t( int iTask, float value )
    {
        task = iTask;
        flValue = value;
        iValue = (int)value;

        vecValue.Invalidate();
        iszValue = NULL_STRING;
        pszValue = NULL;
    }

    BotTaskInfo_t( int iTask, const char *value )
    {
        task = iTask;
        iszValue = MAKE_STRING( value );

        vecValue.Invalidate();
        flValue = 0;
        pszValue = NULL;
    }

    BotTaskInfo_t( int iTask, CBaseEntity *value )
    {
        task = iTask;
        pszValue = value;

        vecValue.Invalidate();
        flValue = 0;
        iszValue = NULL_STRING;
    }

    int task;

    Vector vecValue;
    float flValue;
    int iValue;
    string_t iszValue;
    EHANDLE pszValue;
};

#endif // BOT_CONDITIONS_H