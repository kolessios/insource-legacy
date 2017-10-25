//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "alienfx.h"

#include "hud_macros.h"

#include "c_in_player.h"
#include "weapon_base.h"

#include "in_shareddefs.h"
#include "usermessages.h"

#define _X86_
#include "wtypes.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

HINSTANCE FXDLL = NULL;

CAlienFX g_AlienFX;
CAlienFX *TheAlienFX = &g_AlienFX;

#define DEFAULT_COLOR LFX_WHITE | LFX_FULL_BRIGHTNESS

//================================================================================
// Comandos
//================================================================================

DECLARE_COMMAND( cl_alienfx_enabled, "1", "Activa la compatibilidad con AlienFX", FCVAR_ARCHIVE )

//================================================================================
//================================================================================
void __MsgFunc_AlienFX( bf_read &msg )
{
    // No hay AlienFX en el equipo
    if ( !TheAlienFX || !TheAlienFX->IsEnabled() )
        return;

    TheAlienFX->OnMessage( msg );
}

//================================================================================
// Constructor
//================================================================================
CAlienFX::CAlienFX() : CAutoGameSystemPerFrame( "CAlienFX" )
{
    m_bStarted = false;
    m_PulseState = 0;

    m_LightTimer.Invalidate();
    m_PulseTimer.Start(1);
}

//================================================================================
// Carga el archivo LightFX.dll del equipo
//================================================================================
bool CAlienFX::Init()
{
#ifdef DEBUG
	//if ( !cl_alienfx_enabled.GetBool() )
		return true;
#endif

    FXDLL = LoadLibrary(_T( LFX_DLL_NAME ));

    // No se ha podido cargar la librería
    if ( !FXDLL )
        return true;

    // Enlazamos cada función del .dll
    FXInit				= (LFX2INITIALIZE)GetProcAddress( FXDLL, LFX_DLL_INITIALIZE );
    FXReset				= (LFX2RESET)GetProcAddress( FXDLL, LFX_DLL_RESET );
    FXLight				= (LFX2LIGHT)GetProcAddress( FXDLL, LFX_DLL_LIGHT );
    FXUpdate			= (LFX2UPDATE)GetProcAddress( FXDLL, LFX_DLL_UPDATE );
    FXRelease			= (LFX2RELEASE)GetProcAddress( FXDLL, LFX_DLL_RELEASE );
    FXLightActionColor  = (LFX2ACTIONCOLOR)GetProcAddress( FXDLL, LFX_DLL_ACTIONCOLOR );

	// Tratamos de iniciar el sistema
    int init = FXInit();

    // Se ha encontrado un dispositivo compatible.
    if ( init == LFX_SUCCESS )
    {
        SetColor( LFX_ALL, DEFAULT_COLOR );
        DevMsg("Alien FX Initialized!\n");

        // Escuchamos por los eventos desde servidor
        HOOK_MESSAGE( AlienFX );
		
		// Hemos iniciado :)
        m_bStarted = true;
        return true;
    }
    else if ( init == LFX_ERROR_NODEVS )
        DevWarning("Alien FX No Devices Detected\n");
    else if ( init == LFX_ERROR_NOLIGHTS )
        DevWarning("Alien FX No Lights Available\n");
    else
        DevWarning("Alien FX Generic Failure\n");

    return true;
}

//================================================================================
// Libera el archivo LightFX.dll y restaura la combinación de luces del usuario
//================================================================================
void CAlienFX::Shutdown()
{
    if ( m_bStarted )
        FXRelease();

    FreeLibrary( FXDLL );
}

//================================================================================
// Es llamado cada frame
//================================================================================
void CAlienFX::Update( float frametime )
{
    // No podemos hacerlo
    if ( !IsEnabled() )
        return;

    // Hay un color ocupando el lugar ahora mismo
    if ( m_LightTimer.HasStarted() )
    {
        // Dejemos que termine....
        if ( !m_LightTimer.IsElapsed() )
            return;

        m_LightTimer.Invalidate();
    }

    // Actualizamos las luces dependiendo
    // del estado actual del Jugador
    UpdateStatus();

    // Actualizamos
    FXUpdate();
}

