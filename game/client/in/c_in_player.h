//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef C_IN_PLAYER_H
#define C_IN_PLAYER_H

#ifdef _WIN32
#pragma once
#endif

#include "in_playeranimsystem.h"
#include "c_baseplayer.h"

#include "baseparticleentity.h"
#include "c_basetempentity.h"

#include "beamdraw.h"
#include "beam_shared.h"
#include "flashlighteffect.h"

#include "weapon_base.h"

namespace vgui
{
    class Frame;
};

//================================================================================
// Tipos de linterna
//================================================================================
enum
{
    FLASHLIGHT_NORMAL = 0,
    FLASHLIGHT_DEJECTED,
    FLASHLIGHT_MUZZLEFLASH,

    LAST_FLASHLIGHT
};

#define GetFlashlight( name ) m_Flashlight[FLASHLIGHT_##name]
#define GetFlashlightBeam( name ) m_FlashlightBeam[FLASHLIGHT_##name]

//================================================================================
// Base para la creación de Jugadores
//================================================================================
class C_Player : public C_BasePlayer
{
    DECLARE_CLASS( C_Player, C_BasePlayer );
    DECLARE_CLIENTCLASS();
    DECLARE_PREDICTABLE();
    DECLARE_INTERPOLATION();

public:
    C_Player();
    ~C_Player();

    // Utilidades
	virtual bool IsCrouching() const {
		return (GetFlags() & FL_DUCKING) ? true : false;
	}

	virtual bool IsOnGround() const {
		return (GetFlags() & FL_ONGROUND) ? true : false;
	}

	virtual bool IsOnGodMode() const {
		return (GetFlags() & FL_GODMODE) ? true : false;
	}

	virtual bool IsUnderAttack() const {
		return m_bUnderAttack;
	}

	virtual bool IsOnCombat() const {
		return m_bOnCombat;
	}

    virtual int GetButtons() { return m_nButtons; }
    virtual bool IsButtonPressing( int btn ) { return ((m_nButtons & btn)) ? true : false; }
    virtual bool IsButtonPressed( int btn ) { return ((m_afButtonPressed & btn)) ? true : false; }
    virtual bool IsButtonReleased( int btn ) { return ((m_afButtonReleased & btn)) ? true : false; }

    // Estaticos
    static C_Player *GetLocalInPlayer( int nSlot = -1 );
    virtual bool ShouldRegenerateOriginFromCellBits() const;

    // Principales
    virtual void PreThink();
    virtual void PostThink();
    virtual bool Simulate();

    virtual void UpdateLookAt();
    virtual void TakeDamage( const CTakeDamageInfo &info );

