//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#ifndef ENT_HOST_H
#define ENT_HOST_H

#ifdef _WIN32
#pragma once
#endif

#include "ent\ent_component.h"

#include "basecombatcharacter.h"
#include "weapon_base.h"
#include "in_attribute.h"
#include "in_playeranimsystem.h"

#include "bots\squad.h"

class IBot;
class CEntPlayer;
class CEntCharacter;

//================================================================================
//================================================================================
class CEntHost
{
public:
    DECLARE_CLASS_NOBASE( CEntHost );
    DECLARE_SIMPLE_DATADESC();
    DECLARE_EMBEDDED_NETWORKVAR();

    CEntHost( CBaseCombatCharacter *character );

    //
    CBaseCombatCharacter *GetHost() const {
        return m_pHost;
    }

    bool IsPlayer() const {
        return m_pHost->IsPlayer();
    }

    CEntPlayer *GetPlayer() const;
    CEntCharacter *GetCharacter() const;

    //
    IBot *GetBotController() const {
        return m_pBotController;
    }

    virtual void SetBotController( IBot *pBot );
    virtual void SetUpBot();

    //
    virtual Class_T Classify() {
        return CLASS_PLAYER;
    }

    //
    virtual bool IsIdle() const;
    virtual bool IsAlerted() const;
    virtual bool IsUnderAttack() const;
    virtual bool IsOnCombat() const;

    virtual bool IsOnGround() const;
    virtual bool IsOnGodMode() const;
    virtual bool IsOnBuddhaMode() const;

    //
    virtual int GetButtons() const;
    virtual bool IsButtonPressing( int btn ) const;
    virtual bool IsButtonPressed( int btn ) const;
    virtual bool IsButtonReleased( int btn ) const;

    virtual void DisableButtons( int nButtons );
    virtual void EnableButtons( int nButtons );
    virtual void EnableAllButtons();

    virtual void ForceButtons( int nButtons );
    virtual void UnforceButtons( int nButtons );

    //
    virtual void InitialSpawn();
    virtual void Spawn();
    virtual void Precache();

    //
    virtual void PreThink();
    virtual void Think();
    virtual void PostThink();

    // Model
    virtual const char *GetHostModel();
    virtual void SetUpModel();
    virtual gender_t GetHostGender();

    // Viewmodel
    virtual void CreateViewModel( int index = 0 );
    virtual void SetUpHands();
    virtual void CreateHands( int index, int parent = 0 );
    virtual const char *GetHandsModel( int handsindex ) const;

    //
    virtual CBaseEntity *GetEnemy() const;
    virtual CBaseWeapon *GetActiveWeapon() const;
    virtual CBaseEntity	*GiveNamedItem( const char *szName, int iSubType = 0, bool removeIfNotCarried = true );

    //
    template <typename COMPONENT>
    COMPONENT *GetComponent();

    virtual CEntComponent *GetComponent( int id );

    virtual void AddComponent( int id );
    virtual void AddComponent( CEntComponent *component );

    virtual void CreateComponents();
    virtual void UpdateComponents();

    //
    virtual void AddAttribute( const char *name );
    virtual void AddAttribute( CAttribute *attribute );

    virtual void CreateAttributes();

    virtual void PreUpdateAttributes();
    virtual void UpdateAttributes();

    virtual CAttribute *GetAttribute( const char *name );
    virtual void AddAttributeModifier( const char *name );

    // Escuadron
    virtual CSquad *GetSquad() {
        return m_pSquad;
    }

    virtual void SetSquad( CSquad *pSquad );
    virtual void SetSquad( const char *name );

    virtual void OnNewLeader( CPlayer *pMember );
    virtual void OnMemberTakeDamage( CPlayer *pMember, const CTakeDamageInfo &info );
    virtual void OnMemberDeath( CPlayer *pMember, const CTakeDamageInfo &info );
    virtual void OnMemberReportEnemy( CPlayer *pMember, CBaseEntity *pEnemy );

    // Senses
    CAI_Senses *GetSenses() {
        return m_pSenses;
    }

