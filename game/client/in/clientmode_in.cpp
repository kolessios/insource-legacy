#include "cbase.h"
#include "clientmode_in.h"

#include "vgui_int.h"
#include "ienginevgui.h"
#include "cdll_client_int.h"
#include "engine/IEngineSound.h"

#include "viewpostprocess.h"

#include "panelmetaclassmgr.h"
#include "gameui_interface.h"

#include "c_in_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
// Comandos
//================================================================================

ConVar default_fov( "default_fov", "90", FCVAR_CHEAT );
ConVar fov_desired( "fov_desired", "90", FCVAR_USERINFO, "Sets the base field-of-view.", true, 1.0, true, 75.0 );

DECLARE_CMD( mat_fxaa_enable, "0", "", FCVAR_ARCHIVE );
DECLARE_CMD( mat_sunrays_enable, "0", "", FCVAR_ARCHIVE );
DECLARE_CMD( mat_desaturation_enable, "0", "", FCVAR_ARCHIVE );

//================================================================================
//================================================================================

extern void UpdateScreenEffectTexture();

vgui::HScheme g_hVGuiCombineScheme = 0;

static IClientMode *g_pClientMode[ MAX_SPLITSCREEN_PLAYERS ];
BaseClientMode g_ClientModeNormal[ MAX_SPLITSCREEN_PLAYERS ];

static BaseClientModeFullscreen g_FullscreenClientMode;

// these vgui panels will be closed at various times (e.g. when the level ends/starts)
static char const *s_CloseWindowNames[]={
    "InfoMessageWindow",
    "SkipIntro",
};

#define SCREEN_FILE "scripts/vgui_screens.txt"

// See interface.h/.cpp for specifics:  basically this ensures that we actually Sys_UnloadModule the dll and that we don't call Sys_LoadModule 
//  over and over again.
static CDllDemandLoader g_GameUI( "gameui" );

static CModeManager g_ModeManager;
IVModeManager *modemanager = ( IVModeManager * )&g_ModeManager;

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
// Funciones externas
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

//================================================================================
//================================================================================
IClientMode *GetClientMode()
{
    ASSERT_LOCAL_PLAYER_RESOLVABLE();
    return g_pClientMode[ GET_ACTIVE_SPLITSCREEN_SLOT() ];
}

//================================================================================
//================================================================================
IClientMode *GetClientModeNormal()
{
    ASSERT_LOCAL_PLAYER_RESOLVABLE();
    return &g_ClientModeNormal[ GET_ACTIVE_SPLITSCREEN_SLOT() ];
}

//================================================================================
//================================================================================
BaseClientMode* GetBaseClientMode()
{
    ASSERT_LOCAL_PLAYER_RESOLVABLE();
    return &g_ClientModeNormal[ GET_ACTIVE_SPLITSCREEN_SLOT() ];
}

//================================================================================
//================================================================================
IClientMode *GetFullscreenClientMode( void )
{
    return &g_FullscreenClientMode;
}

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
// CModeManager
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

//================================================================================
//================================================================================
void CModeManager::Init()
{
    for( int i = 0; i < MAX_SPLITSCREEN_PLAYERS; ++i )
    {
        ACTIVE_SPLITSCREEN_PLAYER_GUARD( i );
        g_pClientMode[ i ] = GetClientModeNormal();
    }

    PanelMetaClassMgr()->LoadMetaClassDefinitionFile( SCREEN_FILE );
    //GetClientVoiceMgr()->SetHeadLabelOffset( 40 );
}

//================================================================================
//================================================================================
void CModeManager::LevelInit( const char *newmap )
{
    for( int i = 0; i < MAX_SPLITSCREEN_PLAYERS; ++i )
    {
        ACTIVE_SPLITSCREEN_PLAYER_GUARD( i );
        GetClientMode()->LevelInit( newmap );
    }
}

//================================================================================
//================================================================================
void CModeManager::LevelShutdown( void )
{
    for( int i = 0; i < MAX_SPLITSCREEN_PLAYERS; ++i )
    {
        ACTIVE_SPLITSCREEN_PLAYER_GUARD( i );
        GetClientMode()->LevelShutdown();
    }
}

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
// BaseClientMode
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

//================================================================================
//================================================================================
void BaseClientMode::Init()
{
    BaseClass::Init();

	// Escuchamos el evento de carga de mapa
    gameeventmanager->AddListener( this, "game_newmap", false );
}

//================================================================================
//================================================================================
void BaseClientMode::InitViewport()
{
    m_pViewport = new CHudViewport();
    m_pViewport->Start( gameuifuncs, gameeventmanager );
}

//================================================================================
//================================================================================
void BaseClientMode::Shutdown()
{
}

//================================================================================
//================================================================================
void BaseClientMode::LevelInit( const char *newmap )
{
    // reset ambient light
    static ConVarRef mat_ambient_light_r( "mat_ambient_light_r" );
    static ConVarRef mat_ambient_light_g( "mat_ambient_light_g" );
    static ConVarRef mat_ambient_light_b( "mat_ambient_light_b" );

    if ( mat_ambient_light_r.IsValid() )
        mat_ambient_light_r.SetValue( "0" );
    if ( mat_ambient_light_g.IsValid() )
        mat_ambient_light_g.SetValue( "0" );
    if ( mat_ambient_light_b.IsValid() )
        mat_ambient_light_b.SetValue( "0" );

    // Base
    BaseClass::LevelInit(newmap);

    // clear any DSP effects
    CLocalPlayerFilter filter;
    enginesound->SetRoomType( filter, 0 );
    enginesound->SetPlayerDSP( filter, 0, true );
}

