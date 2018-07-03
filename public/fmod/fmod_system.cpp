//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#include "cbase.h"
#include "fmod_system.h"

#include "filesystem.h"
#include "tier2/fileutils.h"
#include "soundchars.h"

#include "inc/fmod_errors.h"

#ifdef CLIENT_DLL
#include "c_in_player.h"
#include "hud_macros.h"
#include "physpropclientside.h"
#include "mapentities_shared.h"
#include "c_world.h"
#include "solidsetdefaults.h"
#include "bsptreedata.h"
#include "c_func_brush.h"
#include "cliententitylist.h"

#ifdef USE_PHONON
#include "steamaudio_system.h"
#endif
#else
#include "in_player.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CFMODSoundSystem g_FMODSoundSystem;
CFMODSoundSystem *TheFMODSoundSystem = &g_FMODSoundSystem;

//================================================================================
// Logging System
// Only for the current file, this should never be in a header.
//================================================================================

#define Msg(...) Log_Msg(LOG_FMOD, __VA_ARGS__)
#define Warning(...) Log_Warning(LOG_FMOD, __VA_ARGS__)

//================================================================================
// Commands
//================================================================================

static ConVar snd_fmod_listener_disable("snd_fmod_listener_disable", "0", FCVAR_CHEAT, "");

#ifdef CLIENT_DLL
//================================================================================
//================================================================================
void __MsgFunc_EmitSound(bf_read &msg)
{
    if( !TheFMODSoundSystem )
        return;

    TheFMODSoundSystem->ReceiveMessage(msg);
}
#endif

//================================================================================
//================================================================================
CFMODSoundSystem::CFMODSoundSystem()
{
#ifdef CLIENT_DLL
    m_bScenePrepared = false;
#endif
}

//================================================================================
//================================================================================
CFMODSoundSystem::~CFMODSoundSystem()
{
}

#ifdef CLIENT_DLL
//================================================================================
//================================================================================
void CFMODSoundSystem::FAILCHECK(FMOD_RESULT result)
{
    Assert(result == FMOD_OK);

    if( result == FMOD_OK ) {
        return;
    }

    Error(FMOD_ErrorString(result));
}

//================================================================================
//================================================================================
void CFMODSoundSystem::WARNCHECK(FMOD_RESULT result)
{
    Assert(result == FMOD_OK);

    if( result == FMOD_OK ) {
        return;
    }

    Warning(UTIL_VarArgs("%s\n", FMOD_ErrorString(result)));
}

//================================================================================
//================================================================================
bool CFMODSoundSystem::Init()
{
    unsigned int version;

    // Create FMOD Studio System
    FAILCHECK(FMOD::Studio::System::create(&m_pSystem));

    // Obtain Low Level System
    FAILCHECK(m_pSystem->getLowLevelSystem(&m_pLowLevelSystem));

    // Obtain FMOD Version
    FAILCHECK(m_pLowLevelSystem->getVersion(&version));

    if( version < FMOD_VERSION ) {
        Error("Lib version %08x doesn't match header version %08x", version, FMOD_VERSION);
        return false;
    }

    // Set Plugins Path
    const char *pPluginPath = GetFullPath("bin");
    FAILCHECK(m_pLowLevelSystem->setPluginPath(pPluginPath));

#ifdef USE_PHONON
    // Load Steam Audio Plugin
    unsigned int handle;
    FAILCHECK(m_pLowLevelSystem->loadPlugin("phonon_fmod.dll", &handle));
    Msg("Steam Audio Plugin Loaded\n");
#endif

    // Init FMOD
#ifdef DEBUG
    FAILCHECK(m_pSystem->initialize(1024, FMOD_STUDIO_INIT_NORMAL | FMOD_STUDIO_INIT_LIVEUPDATE, FMOD_INIT_NORMAL, 0));
#else
    FAILCHECK(m_pSystem->initialize(1024, FMOD_STUDIO_INIT_NORMAL, FMOD_INIT_NORMAL, 0));
#endif

    Setup();
    Msg("Initialized\n");
    
    return true;
}

void CFMODSoundSystem::Setup()
{
    // 3D Settings
    LowLevelSystem()->set3DSettings(metersPerValveUnit / 340, valveUnitsPerMeter, baseRolloffFactor);
    LowLevelSystem()->setGeometrySettings(MAX_COORD_FLOAT);

    //
    LowLevelSystem()->setSoftwareFormat(0, FMOD_SPEAKERMODE_STEREO, 0);

    //
    LoadBanks();
}

