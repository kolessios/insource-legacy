//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef BOT_H
#define BOT_H

#ifdef _WIN32
#pragma once
#endif

#include "in_player.h"

#include "nav.h"
#include "nav_path.h"
#include "nav_pathfind.h"
#include "nav_area.h"
#include "nav_mesh.h"

#include "improv_locomotor.h"
#include "utlflags.h."

#include "bot_defs.h"
#include "bot_components.h"
#include "bot_schedules.h"

#include "squad.h"

//================================================================================
// Información
//================================================================================

static int g_botID = 0;

static const char *m_botNames[] =
{
    "Alfonso",
    "Abigail",
    "Chell",
    "Adan",
    "Gordon",
    "Freeman",
    "Troll",
    "Kolesias",
    "Jennifer",
    "Valeria",
    "Alejandra",
    "Andrea",
    "Mario",
    "Maria",
    "Mariana",
	"Steam",
	"Dallas",
    "Hoston",
	"Alex",
	"Sing",
	"Alice"
};

//================================================================================
// Macros
//================================================================================

// Por cada Bot activo
#define FOR_EACH_BOT( content ) for ( int i = 1; i <= gpGlobals->maxClients; ++i ) { \
    CPlayer *pClient = ToInPlayer( UTIL_PlayerByIndex(i) ); \
    if ( !pClient || !pClient->GetAI() ) continue; \
    CBot *pBot = pClient->GetAI(); \
    content \
}

// Creación de Bot
#define CREATE_BOT( classname, position, angles ) TheGameRules->SetPlayerClass( classname ); CreateBot( NULL, position, angles )

#define RELATIONSHIP( entity ) TheGameRules->PlayerRelationship( GetHost(), entity ) 

// VPROF
#define VPROF_BUDGETGROUP_BOTS _T("Bots") 

//================================================================================
// Información de la dificultad del Bot
//================================================================================
class BotSkill
{
public:
    BotSkill();
    BotSkill( int skill );

    virtual bool IsEasy() { return (GetLevel() == SKILL_EASY); }
    virtual bool IsMedium() { return (GetLevel() == SKILL_MEDIUM); }
    virtual bool IsHard() { return (GetLevel() == SKILL_HARD); }
    virtual bool IsVeryHard() { return (GetLevel() == SKILL_VERY_HARD); }
    virtual bool IsHardest() { return (GetLevel() == SKILL_HARDEST); }

    virtual void SetLevel( int skill );
    virtual int GetLevel() { return m_iSkillLevel; }
    virtual const char *GetLevelName();

    virtual float GetMemoryDuration() { return m_flEnemyMemoryDuration; }
    virtual void SetMemoryDuration( float duration ) { m_flEnemyMemoryDuration = duration; }

    virtual float GetPanicDuration() { return m_flPanicDelay; }
    virtual void SetPanicDuration( float duration ) { m_flPanicDelay = duration; }

    virtual AimSpeed GetMinAimSpeed() { return m_iMinAimSpeed; }
    virtual void SetMinAimSpeed( AimSpeed speed ) { m_iMinAimSpeed = speed; }
    virtual AimSpeed GetMaxAimSpeed() { return m_iMaxAimSpeed; }
    virtual void SetMaxAimSpeed( AimSpeed speed ) { m_iMaxAimSpeed = speed; }

    virtual float GetMinAttackRate() { return m_flMinAttackRate; }
    virtual void SetMinAttackRate( float time ) { m_flMinAttackRate = time; }
    virtual float GetMaxAttackRate() { return m_flMaxAttackRate; }
    virtual void SetMaxAttackRate( float time ) { m_flMaxAttackRate = time; }

    virtual HitboxType GetFavoriteHitbox() { return m_iFavoriteHitbox; }
    virtual void SetFavoriteHitbox( HitboxType type ) { m_iFavoriteHitbox = type; }

    virtual float GetAlertDuration() { return m_flAlertDuration; }
    virtual void SetAlertDuration( float duration ) { m_flAlertDuration = duration; }

protected:
    int m_iSkillLevel;
    float m_flEnemyMemoryDuration;
    float m_flPanicDelay;

    AimSpeed m_iMinAimSpeed;
    AimSpeed m_iMaxAimSpeed;

    float m_flMinAttackRate;
    float m_flMaxAttackRate;

    HitboxType m_iFavoriteHitbox;

    float m_flAlertDuration;
};