//================================================================================
//================================================================================
void BaseClientMode::LevelShutdown( void )
{
    BaseClass::LevelShutdown();
}

//================================================================================
//================================================================================
void BaseClientMode::Update()
{
    // Update!
    BaseClass::Update();

    // Actualizamos los efectos PostProcessing
    UpdatePostProcessingEffects();
}

//================================================================================
//================================================================================
void BaseClientMode::FireGameEvent( IGameEvent *event )
{
    const char *eventname = event->GetName();

    if ( Q_strcmp( "game_newmap", eventname ) == 0 )
    {
        engine->ClientCmd("exec newmapsettings\n");
    }
    else
    {
        BaseClass::FireGameEvent(event);
    }
}

//================================================================================
//================================================================================
void BaseClientMode::UpdatePostProcessingEffects()
{
    PostProcessParameters_t params;

    C_Player *pPlayer = C_Player::GetLocalInPlayer();

    // No hay jugador local
    if ( !pPlayer )
        return;

    //
    if ( !pPlayer->GetActivePostProcessController() )
        return;

    // Obtenemos la configuración del mapa
    params = pPlayer->GetActivePostProcessController()->m_PostProcessParameters;

    // Personalizado
    pPlayer->DoPostProcessingEffects( params );

    // Establecemos
    SetPostProcessParams( &params );
}

//================================================================================
//================================================================================
void BaseClientMode::DoPostScreenSpaceEffects( const CViewSetup *pSetup )
{

}

//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------
// BaseClientModeFullscreen
//--------------------------------------------------------------------------------
//--------------------------------------------------------------------------------

//================================================================================
//================================================================================
void BaseClientModeFullscreen::InitViewport()
{
	 BaseClass::BaseClass::InitViewport();

     m_pViewport = new CFullscreenViewport();
     m_pViewport->Start( gameuifuncs, gameeventmanager );
}

//================================================================================
//================================================================================
void BaseClientModeFullscreen::Init() 
{
    // Skip over BaseClass!!!
    BaseClass::BaseClass::Init();

    // Load up the combine control panel scheme
    if ( !g_hVGuiCombineScheme )
    {
		g_hVGuiCombineScheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), IsXbox() ? "resource/ClientScheme.res" : "resource/CombinePanelScheme.res", "CombineScheme" );
		
		if (!g_hVGuiCombineScheme)
		{
			Warning( "Couldn't load combine panel scheme!\n" );
		}
    }
}

void BaseClientModeFullscreen::Shutdown() 
{
}

//================================================================================
//================================================================================

PRECACHE_REGISTER_BEGIN( GLOBAL, ModPostProcessing )
    PRECACHE( MATERIAL, "shaders/postproc_fxaa" )
    PRECACHE( MATERIAL, "shaders/postproc_sunrays" )
    PRECACHE( MATERIAL, "postproc_desaturation" )
    PRECACHE( GAMESOUND, "Hitmark" )
PRECACHE_REGISTER_END()

//================================================================================
//================================================================================
void DoModPostProcessing( IMatRenderContext *pRenderContext, int x, int y, int w, int h )
{
    // extern mat_postprocess_enable, dont works!
    ConVarRef mat_postprocess_enable("mat_postprocess_enable");

    // No queremos estos efectos
    if ( !mat_postprocess_enable.GetBool() )
        return;

    C_Player *pPlayer = C_Player::GetLocalInPlayer();

    if ( !pPlayer )
        return;

    //C_BaseInWeapon *pWeapon = pPlayer->GetActiveInWeapon();

    //
    // FXAA
    //
    if ( mat_fxaa_enable.GetBool() )
    {
        static IMaterial *pMatFXAA = materials->FindMaterial( "shaders/postproc_fxaa", TEXTURE_GROUP_OTHER );

        if ( pMatFXAA )
        {
            UpdateScreenEffectTexture();
            pRenderContext->DrawScreenSpaceRectangle( pMatFXAA, 0, 0, w, h, 0, 0, w - 1, h - 1, w, h );
        }
    }

    //
    // Sun Rays
    //
    if ( mat_sunrays_enable.GetBool() )
    {
        static IMaterial *pMatSunRays = materials->FindMaterial( "shaders/postproc_sunrays", TEXTURE_GROUP_OTHER );

        if ( pMatSunRays )
        {
            UpdateScreenEffectTexture();
            pRenderContext->DrawScreenSpaceRectangle( pMatSunRays, 0, 0, w, h, 0, 0, w - 1, h - 1, w, h );
        }
    }

    //
    // Sun Rays
    //
    if ( mat_desaturation_enable.GetBool() )
    {
        static IMaterial *pMatDesaturation = materials->FindMaterial( "shaders/postproc_desaturation", TEXTURE_GROUP_OTHER );

        if ( pMatDesaturation )
        {
            UpdateScreenEffectTexture();
            pRenderContext->DrawScreenSpaceRectangle( pMatDesaturation, 0, 0, w, h, 0, 0, w - 1, h - 1, w, h );
        }
    }

    // Tenemos un arma
    //if ( pWeapon )
    {
        // Tenemos la mira de acero
        /*if ( pWeapon->IsIronsighted() )
        {
            static IMaterial *pGaussian = materials->FindMaterial( "shaders/ppe_gaussian_blur", TEXTURE_GROUP_OTHER );
            if ( pGaussian )
            {
                UpdateScreenEffectTexture();
                pRenderContext->DrawScreenSpaceRectangle( pGaussian, 0, 0, w, h, 0, 0, w - 1, h - 1, w, h );
            }
        }*/
    }
}