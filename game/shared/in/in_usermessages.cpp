//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "usermessages.h"
#include "shake.h"
#include "voice_gamemgr.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
// Logging System
//================================================================================

DEFINE_LOGGING_CHANNEL_NO_TAGS(LOG_ALIENFX, "AlienFX", 0, LS_MESSAGE, {100, 255, 0, 255}); // Green
DEFINE_LOGGING_CHANNEL_NO_TAGS(LOG_PLAYER, "Player", 0, LS_MESSAGE, {169, 169, 245, 255}); // Purple
DEFINE_LOGGING_CHANNEL_NO_TAGS(LOG_BOTS, "Bots", 0, LS_MESSAGE, {245, 169, 242, 255}); // Pink
DEFINE_LOGGING_CHANNEL_NO_TAGS(LOG_DIRECTOR, "Director", 0, LS_MESSAGE, {46, 204, 250, 255}); // Light Blue
DEFINE_LOGGING_CHANNEL_NO_TAGS(LOG_UI, "UI", 0, LS_MESSAGE, {255, 0, 128, 255}); // wow, much pink

//================================================================================
// Custom User Messages through the Network
//================================================================================

void RegisterUserMessages(void)
{
    LoggingChannelFlags_t;
    usermessages->Register("Geiger", 1);
    usermessages->Register("Train", 1);
    usermessages->Register("HudText", -1);
    usermessages->Register("SayText", -1);
    usermessages->Register("SayText2", -1);
    usermessages->Register("TextMsg", -1);
    usermessages->Register("HudMsg", -1);
    usermessages->Register("ResetHUD", 1);        // called every respawn
    usermessages->Register("GameTitle", 0);
    usermessages->Register("ItemPickup", -1);
    usermessages->Register("ShowMenu", -1);
    usermessages->Register("Shake", 13);
    usermessages->Register("ShakeDir", -1); // directional shake
    usermessages->Register("Tilt", 22);
    usermessages->Register("Fade", 10);
    usermessages->Register("AlienFX", -1);
    usermessages->Register("VGUIMenu", -1);    // Show VGUI menu
    usermessages->Register("Rumble", 3);    // Send a rumble to a controller
    usermessages->Register("Battery", 2);
    usermessages->Register("Damage", -1);
    usermessages->Register("VoiceMask", VOICE_MAX_PLAYERS_DW * 4 * 2 + 1);
    usermessages->Register("RequestState", 0);
    usermessages->Register("CloseCaption", -1); // Show a caption (by string id number)(duration in 10th of a second)
    usermessages->Register("CloseCaptionDirect", -1); // Show a forced caption (by string id number)(duration in 10th of a second)
    usermessages->Register("HintText", -1);    // Displays hint text display
    usermessages->Register("KeyHintText", -1);    // Displays hint text display
    usermessages->Register("SquadMemberDied", 0);
    usermessages->Register("AmmoDenied", 2);
    usermessages->Register("CreditsMsg", 1);
    usermessages->Register("LogoTimeMsg", 4);
    usermessages->Register("AchievementEvent", -1);
    usermessages->Register("UpdateJalopyRadar", -1);
    usermessages->Register("CurrentTimescale", 4);    // Send one float for the new timescale
    usermessages->Register("DesiredTimescale", 13);    // Send timescale and some blending vars
    usermessages->Register("ShowHitmarker", 1);
    usermessages->Register("EmitSound", -1);
}
