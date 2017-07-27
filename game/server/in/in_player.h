//==== Woots 2017. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef IN_PLAYER_H
#define IN_PLAYER_H

#ifdef _WIN32
#pragma once
#endif

#include "player.h"

#include "ai_speech.h"
#include "ai_senses.h"

#include "in_ragdoll.h"
#include "in_shareddefs.h"
#include "in_playeranimsystem.h"

#include "weapon_base.h"
#include "in_attribute.h"

class CBot;
class CSquad;
class CPlayerComponent;
class CBaseProp;

//================================================================================
// Macros
//================================================================================

#define CONVERT_PLAYER_FUNCTION( classname, acrom ) \
    inline classname *To##acrom##Player( CBaseEntity *pPlayer ) { \
        if ( !pPlayer ) \
            return NULL; \
        if ( !pPlayer->IsPlayer() ) \
            return NULL; \
        return dynamic_cast<classname *>( pPlayer ); \
    } \
    inline classname *To##acrom##Player( edict_t *pClient ) { \
        CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance( pClient ); \
        return To##acrom##Player( pPlayer ); \
    } \
    inline classname *To##acrom##Player( int index ) { \
        CBasePlayer *pPlayer = UTIL_PlayerByIndex(index); \
        return To##acrom##Player( pPlayer ); \
    }

//================================================================================
// Base para la creación de Jugadores
//================================================================================
class CPlayer : public CAI_ExpresserHost<CBasePlayer>
{
    DECLARE_CLASS( CPlayer, CAI_ExpresserHost<CBasePlayer> );
    DECLARE_SERVERCLASS();
    DECLARE_DATADESC();

public:
    CPlayer();
    ~CPlayer();
    
    virtual Class_T Classify() { return CLASS_PLAYER; }

    // Utilidades
    virtual bool IsCrouching()  { return (GetFlags() & FL_DUCKING ) ? true : false; }
    virtual bool IsCrouching() const  { return (GetFlags() & FL_DUCKING ) ? true : false; }

    virtual bool IsInGround() { return (GetFlags() & FL_ONGROUND) ? true : false; }
    virtual bool InGodMode() { return (GetFlags() & FL_GODMODE) ? true : false; }
    virtual bool InBuddhaMode() { return (m_debugOverlays & OVERLAY_BUDDHA_MODE) ? true : false; }

	virtual bool IsIdle();
	virtual bool IsAlerted();

    virtual bool IsUnderAttack() { return m_bIsUnderAttack; }
    virtual bool IsInCombat() { return m_bIsInCombat; }

    virtual int GetButtons() { return m_nButtons; }
    virtual bool IsButtonPressing( int btn ) { return ((m_nButtons & btn)) ? true : false; }
    virtual bool IsButtonPressed( int btn ) { return ((m_afButtonPressed & btn)) ? true : false; }
    virtual bool IsButtonReleased( int btn ) { return ((m_afButtonReleased & btn)) ? true : false; }

    // Principales
	//virtual IBotController *GetBotController() { return m_nAIController; }
    template <typename AI>
    AI *GetAIController();
    template <typename AI>
    AI *GetAIController() const;

    virtual CBot *GetAI() { return m_nAIController; }
    virtual CBot *GetAI() const { return m_nAIController; }

    virtual void SetUpAI();
    virtual void SetAI( CBot *pBot );

    virtual int UpdateTransmitState();

    virtual void InitialSpawn();
    virtual void Spawn();
    virtual void EnterToGame( bool forceForBots = false );

    virtual void Connected();

    virtual void PostConstructor( const char * );
    virtual void Precache();

    virtual void PreThink();
	virtual void PlayerThink() { }
    virtual void PostThink();
	virtual void PushawayThink();

    virtual CBaseEntity *GetEnemy();
    virtual CBaseEntity *GetEnemy() const;

    // Tipo de Jugador
    virtual const char *GetPlayerModel();
    virtual void SetUpModel() { }
    virtual const char *GetPlayerType();
    virtual gender_t GetPlayerGender();

    // Cadaver
    virtual CInRagdoll *GetRagdoll();
    virtual bool BecomeRagdollOnClient( const Vector & );
    virtual void CreateRagdollEntity();
    virtual void DestroyRagdoll();

    // Componentes
    template <typename COMPONENT>
    COMPONENT *GetComponent();

    virtual CPlayerComponent *GetComponent( int id );

    virtual void AddComponent( int id );
    virtual void AddComponent( CPlayerComponent *pComponent );

    virtual void CreateComponents();
    virtual void UpdateComponents();

	// Atributos
	virtual void AddAttribute( const char *name );
	virtual void AddAttribute( CAttribute *pAttribute );

	virtual void CreateAttributes();

