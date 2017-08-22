//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#ifndef IBOT_H
#define IBOT_H

#ifdef _WIN32
#pragma once
#endif

#include "bot_defs.h"
#include "in_utils.h"
#include "in_player.h"

class IBotSchedule;
class IBotComponent;

class IBotVision;
class IBotAttack;
class IBotMemory;
class IBotLocomotion;
class IBotFollow;
class IBotDecision;

class CEntityMemory;
class CBotSkill;
class CSquad;

//================================================================================
// Information
//================================================================================

static int g_botID = 0;

//================================================================================
// Macros
//================================================================================

// VPROF
#define VPROF_BUDGETGROUP_BOTS _T("Bots") 

//================================================================================
// Bot Artificial Intelligence Interface
// You can create a custom bot central system using this interface.
//================================================================================
abstract_class IBot : public CPlayerInfo
{
public:
    IBot( CBasePlayer *parent )
    {
        SetDefLessFunc( m_nComponents );
        SetDefLessFunc( m_nSchedules );

        m_Skill = NULL;
        m_iPerformance = BOT_PERFORMANCE_AWAKE;
        m_pParent = parent;
    }

    virtual CPlayer *GetHost() const {
        return dynamic_cast<CPlayer *>( m_pParent );
    }

    virtual BotPerformance GetPerformance() {
        return m_iPerformance;
    }

    virtual void SetPerformance( BotPerformance value ) {
        m_iPerformance = value;
    }

    virtual CBotCmd *GetUserCommand() {
        return m_cmd;
    }

    virtual CBotCmd *GetLastCmd() {
        return m_nLastCmd;
    }

    virtual void SetTacticalMode( int attitude ) {
        m_iTacticalMode = attitude;
    }

    virtual int GetTacticalMode() {
        return m_iTacticalMode;
    }

    virtual CBotSkill *GetSkill() const {
        return m_Skill;
    }

    virtual float GetStateDuration() {
        return (m_iStateTimer.HasStarted()) ? m_iStateTimer.GetRemainingTime() : -1;
    }

    virtual BotState GetState() const {
        return m_iState;
    }

    virtual bool IsIdle() const {
        return (GetState() == STATE_IDLE);
    }

    virtual bool IsAlerted() const {
        return (GetState() == STATE_ALERT);
    }

    virtual bool IsCombating() const {
        return (GetState() == STATE_COMBAT);
    }

    virtual bool IsPanicked() const {
        return (GetState() == STATE_PANIC);
    }

    virtual IBotSchedule *GetActiveSchedule() const {
        return m_nActiveSchedule;
    }

    virtual void Spawn() = 0;
    virtual void Think() = 0;
    virtual void PlayerMove( CBotCmd *cmd ) = 0;

    virtual bool ShouldProcess() = 0;
    virtual void Process( CBotCmd* &cmd ) = 0;

    virtual void MimicThink( int ) = 0;

    virtual void InjectMovement( NavRelativeDirType direction ) = 0;
    virtual void InjectButton( int btn ) = 0;

    virtual CAI_Senses *GetSenses() = 0;
    virtual const CAI_Senses *GetSenses() const = 0;

    virtual void OnLooked( int iDistance ) = 0;
    virtual void OnLooked( CBaseEntity *pEntity ) = 0;
    virtual void OnListened() = 0;

    virtual void OnTakeDamage( const CTakeDamageInfo &info ) = 0;
    virtual void OnDeath( const CTakeDamageInfo &info ) = 0;

    virtual void SetSkill( int level ) = 0;

    virtual void SetState( BotState state, float duration = 3.0f ) = 0;
    virtual void CleanState() = 0;

    virtual void Panic( float duration = -1.0f ) = 0;
    virtual void Alert( float duration = -1.0f ) = 0;
    virtual void Idle() = 0;
    virtual void Combat() = 0;

    virtual void SetCondition( BCOND condition ) = 0;
    virtual void ClearCondition( BCOND condition ) = 0;
    virtual bool HasCondition( BCOND condition ) const = 0;

    virtual void AddComponent( IBotComponent *pComponent ) = 0;

    template<typename COMPONENT>
    COMPONENT *GetComponent( int id ) const = 0;

    virtual void SetUpComponents() = 0;
    virtual void SetUpSchedules() = 0;

    virtual IBotSchedule *GetSchedule( int schedule ) = 0;
    virtual int GetActiveScheduleID() = 0;

    virtual int SelectIdealSchedule() = 0;
    virtual int TranslateSchedule( int schedule ) = 0;
    virtual void UpdateSchedule() = 0;

    virtual bool TaskStart( BotTaskInfo_t *info ) = 0;
    virtual bool TaskRun( BotTaskInfo_t *info ) = 0;
    virtual void TaskComplete() = 0;
    virtual void TaskFail( const char *pWhy ) = 0;

    virtual void SelectPreConditions() = 0;
    virtual void SelectPostConditions() = 0;

    virtual void SelectHealthConditions() = 0;
    virtual void SelectWeaponConditions() = 0;
    virtual void SelectEnemyConditions() = 0;
    virtual void SelectAttackConditions() = 0;

    virtual CBaseEntity *GetEnemy() const = 0;
    virtual CEntityMemory *GetPrimaryThreat() const = 0;

    virtual void SetEnemy( CBaseEntity * pEnemy, bool bUpdate = false ) = 0;
    virtual void SetPeaceful( bool enabled ) = 0;

    virtual CSquad *GetSquad() = 0;
    virtual void SetSquad( CSquad *pSquad ) = 0;
    virtual void SetSquad( const char *name ) = 0;

    virtual bool IsSquadLeader() = 0;
    virtual CPlayer *GetSquadLeader() = 0;
    virtual CBot *GetSquadLeaderAI() = 0;

    virtual void OnMemberTakeDamage( CPlayer *pMember, const CTakeDamageInfo &info ) = 0;
    virtual void OnMemberDeath( CPlayer *pMember, const CTakeDamageInfo &info ) = 0;
    virtual void OnMemberReportEnemy( CPlayer *pMember, CBaseEntity *pEnemy ) = 0;

    virtual bool ShouldShowDebug() = 0;
    virtual void DebugDisplay() = 0;
    virtual void DebugScreenText( const char *pText, Color color = Color( 255, 255, 255, 150 ), float yPosition = -1, float duration = 0.15f ) = 0;
    virtual void DebugAddMessage( char *format, ... ) = 0;

    virtual IBotVision *GetVision() const = 0;
    virtual IBotAttack *GetAttack() const = 0;
    virtual IBotMemory *GetMemory() const = 0;
    virtual IBotLocomotion *GetLocomotion() const = 0;
    virtual IBotFollow *GetFollow() const = 0;
    virtual IBotDecision *GetDecision() const = 0;

protected:
    BotState m_iState;
    CBotSkill *m_Skill;
    int m_iTacticalMode;
    BotPerformance m_iPerformance;
    CountdownTimer m_iStateTimer;

    // Components
    CUtlMap<int, IBotComponent *> m_nComponents;

    // Schedules
    IBotSchedule *m_nActiveSchedule;
    CUtlMap<int, IBotSchedule *> m_nSchedules;

    // Cmd
    CBotCmd *m_nLastCmd;
    CBotCmd *m_cmd;

    // Conditions
    CFlagsBits m_nConditions;

    // Debug
    CUtlVector<DebugMessage> m_debugMessages;
    float m_flDebugYPosition;

    friend class CBotSpawn;
    friend class DirectorManager;
};


