//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#include "cbase.h"
#include "c_in_player.h"

#include "in_gamerules.h"

#include "clientmode_in.h"
#include "sdk_loading_panel.h"
#include "c_basetempentity.h"

#include "dlight.h"
#include "cdll_client_int.h"
#include "c_effects.h"
#include "iefx.h"

#include "collisionutils.h"
#include "playerandobjectenumerator.h"

#include "iinput.h"
#include "input.h"
#include "sdk_input.h"
#include "toolframework_client.h"

#include "c_ai_basenpc.h"
#include "in_buttons.h"
#include "prediction.h"

#include "iviewrender_beams.h"
#include "alienfx.h"

#include "in_playeranimstate_proxy.h"
#include "takedamageinfo.h"
#include "viewrender.h"

#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialvar.h"
#include "imaterialproxydict.h"
#include "proxyentity.h"

#if defined( CPlayer )
    #undef CPlayer
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DECLARE_CHEAT_CMD( cl_player_itproxy, "0.98", "" );

//=================================================================================================================
// This is a last-minute hack to ship Orange Box on the 360!
//=================================================================================================================
class CITMaterialProxy : public CEntityMaterialProxy
{
public:
    CITMaterialProxy()
    {
        m_pMaterial = NULL;
        m_pOriginVar = NULL;
    }

    virtual bool Init( IMaterial *pMaterial, KeyValues *pKeyValues )
    {
        m_pMaterial = pMaterial;
        bool found;
        m_pOriginVar = m_pMaterial->FindVar( "$ITAmount", &found );

        if( !found )
        {
            m_pOriginVar = NULL;
            return false;
        }

        return true;
    }

    virtual void OnBind( C_BaseEntity *pC_BaseEntity )
    {
        //const Vector &origin = pC_BaseEntity->GetAbsOrigin();
        m_pOriginVar->SetFloatValue( cl_player_itproxy.GetFloat() );
    }

    virtual IMaterial *GetMaterial()
    {
        return m_pMaterial;
    }

protected:
    IMaterial *m_pMaterial;
    IMaterialVar *m_pOriginVar;
};

EXPOSE_MATERIAL_PROXY( CITMaterialProxy, IT );

//================================================================================
// Logging System
// Only for the current file, this should never be in a header.
//================================================================================

#define Msg(...) Log_Msg(LOG_PLAYER, __VA_ARGS__)
#define Warning(...) Log_Warning(LOG_PLAYER, __VA_ARGS__)

//================================================================================
// Comandos
//================================================================================

DECLARE_CHEAT_CMD( cl_playermodel_force_draw, "0", "Indica si se debe mostrar el modelo del Jugador en todo momento" );

DECLARE_CHEAT_CMD( cl_flashlight_fov, "40", "" );
DECLARE_CHEAT_CMD( cl_flashlight_far, "750", "" );
DECLARE_CHEAT_CMD( cl_flashlight_linear, "3", "" );

DECLARE_CHEAT_CMD( cl_flashlight_beam_haloscale, "10", "" );
//DECLARE_CHEAT_CMD( cl_flashlight_beam_endwidth, "10", "" );
DECLARE_CHEAT_CMD( cl_flashlight_beam_fadelength, "300", "" );
DECLARE_CHEAT_CMD( cl_flashlight_beam_brightness, "32", "" );
//DECLARE_CHEAT_CMD( cl_flashlight_beam_segments, "10", "" );

//DECLARE_COMMAND( cl_bullet_damage_effect, "1", "", 0 );

// How fast to avoid collisions with center of other object, in units per second
#define AVOID_SPEED 2000.0f
extern ConVar cl_forwardspeed;
extern ConVar cl_backspeed;
extern ConVar cl_sidespeed;

extern ConVar muzzleflash_light;

//================================================================================
// Información y Red
//================================================================================

IMPLEMENT_NETWORKCLASS_ALIASED( Player, DT_BaseInPlayer )

// Local Player
BEGIN_RECV_TABLE_NOBASE( C_Player, DT_BaseInLocalPlayerExclusive )
    RecvPropVector( RECVINFO_NAME( m_vecNetworkOrigin, m_vecOrigin ) ),
END_RECV_TABLE()

// Other Players
BEGIN_RECV_TABLE_NOBASE( C_Player, DT_InBaseNonLocalPlayerExclusive )
    RecvPropVector( RECVINFO_NAME( m_vecNetworkOrigin, m_vecOrigin ) ),
END_RECV_TABLE()

BEGIN_NETWORK_TABLE( C_Player, DT_BaseInPlayer )
    RecvPropBool( RECVINFO(m_bMovementDisabled) ),
    RecvPropBool( RECVINFO(m_bAimingDisabled) ),
    RecvPropInt( RECVINFO( m_iButtonsDisabled ) ),
    RecvPropInt( RECVINFO( m_iButtonsForced ) ),

    RecvPropInt( RECVINFO(m_iOldHealth) ),

    RecvPropBool( RECVINFO(m_bFlashlightEnabled) ),
    RecvPropBool( RECVINFO(m_bSprinting) ),
    RecvPropBool( RECVINFO( m_bSneaking ) ),

    RecvPropBool( RECVINFO( m_bOnCombat ) ),
    RecvPropBool( RECVINFO( m_bUnderAttack ) ),

    RecvPropInt( RECVINFO(m_iPlayerStatus) ),
    RecvPropInt( RECVINFO( m_iPlayerState ) ),
    RecvPropInt( RECVINFO( m_iPlayerClass ) ),

    RecvPropInt( RECVINFO(m_iDejectedTimes) ),

    RecvPropFloat( RECVINFO(m_flHelpProgress) ),
    RecvPropFloat( RECVINFO(m_flClimbingHold) ),

    RecvPropInt( RECVINFO(m_iEyeAngleZ) ),
    RecvPropBool( RECVINFO(m_bIsBot) ),

    RecvPropFloat( RECVINFO( m_angEyeAngles[0] ) ),
    RecvPropFloat( RECVINFO( m_angEyeAngles[1] ) ),

    RecvPropDataTable( "localdata", 0, 0, &REFERENCE_RECV_TABLE(DT_BaseInLocalPlayerExclusive) ),
    RecvPropDataTable( "nonlocaldata", 0, 0, &REFERENCE_RECV_TABLE(DT_InBaseNonLocalPlayerExclusive) ),
END_RECV_TABLE()