//================================================================================
//================================================================================
void CFMODSoundSystem::Shutdown()
{
    System()->release();
    delete m_pSystem;
    m_pSystem = NULL;
}

//================================================================================
//================================================================================
void CFMODSoundSystem::LoadBanks()
{
    System()->unloadAll();

    KeyValues *pRoot = new KeyValues("game_sound_banks_manifest");
    KeyValues::AutoDelete autoDelete(pRoot);

    pRoot->LoadFromFile(filesystem, "scripts/game_sound_banks_manifest.txt", NULL);

    FMOD::Studio::Bank *bank = NULL;

    FOR_EACH_VALUE(pRoot, pValue)
    {
        const char *pRelativePath = pValue->GetString();
        FAILCHECK(System()->loadBankFile(GetFullPath(pRelativePath), FMOD_STUDIO_LOAD_BANK_NORMAL, &bank));

        Msg("%s - Bank loaded!\n", pRelativePath);
    }
}

//================================================================================
//================================================================================
void CFMODSoundSystem::Update(float frametime)
{
    // Update listener position
    UpdateListener();

    // Update FMOD
    WARNCHECK(System()->update());
}

//================================================================================
//================================================================================
void CFMODSoundSystem::UpdateListener()
{
    CPlayer *pPlayer = ToInPlayer(CBasePlayer::GetLocalPlayer());

    if( !pPlayer ) {
        return;
    }

    if( physenv && !m_bScenePrepared ) {
        SetupScene();
    }

    if( snd_fmod_listener_disable.GetBool() ) {
        return;
    }

    // Each player of split screen counts.
    int listenerCount = pPlayer->GetSplitScreenPlayers().Count() + 1;
    System()->setNumListeners(listenerCount);

    FOR_EACH_VALID_SPLITSCREEN_PLAYER(it)
    {
        int nIndex = engine->GetSplitScreenPlayer(it);
        pPlayer = ToInPlayer(ClientEntityList().GetBaseEntity(nIndex));

        if( !pPlayer )
            continue;

        Vector playerEar, playerVelocity, playerForward, playerUp;

        playerEar = pPlayer->EarPosition();
        playerVelocity = pPlayer->GetLocalVelocity();
        //AngleVectors(pPlayer->GetLocalAngles(), &playerForward, NULL, &playerUp);
        pPlayer->EyeVectors(&playerForward, NULL, &playerUp);

        FMOD_3D_ATTRIBUTES attributes = {{0}};

        attributes.position = {playerEar.x, playerEar.y, playerEar.z};
        attributes.velocity = {playerVelocity.x, playerVelocity.y, playerVelocity.z};
        attributes.forward = {playerForward.x, playerForward.y, playerForward.z};
        attributes.up = {-playerUp.x, -playerUp.y, -playerUp.z};

        //NDebugOverlay::VertArrow(playerEar, playerEar + playerUp * 10.0f, 10.0f, 255, 0, 0, 130.0f, true, 0.1f);
        NDebugOverlay::Box(Vector(190, 222, 124), Vector(-5, -5, -5), Vector(5, 5, 5), 255, 255, 255, 255, 0.1f);

        System()->setListenerAttributes(it, &attributes);
        System()->setListenerWeight(it, 1.0f);
    }
}

//================================================================================
//================================================================================
void CFMODSoundSystem::ReceiveMessage(bf_read & msg)
{
    CSoundParameters params;
    EmitSound_t ep;

    int entindex = msg.ReadShort();

    // Channel
    params.channel = ep.m_nChannel = msg.ReadShort();

    // Sound file
    msg.ReadString(params.soundname, sizeof(params.soundname));
    ep.m_pSoundName = params.soundname;

    // Volume, pich, level
    params.volume = ep.m_flVolume = msg.ReadFloat();
    params.pitch = ep.m_nPitch = msg.ReadFloat();
    params.soundlevel = ep.m_SoundLevel = (soundlevel_t)msg.ReadShort();

    // Flags
    ep.m_nFlags = msg.ReadLong();

    // Origin
    Vector vecOrigin;
    msg.ReadBitVec3Coord(vecOrigin);

    if( vecOrigin != vec3_invalid ) {
        ep.m_pOrigin = &vecOrigin;
    }

    CLocalPlayerFilter filter;
    EmitSound(filter, entindex, params, ep);
}

//================================================================================
//================================================================================
void CFMODSoundSystem::LevelInitPreEntity()
{
    HOOK_MESSAGE(EmitSound);
}