//================================================================================
// Memory about an entity
//================================================================================
class CEntityMemory
{
public:
    DECLARE_CLASS_NOBASE( CEntityMemory );

    CEntityMemory( IBot *pBot, CBaseEntity *pEntity, CBaseEntity *pInformer = NULL )
    {
        m_hEntity = pEntity;
        m_hInformer = pInformer;
        m_vecLastPosition.Invalidate();
        m_bVisible = false;
        m_LastVisible.Invalidate();
        m_LastUpdate.Invalidate();
        m_pBot = pBot;
    }

    virtual CBaseEntity *GetEntity() const
    {
        return m_hEntity.Get();
    }

    virtual bool Is( CBaseEntity *pEntity ) const
    {
        return (GetEntity() == pEntity);
    }

    virtual CBaseEntity *GetInformer() const
    {
        return m_hInformer.Get();
    }

    virtual void SetInformer( CBaseEntity *pInformer )
    {
        m_hInformer = pInformer;
    }

    virtual void UpdatePosition( const Vector &pos )
    {
        m_vecLastPosition = pos;
        m_LastUpdate.Start();
    }

    virtual void UpdateVisibility( bool visible )
    {
        m_bVisible = visible;

        if ( visible ) {
            m_LastVisible.Start();
        }
    }

    virtual bool IsVisible() const
    {
        return m_bVisible;
    }