    // Daño
    virtual bool ShouldBleed( const CTakeDamageInfo &info, int hitgroup = HITGROUP_GENERIC );
    virtual void TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );

    // Velocidad
    virtual float GetSpeed();
    virtual void SpeedModifier( float &speed );
    virtual void UpdateSpeed();

    // Aguante
    virtual float GetStamina() { return 0.0f; }

    // Estres
    virtual float GetStress() { return 0.0f; }

    virtual float GetLocalStress() { return m_flLocalStress; }
    virtual void SetLocalStress( float level );
    virtual void AddLocalStress( float level );
    virtual void UpdateLocalStress();

    // Correr y Caminar
    virtual void UpdateMovementType();

    virtual bool CanSprint();
    virtual void StartSprint() { m_bSprinting = true; }
    virtual void StopSprint() { m_bSprinting = false; }
    virtual bool IsSprinting() { return m_bSprinting; }

    virtual bool CanSneak();
    virtual void StartSneaking() {
		m_bSneaking = true; }
    virtual void StopSneaking() {
		m_bSneaking = false; }
    virtual bool IsSneaking() { return m_bSneaking; }

    // Condición
    virtual int GetDejectedTimes() { return m_iDejectedTimes; }
    virtual float GetHelpProgress() { return m_flHelpProgress; }
    virtual float GetClimbingHold() { return m_flClimbingHold; }

    virtual bool IsDejected() { return (m_iPlayerStatus == PLAYER_STATUS_DEJECTED || m_iPlayerStatus == PLAYER_STATUS_CLIMBING); }
    virtual int GetPlayerStatus() { return m_iPlayerStatus; }
    virtual int GetPlayerStatus() const { return m_iPlayerStatus; }

    // Estado
    virtual int GetPlayerState() { return m_iPlayerState; }
    virtual int GetPlayerState() const { return m_iPlayerState; }
    virtual bool IsActive() { return (m_iPlayerState == PLAYER_STATE_ACTIVE); }

    // Clase
    virtual int GetPlayerClass() { return m_iPlayerClass; }
    virtual int GetPlayerClass() const { return m_iPlayerClass; }

    // Sonidos/Música
    virtual float SoundDesire( const char *soundName, int channel ) { return 0.0f; }
    virtual void OnSoundPlay( const char *soundName ) { }
    virtual void OnSoundStop( const char *soundName ) { }

    // Posición y Render
    virtual C_Player *GetActivePlayer( int mode = OBS_MODE_IN_EYE );
    virtual C_BaseAnimating *GetRagdoll();

    virtual bool IsLocalPlayerWatchingMe( int mode = OBS_MODE_IN_EYE );
    //virtual bool IsSplitScreenPartnerWatchingMe( int mode = OBS_MODE_IN_EYE );

    virtual const QAngle &EyeAngles();
    virtual const QAngle &GetRenderAngles();

    virtual void UpdateVisibility();

    virtual IClientModelRenderable*	GetClientModelRenderable();
    virtual bool ShouldDrawLocalPlayer();
    virtual bool ShouldDrawExternalPlayer();
    virtual bool ShouldDraw();
    virtual bool ShouldForceDrawInFirstPersonForShadows();

    virtual int	DrawModel( int flags, const RenderableInstance_t &instance );

    virtual ShadowType_t ShadowCastType();
    virtual bool ShouldReceiveProjectedTextures( int flags );

    // Armas
    C_BaseWeapon *GetBaseWeapon();

    // Animaciones
    virtual CPlayerAnimationSystem *AnimationSystem() { return m_pAnimationSystem; }
    virtual void CreateAnimationSystem();

    virtual void SetAnimation( PLAYER_ANIM );
    virtual void DoAnimationEvent( PlayerAnimEvent_t nEvent, int nData = 0, bool bPredicted = false );

    virtual void UpdateClientSideAnimation();
    virtual void UpdatePoseParams() { }

    virtual void PostDataUpdate( DataUpdateType_t );
    virtual void OnDataChanged( DataUpdateType_t );

    virtual CStudioHdr *OnNewModel();
    virtual void InitializePoseParams();

    // Linterna
    virtual int FlashlightIsOn() { return IsEffectActive(EF_DIMLIGHT); }

    virtual const char *GetFlashlightTextureName() const;
    virtual const char *GetFlashlightWeaponAttachment();
    virtual float GetFlashlightFOV() const;
    virtual float GetFlashlightFarZ();
    virtual float GetFlashlightLinearAtten();
    virtual void GetFlashlightOffset( const Vector &vecForward, const Vector &vecRight, const Vector &vecUp, Vector *pVecOffset ) const;

    virtual bool CreateFlashlight();
    virtual void DestroyFlashlight();
    virtual void UpdateFlashlight();

    virtual bool CreateBeam( Vector vecStart, Vector vecEnd );
    virtual void DestroyBeam();
    virtual void UpdateBeam();

    virtual void ShowMuzzleFlashlight();

    virtual void GetFlashlightPosition( C_Player *pPlayer, Vector &vecPosition, Vector &vecForward, Vector &vecRight, Vector &vecUp, bool bFromWeapon = true, const char *pAttachment = NULL );

    // Camara
	virtual void DecayPunchAngle();
	
    virtual bool ShouldUseViewModel();

    virtual bool IsThirdPerson();
    virtual bool IsFirstPerson();
    virtual bool IsActiveSplitScreenPlayer();

    virtual Vector GetChaseCamViewOffset( CBaseEntity *target );

    virtual bool GetEyesView( C_BaseAnimating *pEntity, Vector& eyeOrigin, QAngle& eyeAngles, int secureDistance );
    virtual void CalcPlayerView( Vector& eyeOrigin, QAngle& eyeAngles, float& fov );

    virtual void CalcInEyeCamView( Vector& eyeOrigin, QAngle& eyeAngles, float& fov );

    // Efectos
    virtual bool IsMovementDisabled() { return m_bMovementDisabled; }
    virtual bool IsAimingDisabled() { return m_bAimingDisabled; }

    virtual void ProcessMuzzleFlashEvent();

    virtual void DoPostProcessingEffects( PostProcessParameters_t &params );
	virtual void DoStressContrastEffect( PostProcessParameters_t &params );

    virtual bool TestMove( const Vector &pos, float fVertDist, float radius, const Vector &objPos, const Vector &objDir );
    virtual void PerformClientSideObstacleAvoidance( float flFrameTime, CUserCmd *pCmd );
    virtual bool CreateMove( float flInputSampleTime, CUserCmd *pCmd );

    // Collision Bounds
    virtual const Vector GetPlayerMins( void ) const;
    virtual const Vector GetPlayerMaxs( void ) const;
    virtual void UpdateCollisionBounds( void );

public:
    // Compartido
    virtual Activity TranslateActivity( Activity actBase );

    virtual Vector Weapon_ShootPosition();
    virtual Vector Weapon_ShootDirection();

	virtual void FireBullets( const FireBulletsInfo_t &info );
	virtual void OnFireBullets( const FireBulletsInfo_t &info );
    virtual bool ShouldDrawUnderwaterBulletBubbles();

public:
    QAngle m_angEyeAngles;
    CInterpolatedVar<QAngle> m_iv_angEyeAngles;

    CFlashlightEffect *m_Flashlight[LAST_FLASHLIGHT]; // Linternas
    Beam_t *m_FlashlightBeam[LAST_FLASHLIGHT];

    bool m_bMovementDisabled;
    bool m_bAimingDisabled;
    int m_iButtonsDisabled;
    int m_iButtonsForced;

    int m_iOldHealth;

    bool m_bFlashlightEnabled;
    bool m_bSprinting;
    bool m_bSneaking;
	float m_flLocalStress;

    bool m_bOnCombat;
    bool m_bUnderAttack;

    IntervalTimer m_UnderAttackTimer;
    IntervalTimer m_OnCombatTimer;

    int m_iPlayerStatus;
    int m_iPlayerState;
    int m_iPlayerClass;

    int m_iDejectedTimes;
    float m_flHelpProgress;
    float m_flClimbingHold;

    int m_iEyeAngleZ;
    bool m_bIsBot;

    CountdownTimer m_nBlinkTimer;

protected:
    CPlayerAnimationSystem *m_pAnimationSystem;

private:
    C_Player( const C_Player & );
};

inline C_Player *ToInPlayer( CBaseEntity *pPlayer )
{
    if ( !pPlayer )
        return NULL;

    if ( !pPlayer->IsPlayer() )
        return NULL;

    return dynamic_cast<C_Player *>( pPlayer );
}

/*
inline C_Player *ToInPlayer( edict_t *pClient )
{
    CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance( pClient );
    return ToInPlayer( pPlayer );
}
*/

#endif // C_IN_PLAYER_H