    const CAI_Senses *GetSenses() const {
        return m_pSenses;
    }

    virtual void CreateSenses();

    virtual void SetDistLook( float flDistLook );

    virtual bool ShouldIgnoreSound( CSound *pSound ) {
        return false;
    }

    virtual int GetSoundInterests();
    virtual int GetSoundPriority( CSound *pSound );

    virtual bool QueryHearSound( CSound *pSound );
    virtual bool QuerySeeEntity( CBaseEntity *pEntity, bool bOnlyHateOrFearIfNPC = false );

    virtual void OnLooked( int iDistance );
    virtual void OnListened();

    virtual void OnSeeEntity( CBaseEntity *pEntity ) { }

    virtual bool ShouldNotDistanceCull() {
        return false;
    }

    virtual CSound *GetLoudestSoundOfType( int iType );
    virtual bool SoundIsVisible( CSound *pSound );

    virtual CSound *GetBestSound( int validTypes = ALL_SOUNDS );
    virtual CSound *GetBestScent( void );

    virtual float HearingSensitivity() {
        return 1.0;
    }

    virtual bool OnlySeeAliveEntities( void ) {
        return true;
    }

    // Animation System
    virtual CPlayerAnimationSystem *GetAnimationSystem() const {
        return m_pAnimationSystem;
    }

    virtual void HandleAnimEvent( animevent_t *event );
    virtual void DoAnimationEvent( PlayerAnimEvent_t nEvent, int data = 0, bool bPredicted = false );

    // Sounds
    virtual float SoundDesire( const char *soundname, int channel ) {
        return 0.0f;
    }

    virtual void OnSoundPlay( const char *soundname ) { }
    virtual void OnSoundStop( const char *soundname ) { }

    virtual void FootstepSound();
    virtual void PainSound( const CTakeDamageInfo &info );
    virtual void DeathSound( const CTakeDamageInfo &info );

    // Effects
    virtual void SetFlashlightEnabled( bool enabled );
    virtual int FlashlightIsOn();
    virtual void FlashlightTurnOn();
    virtual void FlashlightTurnOff();
    virtual bool IsIlluminatedByFlashlight( CBaseEntity *pEntity, float *flReturnDot );

    // Damage
    virtual CTakeDamageInfo GetLastDamage() {
        return m_LastDamage;
    }

    virtual IntervalTimer GetLastDamageTimer() {
        return m_LastDamageTimer;
    }