    virtual bool IsVisibleRecently( float seconds ) const
    {
        return m_LastVisible.IsLessThen( seconds );
    }

    virtual bool WasEverVisible() const
    {
        return m_LastVisible.HasStarted();
    }

    virtual float GetElapsedTimeSinceVisible() const {
        return m_LastVisible.GetElapsedTime();
    }

    virtual float GetTimeLastVisible() const
    {
        return m_LastVisible.GetStartTime();
    }

    virtual float GetTimeLastKnown() const {
        return m_LastUpdate.GetStartTime();
    }

    virtual bool IsUpdatedRecently( float seconds ) const {
        return m_LastUpdate.IsLessThen( seconds );
    }

    virtual float GetFrameLastUpdate() const {
        return m_flFrameLastUpdate;
    }

    virtual void MarkLastFrame() {
        m_flFrameLastUpdate = gpGlobals->absoluteframetime;
    }

    virtual const Vector GetLastKnownPosition() const
    {
        return m_vecLastPosition;
    }

    virtual const HitboxPositions GetHitbox() const {
        return m_Hitbox;
    }

    virtual const HitboxPositions GetVisibleHitbox() const {
        return m_VisibleHitbox;
    }

    virtual bool IsHitboxVisible( HitboxType part ) {
        switch ( part ) {
            case HITGROUP_HEAD:
                return (m_VisibleHitbox.head.IsValid());
                break;

            case HITGROUP_CHEST:
            default:
                return (m_VisibleHitbox.chest.IsValid());
                break;

            case HITGROUP_LEFTLEG:
                return (m_VisibleHitbox.leftLeg.IsValid());
                break;

            case HITGROUP_RIGHTLEG:
                return (m_VisibleHitbox.rightLeg.IsValid());
                break;
        }

        return false;
    }

    virtual CNavArea *GetLastKnownArea() const
    {
        return TheNavMesh->GetNearestNavArea( m_vecLastPosition );
    }

    virtual float GetDistance() const
    {
        return m_pBot->GetAbsOrigin().DistTo( m_vecLastPosition );
    }

    virtual float GetDistanceSquare() const
    {
        return m_pBot->GetAbsOrigin().DistToSqr( m_vecLastPosition );
    }

    virtual bool IsInRange( float distance ) const
    {
        return (GetDistance() <= distance);
    }

    virtual int GetRelationship() const {
        return TheGameRules->PlayerRelationship( m_pBot->GetHost(), GetEntity() );
    }

    virtual void Maintain() {
        UpdatePosition( GetLastKnownPosition() );
    }