//================================================================================
//================================================================================
void CFMODSoundSystem::LevelInitPostEntity()
{
    //ParseAllEntities(engine->GetMapEntitiesString());

    FMOD::Studio::EventDescription* eventDescription = NULL;
    WARNCHECK(System()->getEvent("event:/Test3D", &eventDescription));

    FMOD::Studio::EventInstance* eventInstance = NULL;
    WARNCHECK(eventDescription->createInstance(&eventInstance));

    //eventInstance->setParameterValue("RPM", 650.0f);
    eventInstance->start();

    FMOD_3D_ATTRIBUTES attributes = {{0}};
    attributes.position = {190,222,124};

    eventInstance->set3DAttributes(&attributes);
    Msg("Bank Instance! %s \n", GetFullSoundPath("desktop/Master Bank.bank"));
}

//================================================================================
//================================================================================
void CFMODSoundSystem::LevelShutdownPreEntity()
{
    System()->setNumListeners(0);
}

//================================================================================
//================================================================================
void CFMODSoundSystem::LevelShutdownPostEntity()
{
    m_bScenePrepared = false;
}

void CFMODSoundSystem::ParseAllEntities(const char * pMapData)
{
    int nEntities = 0;

    char szTokenBuffer[MAPKEY_MAXLENGTH];

    //
    //  Loop through all entities in the map data, creating each.
    //
    for( ; true; pMapData = MapEntity_SkipToNextEntity(pMapData, szTokenBuffer) ) {
        //
        // Parse the opening brace.
        //
        char token[MAPKEY_MAXLENGTH];
        pMapData = MapEntity_ParseToken(pMapData, token);

        //
        // Check to see if we've finished or not.
        //
        if( !pMapData )
            break;

        if( token[0] != '{' ) {
            Error("MapEntity_ParseAllEntities: found %s when expecting {", token);
            continue;
        }

        //
        // Parse the entity and add it to the spawn list.
        //

        pMapData = ParseEntity(pMapData);

        nEntities++;
    }
}

const char * CFMODSoundSystem::ParseEntity(const char * pEntData)
{
    CEntityMapData entData((char*)pEntData);
    char className[MAPKEY_MAXLENGTH];

    MDLCACHE_CRITICAL_SECTION();

    if( !entData.ExtractValue("classname", className) ) {
        Error("classname missing from entity!\n");
    }

    /*if( !Q_strcmp(className, "prop_physics_multiplayer") ) {
        // always force clientside entitis placed in maps
        C_PhysPropClientside *pEntity = C_PhysPropClientside::CreateNew(true);

        if( pEntity ) {	// Set up keyvalues.
            pEntity->ParseMapData(&entData);

            if( !pEntity->Initialize() )
                pEntity->Release();

            return entData.CurrentBufferPosition();
        }
    }

    if( !Q_strcmp(className, "func_proprrespawnzone") ) {
        DebuggerBreakIfDebugging();
    }
    */

    // Just skip past all the keys.
    char keyName[MAPKEY_MAXLENGTH];
    char value[MAPKEY_MAXLENGTH];
    if( entData.GetFirstKey(keyName, value) ) {
        do {
        }
        while( entData.GetNextKey(keyName, value) );
    }

    //
    // Return the current parser position in the data block
    //
    return entData.CurrentBufferPosition();
}

//================================================================================
//================================================================================
void CFMODSoundSystem::SetupScene()
{
    //
    CreateGeometry();

    //
    m_bScenePrepared = true;
}