//================================================================================
// Actualizamos las luces dependiendo del entorno
//================================================================================
void CAlienFX::UpdateStatus()
{
	// Jugador local
    C_Player *pPlayer = C_Player::GetLocalInPlayer();

    // No hay Jugador!
    if ( !pPlayer )
    {
        SetColor( LFX_ALL, DEFAULT_COLOR );
        return;
    }
	
	// Sin vida, apagamos todas las luces
    if ( !pPlayer->IsAlive() )
    {
        SetColor( LFX_ALL, LFX_BLACK | LFX_FULL_BRIGHTNESS );
        return;
    }

	#ifdef APOCALYPSE
    //
    // Salud
    //
    int health = pPlayer->GetHealth();

    if ( pPlayer->GetPlayerStatus() == PLAYER_STATUS_FALLING )
    {
        UpdatePulse( 0.05f );
        return;
    }
    if ( pPlayer->IsDejected() )
    {
        UpdatePulse( 0.5f );
        return;
    }
    else if ( health < 5 )
    {
        UpdatePulse( 0.7f );
        return;
    }
    else if ( health < 20 )
        SetColor( LFX_ALL_FRONT, LFX_RED | LFX_FULL_BRIGHTNESS );
    else if ( health < 50 )
        SetColor( LFX_ALL_FRONT, LFX_ORANGE | LFX_FULL_BRIGHTNESS );
    else if ( health < 80 )
        SetColor( LFX_ALL_FRONT, LFX_YELLOW | LFX_FULL_BRIGHTNESS );
    else
        SetColor( LFX_ALL_FRONT, LFX_GREEN | LFX_FULL_BRIGHTNESS );

    //
    // Munición
    //
    CBaseWeapon *pWeapon = pPlayer->GetBaseWeapon();

    if ( !pWeapon || pWeapon->IsMeleeWeapon() )
        SetColor( LFX_ALL_RIGHT, LFX_WHITE | LFX_HALF_BRIGHTNESS );
    else
    {
        int ammo    = pPlayer->GetAmmoCount( pWeapon->GetPrimaryAmmoType() );
        int max        = 200;

        if ( ammo < roundup(max*0.4) )
            SetColor( LFX_ALL_RIGHT, LFX_RED | LFX_FULL_BRIGHTNESS );
        else if ( ammo < roundup(max*0.6) )
            SetColor( LFX_ALL_RIGHT, LFX_ORANGE | LFX_FULL_BRIGHTNESS );
        else if ( ammo < roundup(max*0.8) )
            SetColor( LFX_ALL_RIGHT, LFX_YELLOW | LFX_FULL_BRIGHTNESS );
        else
            SetColor( LFX_ALL_RIGHT, LFX_GREEN | LFX_FULL_BRIGHTNESS );
    }

    //
    // Estado
    //
    if ( pPlayer->IsUnderAttack() )
        SetColor( LFX_ALL_LEFT, LFX_RED | LFX_FULL_BRIGHTNESS );
    else if ( pPlayer->IsOnCombat() )
        SetColor( LFX_ALL_LEFT, LFX_ORANGE | LFX_FULL_BRIGHTNESS );
    else
        SetColor( LFX_ALL_LEFT, LFX_CYAN | LFX_FULL_BRIGHTNESS );
	#endif
}

//================================================================================
//================================================================================
void CAlienFX::UpdatePulse( float interval )
{
    if ( !m_PulseTimer.IsElapsed() )
        return;

    C_Player *pPlayer = C_Player::GetLocalInPlayer();

    if ( pPlayer->GetPlayerStatus() == PLAYER_STATUS_FALLING )
    {
        switch( m_PulseState )
        {
            case 0:
            default:
                SetColor( LFX_ALL, LFX_RED | LFX_FULL_BRIGHTNESS );
                m_PulseState = 1;
            break;

            case 1:
                SetColor( LFX_ALL, LFX_YELLOW | LFX_FULL_BRIGHTNESS );
                m_PulseState = 2;
            break;

            case 2:
                SetColor( LFX_ALL, LFX_RED | LFX_FULL_BRIGHTNESS );
                m_PulseState = 3;
            break;

            case 3:
                SetColor( LFX_ALL, LFX_ORANGE | LFX_HALF_BRIGHTNESS );
                m_PulseState = 4;
            break;

            case 4:
                SetColor( LFX_ALL, LFX_RED | LFX_HALF_BRIGHTNESS );
                m_PulseState = 5;
            break;

            case 5:
                SetColor( LFX_ALL, LFX_BLACK | LFX_FULL_BRIGHTNESS );
                m_PulseState = 0;
            break;
        }
    }
    else
    {
        switch( m_PulseState )
        {
            case 0:
            default:
                SetColor( LFX_ALL, LFX_RED | LFX_FULL_BRIGHTNESS );
                m_PulseState = 1;
            break;

            case 1:
                SetColor( LFX_ALL, LFX_BLACK | LFX_FULL_BRIGHTNESS );
                m_PulseState = 0;
            break;
        }
    }

    m_PulseTimer.Start( interval );
}

//================================================================================
// Devuelve si la funcionalidad de AlienFX esta activada
//================================================================================
bool CAlienFX::IsEnabled()
{
    // No se pudo iniciar la DLL
    if ( !m_bStarted )
        return false;

    return cl_alienfx_enabled.GetBool();
}

//================================================================================
// Hemos recibido un mensaje desde el servidor
//================================================================================
void CAlienFX::OnMessage( bf_read &msg )
{
    if ( !IsEnabled() )
        return;

    int command = msg.ReadShort();

    // Comando
    switch( command )
    {
        // Establecer un nuevo color
        case ALIENFX_SETCOLOR:
            int light        = msg.ReadLong();
            int color        = msg.ReadLong();
            float duration    = msg.ReadFloat();

            // Establecemos el color y la duración
            TheAlienFX->SetActiveColor( light, color, duration );
        break;
    }
}

//================================================================================
//================================================================================
void CAlienFX::SetColor( int device, int color, float duration )
{
    if ( !IsEnabled() )
        return;

    FXLight( device, color );

    if ( duration > 0 )
        m_LightTimer.Start( duration );
}

//================================================================================
//================================================================================
void CAlienFX::SetActiveColor( int device, int color, float duration )
{
    if ( !IsEnabled() )
        return;

    //FXReset();
    SetColor( device, color, duration );
    FXUpdate();
}

//================================================================================
//================================================================================
void CAlienFX::SetActionColor( int device, int color, int action )
{
    if ( !IsEnabled() )
        return;

    FXLightActionColor( device, color, action );
}