	virtual void PreUpdateAttributes();
	virtual void UpdateAttributes();

	virtual CAttribute *GetAttribute( const char *name );
	virtual void AddAttributeModifier( const char *name );

    // Armas
    CBaseWeapon *GetBaseWeapon();

	virtual CBaseEntity	*GiveNamedItem( const char *szName, int iSubType = 0, bool removeIfNotCarried = true );

	//virtual bool Weapon_HasWeapon();

    // Expresser
    virtual void CreateExpresser();
    virtual void ModifyOrAppendCriteria( AI_CriteriaSet & );
    //virtual IResponseSystem *GetResponseSystem() { return BaseClass::GetResponseSystem(); }
    
    virtual bool SpeakIfAllowed( AIConcept_t concept, const char *modifiers = NULL, char *pszOutResponseChosen = NULL, size_t bufsize = 0, IRecipientFilter *filter = NULL );
    virtual bool SpeakConcept( AI_Response& response, int iConcept );
    virtual bool SpeakConceptIfAllowed( int iConcept, const char *modifiers = NULL, char *pszOutResponseChosen = NULL, size_t bufsize = 0, IRecipientFilter *filter = NULL );

    virtual bool CanHearAndReadChatFrom( CBasePlayer * );
    virtual bool CanSpeak() { return true; }

    virtual CAI_Expresser *GetExpresser() { return m_pExpresser; }
    virtual CMultiplayer_Expresser *GetMultiplayerExpresser() { return m_pExpresser; }

    // Senses
    CAI_Senses *GetSenses() { return m_pSenses; }
    const CAI_Senses *GetSenses() const { return m_pSenses; }

    virtual void CreateSenses();

    virtual void SetDistLook( float flDistLook );
    virtual bool ShouldIgnoreSound( CSound *pSound ) { return false; }

    virtual int GetSoundInterests();
    virtual int GetSoundPriority( CSound *pSound );

    virtual bool QueryHearSound( CSound *pSound );
    virtual bool QuerySeeEntity( CBaseEntity *pEntity, bool bOnlyHateOrFearIfNPC = false );

    virtual void OnLooked( int iDistance );
    virtual void OnListened();

    virtual void OnSeeEntity( CBaseEntity *pEntity ) { }
    virtual bool ShouldNotDistanceCull() { return false; }

    virtual CSound *GetLoudestSoundOfType( int iType );
    virtual bool SoundIsVisible( CSound *pSound );

    virtual CSound *GetBestSound( int validTypes = ALL_SOUNDS );
    virtual CSound *GetBestScent( void );

    virtual float HearingSensitivity() { return 1.0; }
    virtual bool OnlySeeAliveEntities( void ) { return true; }

    // Animaciones
    virtual CPlayerAnimationSystem *AnimationSystem() { return m_pAnimState; }
    virtual void HandleAnimEvent( animevent_t *event );

    virtual void FixAngles();
    virtual void SetAnimation( PLAYER_ANIM );

    virtual void DoAnimationEvent( PlayerAnimEvent_t nEvent, int nData = 0, bool bPredicted = false );

    // Sonidos
	virtual float SoundDesire( const char *soundname, int channel ) { return 0.0f; }

	virtual void OnSoundPlay( const char *soundname ) { }
	virtual void OnSoundStop( const char *soundname ) { }

    virtual void EmitPlayerSound( const char * );
    virtual void StopPlayerSound( const char * );

    virtual void FootstepSound();
    virtual void PainSound( const CTakeDamageInfo & );
    virtual void DeathSound( const CTakeDamageInfo & );

    // Flashlight y efectos
    virtual void SetFlashlightEnabled( bool );
    virtual int FlashlightIsOn();
    virtual void FlashlightTurnOn();
    virtual void FlashlightTurnOff();

    // Daño
    virtual CTakeDamageInfo GetLastDamage() { return m_nLastDamageInfo; }
    virtual IntervalTimer GetLastDamageTimer() { return m_nLastDamageTimer; }