//================================================================================
//================================================================================
void CFMODSoundSystem::CreateGeometry()
{
    for( C_BaseEntity *pEntity = ClientEntityList().FirstBaseEntity(); pEntity; pEntity = ClientEntityList().NextBaseEntity(pEntity) ) {
        const char *pClassname = pEntity->GetClassname();

        if( Q_strcmp(pClassname, "class C_FuncBrush") != 0 ) {
            continue;
        }

        if( !pEntity->VPhysicsGetObject() ) {
            Warning("func_brush without physics!\n");
            continue;
        }

        //const CPhysCollide *pCollide = pEntity->VPhysicsGetObject()->GetCollide();
        vcollide_t *pCollide = modelinfo->GetVCollide(pEntity->GetModelIndex());

        if( !pCollide || pCollide->solidCount < 1 ) {
            Warning("func_brush without collide!\n");
            continue;
        }

        Vector *verts = NULL;
        int vertCount = physcollision->CreateDebugMesh(pCollide->solids[0], &verts);

        FMOD::Geometry *pGeometry;
        FAILCHECK(LowLevelSystem()->createGeometry(MAX_MAP_BRUSHES, MAX_MAP_VERTS, &pGeometry));

        FMOD_VECTOR vertices = {verts->x, verts->y, verts->z};
        int index;
        WARNCHECK(pGeometry->addPolygon(0.8f, 0.8f, false, vertCount, &vertices, &index));

        Vector vecPosition = pEntity->GetLocalOrigin();
        FMOD_VECTOR position = {vecPosition.x, vecPosition.y, vecPosition.z};
        WARNCHECK(pGeometry->setPosition(&position));

        Vector vecForward, vecUp;
        FMOD_VECTOR forward, up;
        AngleVectors(pEntity->GetLocalAngles(), &vecForward, NULL, &vecUp);

        forward = {vecForward.x, vecForward.y, vecForward.z};
        up = {vecUp.x, vecUp.y, vecUp.z};
        WARNCHECK(pGeometry->setRotation(&forward, &up));

        Msg("Geometry created!\n");

        /*static color32 debugColor = {0,255,255,0};
        matrix3x4_t matrix;
        pEntity->VPhysicsGetObject()->GetPositionMatrix(&matrix);
        engine->DebugDrawPhysCollide(pCollide->solids[0], NULL, matrix, debugColor);*/
    }
}
#endif

//================================================================================
//================================================================================
const char *CFMODSoundSystem::GetFullPath(const char * relativePath)
{
    char fullpath[512];
    g_pFullFileSystem->RelativePathToFullPath(relativePath, "GAME", fullpath, sizeof(fullpath));

    return fullpath;
}

//================================================================================
//================================================================================
const char *CFMODSoundSystem::GetFullSoundPath(const char *relativePath)
{
    relativePath = PSkipSoundChars(relativePath);
    relativePath = UTIL_VarArgs("sound/%s", relativePath);

    return GetFullPath(relativePath);
}

//================================================================================
//================================================================================
void CFMODSoundSystem::EmitSound(IRecipientFilter& filter, int entindex, CSoundParameters params, const EmitSound_t & ep)
{
#ifdef CLIENT_DLL
    // Full path
    const char *pSoundPath = GetFullSoundPath(params.soundname);

    FMOD_RESULT result;

    CBaseEntity *pEntity = CBaseEntity::Instance(entindex);
    FMOD::Channel *channel = NULL;
    FMOD::Sound *pSound = NULL;

    // Sound creation
    result = LowLevelSystem()->createStream(pSoundPath, FMOD_3D, 0, &pSound);
    Assert(result == FMOD_OK);

    // Failed
    if( result != FMOD_OK ) {
        Warning("There was a problem creating Streaming of a sound '%s': %s\n", params.soundname, FMOD_ErrorString(result));
        return;
    }

    // 
    float maxAudible = (2 * SOUND_NORMAL_CLIP_DIST) / SNDLVL_TO_ATTN(params.soundlevel);
    pSound->set3DMinMaxDistance(0.5f * valveUnitsPerMeter, maxAudible * valveUnitsPerMeter);

    // Channel creation and play
    result = LowLevelSystem()->playSound(pSound, 0, true, &channel);
    Assert(result == FMOD_OK);

    // Failed
    if( result != FMOD_OK ) {
        Warning("There was a problem playing the sound '%s': %s\n", params.soundname, FMOD_ErrorString(result));
        return;
    }

    Vector soundPosition, soundVelocity;

    if( ep.m_pOrigin ) {
        soundPosition = *ep.m_pOrigin;
    }
    else if( pEntity ) {
        soundPosition = pEntity->GetLocalOrigin();
    }

    FMOD_VECTOR position = {soundPosition.x, soundPosition.y, soundPosition.z};
    FMOD_VECTOR velocity = {0.0f, 0.0f, 0.0f};

    channel->set3DAttributes(&position, &velocity);
    channel->setVolume(params.volume);
    channel->setPitch(params.pitch / 100.0f);
    channel->setPaused(false);
#else
    Vector origin = vec3_invalid;

    if( ep.m_pOrigin ) {
        origin = *ep.m_pOrigin;
    }

    UserMessageBegin(filter, "EmitSound");
    WRITE_SHORT(entindex);
    WRITE_SHORT(params.channel);
    WRITE_STRING(params.soundname);
    WRITE_FLOAT(params.volume);
    WRITE_FLOAT(params.pitch);
    WRITE_SHORT(params.soundlevel);
    WRITE_LONG(ep.m_nFlags);
    WRITE_VEC3COORD(origin);
    MessageEnd();
#endif
}