//================================================================================
// Almacena la información de un enemigo
//================================================================================
class CEnemyMemory 
{
public:
    CEnemyMemory()
    {
        m_nEnemy = NULL;
        m_nReported = NULL;
        m_vecLastPosition.Invalidate();
        m_ExpireTimer.Invalidate();
    }

    CEnemyMemory( CBaseEntity *pEnemy, const Vector &vec, float duration, int frame, HitboxPositions info )
    {
        SetEnemy( pEnemy );
        SetLastPosition( vec );
        SetDuration( duration );
        SetBodyPositions( info );
		SetFrame( frame );
    }

	CEnemyMemory( CBaseEntity *pEnemy, const Vector &vec, float duration, int frame )
    {
        SetEnemy( pEnemy );
        SetLastPosition( vec );
        SetDuration( duration );
		SetFrame( frame );
    }

    CBaseEntity *GetEnemy() { return m_nEnemy.Get(); }
    void SetEnemy( CBaseEntity *pEnemy ) { m_nEnemy = pEnemy; }

    CBaseEntity *ReportedBy() { return m_nReported.Get(); }
    void SetReportedBy( CBaseEntity *pAllied ) { m_nReported = pAllied;  }

    const Vector GetLastPosition() const { return m_vecLastPosition; }
    void SetLastPosition( const Vector &vec ) { m_vecLastPosition = vec; }

    void SetDuration( float duration ) { m_ExpireTimer.Start(duration); }
    bool HasExpired() { return (m_ExpireTimer.HasStarted() && m_ExpireTimer.IsElapsed()); }
    CountdownTimer ExpireTimer() { return m_ExpireTimer; }

    HitboxPositions GetBodyPositions() { return m_BodyPositions; }
    void SetBodyPositions( HitboxPositions info ) { m_BodyPositions = info; }

	virtual int GetFrame() { return m_iFrame; }
	virtual void SetFrame( int frame ) { m_iFrame = frame; }

	void GetHitboxPosition( Vector &vecPosition, HitboxType part );
	bool GetHitboxPosition( CBaseEntity *pViewer, Vector &vecPosition, HitboxType part );
    bool GetVisibleHitboxPosition( CBaseEntity *pViewer, Vector &vecPosition, HitboxType favorite );

    bool IsLastPositionVisible( CBaseEntity *pViewer );
	bool IsHitboxVisible( CBaseEntity *pViewer, HitboxType part = HITGROUP_HEAD );
    bool IsAnyHitboxVisible( CBaseEntity *pViewer );
    bool IsVisible( CBaseEntity *pViewer );

    
protected:
    EHANDLE m_nEnemy;
    Vector m_vecLastPosition;
    HitboxPositions m_BodyPositions;
    CountdownTimer m_ExpireTimer;
    EHANDLE m_nReported;
	int m_iFrame;
};

//================================================================================
// Inteligencia Artificial para crear un Jugador controlado por el sistema
//================================================================================
class CBot : public CPlayerInfo
{
public:
    CBot();

    virtual CBasePlayer *GetParent() { return m_pParent; }
    virtual CPlayer *GetHost() { return ToInPlayer(m_pParent); }
    virtual CPlayer *GetHost() const { return ToInPlayer(m_pParent); }

    // Principales
    virtual BotPerformance GetPerformance() { return m_iPerformance; }
    virtual void SetPerformance( BotPerformance value ) { m_iPerformance = value; }

    virtual void SetParent( CBasePlayer *parent );

    virtual void Spawn();
    virtual void Think();
    virtual void PlayerMove( CBotCmd *cmd );
    virtual void ApplyDebugCommands();

    virtual bool ShouldOptimize();
    virtual bool OptimizeThisFrame();

    virtual bool ShouldProcess();
    virtual void Process( CBotCmd* & );

    virtual void MimicThink( int );

    virtual CBotCmd *GetCmd() { return m_cmd; }
    virtual CBotCmd *GetLastCmd() { return m_nLastCmd; }

    virtual void InjectMovement( NavRelativeDirType direction );
    virtual void InjectButton( int btn );

    virtual void Possess( CPlayer *pPlayer );

    // Senses
    CAI_Senses *GetSenses() { return GetHost()->GetSenses(); }
    const CAI_Senses *GetSenses() const { return GetHost()->GetSenses(); }