    virtual bool ShouldBleed( const CTakeDamageInfo &info, int hitgroup );
    virtual void TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );

    virtual void DamageEffect( const CTakeDamageInfo &info );
    virtual bool CanTakeDamage( const CTakeDamageInfo &info );

    virtual int OnTakeDamage( const CTakeDamageInfo &info );
    virtual int OnTakeDamage_Alive( CTakeDamageInfo &info );

    virtual void ApplyDamage( const CTakeDamageInfo &info );

    virtual void Event_Killed( const CTakeDamageInfo &info );
    virtual bool Event_Gibbed( const CTakeDamageInfo &info );
    virtual void Event_Dying( const CTakeDamageInfo &info );

    virtual void CreateRagdoll();

    //
    virtual float GetStamina() const;
    virtual float GetStress() const;

    // 
    virtual int GetDejectedTimes() const {
        return m_iDejectedTimes;
    }

    virtual float GetHelpProgress() const {
        return m_flHelpProgress;
    }

    virtual float GetClimbingHold() const {
        return m_flClimbingHold;
    }

    virtual bool IsDejected() const {
        return (m_iStatus == PLAYER_STATUS_DEJECTED || m_iStatus == PLAYER_STATUS_CLIMBING);
    }

    virtual bool IsBeingHelped() const {
        return (m_flHelpProgress > 0.0f);
    }

    virtual void RaiseFromDejected( CPlayer *rescuer );

    virtual int GetStatus() const {
        return m_iStatus;
    }

    virtual void OnStatus( int oldStatus, int status );
    virtual void SetStatus( int status );

    // State
    virtual int GetState() const {
        return m_iState;
    }

    virtual bool IsActive() {
        return (m_iState == PLAYER_STATE_ACTIVE);
    }

    virtual void EnterState( int status );
    virtual void LeaveState( int status ) { }
    virtual void SetState( int status );

    // Class
    virtual int GetClass() const {
        return m_iClass;
    }

    virtual void OnClass( int iClass ) { }
    virtual void SetClass( int iClass );

    // Speed
    virtual float GetSpeed();
    virtual void SpeedModifier( float &speed );
    virtual void UpdateSpeed();

    //
    virtual void UpdateMovementType();

    virtual bool CanSprint() const;

    virtual void StartSprint() {
        m_bSprinting = true;
    }

    virtual void StopSprint() {
        m_bSprinting = false;
    }

    virtual bool IsSprinting() const {
        return m_bSprinting;
    }

    virtual bool CanSneak() const;

    virtual void StartSneaking() {
        m_bSneaking = true;
    }

    virtual void StopSneaking() {
        m_bSneaking = false;
    }

    virtual bool IsSneaking() const {
        return m_bSneaking;
    }

    virtual void Jump();

    // Camera
    virtual bool GetEyesView( CBaseAnimating *pEntity, Vector& eyeOrigin, QAngle& eyeAngles, int secureDistance );
    virtual void CalcPlayerView( Vector& eyeOrigin, QAngle& eyeAngles, float& fov );

    // Collision Bounds
    virtual const Vector GetMins() const;
    virtual const Vector GetMaxs() const;
    virtual void UpdateCollisionBounds();

    // Utils
    virtual const char* GetCommandValue( const char *command );
    virtual void ExecuteCommand( const char *command );

    virtual int GetDifficultyLevel();

    virtual void Kick();
    virtual void Spectate( CPlayer *target = NULL );

    virtual bool IsLocalPlayerWatchingMe();
    virtual CPlayer *GetActivePlayer();

    virtual void ImpulseCommands();
    virtual void CheatImpulseCommands( int iImpulse );
    virtual bool ClientCommand( const CCommand &cmd );

    virtual const char *GetSpawnEntityName();
    virtual CBaseEntity *EntSelectSpawnPoint();

    virtual void PhysObjectSleep();
    virtual void PhysObjectWake();

    virtual void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
    virtual void Touch( CBaseEntity *pOther );

    virtual void InjectMovement( NavRelativeDirType direction );
    virtual void InjectButton( int btn );

    virtual int ObjectCaps();

public:
    virtual void CreateAnimationSystem();
    virtual Activity TranslateActivity( Activity actBase );
    virtual void FireBullets( const FireBulletsInfo_t & );

public:
    virtual bool IsAlive() const {
        return GetHost()->IsAlive();
    }

protected:
    CBaseCombatCharacter *m_pHost;
    CAI_Senses *m_pSenses;
    CPlayerAnimationSystem *m_pAnimationSystem;
    CSquad *m_pSquad;
    IBot *m_pBotController;

    IntervalTimer m_LastDamageTimer;
    CTakeDamageInfo m_LastDamage;

    int m_iDejectedTimes;
    float m_flHelpProgress;
    float m_flClimbingHold;
    int m_iStatus;
    int m_iState;

    int m_iClass;

    bool m_bSprinting;
    bool m_bSneaking;

    IntervalTimer m_CombatTimer;
    IntervalTimer m_UnderAttackTimer;

    bool m_bIsBot;
    bool m_bUnderAttack;
    bool m_bOnCombat;

    bool m_bFlashlightEnabled;

    CUtlVector<CEntComponent *> m_Components;
    CUtlVector<CAttribute *> m_Attributes;
};

template<typename COMPONENT>
inline COMPONENT *CEntHost::GetComponent()
{
    FOR_EACH_VEC( m_Components, key )
    {
        COMPONENT *pComponent = dynamic_cast<COMPONENT *>(m_nComponents.Element( key ));

        if ( !pComponent )
            continue;

        return pComponent;
    }

    return NULL;
}

#endif