    virtual bool GetVisibleHitboxPosition( Vector &vecPosition, HitboxType favorite ) {
        if ( IsHitboxVisible( favorite ) ) {
            switch ( favorite ) {
                case HITGROUP_HEAD:
                    vecPosition = m_VisibleHitbox.head;
                    return true;
                    break;

                case HITGROUP_CHEST:
                default:
                    vecPosition = m_VisibleHitbox.chest;
                    return true;
                    break;

                case HITGROUP_LEFTLEG:
                    vecPosition = m_VisibleHitbox.leftLeg;
                    return true;
                    break;

                case HITGROUP_RIGHTLEG:
                    vecPosition = m_VisibleHitbox.rightLeg;
                    return true;
                    break;
            }
        }

        if ( favorite != HITGROUP_CHEST && IsHitboxVisible( HITGROUP_CHEST ) ) {
            vecPosition = m_VisibleHitbox.chest;
            return true;
        }

        if ( favorite != HITGROUP_HEAD && IsHitboxVisible( HITGROUP_HEAD ) ) {
            vecPosition = m_VisibleHitbox.head;
            return true;
        }

        if ( favorite != HITGROUP_LEFTLEG && IsHitboxVisible( HITGROUP_LEFTLEG ) ) {
            vecPosition = m_VisibleHitbox.leftLeg;
            return true;
        }

        if ( favorite != HITGROUP_RIGHTLEG && IsHitboxVisible( HITGROUP_RIGHTLEG ) ) {
            vecPosition = m_VisibleHitbox.rightLeg;
            return true;
        }

        return false;
    }

    virtual void UpdateHitboxAndVisibility() {
        UpdateVisibility( false );

        m_Hitbox.Reset();
        m_VisibleHitbox.Reset();

        Utils::GetHitboxPositions( GetEntity(), m_Hitbox );

        CPlayer *pPlayer = m_pBot->GetHost();

        if ( m_Hitbox.head.IsValid() ) {
            if ( pPlayer->IsAbleToSee( m_Hitbox.head ) ) {
                m_VisibleHitbox.head = m_Hitbox.head;
                UpdateVisibility( true );
            }
        }

        if ( m_Hitbox.chest.IsValid() ) {
            if ( pPlayer->IsAbleToSee( m_Hitbox.chest ) ) {
                m_VisibleHitbox.chest = m_Hitbox.chest;
                UpdateVisibility( true );
            }
        }

        if ( m_Hitbox.leftLeg.IsValid() ) {
            if ( pPlayer->IsAbleToSee( m_Hitbox.leftLeg ) ) {
                m_VisibleHitbox.leftLeg = m_Hitbox.leftLeg;
                UpdateVisibility( true );
            }
        }

        if ( m_Hitbox.rightLeg.IsValid() ) {
            if ( pPlayer->IsAbleToSee( m_Hitbox.rightLeg ) ) {
                m_VisibleHitbox.rightLeg = m_Hitbox.rightLeg;
                UpdateVisibility( true );
            }
        }
    }

protected:
    EHANDLE m_hEntity;
    EHANDLE m_hInformer;

    IBot *m_pBot;

    Vector m_vecLastPosition;

    HitboxPositions m_Hitbox;
    HitboxPositions m_VisibleHitbox;

    bool m_bVisible;

    IntervalTimer m_LastVisible;
    IntervalTimer m_LastUpdate;

    float m_flFrameLastUpdate;
};

//================================================================================
// Memory about certain information
//================================================================================
class CDataMemory : public CMultidata
{
public:
    DECLARE_CLASS_GAMEROOT( CDataMemory, CMultidata );

    CDataMemory() : BaseClass()
    {
    }

    CDataMemory( int value ) : BaseClass( value )
    {
    }

    CDataMemory( Vector value ) : BaseClass( value )
    {
    }

    CDataMemory( float value ) : BaseClass( value )
    {
    }

    CDataMemory( const char *value ) : BaseClass( value )
    {
    }

    CDataMemory( CBaseEntity *value ) : BaseClass( value )
    {
    }