    virtual void OnLooked( int iDistance );
    virtual void OnLooked( CBaseEntity *pEntity );
    virtual void OnListened();

    virtual void OnTakeDamage( const CTakeDamageInfo &info );
    virtual void OnDeath( const CTakeDamageInfo &info );

    // States & Skills
    virtual void SetTacticalMode( int attitude ) { m_iTacticalMode = attitude; }
    virtual int GetTacticalMode() { return m_iTacticalMode; }

    virtual void SetSkill( int level );
    virtual BotSkill *GetSkill() { return m_Skill; }

    virtual float GetStateDuration() { return (m_iStateTimer.HasStarted()) ? m_iStateTimer.GetRemainingTime() : -1;  }
    virtual BotState GetState() { return m_iState; }
    virtual void SetState( BotState state, float duration = 3.0f );
    virtual void CleanState();

    virtual bool IsIdle() { return (GetState() == STATE_IDLE); }
    virtual bool IsAlerted() { return (GetState() == STATE_ALERT); }
    virtual bool IsCombating() { return (GetState() == STATE_COMBAT); }
    virtual bool IsPanicked() { return (GetState() == STATE_PANIC); }

    virtual void Panic( float duration = -1.0f );
    virtual void Alert( float duration = -1.0f );
    virtual void Idle();
    virtual void Combat();

	// Aim Component
    virtual bool IsAiming();
	virtual bool IsAimingReady();
	virtual int GetLookPriority();
	virtual AimSpeed GetAimSpeed();
	virtual CBaseEntity *GetLookTarget();
	virtual const Vector &GetLookingAt();

	virtual void ClearLook();
    virtual void LookAt( const char *pDesc, CBaseEntity *pEntity, int priority = PRIORITY_VERY_LOW, float duration = 1.0f );
	virtual void LookAt( const char *pDesc, CBaseEntity *pEntity, const Vector &vecLook, int priority = PRIORITY_VERY_LOW, float duration = 1.0f );
    virtual void LookAt( const char *pDesc, const Vector &vecLook, int priority = PRIORITY_VERY_LOW, float duration = 1.0f );

	virtual bool ShouldLookInterestingSpot();
    virtual bool ShouldAimOnlyVisibleInterestingSpots() { return false; }
	virtual bool ShouldLookRandomSpot();
	virtual bool ShouldLookSquadMember();
	virtual bool ShouldAimDangerSpot() { return true; }
    virtual bool ShouldLookEnemy();

	// Attack Component
	virtual void FiregunAttack();
    virtual void MeleeWeaponAttack();

	virtual bool IsUsingFiregun();
    virtual bool CanShot();
	virtual bool CanDuckShot();

	// Follow Component
	virtual CBaseEntity *GetFollowing();
    virtual bool IsFollowingSomeone();
    virtual bool IsFollowingPlayer();
    virtual bool IsFollowingPaused();

	virtual bool ShouldFollow();

	virtual void StartFollow( CBaseEntity *pEntity, bool bFollow = true );
	virtual void StartFollow( const char *pEntityName, bool bFollow = true );
    virtual void StopFollow();
    virtual void PauseFollow();
    virtual void ResumeFollow();

    // Navigation Component
	virtual bool HasDestination();
	virtual bool IsNavigationDisabled();
	virtual void SetNavigationDisabled( bool value );

	virtual const Vector &GetDestination();
	virtual const Vector &GetNextSpot();

	virtual float GetDestinationDistanceLeft();
	virtual float GetDestinationDistanceTolerance();

	virtual int GetNavigationPriority();

	virtual bool ShouldUpdateNavigation();
    virtual bool ShouldTeleport( const Vector &vecGoal );
	virtual bool ShouldWiggle();

	virtual void SetDestination( const Vector &vecDestination, int priority = PRIORITY_VERY_LOW );
    virtual void SetDestination( CBaseEntity *pEntity, int priority = PRIORITY_VERY_LOW );
    virtual void SetDestination( CNavArea *pArea, int priority = PRIORITY_VERY_LOW );
	virtual void StopNavigation();

    virtual bool ShouldRun();
    virtual bool ShouldWalk();
    virtual bool ShouldCrouch();
    virtual bool ShouldJump();

	virtual bool IsJumping() const;

    virtual void Crouch();
    virtual void StandUp();
    virtual bool IsCrouching() const { return m_bNeedCrouch; }

    virtual void Run();
    virtual bool IsRunning() const { return m_bNeedRun; }