    virtual bool ShouldBleed( const CTakeDamageInfo &info, int hitgroup );
    virtual void TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );
    virtual void DamageEffect( const CTakeDamageInfo &inputInfo );
	virtual bool CanTakeDamage( const CTakeDamageInfo &inputInfo );

	virtual int OnTakeDamage( const CTakeDamageInfo &inputInfo );
    virtual int OnTakeDamage_Alive( CTakeDamageInfo &inputInfo );
	virtual void ApplyDamage( const CTakeDamageInfo &inputInfo );

    virtual void DropResources();

    virtual void Event_Killed( const CTakeDamageInfo &info );
	virtual bool Event_Gibbed( const CTakeDamageInfo &info );
	virtual void Event_Dying( const CTakeDamageInfo &info );

    virtual void PlayerDeathThink();
    virtual void PlayerDeathPostThink();

    // Condición
    virtual int GetDejectedTimes() { return m_iDejectedTimes; }
    virtual float GetHelpProgress() { return m_flHelpProgress; }
    virtual float GetClimbingHold() { return m_flClimbingHold; }

    virtual bool IsDejected() { return (m_iPlayerStatus == PLAYER_STATUS_DEJECTED || m_iPlayerStatus == PLAYER_STATUS_CLIMBING); }
    virtual bool IsBeingHelped() { return (m_flHelpProgress > 0.0f); }

    virtual int GetPlayerStatus() { return m_iPlayerStatus; }
    virtual int GetPlayerStatus() const { return m_iPlayerStatus; }

    virtual void OnPlayerStatus( int oldStatus, int status );
    virtual void SetPlayerStatus( int status );

    virtual void RaiseFromDejected( CPlayer * );

    // Estado
    virtual int GetPlayerState() { return m_iPlayerState; }
    virtual int GetPlayerState() const { return m_iPlayerState; }
    virtual bool IsActive() { return (m_iPlayerState == PLAYER_STATE_ACTIVE); }

    virtual void EnterPlayerState( int status );
    virtual void LeavePlayerState( int status ) { }
    virtual void SetPlayerState( int status );

    // Clase
    virtual int GetPlayerClass() { return m_iPlayerClass; }
    virtual int GetPlayerClass() const { return m_iPlayerClass; }

    virtual void OnPlayerClass( int playerClass ) { }
    virtual void SetPlayerClass( int playerClass );

    // Velocidad
    virtual float GetSpeed();
    virtual void SpeedModifier( float &speed );
    virtual void UpdateSpeed();

    // Aguante
    virtual float GetStamina();

    // Estres
    virtual float GetStress();

    // Correr y Caminar
    virtual void UpdateMovementType();

    virtual bool AllowSprint();
    virtual void StartSprint() { m_bSprinting = true; }
    virtual void StopSprint() { m_bSprinting = false; }
    virtual bool IsSprinting() { return m_bSprinting; }

    virtual bool AllowWalk();
    virtual void StartWalking() { m_bWalking = true; }
    virtual void StopWalking() { m_bWalking = false; }
    virtual bool IsWalking() { return m_bWalking; }

    virtual void Jump();

    // Viewmodel
    virtual void CreateViewModel( int viewmodelindex = 0 );

    virtual void SetUpHands();
    virtual void CreateHands( int handsindex, int viewmodelparent = 0 );
    virtual const char *GetHandsModel( int handsindex );

    // Camara
    virtual bool GetEyesView( CBaseAnimating *pEntity, Vector& eyeOrigin, QAngle& eyeAngles, int secureDistance );
    virtual void CalcPlayerView( Vector& eyeOrigin, QAngle& eyeAngles, float& fov );

    // Collision Bounds
    virtual const Vector GetPlayerMins() const;
    virtual const Vector GetPlayerMaxs() const;
    virtual void UpdateCollisionBounds();

    // Escuadron
    virtual CSquad *GetSquad() { return m_nSquad; }
    virtual void SetSquad( CSquad *pSquad );
    virtual void SetSquad( const char *name );

    virtual void OnNewLeader( CPlayer *pMember );
    virtual void OnMemberTakeDamage( CPlayer *pMember, const CTakeDamageInfo &info );
    virtual void OnMemberDeath( CPlayer *pMember, const CTakeDamageInfo &info );
    virtual void OnMemberReportEnemy( CPlayer *pMember, CBaseEntity *pEnemy );

    // Utilidades
    virtual bool FEyesVisible( CBaseEntity *pEntity, int traceMask = MASK_VISIBLE, CBaseEntity **ppBlocker = NULL );
    virtual bool FEyesVisible( const Vector &vecTarget, int traceMask = MASK_VISIBLE, CBaseEntity **ppBlocker = NULL );

    virtual const char* GetCommandValue( const char * );
    virtual void ExecuteCommand( const char * );

	virtual float GetApproximateFallDamage( float height ) const;
	virtual bool ShouldAutoaim();

    virtual int GetDifficultyLevel();

    virtual void Kick();
    virtual bool Possess( CPlayer *pOther );
    virtual void Spectate( CPlayer * = NULL );

    virtual bool IsLocalPlayerWatchingMe();
	virtual CPlayer *GetActivePlayer();

    virtual void ImpulseCommands();
    virtual void CheatImpulseCommands( int iImpulse );
    virtual bool ClientCommand( const CCommand & );

    virtual const char *GetSpawnEntityName();
    virtual CBaseEntity *EntSelectSpawnPoint();

    virtual void PhysObjectSleep();
    virtual void PhysObjectWake();

    virtual bool IsMovementDisabled() { return m_bMovementDisabled; }
    virtual void EnableMovement() { m_bMovementDisabled = false; }
    virtual void DisableMovement() { m_bMovementDisabled = true; }

    virtual bool IsAimingDisabled() { return m_bAimingDisabled; }
    virtual void EnableAiming() { m_bAimingDisabled = false; }
    virtual void DisableAiming() { m_bAimingDisabled = true; }

    virtual void DisableButtons( int nButtons );
    virtual void EnableButtons( int nButtons );
    virtual void RestoreButtons();

    virtual void ForceButtons( int nButtons );
    virtual void UnforceButtons( int nButtons );

    virtual void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
    virtual void Touch( CBaseEntity *pOther );

    virtual void InjectMovement( NavRelativeDirType direction );
    virtual void InjectButton( int btn );
    virtual void PlayerRunCommand( CUserCmd *ucmd, IMoveHelper *moveHelper );

    virtual int ObjectCaps();
    virtual void SnapEyeAnglesZ( int );

    // Debug
    virtual void DebugAddMessage( char *format, ... );
    virtual void DebugDisplay();
    virtual void DebugScreenText( const char *pText, Color color = Color(255, 255, 255, 150), float yPosition = -1, float duration = 0.15f );

