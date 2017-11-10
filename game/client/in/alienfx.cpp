//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#include "cbase.h"
#include "alienfx.h"

#include "hud_macros.h"

#include "c_in_player.h"
#include "weapon_base.h"
#include "usermessages.h"

#define _X86_
#include "wtypes.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

HINSTANCE FXDLL = NULL;

CAlienFX g_AlienFX;
CAlienFX *TheAlienFX = &g_AlienFX;

//================================================================================
// Logging System
// Only for the current file, this should never be in a header.
//================================================================================

#define Msg(...) Log_Msg(LOG_ALIENFX, __VA_ARGS__)
#define Warning(...) Log_Warning(LOG_ALIENFX, __VA_ARGS__)

//================================================================================
// Configuration
//================================================================================

#define ALIENFX_DEFAULT_COLOR LFX_WHITE | LFX_FULL_BRIGHTNESS

//================================================================================
// Commands
//================================================================================

DECLARE_CMD(cl_alienfx_enabled, "1", "Enable compatibility with AlienFX", FCVAR_ARCHIVE)

//================================================================================
//================================================================================
void __MsgFunc_AlienFX(bf_read &msg)
{
    if ( !TheAlienFX || !TheAlienFX->IsEnabled() )
        return;

    TheAlienFX->OnMessage(msg);
}

//================================================================================
// Constructor
//================================================================================
CAlienFX::CAlienFX() : CAutoGameSystemPerFrame("CAlienFX")
{
    m_bStarted = false;
    m_PulseState = 0;

    m_LightTimer.Invalidate();
    m_PulseTimer.Start(1);
}

//================================================================================
// Try to start AlienFX
//================================================================================
bool CAlienFX::Init()
{
    // Disabled
    if ( !cl_alienfx_enabled.GetBool() )
        return true;

    // We load the library, it should be installed on all computers with AlienFX.
    FXDLL = LoadLibrary(_T(LFX_DLL_NAME));

    // This computer does not have AlienFX
    if ( !FXDLL ) {
#ifdef DEBUG
        Warning("This computer does not have AlienFX\n");
#endif
        return true;
    }

    // We link each function of the library
    FXInit = (LFX2INITIALIZE)GetProcAddress(FXDLL, LFX_DLL_INITIALIZE);
    FXReset = (LFX2RESET)GetProcAddress(FXDLL, LFX_DLL_RESET);
    FXLight = (LFX2LIGHT)GetProcAddress(FXDLL, LFX_DLL_LIGHT);
    FXUpdate = (LFX2UPDATE)GetProcAddress(FXDLL, LFX_DLL_UPDATE);
    FXRelease = (LFX2RELEASE)GetProcAddress(FXDLL, LFX_DLL_RELEASE);
    FXLightActionColor = (LFX2ACTIONCOLOR)GetProcAddress(FXDLL, LFX_DLL_ACTIONCOLOR);

    // We ask the library to initialize
    int init = FXInit();

    // Se ha encontrado un dispositivo compatible.
    if ( init == LFX_SUCCESS ) {
        SetColor(LFX_ALL, ALIENFX_DEFAULT_COLOR);
        Msg("Initialized!\n");

        m_bStarted = true;
        return true;
    }
    else if ( init == LFX_ERROR_NODEVS ) {
        Warning("No valid device found\n");
    }
    else if ( init == LFX_ERROR_NOLIGHTS ) {
        Warning("The lights have not been found.\n");
    }
    else {
        Warning("Failure!\n");
    }

    return true;
}

//================================================================================
// Unlocks the library and restores the user's color scheme.
//================================================================================
void CAlienFX::Shutdown()
{
    if ( m_bStarted ) {
        FXRelease();
    }

    FreeLibrary(FXDLL);
}

//================================================================================
// Called in each frame
//================================================================================
void CAlienFX::Update(float frametime)
{
    if ( !IsEnabled() )
        return;

    // There is an active color that we want to last, we let it finish.
    if ( m_LightTimer.HasStarted() ) {
        if ( !m_LightTimer.IsElapsed() )
            return;

        m_LightTimer.Invalidate();
    }

    // Player Lights!
    UpdatePlayerLights();

    // Send the updates
    FXUpdate();
}

//================================================================================
// Update the colors according to what the player wants
//================================================================================
void CAlienFX::UpdatePlayerLights()
{
    C_Player *pPlayer = C_Player::GetLocalInPlayer();

    // There is no player, we are in the menu
    if ( !pPlayer ) {
        SetColor(LFX_ALL, ALIENFX_DEFAULT_COLOR);
        return;
    }

    pPlayer->UpdateAlienFX();
}

//================================================================================
// Utility function to create a fast pulse effect (change between colors)
//================================================================================
void CAlienFX::UpdatePulse(float interval)
{
    if ( !m_PulseTimer.IsElapsed() )
        return;

    switch ( m_PulseState ) {
        case 0:
        default:
            SetColor(LFX_ALL, LFX_RED | LFX_FULL_BRIGHTNESS);
            m_PulseState = 1;
            break;

        case 1:
            SetColor(LFX_ALL, LFX_YELLOW | LFX_FULL_BRIGHTNESS);
            m_PulseState = 2;
            break;

        case 2:
            SetColor(LFX_ALL, LFX_ORANGE | LFX_FULL_BRIGHTNESS);
            m_PulseState = 3;
            break;

        case 3:
            SetColor(LFX_ALL, LFX_RED | LFX_FULL_BRIGHTNESS);
            m_PulseState = 4;
            break;

        case 4:
            SetColor(LFX_ALL, LFX_BLACK | LFX_FULL_BRIGHTNESS);
            m_PulseState = 0;
            break;
    }

    m_PulseTimer.Start(interval);
}

//================================================================================
// Returns if AlienFX is activated.
//================================================================================
bool CAlienFX::IsEnabled()
{
    if ( !m_bStarted )
        return false;

    return cl_alienfx_enabled.GetBool();
}

//================================================================================
// We received a message from the server
//================================================================================
void CAlienFX::OnMessage(bf_read &msg)
{
    if ( !IsEnabled() )
        return;

    int command = msg.ReadShort();

    switch ( command ) {
        // Set a new color!
        case ALIENFX_SETCOLOR:
        {
            int light = msg.ReadLong();
            int color = msg.ReadLong();
            float duration = msg.ReadFloat();

            TheAlienFX->SetActiveColor(light, color, duration);
            Msg("The server tells us to change the color.\n");
            break;
        }
    }
}

//================================================================================
// Sets a color, but does not activate it until the frame ends.
//================================================================================
void CAlienFX::SetColor(int device, int color, float duration)
{
    if ( !IsEnabled() )
        return;

    FXLight(device, color);

    if ( duration > 0 ) {
        m_LightTimer.Start(duration);
    }
}

//================================================================================
// Set a color and activate it instantly.
// It should only be used outside the "Update()" cycle and carefully.
//================================================================================
void CAlienFX::SetActiveColor(int device, int color, float duration)
{
    if ( !IsEnabled() )
        return;

    SetColor(device, color, duration);
    FXUpdate();
}

//================================================================================
// ???
//================================================================================
void CAlienFX::SetActionColor(int device, int color, int action)
{
    if ( !IsEnabled() )
        return;

    FXLightActionColor(device, color, action);
}