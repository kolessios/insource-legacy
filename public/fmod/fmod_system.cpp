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
#include "hud_macros.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CFMODSoundSystem g_FMODSoundSystem;
CFMODSoundSystem *TheFMODSoundSystem = &g_FMODSoundSystem;

#ifdef CLIENT_DLL
//================================================================================
//================================================================================
void __MsgFunc_EmitSound(bf_read &msg)
{
    if ( !TheFMODSoundSystem )
        return;

    TheFMODSoundSystem->ReceiveMessage(msg);
}
#endif

//================================================================================
//================================================================================
CFMODSoundSystem::CFMODSoundSystem()
{
}

//================================================================================
//================================================================================
CFMODSoundSystem::~CFMODSoundSystem()
{
}

#ifdef CLIENT_DLL
//================================================================================
//================================================================================
bool CFMODSoundSystem::Init()
{
    FMOD_RESULT result;
    unsigned int version;

    // Creating FMOD system
    result = FMOD::System_Create(&m_pSystem);

    // Failure
    if ( result != FMOD_OK ) {
        Warning("[FMOD] System creation failure: %s\n", FMOD_ErrorString(result));
        return false;
    }

    // We get the version
    result = m_pSystem->getVersion(&version);

    // Failure
    if ( result != FMOD_OK ) {
        Warning("[FMOD] Get Version failure: %s\n", FMOD_ErrorString(result));
        return false;
    }

    if ( version < FMOD_VERSION ) {
        Warning("[FMOD] lib version %08x doesn't match header version %08x", version, FMOD_VERSION);
        return false;
    }

    // Init FMOD
    result = m_pSystem->init(32, FMOD_INIT_NORMAL, 0);

    // Failure
    if ( result != FMOD_OK ) {
        Warning("[FMOD] Init failure: %s\n", FMOD_ErrorString(result));
        return false;
    }

    // 3D Settings
    m_pSystem->set3DSettings(1.0f, METERS_TO_VALVEUNITS(1.0f), baseRolloffFactor);
    m_pSystem->setGeometrySettings(MAX_COORD_INTEGER);

    Msg("[FMOD] Init finished\n");
    HOOK_MESSAGE(EmitSound);

    return true;
}

//================================================================================
//================================================================================
void CFMODSoundSystem::Shutdown()
{
    m_pSystem->close();
    m_pSystem->release();

    delete m_pSystem;
    m_pSystem = NULL;
}

//================================================================================
//================================================================================
void CFMODSoundSystem::Update(float frametime)
{
    FMOD_RESULT result;

    // We update the position of the ears
    UpdateListener();

    // We update FMOD
    result = m_pSystem->update();
    Assert(result == FMOD_OK);

    // Failure
    if ( result != FMOD_OK ) {
        Warning("[FMOD] Update failure: %s\n", FMOD_ErrorString(result));
        return;
    }
}

//================================================================================
//================================================================================
void CFMODSoundSystem::UpdateListener()
{
    CBasePlayer *pPlayer = CBasePlayer::GetLocalPlayer();
    
    if ( !pPlayer ) {
        
        return;
    }

    // Each player of split screen counts.
    int listenerCount = pPlayer->GetSplitScreenPlayers().Count() + 1;
    m_pSystem->set3DNumListeners(listenerCount);

    FOR_EACH_VALID_SPLITSCREEN_PLAYER(it)
    {
        int nIndex = engine->GetSplitScreenPlayer(it);
        pPlayer = ToBasePlayer(ClientEntityList().GetBaseEntity(nIndex));

        if ( !pPlayer )
            continue;

        Vector playerEar, playerVelocity, playerForward, playerUp;

        playerEar = pPlayer->EarPosition();
        playerVelocity = pPlayer->GetAbsVelocity();
        //AngleVectors(pPlayer->GetLocalAngles(), &playerForward, NULL, &playerUp);
        pPlayer->EyeVectors(&playerForward, NULL, &playerUp);

        // Player Position
        FMOD_VECTOR position = {playerEar.x, playerEar.y, playerEar.z};

        // Player Velocity
        FMOD_VECTOR velocity = {playerVelocity.x, playerVelocity.y, playerVelocity.z};

        // Player Vectors
        FMOD_VECTOR forward = {playerForward.x, playerForward.y, playerForward.z};
        FMOD_VECTOR up = {playerUp.x, playerUp.y, playerUp.z};

        m_pSystem->set3DListenerAttributes(it, &position, &velocity, &forward, &up);
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
    params.soundlevel = ep.m_SoundLevel =(soundlevel_t)msg.ReadShort();

    // Flags
    ep.m_nFlags = msg.ReadLong();

    // Origin
    Vector vecOrigin;
    msg.ReadBitVec3Coord(vecOrigin);

    if ( vecOrigin != vec3_invalid ) {
        ep.m_pOrigin = &vecOrigin;
    }

    CLocalPlayerFilter filter;
    EmitSound(filter, entindex, params, ep);
}

//================================================================================
//================================================================================
void CFMODSoundSystem::LevelInitPreEntity()
{
}

//================================================================================
//================================================================================
void CFMODSoundSystem::ErrorCheck(FMOD_RESULT result)
{
    /*
    switch ( result ) {

    }
    */

    if ( result != FMOD_OK ) {
        Warning("[FMOD] %s\n", FMOD_ErrorString(result));
    }
}
#endif

//================================================================================
//================================================================================
const char * CFMODSoundSystem::GetFullSoundPath(const char *relativePath)
{
    relativePath = PSkipSoundChars(relativePath);
    relativePath = UTIL_VarArgs("sound/%s", relativePath);

    char fullpath[512];
    g_pFullFileSystem->RelativePathToFullPath(relativePath, "GAME", fullpath, sizeof(fullpath));

    return fullpath;
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
    result = System()->createStream(pSoundPath, FMOD_3D, 0, &pSound);
    Assert(result == FMOD_OK);
    
    // Failed
    if ( result != FMOD_OK ) {
        Warning("[CFMODSoundSystem] There was a problem creating Streaming of a sound '%s': %s\n", params.soundname, FMOD_ErrorString(result));
        return;
    }

    // 
    float maxAudible = (2 * SOUND_NORMAL_CLIP_DIST) / SNDLVL_TO_ATTN(params.soundlevel);
    pSound->set3DMinMaxDistance(0.5f * valveUnitsPerMeter, maxAudible * valveUnitsPerMeter);

    // Channel creation and play
    result = System()->playSound(pSound, 0, true, &channel);
    Assert(result == FMOD_OK);
    
    // Failed
    if ( result != FMOD_OK ) {
        Warning("[CFMODSoundSystem] ThereThere was a problem playing the sound '%s': %s\n", params.soundname, FMOD_ErrorString(result));
        return;
    }

    Vector soundPosition, soundVelocity;

    if ( ep.m_pOrigin ) {
        soundPosition = *ep.m_pOrigin;
    }
    else if ( pEntity ) {
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

    if ( ep.m_pOrigin ) {
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