    virtual void Reset() {
        BaseClass::Reset();

        m_LastUpdate.Invalidate();
        m_flForget = -1.0f;
    }

    virtual void OnSet() {
        m_LastUpdate.Start();
    }

    bool IsExpired() {
        // Never expires
        if ( m_flForget <= 0.0f )
            return false;

        return (m_LastUpdate.GetElapsedTime() >= m_flForget);
    }

    float GetElapsedTimeSinceUpdated() const {
        return m_LastUpdate.GetElapsedTime();
    }

    float GetTimeLastUpdate() const {
        return m_LastUpdate.GetStartTime();
    }

    void ForgetIn( float time ) {
        m_flForget = time;
    }

protected:
    IntervalTimer m_LastUpdate;
    float m_flForget;
};

//================================================================================
// Bot difficulty information
//================================================================================
class CBotSkill
{
public:
    CBotSkill();
    CBotSkill( int skill );

    virtual bool IsEasy() {
        return (GetLevel() == SKILL_EASY);
    }

    virtual bool IsMedium() {
        return (GetLevel() == SKILL_MEDIUM);
    }

    virtual bool IsHard() {
        return (GetLevel() == SKILL_HARD);
    }

    virtual bool IsVeryHard() {
        return (GetLevel() == SKILL_VERY_HARD);
    }

    virtual bool IsUltraHard() {
        return (GetLevel() == SKILL_ULTRA_HARD);
    }

    virtual bool IsRealistic() {
        return (GetLevel() == SKILL_IMPOSIBLE);
    }

    virtual bool IsEasiest() {
        return (GetLevel() == SKILL_EASIEST);
    }

    virtual bool IsHardest() {
        return (GetLevel() == SKILL_HARDEST);
    }

    virtual void SetLevel( int skill );

    virtual int GetLevel() {
        return m_iSkillLevel;
    }

    virtual const char *GetLevelName();

    virtual float GetMemoryDuration() {
        return m_flEnemyMemoryDuration;
    }

    virtual void SetMemoryDuration( float duration ) {
        m_flEnemyMemoryDuration = duration;
    }

    virtual float GetPanicDuration() {
        return m_flPanicDelay;
    }

    virtual void SetPanicDuration( float duration ) {
        m_flPanicDelay = duration;
    }

    virtual int GetMinAimSpeed() {
        return m_iMinAimSpeed;
    }

    virtual void SetMinAimSpeed( int speed ) {
        m_iMinAimSpeed = speed;
    }

    virtual int GetMaxAimSpeed() {
        return m_iMaxAimSpeed;
    }

    virtual void SetMaxAimSpeed( int speed ) {
        m_iMaxAimSpeed = speed;
    }

    virtual float GetMinAttackRate() {
        return m_flMinAttackRate;
    }

    virtual void SetMinAttackRate( float time ) {
        m_flMinAttackRate = time;
    }

    virtual float GetMaxAttackRate() {
        return m_flMaxAttackRate;
    }

    virtual void SetMaxAttackRate( float time ) {
        m_flMaxAttackRate = time;
    }

    virtual HitboxType GetFavoriteHitbox() {
        return m_iFavoriteHitbox;
    }

    virtual void SetFavoriteHitbox( HitboxType type ) {
        m_iFavoriteHitbox = type;
    }

    virtual float GetAlertDuration() {
        return m_flAlertDuration;
    }

    virtual void SetAlertDuration( float duration ) {
        m_flAlertDuration = duration;
    }

protected:
    int m_iSkillLevel;
    float m_flEnemyMemoryDuration;
    float m_flPanicDelay;

    int m_iMinAimSpeed;
    int m_iMaxAimSpeed;

    float m_flMinAttackRate;
    float m_flMaxAttackRate;

    HitboxType m_iFavoriteHitbox;

    float m_flAlertDuration;
};

#endif