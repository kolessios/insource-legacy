//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef DIRECTOR_DEFS_H
#define DIRECTOR_DEFS_H

#ifdef _WIN32
#pragma once
#endif

//================================================================================
// Configuración
//================================================================================

#define DIRECTOR_TOLERANCE_ENTITIES 100

//================================================================================
// Tipos de hijo
//================================================================================
enum MinionType
{
    CHILD_TYPE_COMMON = 0,
    CHILD_TYPE_AMBIENT,
    CHILD_TYPE_BOSS,
    LAST_CHILD_TYPE
};

static const char *g_MinionTypes[LAST_CHILD_TYPE]
{
    "common",
    "ambient",
    "boss"
};

//================================================================================

//================================================================================
struct Minions_t
{
    int alive;
    int tooClose;
    int created;
    int lastSpawn;
};

class CMinionInfo
{
public:
    char unit[100];
    int spawnChance;
    int spawnInterval;
    MinionType type;
    char population[100];
    bool isTemplate;
    int maxUnits;

    int alive;
    int created;
    int lastSpawn;
    CountdownTimer nextSpawn;
};

typedef CUtlVector<CMinionInfo *> PopulationList;

//================================================================================
// Estados del Director
//================================================================================
enum DirectorStatus
{
    STATUS_INVALID = 0,
    STATUS_NORMAL,
    STATUS_PANIC,
    STATUS_FINALE,
    STATUS_BOSS,
    STATUS_GAMEOVER,

    LAST_DIRECTOR_STATUS
};

static const char *g_DirectorStatus[LAST_DIRECTOR_STATUS] =
{
    "INVALID",
    "NORMAL",
    "PANIC",
    "FINALE",
    "BOSS",
    "GAMEOVER"
};

//================================================================================
// Fases del Director
//================================================================================
enum DirectorPhase
{
    PHASE_INVALID = 0,
    PHASE_RELAX,
    PHASE_BUILD_UP,
    PHASE_SUSTAIN,
    PHASE_FADE,
    PHASE_EVENT,

    LAST_DIRECTOR_PHASE
};

static const char *g_DirectorPhase[LAST_DIRECTOR_PHASE] =
{
    "INVALID",
    "RELAX",
    "BUILD_UP",
    "SUSTAIN",
    "FADE",
    "EVENT"
};

//================================================================================
// Enojo del Director
//================================================================================
enum DirectorAngry
{
    ANGRY_INVALID = 0,
    ANGRY_LOW,			// Actuemos un rato :) - EASY
    ANGRY_MEDIUM,		// Un poco más serio   - MEDIUM
    ANGRY_HIGH,			// Deben morir...      - HIGH
    ANGRY_CRAZY,		// jajajAjJJAJAJA      - VERY_HIGH

    LAST_DIRECTOR_ANGRY
};

static const char *g_DirectorAngry[LAST_DIRECTOR_ANGRY] =
{
    "INVALID",
    "LOW",
    "MEDIUM",
    "HIGH",
    "CRAZY",
};

//================================================================================
//================================================================================

#define FOR_EACH_MINION_TYPE(t) for ( int t = 0; t < LAST_CHILD_TYPE; ++t )



#endif // DIRECTOR_DEFS_H