BEGIN_PREDICTION_DATA( C_Player )
    // Real Prediction
    //DEFINE_PRED_FIELD( m_iOldHealth, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
    DEFINE_PRED_FIELD( m_bOnCombat, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
    DEFINE_PRED_FIELD( m_bUnderAttack, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
    //DEFINE_PRED_FIELD_TOL( m_flStamina, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, 0.5f ),
    DEFINE_PRED_FIELD_TOL( m_flHelpProgress, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, 0.5f ), // TODO
    DEFINE_PRED_FIELD_TOL( m_flClimbingHold, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, 0.5f ),
    DEFINE_PRED_FIELD( m_bSprinting, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),

    // Only for Debug
    DEFINE_PRED_FIELD( m_bMovementDisabled, FIELD_BOOLEAN, FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
    DEFINE_PRED_FIELD( m_iPlayerStatus, FIELD_BOOLEAN, FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
    DEFINE_PRED_FIELD( m_iDejectedTimes, FIELD_BOOLEAN, FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),

    DEFINE_PRED_FIELD( m_nSequence, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ), 
    DEFINE_PRED_FIELD( m_flPlaybackRate, FIELD_FLOAT, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
    DEFINE_PRED_FIELD( m_flCycle, FIELD_FLOAT, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ), 
    //DEFINE_PRED_FIELD( m_nNewSequenceParity, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
    //DEFINE_PRED_FIELD( m_nResetEventsParity, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
END_PREDICTION_DATA()

//================================================================================
// Constructor
//================================================================================
C_Player::C_Player() : m_iv_angEyeAngles( "C_Player::m_iv_angEyeAngles" )
{
    //
    SetPredictionEligible( true );

    // Viewangles (Lo mismo que pl.v_angle)
    m_angEyeAngles.Init();
    AddVar( &m_angEyeAngles, &m_iv_angEyeAngles, LATCH_SIMULATION_VAR );

    // Parpadeo
    m_nBlinkTimer.Invalidate();
}

//================================================================================
// Constructor
//================================================================================
C_Player::~C_Player()
{
    // Liberamos el sistema de animación
    if ( GetAnimationSystem() )
        GetAnimationSystem()->Release();

    // Linternas
    DestroyFlashlight();
    DestroyBeam();
}

//================================================================================
//================================================================================
C_Player *C_Player::GetLocalInPlayer( int nSlot )
{
    return ToInPlayer( C_BasePlayer::GetLocalPlayer(nSlot) );
}

//================================================================================
//================================================================================
bool C_Player::ShouldRegenerateOriginFromCellBits() const
{
    //return BaseClass::ShouldRegenerateOriginFromCellBits();
    return true;
}

//================================================================================
// Pensamiento (Solo jugador local)
//================================================================================
void C_Player::PreThink()
{
    QAngle vTempAngles = GetLocalAngles();

    if ( GetLocalPlayer() == this )
        vTempAngles[PITCH] = EyeAngles()[PITCH];
    else
        vTempAngles[PITCH] = m_angEyeAngles[PITCH];

    if ( vTempAngles[YAW] < 0.0f )
        vTempAngles[YAW] += 360.0f;

    SetLocalAngles( vTempAngles );

    // BaseClass
    BaseClass::PreThink();

    RemoveFlag( FL_ATCONTROLS );

    if ( IsAlive() )
    {
        // Detección al correr
        UpdateMovementType();
    }
}

//================================================================================
// Pensamiento (Solo jugador local)
//================================================================================
void C_Player::PostThink()
{
    // BaseClass
    BaseClass::PostThink();

    // Store the eye angles pitch so the client can compute its animation state correctly.
	m_angEyeAngles = EyeAngles();

    // Velocidad del Jugador
    UpdateSpeed();

    // Mientras sigas vivo
    if ( IsAlive() )
    {
		// Estrés local
		UpdateLocalStress();

        // Estamos en combate
		m_bOnCombat = ( m_OnCombatTimer.HasStarted() && m_OnCombatTimer.IsLessThen(10.0f) );
            
        // Estamos bajo ataque
		m_bUnderAttack = ( m_UnderAttackTimer.HasStarted() && m_UnderAttackTimer.IsLessThen(5.0f) ) ;

        // Colgando por nuestra vida
        // Predicción en Cliente
        if ( GetPlayerStatus() == PLAYER_STATUS_CLIMBING ) 
        {
            // Presionar la tecla de uso ayuda a sostenerse
            if ( IsButtonPressed(IN_USE) ) 
                m_flClimbingHold += 0.5f;

            // Perdiendo fuerza!
            m_flClimbingHold -= 8 * gpGlobals->frametime;
        }
    }
}

//================================================================================
// Como el pensamiento, pero para todos los jugadores
//================================================================================
bool C_Player::Simulate()
{
    QAngle vTempAngles = GetLocalAngles();
	vTempAngles[PITCH] = m_angEyeAngles[PITCH];
    SetLocalAngles( vTempAngles );
    SetLocalAnglesDim( X_INDEX, 0 );

    int ss = 0;

    // Jugador local
    if ( IsLocalPlayer() )
    {
        ss = GetSplitScreenPlayerSlot();
    }
    else
    {
        // NOTA: Simulate() en jugadores externos solo se llama desde el primer jugador local (En una pantalla dividida)
        //HACK_GETLOCALPLAYER_GUARD( "C_Player::Simulate External Player" );

        // Usamos los angulos que nos envia el servidor
        SetLocalAngles( m_angEyeAngles );

		// Predicción del punch
		DecayPunchAngle();
    }

    ACTIVE_SPLITSCREEN_PLAYER_GUARD( ss );

    // Luz de la linterna
    UpdateFlashlight();

    // Destello
    UpdateBeam();

    // Mientras sigas vivo
    if ( IsAlive() )
    {
        UpdateLookAt();
    }

    // BaseClass
    return BaseClass::Simulate();
}

//================================================================================
//================================================================================
void C_Player::UpdateLookAt()
{
    // Parpadeamos!
    if ( m_nBlinkTimer.IsElapsed() )
    {
        m_blinktoggle = !m_blinktoggle;
        m_nBlinkTimer.Start( RandomFloat(1.5, 4.0f) );
    }

    Vector vecForward;
    AngleVectors( EyeAngles(), &vecForward );

    // Miramos hacia enfrente
    m_viewtarget = EyePosition() + 50.0f * vecForward;
}

//================================================================================
// [Evento] Hemos recibido daño
//================================================================================
void C_Player::TakeDamage( const CTakeDamageInfo &info )
{
	BaseClass::TakeDamage( info );

	//Msg("%i \n", info.GetDamageType());

    // Estamos bajo ataque!
    m_UnderAttackTimer.Start();
    m_OnCombatTimer.Start();
}

//================================================================================
//================================================================================
void C_Player::SetLocalStress( float level ) 
{
	m_flLocalStress = level;
    m_flLocalStress = clamp( m_flLocalStress, 0.0f, 100.0f );
}

//================================================================================
//================================================================================
void C_Player::AddLocalStress( float level ) 
{
	float flStress = GetLocalStress();
    flStress += level;

    SetLocalStress( flStress );
}

//================================================================================
//================================================================================
void C_Player::UpdateLocalStress() 
{
	ConVarRef sv_player_stress_drain("sv_player_stress_drain");

	float flStress = GetLocalStress();

    // Llenos
    if ( flStress <= 0 )
        return;

    flStress -= (sv_player_stress_drain.GetFloat() * 2) * gpGlobals->frametime;
    SetLocalStress( flStress );
}

//================================================================================
// Devuelve el Jugador activo (el mismo o al que esta viendo en modo espectador)
//================================================================================
C_Player *C_Player::GetActivePlayer( int mode )
{
    // Estamos viendo a un Jugador
    if ( IsObserver() && GetObserverMode() == mode && GetObserverTarget() )
        return ToInPlayer( GetObserverTarget() );

    return this;

}

//================================================================================
// Devuelve el [C_BaseAnimating] que representa el cadaver del Jugador.
// Devuelve el jugador en si mismo si no hay cadaver o esta reproduciendo la animación de muerte
//================================================================================
C_BaseAnimating * C_Player::GetRagdoll() 
{
    // Sigues vivo
    // TODO: Quizá esto no debería estar aquí
    if ( IsAlive() )
        return this;

    C_Player *pPlayer = GetActivePlayer();

    CBaseAnimating *pRagdoll = ( m_lifeState == LIFE_DYING ) ? pPlayer : pPlayer->m_pClientsideRagdoll;

    if ( !pRagdoll )
        pRagdoll = pPlayer;
    
    return pRagdoll;
}

//================================================================================
// Devuelve si el jugador local esta mirando este jugador en modo espectador
//================================================================================
bool C_Player::IsLocalPlayerWatchingMe( int mode )
{
    //if ( IsLocalPlayer() )
        //return false;

    if ( !IsLocalPlayerSpectator() )
        return false;

    if ( GetSpectatorMode() != mode )
        return false;

    return ( GetSpectatorTarget() == entindex() );
}

//================================================================================
// Devuelve si un compañero de la pantalla partida esta mirando este jugador
// en modo espectador.
//================================================================================
/*
bool C_Player::IsSplitScreenPartnerWatchingMe( int mode )
{
    if ( !IsLocalPlayer() )
        return false;

    C_Player *pPartner = ToInPlayer( GetSplitScreenViewPlayer() );

    if ( !pPartner || pPartner == this )
        return false;

    if ( !pPartner->IsObserver() )
        return false;

    if ( pPartner->GetObserverMode() != mode )
        return false;

    return (pPartner->GetObserverTarget() == this);
}
*/

//================================================================================
//================================================================================
const QAngle &C_Player::EyeAngles()
{
    if ( IsLocalPlayer() )
        return BaseClass::EyeAngles();
    else
        return m_angEyeAngles;
}

//================================================================================
//================================================================================
const QAngle& C_Player::GetRenderAngles()
{
    if ( IsRagdoll() )
    {
        return vec3_angle;
    }
    else
    {
        if ( GetAnimationSystem() )
            return GetAnimationSystem()->GetRenderAngles();
        else
            return BaseClass::GetRenderAngles();
    }
}

//================================================================================
//================================================================================
void C_Player::UpdateVisibility()
{
    BaseClass::UpdateVisibility();
}

//================================================================================
//================================================================================
IClientModelRenderable *C_Player::GetClientModelRenderable()
{
    return NULL;
}

//================================================================================
// Devuelve si es posible renderizar el modelo del jugador local
// Nota: Solo es llamado por VIEW_MAIN
//================================================================================
bool C_Player::ShouldDrawLocalPlayer()
{
    if ( cl_playermodel_force_draw.GetBool() )
        return true;

    if ( !IsActiveSplitScreenPlayer() )
        return true;

    if ( IsFirstPerson() && ShouldForceDrawInFirstPersonForShadows() )
        return true;

    if ( IsThirdPerson() )
        return true;

    if ( ToolsEnabled() && ToolFramework_IsThirdPersonCamera() )
        return true;

    return false;
}

//================================================================================
// Devuelve si es posible renderizar el modelo de otro jugador
// Nota: Solo es llamado por VIEW_MAIN
//================================================================================
bool C_Player::ShouldDrawExternalPlayer()
{
    if ( IsLocalPlayerWatchingMe() ) {
        if ( !ShouldForceDrawInFirstPersonForShadows() )
            return false;
    }

    return true;
}

//================================================================================
// Devuelve si es necesario renderizar el modelo del jugador
// Nota: Solo es llamado por VIEW_MAIN
// SI DEVUELVE TRUE:
// - Agregara la entidad al Leaf System (Sombras en tiempo real)
// - IsVisible() devolvera true
// - Se añadira sombra estatica a la entidad (BUG: No funciona en pantalla dividida)
// - Se empezara a llamar a DrawModel()
//================================================================================
bool C_Player::ShouldDraw()
{
    // Animación de muerte
    // TODO: Detectar si realmente tenemos animacion de muerte
    if ( m_lifeState == LIFE_DYING )
        return true;

    // Jugador local
    if ( IsLocalPlayer() ) {
        // No debemos renderizar el jugador local
        if ( !ShouldDrawLocalPlayer() )
            return false;
    }
    else {
        // No debemos renderizar otros jugadores
        if ( !ShouldDrawExternalPlayer() )
            return false;
    }

    // BaseClass
    // Devuelve false si la entidad tiene NODRAW
    return BaseClass::ShouldDraw();
}

//================================================================================
// Devuelve si se debe renderizar el modelo en primera persona.
// Solo se renderizara en reflejos y sombras.
//================================================================================
bool C_Player::ShouldForceDrawInFirstPersonForShadows()
{
    return true;
}

//================================================================================
// Renderiza el modelo
//================================================================================
int C_Player::DrawModel( int flags, const RenderableInstance_t &instance )
{
    if ( CurrentViewID() == VIEW_MAIN ) {
        if ( IsLocalPlayer() ) {
            if ( IsActiveSplitScreenPlayer() ) {
                if ( IsFirstPerson() ) {
                    if ( !GetViewEntity() )
                        return 0;
                }
            }
            else {
                if ( IsLocalPlayerWatchingMe() )
                    return 0;
            }
        }
        else {
            if ( IsLocalPlayerWatchingMe() )
                return 0;
        }
    }

    return BaseClass::DrawModel( flags, instance );
}

//================================================================================
// Devuelve el tipo de sombra estatica que debemos proyectar.
//================================================================================
ShadowType_t C_Player::ShadowCastType()
{
    HACK_GETLOCALPLAYER_GUARD( "C_Player::ShadowCastType" );
    //ACTIVE_SPLITSCREEN_PLAYER_GUARD( GetSplitScreenPlayerSlot() );

    if ( !IsVisible() )
        return SHADOWS_NONE;

    return SHADOWS_RENDER_TO_TEXTURE;
}

//================================================================================
//================================================================================
bool C_Player::ShouldReceiveProjectedTextures( int flags )
{
    HACK_GETLOCALPLAYER_GUARD( "C_Player::ShouldReceiveProjectedTextures" );
    //ACTIVE_SPLITSCREEN_PLAYER_GUARD( GetSplitScreenPlayerSlot() );

    if ( !IsVisible() )
        return false;

    return true;
}

//================================================================================
//================================================================================
C_BaseWeapon *C_Player::GetBaseWeapon() 
{
    return ToBaseWeapon(GetActiveWeapon());
}

//================================================================================
//================================================================================
void C_Player::UpdateClientSideAnimation()
{
    // Actualizamos el sistema de animaciones
    if ( GetAnimationSystem() )
        GetAnimationSystem()->Update();

    // Actualizamos Poses
    UpdatePoseParams();

    // Base!
    BaseClass::UpdateClientSideAnimation();
}

//================================================================================
//================================================================================
void C_Player::SetAnimation( PLAYER_ANIM nAnim ) 
{
    if ( nAnim == PLAYER_WALK || nAnim == PLAYER_IDLE ) return;

    // Recarga
    if ( nAnim == PLAYER_RELOAD ) 
    {
        DoAnimationEvent( PLAYERANIMEVENT_RELOAD );
    }
    else if ( nAnim == PLAYER_JUMP ) 
    {
        DoAnimationEvent( PLAYERANIMEVENT_JUMP );
    }
    else 
    {
        Assert( !"Client Deprecated SetAnimation" );
    }
}

//================================================================================
//================================================================================
void C_Player::DoAnimationEvent( PlayerAnimEvent_t nEvent, int nData, bool bPredicted )
{
    // Hemos disparado, entramos en combate
    // TODO: Mover a una función separada?
    if ( nEvent == PLAYERANIMEVENT_ATTACK_PRIMARY )
    {
		// Estamos en combate
        m_OnCombatTimer.Start();

		// Mostramos la luz de combate
        ShowMuzzleFlashlight();

        if ( IsLocalPlayer() )
		{
            TheAlienFX->SetActiveColor( LFX_ALL_RIGHT, LFX_WHITE | LFX_FULL_BRIGHTNESS, 0.1f );
		}
		else
		{
			// Agregamos el ViewKick
			if ( GetBaseWeapon() )
				GetBaseWeapon()->AddViewKick();
		}
    }

    MDLCACHE_CRITICAL_SECTION();

    // Procesamos la animación en el cliente
    if ( GetAnimationSystem() )
        GetAnimationSystem()->DoAnimationEvent( nEvent, nData );
}

//================================================================================
//================================================================================
void C_Player::PostDataUpdate( DataUpdateType_t updateType )
{
    // C_BaseEntity assumes we're networking the entity's angles, so pretend that it
    // networked the same value we already have.
    SetNetworkAngles( GetLocalAngles() );

    if ( updateType == DATA_UPDATE_CREATED )
    {
        // Creamos el sistema de animaciones
        CreateAnimationSystem();

        // Creamos la linterna
        CreateFlashlight();
    }
    
    // Base!
    BaseClass::PostDataUpdate( updateType );
}

//================================================================================
//================================================================================
void C_Player::OnDataChanged( DataUpdateType_t type )
{
    // Base!
    BaseClass::OnDataChanged( type );

    // Make sure we're thinking
    if ( type == DATA_UPDATE_CREATED )
    {
        SetNextClientThink( CLIENT_THINK_ALWAYS );
    }

    //
    UpdateVisibility();
}

//================================================================================
//================================================================================
CStudioHdr *C_Player::OnNewModel()
{
    CStudioHdr *pHDR = BaseClass::OnNewModel();
    InitializePoseParams();

    // Reset the players animation states, gestures
    if ( GetAnimationSystem() )
        GetAnimationSystem()->OnNewModel();

    return pHDR;
}

//================================================================================
//================================================================================
void C_Player::InitializePoseParams()
{
    if ( !IsAlive() )
        return;

    CStudioHdr *pHDR = GetModelPtr();

    if ( !pHDR )
        return;

    for ( int i = 0; i < pHDR->GetNumPoseParameters() ; i++ )
        SetPoseParameter( pHDR, i, 0.0 );
}

//================================================================================
//================================================================================
const char *C_Player::GetFlashlightTextureName() const
{
    return "effects/flashlight001";
}

//================================================================================
//================================================================================
const char *C_Player::GetFlashlightWeaponAttachment()
{
    return "flashlight"; // flashlight
}

//================================================================================
//================================================================================
float C_Player::GetFlashlightFOV() const
{
    return cl_flashlight_fov.GetFloat();
}

//================================================================================
//================================================================================
float C_Player::GetFlashlightFarZ()
{
    return cl_flashlight_far.GetFloat();
}

//================================================================================
//================================================================================
float C_Player::GetFlashlightLinearAtten()
{
    return cl_flashlight_linear.GetFloat();
}

//================================================================================
//================================================================================
void C_Player::GetFlashlightOffset( const Vector &vecForward, const Vector &vecRight, const Vector &vecUp, Vector *pVecOffset ) const
{
    // TODO ?
    Assert( false );
}

//================================================================================
//================================================================================
bool C_Player::CreateFlashlight() 
{
    if ( GetFlashlight(NORMAL) )
        return true;

    GetFlashlight(NORMAL) = new CFlashlightEffect( index, GetFlashlightTextureName() );
    Assert( GetFlashlight(NORMAL) );

    // Ha ocurrido un problema
    if ( !GetFlashlight(NORMAL) )
    {
        Warning("Ha ocurrido un problema al crear la linterna de %s \n", GetPlayerName());
        return false;
    }

    // Configuración de la linterna
    GetFlashlight(NORMAL)->Init();

    return true;
}

//================================================================================
// Apaga nuestra linterna
//================================================================================
void C_Player::DestroyFlashlight()
{
    // Apagamos la linterna
    if ( GetFlashlight(NORMAL) )
    {
        GetFlashlight(NORMAL)->TurnOff();
        GetFlashlight(NORMAL) = NULL;
    }
}

//================================================================================
// Actualiza la linterna
//================================================================================
void C_Player::UpdateFlashlight()
{
    if ( !GetFlashlight( NORMAL ) )
        return;

    // Obtenemos el jugador o el jugador a quien esta viendo en modo espectador
    C_Player *pPlayer = GetActivePlayer();

    if ( !pPlayer || !pPlayer->FlashlightIsOn() || !pPlayer->IsAlive() ) {
        GetFlashlight( NORMAL )->TurnOff();
        return;
    }

    ConVarRef sv_flashlight_weapon( "sv_flashlight_weapon" );
    GetFlashlightPosition( pPlayer, m_vecFlashlightOrigin, m_vecFlashlightForward, m_vecFlashlightRight, m_vecFlashlightUp, sv_flashlight_weapon.GetBool() );

    // Linterna de otro jugador
    if ( !IsLocalPlayer() || GetSplitScreenViewPlayer() != this ) {
        ConVarRef sv_flashlight_realistic( "sv_flashlight_realistic" );

        // No queremos ver las linternas de jugadores muertos
        // Si estamos viendo a este jugador en modo espectador, nuestra linterna actuara como el de el
        if ( !IsAlive() || IsLocalPlayerWatchingMe() ) {
            GetFlashlight( NORMAL )->TurnOff();
            return;
        }

        // El servidor no quiere luces dinámicas para las linternas de otros jugadores.
        if ( !sv_flashlight_realistic.GetBool() ) {
            GetFlashlight( NORMAL )->TurnOff();

            trace_t tr;
            UTIL_TraceLine( m_vecFlashlightOrigin, m_vecFlashlightOrigin + (m_vecFlashlightForward * pPlayer->GetFlashlightFarZ()), MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );

            // Usamos una linterna barata en recursos
            dlight_t *el = effects->CL_AllocDlight( index );
            el->origin = tr.endpos;
            el->radius = pPlayer->GetFlashlightFOV() + 35.0f;
            el->decay = el->radius / 0.05f;
            el->die = gpGlobals->curtime + 0.001f;
            el->color.r = 255;
            el->color.g = 255;
            el->color.b = 255;
            el->color.exponent = 2;
            return;
        }

        // Las linternas de otros jugadores son de baja calidad
        GetFlashlight( NORMAL )->SetShadows( true, false, 0, 2.5f );
    }

    GetFlashlight( NORMAL )->SetFOV( pPlayer->GetFlashlightFOV() );
    GetFlashlight( NORMAL )->SetFar( pPlayer->GetFlashlightFarZ() );
    GetFlashlight( NORMAL )->SetBrightScale( pPlayer->GetFlashlightLinearAtten() );
    GetFlashlight( NORMAL )->TurnOn();

    /*if ( !IsLocalPlayer() )
    {
        trace_t tr;
        UTIL_TraceLine( m_vecFlashlightOrigin, m_vecFlashlightOrigin + (m_vecFlashlightForward * pPlayer->GetFlashlightFarZ()), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );

        float distance = tr.startpos.DistTo( tr.endpos );
        distance = (distance / 3);

        Vector vecEnd = ( tr.endpos + (distance * -m_vecFlashlightForward) );

        // Usamos una linterna barata en recursos
        dlight_t *el        = effects->CL_AllocDlight( index );
        el->origin          = vecEnd;
        el->radius          = pPlayer->GetFlashlightFOV() + 160.0f;
        el->decay           = el->radius / 0.05f;
        el->die             = gpGlobals->curtime + 0.001f;
        el->color.r         = 255;
        el->color.g         = 255;
        el->color.b         = 255;
        el->color.exponent  = 0.2;
    }*/

    // Actualizamos la linterna
    GetFlashlight( NORMAL )->Update( m_vecFlashlightOrigin, m_vecFlashlightForward, m_vecFlashlightRight, m_vecFlashlightUp );
}

//================================================================================
//================================================================================
bool C_Player::CreateBeam( Vector vecStart, Vector vecEnd )
{
    if ( GetFlashlightBeam( NORMAL ) )
        return true;

    BeamInfo_t info;
    info.m_nType = TE_BEAMPOINTS;
    info.m_vecStart = vecStart;
    info.m_vecEnd = vecEnd;
    info.m_pszModelName = "sprites/glow_test02.vmt";
    info.m_pszHaloName = "sprites/light_glow03.vmt";
    info.m_flHaloScale = cl_flashlight_beam_haloscale.GetFloat();
    info.m_flWidth = 8.0f;
    info.m_flEndWidth = info.m_flWidth;
    info.m_flAmplitude = 0;
    info.m_flBrightness = cl_flashlight_beam_brightness.GetFloat();
    info.m_flSpeed = 0.0f;
    info.m_nStartFrame = 0.0;
    info.m_flFrameRate = 0.0;
    info.m_flRed = 255.0;
    info.m_flGreen = 255.0;
    info.m_flBlue = 255.0;
    info.m_bRenderable = true;
    info.m_flLife = 0;
    //info.m_nFlags       = FBEAM_FOREVER | FBEAM_HALOBEAM;
    info.m_nFlags = FBEAM_SHADEOUT | FBEAM_NOTILE;
    info.m_flFadeLength = cl_flashlight_beam_fadelength.GetFloat();

    GetFlashlightBeam( NORMAL ) = beams->CreateBeamPoints( info );
    Assert( GetFlashlightBeam( NORMAL ) );

    // Ha ocurrido un problema
    if ( !GetFlashlightBeam( NORMAL ) ) {
        Warning( "Ha ocurrido un problema al crear el destello de %s.\n", GetPlayerName() );
        return false;
    }

    return true;
}

//================================================================================
//================================================================================
void C_Player::DestroyBeam()
{
    // Apagamos el destello
    if ( GetFlashlightBeam( NORMAL ) ) {
        GetFlashlightBeam( NORMAL )->flags = 0;
        GetFlashlightBeam( NORMAL )->die = gpGlobals->curtime - 1;
        GetFlashlightBeam( NORMAL ) = NULL;
    }
}

//================================================================================
//================================================================================
void C_Player::UpdateBeam()
{
    //if ( !GetFlashlight(NORMAL)Beam )
    //  return;

    // Linterna de otro jugador
    if ( !IsLocalPlayer() || GetSplitScreenViewPlayer() != this ) {
        // No queremos ver las linternas de otros jugadores muertos
        // Si estamos viendo a este jugador en modo espectador, nuestra linterna actuara como el de el
        if ( !IsAlive() || IsLocalPlayerWatchingMe()  ) {
            DestroyBeam();
            return;
        }
    }

    // Obtenemos el jugador o el jugador a quien esta viendo en modo espectador
    C_Player *pPlayer = GetActivePlayer();

    // Destruimos la linterna
    if ( !pPlayer || !pPlayer->FlashlightIsOn() || !pPlayer->IsAlive() ) {
        DestroyBeam();
        return;
    }

    trace_t tr;
    UTIL_TraceLine( m_vecFlashlightOrigin, m_vecFlashlightOrigin + (m_vecFlashlightForward * 30), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &tr );

    if ( !GetFlashlightBeam( NORMAL ) && !CreateBeam( tr.startpos, tr.endpos ) )
        return;

    //debugoverlay->AddBoxOverlay( tr.startpos, Vector(-1,-1,-1), Vector(1,1,1), QAngle( 0, 0, 0), 255, 0, 0, 255, 0.1f );
    //debugoverlay->AddBoxOverlay( tr.endpos, Vector(-1,-1,-1), Vector(1,1,1), QAngle( 0, 0, 0), 0, 255, 0, 255, 0.1f );

    // Actualizamos
    BeamInfo_t info;
    info.m_vecStart = tr.startpos;
    info.m_vecEnd = tr.endpos;
    info.m_flRed = 255.0;
    info.m_flGreen = 255.0;
    info.m_flBlue = 255.0;
    beams->UpdateBeamInfo( GetFlashlightBeam( NORMAL ), info );
}

//================================================================================
// Muestra la luz de disparo de arma
//================================================================================
void C_Player::ShowMuzzleFlashlight()
{
    // Obtenemos la ubicación para la linterna
    Vector vecPosition, vecForward, vecRight, vecUp;
    GetFlashlightPosition( this, vecPosition, vecForward, vecRight, vecUp, true, "muzzle" );

    // Muzzleflash Simple
    if ( muzzleflash_light.GetBool() ) {
        // Make an elight
        dlight_t *el = effects->CL_AllocDlight( index );
        el->origin = vecPosition;
        el->radius = RandomFloat( 145.0f, 316.0f );
        el->decay = 0.05f;
        el->die = gpGlobals->curtime + 0.001f;
        el->color.r = 224;
        el->color.g = 209;
        el->color.b = 108;
        el->color.exponent = 5;
    }
}

//================================================================================
//================================================================================
void C_Player::GetFlashlightPosition( C_Player *pPlayer, Vector &vecPosition, Vector &vecForward, Vector &vecRight, Vector &vecUp, bool bFromWeapon, const char *attachment )
{
    if ( !pPlayer )
        return;

    // No tenemos un arma
    if ( !pPlayer->GetActiveWeapon() )
        bFromWeapon = false;

    // Información de origen de la luz
    C_BaseAnimating *pParent = pPlayer;
    int iAttachment = -1;
    const char *pAttachment = (attachment) ? attachment : "eyes";

    // Fix Angles
    bool fixAngles = false;

    // La luz debe venir de la boquilla de nuestra arma
    if ( bFromWeapon ) {
        if ( !attachment )
            pAttachment = GetFlashlightWeaponAttachment();

        if ( pPlayer->IsLocalPlayer() ) {
            if ( pPlayer->IsActiveSplitScreenPlayer() ) {
                if ( pPlayer->IsFirstPerson() ) {
                    pParent = pPlayer->GetViewModel();
                }
                else {
                    pParent = this;
                    fixAngles = true;
                }
            }
            /*else if ( pPlayer->IsSplitScreenPartnerWatchingMe() ) {
                pParent = pPlayer->GetViewModel();
            }*/
            else {
                pParent = this; // pPlayer->GetActiveWeapon()
                fixAngles = true;
            }
        }
        else {
            if ( pPlayer->IsLocalPlayerWatchingMe() ) {
                pParent = pPlayer->GetViewModel();
            }
            else {
                pParent = this;
                fixAngles = false;
            }
        }
        
        /*
        // No es el Jugador local
        // Y el jugador local no me esta mirando en modo espectador
        if ( !IsLocalPlayer( pPlayer ) && !pPlayer->IsLocalPlayerWatchingMe() ) {
            // Usamos el arma
            pParent = pPlayer->GetActiveWeapon();
            fixAngles = true;
        }

        // Jugador Local
        // o el jugador local me esta mirando
        else {
            {
                // Usamos el viewmodel
                pParent = pPlayer->GetViewModel();
            }
        }
        */
    }

    // El arma o el viewmodel es inválido
    // Entonces que venga magicamente de nuestro ojos
    if ( !pParent ) {
        vecPosition = EyePosition();
        pPlayer->EyeVectors( &vecForward, &vecRight, &vecUp );
        vecPosition = vecPosition + 25.0f * vecForward;
        return;
    }

    iAttachment = pParent->LookupAttachment( pAttachment );

    // El acoplamiento no existe
    // Estamos en primera persona y no proviene del arma
    if ( iAttachment <= 0 /*|| (pPlayer->IsFirstPerson() && !bFromWeapon)*/ ) {
        vecPosition = EyePosition();
        pPlayer->EyeVectors( &vecForward, &vecRight, &vecUp );
        vecPosition = vecPosition + 25.0f * vecForward;
    }
    else {
        QAngle eyeAngles;
        pParent->GetAttachment( iAttachment, vecPosition, eyeAngles );

        // @FIX!
        if ( fixAngles ) {
            AngleVectors( eyeAngles, &vecRight, &vecUp, &vecForward );
        }
        else {
            AngleVectors( eyeAngles, &vecForward, &vecRight, &vecUp );
        }
    }
}

//================================================================================
// Actualiza el movimiento de la cámara por recibir un golpe (o parecido)
//--------------------------------------------------------------------------------
// Esta función se ejecuta en gamemovement.cpp:1394 pero solo para el jugador local
// al procesar el movimiento, necesitamos hacer lo mismo para los demás jugadores
// y así obtener una vista precisa de su camara en el modo espectador.
//================================================================================
#define PUNCH_DAMPING 9.0f
#define PUNCH_SPRING_CONSTANT	65.0f

void C_Player::DecayPunchAngle() 
{
	// El jugador local procesa esto por su cuenta
	if ( IsLocalPlayer() )
		return;

	if ( m_Local.m_vecPunchAngle->LengthSqr() > 0.001 || m_Local.m_vecPunchAngleVel->LengthSqr() > 0.001 )
	{
		m_Local.m_vecPunchAngle += m_Local.m_vecPunchAngleVel * gpGlobals->frametime;
		float damping = 1 - (PUNCH_DAMPING * gpGlobals->frametime);
		
		if ( damping < 0 )
		{
			damping = 0;
		}
		m_Local.m_vecPunchAngleVel *= damping;
		 
		// torsional spring
		// UNDONE: Per-axis spring constant?
		float springForceMagnitude = PUNCH_SPRING_CONSTANT * gpGlobals->frametime;
		springForceMagnitude = clamp(springForceMagnitude, 0, 2 );
		m_Local.m_vecPunchAngleVel -= m_Local.m_vecPunchAngle * springForceMagnitude;

		// don't wrap around
		m_Local.m_vecPunchAngle.Init( 
			clamp(m_Local.m_vecPunchAngle->x, -89, 89 ), 
			clamp(m_Local.m_vecPunchAngle->y, -179, 179 ),
			clamp(m_Local.m_vecPunchAngle->z, -89, 89 ) );
	}
	else
	{
		m_Local.m_vecPunchAngle.Init( 0, 0, 0 );
		m_Local.m_vecPunchAngleVel.Init( 0, 0, 0 );
	}
}

//================================================================================
// Devuelve si deberíamos usar el Viewmodel para ciertas acciones
//================================================================================
bool C_Player::ShouldUseViewModel()
{
    C_BaseViewModel *vm = GetViewModel();

    // No tenemos viewmodel
    if ( !vm )
        return false;

    ConVarRef vm_draw_always("vm_draw_always");

    if ( vm_draw_always.GetBool() )
        return true;

    return IsFirstPerson();
}

//================================================================================
// Devuelve si el jugador (solo local) esta en tercera persona
//================================================================================
bool C_Player::IsThirdPerson()
{
    Assert( IsLocalPlayer() );

    if ( !IsLocalPlayer() )
        return false;

    int nSlot = GetSplitScreenPlayerSlot();
    ACTIVE_SPLITSCREEN_PLAYER_GUARD( nSlot );

    return SDKInput()->CAM_IsThirdPerson( nSlot ) == 1;
}

//================================================================================
// Devuelve si el jugador (solo local) esta en primera persona
//================================================================================
bool C_Player::IsFirstPerson()
{
    Assert( IsLocalPlayer() );

    if ( !IsLocalPlayer() )
        return false;

    int nSlot = GetSplitScreenPlayerSlot();
    ACTIVE_SPLITSCREEN_PLAYER_GUARD( nSlot );

    return SDKInput()->CAM_IsThirdPerson( nSlot ) == 0;
}

//================================================================================
//================================================================================
bool C_Player::IsActiveSplitScreenPlayer()
{
    Assert( IsLocalPlayer() );

    if ( !IsLocalPlayer() )
        return false;

    return (GetSplitScreenViewPlayer() == this);
}

//================================================================================
//================================================================================
Vector C_Player::GetChaseCamViewOffset( CBaseEntity *target ) 
{
    C_Player *pPlayer = ToInPlayer( target );
	
	if ( pPlayer && pPlayer->IsAlive() )
	{
        // Agachado
		if ( pPlayer->IsCrouching() )
			return VEC_DUCK_VIEW;

        // Incapacitado
        if ( pPlayer->IsDejected() )
			return TheGameRules->GetInViewVectors()->m_vDejectedView;

		// Visión normal
		return VEC_VIEW;
	}

	// assume it's the players ragdoll
	return VEC_DEAD_VIEWHEIGHT;
}

//================================================================================
//================================================================================
void C_Player::CalcInEyeCamView( Vector &eyeOrigin, QAngle &eyeAngles, float &fov ) 
{
    //ACTIVE_SPLITSCREEN_PLAYER_GUARD( this );

    C_Player *pPlayer = ToInPlayer( GetObserverTarget() );

	if ( !pPlayer ) 
	{
		// just copy a save in-map position
		VectorCopy( EyePosition(), eyeOrigin );
		VectorCopy( EyeAngles(), eyeAngles );
		return;
	}

    pPlayer->CalcPlayerView( eyeOrigin, eyeAngles, fov );

	eyeOrigin.z = GetAbsOrigin().z;
	eyeOrigin.z += GetViewOffset().z;

	//VectorAdd( eyeAngles, pPlayer->m_Local.m_vecPunchAngle, eyeAngles );
}

//================================================================================
// Called in each frame to update the colors of the AlienFX lights.
//================================================================================
void C_Player::UpdateAlienFX()
{
    // Without life, we turn off the lights.
    if ( !IsAlive() ) {
        TheAlienFX->SetColor(LFX_ALL, LFX_BLACK | LFX_FULL_BRIGHTNESS);
        return;
    }

    // Front Light
    // Health
    {
        int health = GetHealth();

        if ( GetPlayerStatus() == PLAYER_STATUS_FALLING ) {
            TheAlienFX->UpdatePulse(0.05f);
            return;
        }
        else if ( IsDejected() ) {
            TheAlienFX->UpdatePulse(0.5f);
            return;
        }
        else if ( health < 5 ) {
            TheAlienFX->UpdatePulse(0.7f);
            return;
        }
        else if ( health < 20 ) {
            TheAlienFX->SetColor(LFX_ALL_FRONT, LFX_RED | LFX_FULL_BRIGHTNESS);
        }
        else if ( health < 50 ) {
            TheAlienFX->SetColor(LFX_ALL_FRONT, LFX_ORANGE | LFX_FULL_BRIGHTNESS);
        }
        else if ( health < 80 ) {
            TheAlienFX->SetColor(LFX_ALL_FRONT, LFX_YELLOW | LFX_FULL_BRIGHTNESS);
        }
        else {
            TheAlienFX->SetColor(LFX_ALL_FRONT, LFX_GREEN | LFX_FULL_BRIGHTNESS);
        }
    }

    // Right Light
    // Ammo
    {
        CBaseWeapon *pWeapon = GetBaseWeapon();

        if ( !pWeapon || pWeapon->IsMeleeWeapon() ) {
            TheAlienFX->SetColor(LFX_ALL_RIGHT, LFX_WHITE | LFX_HALF_BRIGHTNESS);
        }
        else {
            int ammo = GetAmmoCount(pWeapon->GetPrimaryAmmoType());
            int max = 200;

            if ( ammo < roundup(max*0.4) ) {
                TheAlienFX->SetColor(LFX_ALL_RIGHT, LFX_RED | LFX_FULL_BRIGHTNESS);
            }
            else if ( ammo < roundup(max*0.6) ) {
                TheAlienFX->SetColor(LFX_ALL_RIGHT, LFX_ORANGE | LFX_FULL_BRIGHTNESS);
            }
            else if ( ammo < roundup(max*0.8) ) {
                TheAlienFX->SetColor(LFX_ALL_RIGHT, LFX_YELLOW | LFX_FULL_BRIGHTNESS);
            }
            else {
                TheAlienFX->SetColor(LFX_ALL_RIGHT, LFX_GREEN | LFX_FULL_BRIGHTNESS);
            }
        }
    }

    // Left Light
    // Status
    {
        if ( IsUnderAttack() ) {
            TheAlienFX->SetColor(LFX_ALL_LEFT, LFX_RED | LFX_FULL_BRIGHTNESS);
        }
        else if ( IsOnCombat() ) {
            TheAlienFX->SetColor(LFX_ALL_LEFT, LFX_ORANGE | LFX_FULL_BRIGHTNESS);
        }
        else {
            TheAlienFX->SetColor(LFX_ALL_LEFT, LFX_CYAN | LFX_FULL_BRIGHTNESS);
        }
    }
}

//================================================================================
// Transmite la luz al disparar un arma de fuego
//================================================================================
void C_Player::ProcessMuzzleFlashEvent()
{
    return BaseClass::ProcessMuzzleFlashEvent();
}

//================================================================================
//================================================================================
void C_Player::DoPostProcessingEffects( PostProcessParameters_t &params )
{
	
}

//================================================================================
//================================================================================
void C_Player::DoStressContrastEffect( PostProcessParameters_t &params ) 
{
	C_Player *pPlayer = GetActivePlayer();

	float stress = pPlayer->GetStress();
	if ( stress <= 0 ) return;

	float flContrast = ( stress / 100 );
	flContrast = clamp( flContrast, 0.0f, 1.0f );

	params.m_flParameters[ PPPN_LOCAL_CONTRAST_STRENGTH ]		= flContrast;
	params.m_flParameters[ PPPN_LOCAL_CONTRAST_EDGE_STRENGTH ]  = 0.65f;
}

//================================================================================
// Purpose: Determines if a player can be safely moved towards a point
// Input:   pos - position to test move to, fVertDist - how far to trace downwards to see if the player would fall,
//            radius - how close the player can be to the object, objPos - position of the object to avoid,
//            objDir - direction the object is travelling
//================================================================================
bool C_Player::TestMove( const Vector &pos, float fVertDist, float radius, const Vector &objPos, const Vector &objDir )
{
    trace_t trUp;
    trace_t trOver;
    trace_t trDown;
    float flHit1, flHit2;
    
    UTIL_TraceHull( GetAbsOrigin(), pos, GetPlayerMins(), GetPlayerMaxs(), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &trOver );
    if ( trOver.fraction < 1.0f )
    {
        // check if the endpos intersects with the direction the object is travelling.  if it doesn't, this is a good direction to move.
        if ( objDir.IsZero() ||
            ( IntersectInfiniteRayWithSphere( objPos, objDir, trOver.endpos, radius, &flHit1, &flHit2 ) && 
            ( ( flHit1 >= 0.0f ) || ( flHit2 >= 0.0f ) ) )
            )
        {
            // our first trace failed, so see if we can go farther if we step up.

            // trace up to see if we have enough room.
            UTIL_TraceHull( GetAbsOrigin(), GetAbsOrigin() + Vector( 0, 0, m_Local.m_flStepSize ), 
                GetPlayerMins(), GetPlayerMaxs(), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &trUp );

            // do a trace from the stepped up height
            UTIL_TraceHull( trUp.endpos, pos + Vector( 0, 0, trUp.endpos.z - trUp.startpos.z ), 
                GetPlayerMins(), GetPlayerMaxs(), MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &trOver );

            if ( trOver.fraction < 1.0f )
            {
                // check if the endpos intersects with the direction the object is travelling.  if it doesn't, this is a good direction to move.
                if ( objDir.IsZero() ||
                    ( IntersectInfiniteRayWithSphere( objPos, objDir, trOver.endpos, radius, &flHit1, &flHit2 ) && ( ( flHit1 >= 0.0f ) || ( flHit2 >= 0.0f ) ) ) )
                {
                    return false;
                }
            }
        }
    }

    // trace down to see if this position is on the ground
    UTIL_TraceLine( trOver.endpos, trOver.endpos - Vector( 0, 0, fVertDist ), 
        MASK_SOLID_BRUSHONLY, NULL, COLLISION_GROUP_NONE, &trDown );

    if ( trDown.fraction == 1.0f ) 
        return false;

    return true;
}

//================================================================================
// Client-side obstacle avoidance
//================================================================================
void C_Player::PerformClientSideObstacleAvoidance( float flFrameTime, CUserCmd *pCmd )
{
    // Don't avoid if noclipping or in movetype none
    switch ( GetMoveType() )
    {
        case MOVETYPE_NOCLIP:
        case MOVETYPE_NONE:
        case MOVETYPE_OBSERVER:
            return;
        default:
            break;
    }

    // Try to steer away from any objects/players we might interpenetrate
    Vector size = WorldAlignSize();

    float radius = 0.7f * sqrt( size.x * size.x + size.y * size.y );
    float curspeed = GetLocalVelocity().Length2D();

    //int slot = 1;
    //engine->Con_NPrintf( slot++, "speed %f\n", curspeed );
    //engine->Con_NPrintf( slot++, "radius %f\n", radius );

    // If running, use a larger radius
    float factor = 1.0f;

    if ( curspeed > 150.0f )
    {
        curspeed = MIN( 2048.0f, curspeed );
        factor = ( 1.0f + ( curspeed - 150.0f ) / 150.0f );

        //engine->Con_NPrintf( slot++, "scaleup (%f) to radius %f\n", factor, radius * factor );

        radius = radius * factor;
    }

    Vector currentdir;
    Vector rightdir;

    QAngle vAngles = pCmd->viewangles;
    vAngles.x = 0;

    AngleVectors( vAngles, &currentdir, &rightdir, NULL );
        
    bool istryingtomove = false;
    bool ismovingforward = false;
    if ( fabs( pCmd->forwardmove ) > 0.0f || 
        fabs( pCmd->sidemove ) > 0.0f )
    {
        istryingtomove = true;
        if ( pCmd->forwardmove > 1.0f )
        {
            ismovingforward = true;
        }
    }

    if ( istryingtomove == true )
         radius *= 1.3f;

    CPlayerAndObjectEnumerator avoid( radius );
    partition->EnumerateElementsInSphere( PARTITION_CLIENT_SOLID_EDICTS, GetAbsOrigin(), radius, false, &avoid );

    // Okay, decide how to avoid if there's anything close by
    int c = avoid.GetObjectCount();
    if ( c <= 0 )
        return;

    //engine->Con_NPrintf( slot++, "moving %s forward %s\n", istryingtomove ? "true" : "false", ismovingforward ? "true" : "false"  );

    float adjustforwardmove = 0.0f;
    float adjustsidemove    = 0.0f;

    for ( int i = 0; i < c; i++ )
    {
        C_AI_BaseNPC *obj = dynamic_cast< C_AI_BaseNPC *>(avoid.GetObject( i ));

        if( !obj )
            continue;

        Vector vecToObject = obj->GetAbsOrigin() - GetAbsOrigin();

        float flDist = vecToObject.Length2D();
        
        // Figure out a 2D radius for the object
        Vector vecWorldMins, vecWorldMaxs;
        obj->CollisionProp()->WorldSpaceAABB( &vecWorldMins, &vecWorldMaxs );
        Vector objSize = vecWorldMaxs - vecWorldMins;

        float objectradius = 0.5f * sqrt( objSize.x * objSize.x + objSize.y * objSize.y );

        //Don't run this code if the NPC is not moving UNLESS we are in stuck inside of them.
        if ( !obj->IsMoving() && flDist > objectradius )
              continue;

        if ( flDist > objectradius && obj->IsEffectActive( EF_NODRAW ) )
        {
            obj->RemoveEffects( EF_NODRAW );
        }

        Vector vecNPCVelocity;
        obj->EstimateAbsVelocity( vecNPCVelocity );
        float flNPCSpeed = VectorNormalize( vecNPCVelocity );

        Vector vPlayerVel = GetAbsVelocity();
        VectorNormalize( vPlayerVel );

        float flHit1, flHit2;
        Vector vRayDir = vecToObject;
        VectorNormalize( vRayDir );

        float flVelProduct = DotProduct( vecNPCVelocity, vPlayerVel );
        float flDirProduct = DotProduct( vRayDir, vPlayerVel );

        if ( !IntersectInfiniteRayWithSphere(
                GetAbsOrigin(),
                vRayDir,
                obj->GetAbsOrigin(),
                radius,
                &flHit1,
                &flHit2 ) )
            continue;

        Vector dirToObject = -vecToObject;
        VectorNormalize( dirToObject );

        float fwd = 0;
        float rt = 0;

        float sidescale = 2.0f;
        float forwardscale = 1.0f;
        bool foundResult = false;

        Vector vMoveDir = vecNPCVelocity;
        if ( flNPCSpeed > 0.001f )
        {
            // This NPC is moving. First try deflecting the player left or right relative to the NPC's velocity.
            // Start with whatever side they're on relative to the NPC's velocity.
            Vector vecNPCTrajectoryRight = CrossProduct( vecNPCVelocity, Vector( 0, 0, 1) );
            int iDirection = ( vecNPCTrajectoryRight.Dot( dirToObject ) > 0 ) ? 1 : -1;
            for ( int nTries = 0; nTries < 2; nTries++ )
            {
                Vector vecTryMove = vecNPCTrajectoryRight * iDirection;
                VectorNormalize( vecTryMove );
                
                Vector vTestPosition = GetAbsOrigin() + vecTryMove * radius * 2;

                if ( TestMove( vTestPosition, size.z * 2, radius * 2, obj->GetAbsOrigin(), vMoveDir ) )
                {
                    fwd = currentdir.Dot( vecTryMove );
                    rt = rightdir.Dot( vecTryMove );
                    
                    //Msg( "PUSH DEFLECT fwd=%f, rt=%f\n", fwd, rt );
                    
                    foundResult = true;
                    break;
                }
                else
                {
                    // Try the other direction.
                    iDirection *= -1;
                }
            }
        }
        else
        {
            // the object isn't moving, so try moving opposite the way it's facing
            Vector vecNPCForward;
            obj->GetVectors( &vecNPCForward, NULL, NULL );
            
            Vector vTestPosition = GetAbsOrigin() - vecNPCForward * radius * 2;
            if ( TestMove( vTestPosition, size.z * 2, radius * 2, obj->GetAbsOrigin(), vMoveDir ) )
            {
                fwd = currentdir.Dot( -vecNPCForward );
                rt = rightdir.Dot( -vecNPCForward );

                if ( flDist < objectradius )
                {
                    obj->AddEffects( EF_NODRAW );
                }

                //Msg( "PUSH AWAY FACE fwd=%f, rt=%f\n", fwd, rt );

                foundResult = true;
            }
        }

        if ( !foundResult )
        {
            // test if we can move in the direction the object is moving
            Vector vTestPosition = GetAbsOrigin() + vMoveDir * radius * 2;
            if ( TestMove( vTestPosition, size.z * 2, radius * 2, obj->GetAbsOrigin(), vMoveDir ) )
            {
                fwd = currentdir.Dot( vMoveDir );
                rt = rightdir.Dot( vMoveDir );

                if ( flDist < objectradius )
                {
                    obj->AddEffects( EF_NODRAW );
                }

                //Msg( "PUSH ALONG fwd=%f, rt=%f\n", fwd, rt );

                foundResult = true;
            }
            else
            {
                // try moving directly away from the object
                Vector vTestPosition = GetAbsOrigin() - dirToObject * radius * 2;
                if ( TestMove( vTestPosition, size.z * 2, radius * 2, obj->GetAbsOrigin(), vMoveDir ) )
                {
                    fwd = currentdir.Dot( -dirToObject );
                    rt = rightdir.Dot( -dirToObject );
                    foundResult = true;

                    //Msg( "PUSH AWAY fwd=%f, rt=%f\n", fwd, rt );
                }
            }
        }

        if ( !foundResult )
        {
            // test if we can move through the object
            Vector vTestPosition = GetAbsOrigin() - vMoveDir * radius * 2;
            fwd = currentdir.Dot( -vMoveDir );
            rt = rightdir.Dot( -vMoveDir );

            if ( flDist < objectradius )
            {
                obj->AddEffects( EF_NODRAW );
            }

            //Msg( "PUSH THROUGH fwd=%f, rt=%f\n", fwd, rt );

            foundResult = true;
        }

        // If running, then do a lot more sideways veer since we're not going to do anything to
        //  forward velocity
        if ( istryingtomove )
        {
            sidescale = 6.0f;
        }

        if ( flVelProduct > 0.0f && flDirProduct > 0.0f )
        {
            sidescale = 0.1f;
        }

        float force = 1.0f;
        float forward = forwardscale * fwd * force * AVOID_SPEED;
        float side = sidescale * rt * force * AVOID_SPEED;

        adjustforwardmove    += forward;
        adjustsidemove        += side;
    }

    pCmd->forwardmove    += adjustforwardmove;
    pCmd->sidemove        += adjustsidemove;
    
    // Clamp the move to within legal limits, preserving direction. This is a little
    // complicated because we have different limits for forward, back, and side

    //Msg( "PRECLAMP: forwardmove=%f, sidemove=%f\n", pCmd->forwardmove, pCmd->sidemove );

    float flForwardScale = 1.0f;
    if ( pCmd->forwardmove > fabs( cl_forwardspeed.GetFloat() ) )
    {
        flForwardScale = fabs( cl_forwardspeed.GetFloat() ) / pCmd->forwardmove;
    }
    else if ( pCmd->forwardmove < -fabs( cl_backspeed.GetFloat() ) )
    {
        flForwardScale = fabs( cl_backspeed.GetFloat() ) / fabs( pCmd->forwardmove );
    }
    
    float flSideScale = 1.0f;
    if ( fabs( pCmd->sidemove ) > fabs( cl_sidespeed.GetFloat() ) )
    {
        flSideScale = fabs( cl_sidespeed.GetFloat() ) / fabs( pCmd->sidemove );
    }
    
    float flScale = MIN( flForwardScale, flSideScale );
    pCmd->forwardmove *= flScale;
    pCmd->sidemove *= flScale;

    //Msg( "POSTCLAMP: forwardmove=%f, sidemove=%f\n", pCmd->forwardmove, pCmd->sidemove );
}

//================================================================================
// Procesa el Input del Jugador
//================================================================================
bool C_Player::CreateMove( float flInputSampleTime, CUserCmd *ucmd )
{
    if ( !IsActive() )
        return BaseClass::CreateMove( flInputSampleTime, ucmd );

    // Visión
    // TODO: Animación para moverse a esta posición
    ucmd->viewangles.z = m_iEyeAngleZ;

    // Botones desactivados/forzados
    ucmd->buttons |= m_iButtonsForced;
    ucmd->buttons &= ~m_iButtonsDisabled;        

    // Ahora mismo no podemos movernos ni mover la cámara
    if ( m_bIsBot || GetPlayerStatus() == PLAYER_STATUS_FALLING )
    {
        ucmd->forwardmove = 0;
        ucmd->sidemove = 0;
        ucmd->upmove = 0;
        ucmd->buttons = 0;
        ucmd->viewangles = GetAbsAngles();
        //engine->SetViewAngles( ucmd->viewangles );
        return true;
    }

    // No podemos mover la cámara
    if ( IsAimingDisabled() )
    {
        ucmd->viewangles = m_vecOldViewAngles;
        engine->SetViewAngles( ucmd->viewangles );
    }

    // Movimiento desactivado
    if ( IsMovementDisabled() )
    {
        ucmd->forwardmove = 0;
        ucmd->sidemove = 0;
        ucmd->upmove = 0;
        ucmd->buttons &= ~(IN_JUMP | IN_DUCK);
    }
    else
    {
#ifdef APOCALYPSE
        // TODO: Animación
        if ( ucmd->sidemove < 0 )
            ucmd->viewangles.z -= .5f;

        if ( ucmd->sidemove > 0 )
            ucmd->viewangles.z += .5f;
#endif
    }

    // Base!
    bool result = BaseClass::CreateMove( flInputSampleTime, ucmd );

    // TODO: ?
    if ( !IsInAVehicle() && !IsObserver() )
        PerformClientSideObstacleAvoidance( TICK_INTERVAL, ucmd );

    return result;
}