public:
    // Compartido
    virtual void CreateAnimationSystem();
    virtual Activity TranslateActivity( Activity actBase );

    virtual Vector Weapon_ShootPosition();
    virtual Vector Weapon_ShootDirection();

    virtual void FireBullets( const FireBulletsInfo_t & );
    virtual bool ShouldDrawUnderwaterBulletBubbles();

public:
    CNetworkVar( float, m_flClimbingHold );

protected:
    CPlayerAnimationSystem *m_pAnimState;
    CTakeDamageInfo m_nLastDamageInfo;
    IntervalTimer m_nLastDamageTimer;

    CBaseEntity *m_nSpawnSpot;
    int m_iCurrentConcept;
    
    bool m_bPlayingDeathAnim;

    CMultiplayer_Expresser *m_pExpresser;
    EHANDLE m_nRagdoll;

    CBot *m_nAIController;

    IntervalTimer m_nSlowDamageTimer;
    IntervalTimer m_nIsUnderAttackTimer;
    IntervalTimer m_nIsInCombatTimer;
    IntervalTimer m_nRaiseHelpTimer;

    CUtlVector<CPlayerComponent *> m_nComponents;
	CUtlVector<CAttribute *> m_nAttributes;

    CAI_Senses *m_pSenses;
    CSquad *m_nSquad;

    CUserCmd *m_InjectedCommand;
	
    float m_flDebugPosition;
    CUtlVector<DebugMessage> m_debugMessages;
    
    CNetworkVar( bool, m_bMovementDisabled );
    CNetworkVar( bool, m_bAimingDisabled );
    CNetworkVar( int, m_iButtonsDisabled );
    CNetworkVar( int, m_iButtonsForced );

    CNetworkVar( int, m_iOldHealth );

    CNetworkVar( bool, m_bFlashlightEnabled );
    CNetworkVar( bool, m_bSprinting );
    CNetworkVar( bool, m_bWalking );

    //CNetworkVar( float, m_flStamina );
    //CNetworkVar( float, m_flStress );

    CNetworkQAngle( m_angEyeAngles );

    CNetworkVar( bool, m_bIsInCombat );
    CNetworkVar( bool, m_bIsUnderAttack );

    CNetworkVar( int, m_iPlayerStatus );
    CNetworkVar( int, m_iPlayerState );
    CNetworkVar( int, m_iPlayerClass );

    CNetworkVar( int, m_iDejectedTimes );
    CNetworkVar( float, m_flHelpProgress );    

    CNetworkVar( int, m_iEyeAngleZ );
    CNetworkVar( bool, m_bIsBot );

    friend class CPlayerDejectedComponent;
};

CONVERT_PLAYER_FUNCTION( CPlayer, In )

template<typename COMPONENT>
inline COMPONENT *CPlayer::GetComponent()
{
    FOR_EACH_VEC( m_nComponents, key )
    {
        COMPONENT *pComponent = dynamic_cast<COMPONENT *>( m_nComponents.Element(key) );

        if ( !pComponent )
            continue;

        return pComponent;
    }

    return NULL;
}

template<typename AI>
inline AI * CPlayer::GetAIController()
{
    if ( !m_nAIController )
        return NULL;

    return dynamic_cast<AI *>( m_nAIController );
}

template<typename AI>
inline AI * CPlayer::GetAIController() const
{
    if ( !m_nAIController )
        return NULL;

    return dynamic_cast<AI *>(m_nAIController);
}

#endif // IN_PLAYER_H