    virtual void Walk();
    virtual bool IsWalking() const { return m_bNeedWalk; }

    virtual void NormalWalk();

    /// I.A.
    virtual void SetCondition( BCOND condition );
    virtual void ClearCondition( BCOND condition );
    virtual bool HasCondition( BCOND condition );

    virtual void SetupComponents();
    virtual void AddComponent( CBotComponent *pComponent );

    virtual void SetupSchedules();
    virtual void AddSchedule( CBotSchedule *pSchedule );

    virtual CBotSchedule *GetSchedule( int schedule );
    virtual CBotSchedule *GetActiveSchedule() { return m_nActiveSchedule; }
    virtual int GetActiveScheduleID();

    virtual int SelectIdealSchedule();
    virtual int TranslateSchedule( int schedule ) { return schedule; }
    virtual void UpdateSchedule();

	virtual bool TaskStart( BotTaskInfo_t *info ) { return false; }
	virtual bool TaskRun( BotTaskInfo_t *info ) { return false; }
	virtual void TaskComplete();
	virtual void TaskFail( const char *pWhy );

	virtual bool ShouldHuntEnemy();
    virtual bool ShouldInvestigateSound();
	virtual bool ShouldHide();
	virtual bool ShouldGrabWeapon( CBaseWeapon *pWeapon );
	virtual bool ShouldSwitchToWeapon( CBaseWeapon *pWeapon );
	virtual bool ShouldHelpDejectedFriend( CPlayer *pDejected );

    virtual bool IsLowHealth();
    virtual bool CanMove();
	virtual void SwitchToBestWeapon();

	virtual bool GetNearestCover( Vector *vecPosition );
	virtual bool GetNearestCover();
	virtual bool IsInCoverPosition();

    virtual void SelectConditions();
    virtual void SelectHealthConditions();
    virtual void SelectWeaponConditions();
    virtual void SelectEnemyConditions();
    virtual void SelectAttackConditions();

    // Enemy Component
    virtual void SetPeaceful( bool enabled );

	virtual void MaintainEnemy();
	virtual float GetEnemyDistance();

	virtual bool IsEnemyLost();
	virtual bool IsEnemyLowPriority();
    virtual bool IsBetterEnemy( CBaseEntity *pEnemy, CBaseEntity *pPrevious );

    int GetEnemiesCount();
    int GetNearbyEnemiesCount();

	virtual bool CanBeEnemy( CBaseEntity *pEnemy ) { return true; }

    virtual CBaseEntity *GetIdealEnemy();
    virtual CBaseEntity *GetEnemy();
    virtual void SetEnemy( CBaseEntity *pEnemy, bool bUpdate = false );

	virtual bool IsDangerousEnemy( CBaseEntity *pEnemy = NULL );
    virtual bool ShouldCarefulApproach();

	virtual CEnemyMemory *GetEnemyMemory( int index );
    virtual CEnemyMemory *GetEnemyMemory( CBaseEntity *pEnemy = NULL );
    virtual CEnemyMemory *UpdateEnemyMemory( CBaseEntity *pEnemy, const Vector vecLocation, float duration = -1.0f, CBaseEntity *reported = NULL );

    virtual BCOND ShouldRangeAttack1();
    virtual BCOND ShouldRangeAttack2();
    virtual BCOND ShouldMeleeAttack1() { return BCOND_NONE; }
    virtual BCOND ShouldMeleeAttack2() { return BCOND_NONE; }

    // Escuadron
    virtual CSquad *GetSquad();
    virtual void SetSquad( CSquad *pSquad );
    virtual void SetSquad( const char *name );

    virtual bool IsSquadLeader();
    virtual CPlayer *GetSquadLeader();
    virtual CBot *GetSquadLeaderAI();

    virtual bool ShouldHelpFriend();

    virtual void OnMemberTakeDamage( CPlayer *pMember, const CTakeDamageInfo &info );
    virtual void OnMemberDeath( CPlayer *pMember, const CTakeDamageInfo &info );
    virtual void OnMemberReportEnemy( CPlayer *pMember, CBaseEntity *pEnemy );

    // Utilidades
    virtual bool IsLocalPlayerWatchingMe();

	virtual bool ShouldShowDebug();
    virtual void DebugDisplay();
    virtual void DebugScreenText( const char *pText, Color color = Color(255, 255, 255, 150), float yPosition = -1, float duration = 0.15f );
    virtual void DebugAddMessage( char *format, ... );

    // Components
	virtual CAttackComponent *Attack() { return m_AttackComponent; }
    virtual CAimComponent *Aim() { return m_AimComponent; }
    virtual CNavigationComponent *Navigation() { return m_NavigationComponent; }
	virtual CNavigationComponent *Navigation() const { return m_NavigationComponent; }
    virtual CFriendsComponent *Friends() { return m_FriendShipComponent; }
    virtual CStrategiesComponent *Strategies() { return m_StrategiesComponent; }
    virtual CFollowComponent *Follow() { return m_FollowComponent; }

private:
    CBot( const CBot & );

protected:
    // Components
    CAimComponent *m_AimComponent;
	CAttackComponent *m_AttackComponent;
    CNavigationComponent *m_NavigationComponent;
    CFriendsComponent *m_FriendShipComponent;
    CStrategiesComponent *m_StrategiesComponent;
    CFollowComponent *m_FollowComponent;
    CUtlMap<int, CBotComponent *> m_nComponents;

    // Cmd
    CBotCmd *m_nLastCmd;
    CBotCmd *m_cmd;

    // Tareas
    CHandle<CBaseWeapon> m_nBetterWeapon;
    CHandle<CPlayer> m_nDejectedFriend;

    //
    BotState m_iState;
    BotSkill *m_Skill;
    int m_iTacticalMode;
    BotPerformance m_iPerformance;
    CountdownTimer m_iStateTimer;
    IntervalTimer m_occludedEnemyTimer;

    // Schedules
    CBotSchedule *m_nActiveSchedule;
    CUtlMap<int, CBotSchedule *> m_nSchedules;

    // Condiciones
    CFlagsBits m_nConditions;

    // Navigation
    bool m_bNeedCrouch;
    bool m_bNeedRun;
    bool m_bNeedWalk;

    int m_iRepeatedDamageTimes;
    float m_flDamageAccumulated;

    CUtlVector<DebugMessage> m_debugMessages;
    float m_flDebugYPosition;

    Vector m_vecSpawnSpot;

    friend class CChangeWeaponSchedule;
    friend class CHelpDejectedFriendSchedule;
    friend class CBotSchedule;
    friend class CDefendSpawnSchedule;
    friend class CBotSpawn;
    friend class DirectorManager;
};

//================================================================================
//================================================================================
class BotPathCost
{
public:
    BotPathCost( CPlayer *pPlayer ) 
    {
        m_nHost = pPlayer;
    }

    float operator() ( CNavArea *area, CNavArea *fromArea, const CNavLadder *ladder, const CFuncElevator *elevator, float length )
    {
        float baseDangerFactor = 100.0f;
        float flDist;

        if ( !fromArea )
            return 0.0f;

        // No pasar de un area de salto a otro
        if ( (fromArea->GetAttributes() & NAV_MESH_JUMP) && (area->GetAttributes() & NAV_MESH_JUMP) )
            return -1.0f;

        if ( area->IsBlocked( TEAM_ANY ) || area->IsBlocked( m_nHost->GetTeamNumber() ) )
            return -1.0f;

        if ( !fromArea->IsConnected( area, NUM_DIRECTIONS ) )
            return -1.0f;

        if ( ladder ) {
            // ladders are slow to use
            const float ladderPenalty = 2.0f;
            flDist = ladderPenalty * ladder->m_length;
        }
        else {
            flDist = (area->GetCenter() - fromArea->GetCenter()).Length();
        }

        // Costo por distancia
        float flCost = flDist + fromArea->GetCostSoFar();

        // Obtenemos la diferencia de altura
        // TODO: ComputeHeightChange
        float ourZ = fromArea->GetCenter().z; //fromArea->GetZ( fromArea->GetCenter() );
        float areaZ = area->GetCenter().z;//area->GetZ( area->GetCenter() );

        float fallDistance = ourZ - areaZ;

        if ( ladder && ladder->m_bottom.z < fromArea->GetCenter().z && ladder->m_bottom.z > area->GetCenter().z ) {
            fallDistance = ladder->m_bottom.z - area->GetCenter().z;
        }

        // Agregamos costo si para acceder a esta area es necesario saltar de una gran distancia (y sin agua en la caida)
        if ( !area->IsUnderwater() && !area->IsConnected( fromArea, NUM_DIRECTIONS ) ) {
            float fallDamage = m_nHost->GetApproximateFallDamage( fallDistance );

            if ( fallDamage > 0.0f ) {
                // if the fall would kill us, don't use it
                const float deathFallMargin = 10.0f;

                if ( fallDamage + deathFallMargin >= m_nHost->GetHealth() )
                    return -1.0f;

                // if we need to get there in a hurry, ignore minor pain
                //const float painTolerance = 15.0f * m_nBot->GetProfile()->GetAggression() + 10.0f;
                const float painTolerance = 30.0f;

                if ( fallDamage > painTolerance ) {
                    // cost is proportional to how much it hurts when we fall
                    // 10 points - not a big deal, 50 points - ouch!
                    flCost += 100.0f * fallDamage * fallDamage;
                }
            }
        }

        if ( area->IsUnderwater() ) {
            if ( fallDistance < HalfHumanHeight )
                return -1.0f;

            float penalty = 20.0f;
            flCost += penalty * flDist;
        }

        // Esta por encima de nosotros
        if ( !ladder && fallDistance < 0 ) {
            // No podemos llegar ni con nuestro salto más largo
            if ( fallDistance < -(JumpCrouchHeight) )
                return -1.0f;

            float penalty = 13.0f;
            flCost += penalty * (fabs( fallDistance ) + flDist);
        }

        if ( area->GetAttributes() & (NAV_MESH_CROUCH | NAV_MESH_WALK) ) {
            // these areas are very slow to move through
            //float penalty = (m_route == FASTEST_ROUTE) ? 20.0f : 5.0f;
            float penalty = 5.0f;
            flCost += penalty * flDist;
        }

        if ( area->GetAttributes() & NAV_MESH_JUMP ) {
            const float jumpPenalty = 5.0f;
            flCost += jumpPenalty * flDist;
        }

        if ( area->GetAttributes() & NAV_MESH_AVOID ) {
            const float avoidPenalty = 10.0f;
            flCost += avoidPenalty * flDist;
        }

        if ( area->HasAvoidanceObstacle() ) {
            const float blockedPenalty = 20.0f;
            flCost += blockedPenalty * flDist;
        }

        // add in the danger of this path - danger is per unit length travelled
        flCost += flDist * baseDangerFactor * area->GetDanger( m_nHost->GetTeamNumber() );
        return flCost;
    }

protected:
    CPlayer *m_nHost;
};

//================================================================================
//================================================================================
inline void CBotComponent::SetAI( CBot *pBot )
{
    m_nBot = pBot;
    SetParent( m_nBot->GetParent() );
}

//================================================================================
//================================================================================
inline BotSkill *CBotComponent::GetSkill()
{
    return GetBot()->GetSkill();
}

//================================================================================
//================================================================================
inline void CBotComponent::SetCondition( BCOND condition )
{
    GetBot()->SetCondition( condition );
}

//================================================================================
//================================================================================
inline void CBotComponent::ClearCondition( BCOND condition )
{
    GetBot()->ClearCondition( condition );
}

//================================================================================
//================================================================================
inline bool CBotComponent::HasCondition( BCOND condition )
{
    return GetBot()->HasCondition( condition );
}

//================================================================================
//================================================================================
inline void CBotComponent::InjectButton( int btn )
{
    GetBot()->InjectButton( btn );
}

//================================================================================
//================================================================================
inline float CBotComponent::GetStateDuration() 
{
    return GetBot()->GetStateDuration();
}

//================================================================================
//================================================================================
inline BotState CBotComponent::GetState() 
{ 
    return GetBot()->GetState(); 
}

//================================================================================
//================================================================================
inline void CBotComponent::SetState( BotState state, float duration )
{
    GetBot()->SetState( state, duration );
}

//================================================================================
//================================================================================
inline void CBotComponent::Panic( float duration )
{
    GetBot()->Panic( duration );
}

//================================================================================
//================================================================================
inline void CBotComponent::Alert( float duration )
{
    GetBot()->Alert( duration );
}

//================================================================================
//================================================================================
inline void CBotComponent::Idle()
{
    GetBot()->Idle();
}

//================================================================================
//================================================================================
inline void CBotComponent::Combat()
{
    GetBot()->Combat();
}

extern CPlayer *CreateBot( const char *pPlayername, const Vector *vecPosition, const QAngle *angles );

#endif // BOT_H
