//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef BOT_COMPONENTS_H
#define BOT_COMPONENTS_H

#ifdef _WIN32
#pragma once
#endif

class CBot;
class BotSkill;
class CEnemyMemory;

//================================================================================
// Macros
//================================================================================

#define ADD_ALIAS_COMPONENT( var, className ) var = new className(); \
    AddComponent(var);

#define ADD_COMPONENT( className ) AddComponent( new className() );

#define FOR_EACH_COMPONENT( function ) FOR_EACH_MAP( m_nComponents, key ) {\
        CBotComponent *pComponent = m_nComponents.Element( key );\
        if ( !pComponent ) continue;\
        pComponent->function;\
    }

//================================================================================
// Componente de Inteligencia Artificial
//================================================================================
// Controla ciertas acciones de la I.A. de un Bot y permite procesarlas mientras
// se mantiene una tarea activa
//================================================================================
class CBotComponent : public CPlayerInfo
{
public:
    virtual int GetID() const = 0;
    virtual void SetAI( CBot *pBot );

    virtual CBot *GetBot() { return m_nBot; }
    virtual CBot *GetBot() const { return m_nBot; }

    virtual CBasePlayer *GetParent() { return m_pParent; }

    virtual CPlayer *GetHost() { return ToInPlayer(m_pParent); }
    virtual CPlayer *GetHost() const { return ToInPlayer(m_pParent); }

    // Skill & Conditions
    virtual BotSkill *GetSkill();

    virtual void SetCondition( BCOND condition );
    virtual void ClearCondition( BCOND condition );
    virtual bool HasCondition( BCOND condition );
    virtual void InjectButton( int btn );

    // State
    virtual float GetStateDuration();
    virtual BotState GetState();
    virtual void SetState( BotState state, float duration = 3.0f );

    virtual bool IsIdle() { return (GetState() == STATE_IDLE); }
    virtual bool IsAlerted() { return (GetState() == STATE_ALERT); }
    virtual bool IsCombating() { return (GetState() == STATE_COMBAT); }
    virtual bool IsPanicked() { return (GetState() == STATE_PANIC); }

    virtual void Panic( float duration = -1.0f );
    virtual void Alert( float duration = -1.0f );
    virtual void Idle();
    virtual void Combat();

    // Core
    virtual void OnSpawn() { }
    virtual void OnUpdate( CBotCmd* &cmd ) { }

protected:
    CBot *m_nBot;
};

//================================================================================
// Componente para apuntar
//================================================================================
class CAimComponent : public CBotComponent
{
public:
    virtual int GetID() const { return AIM; }

    virtual void OnSpawn();
    virtual void OnUpdate( CBotCmd* &cmd );

public:
    virtual CountdownTimer GetAimTimer() { return m_nAimTimer; }
    virtual bool IsAimingTimeExpired() { return (m_nAimTimer.HasStarted() && m_nAimTimer.IsElapsed()); }
    virtual bool IsLookAroundBlocked() { return (m_nBlockLookAroundTimer.HasStarted() && !m_nBlockLookAroundTimer.IsElapsed()); }
    virtual bool IsAimingReady() { return m_bInAimRange; }

    virtual int GetPriority() { return m_iAimPriority; }
    virtual void SetPriority( int priority, int isLower = PRIORITY_UNINTERRUPTABLE );

    virtual void GetAimCenter( CBaseEntity *, Vector & );

    virtual AimSpeed GetAimSpeed();
    virtual float GetStiffness();

    virtual CBaseEntity *GetLookTarget() { return m_nEntityAim.Get(); }
    virtual const Vector &GetLookingAt() { return m_vecLookAt; }
    virtual bool IsAiming() { return m_vecLookAt.IsValid(); }

    virtual void Clear();
    virtual void LookAt( const char *pDesc, CBaseEntity *pEntity, int priority = PRIORITY_VERY_LOW, float duration = 1.0f );
	virtual void LookAt( const char *pDesc, CBaseEntity *pEntity, const Vector &vecLook, int priority = PRIORITY_VERY_LOW, float duration = 1.0f );
    virtual void LookAt( const char *pDesc, const Vector &vecLook, int priority = PRIORITY_VERY_LOW, float duration = 1.0f );

	virtual void LookNavigation();

    virtual void BlockLookAround( float duration );
    virtual void LookAround();

    virtual void LookInterestingSpot();
    virtual void LookRandomSpot();
    virtual void LookSquadMember();
    virtual void LookDanger();

    virtual void LookForward( int = PRIORITY_VERY_LOW );
    virtual void Update( CBotCmd* & );

protected:
    friend class CBot;

    EHANDLE m_nEntityAim;

    Vector m_vecLookAt;
    const char *m_pAimDesc;
    int m_iAimPriority;

    float m_lookYawVel = 0.0f;
    float m_lookPitchVel = 0.0f;
    float m_flAimDuration;

    bool m_bInAimRange;

    CountdownTimer m_nRandomAimTimer;
    CountdownTimer m_nIntestingAimTimer;

    CountdownTimer m_nAimTimer;
    CountdownTimer m_nBlockLookAroundTimer;
};

//================================================================================
// Componente para la navegación/movimiento.
//================================================================================
class CNavigationComponent : public CBotComponent, public CImprovLocomotor
{
public:
    virtual int GetID() const { return MOVEMENT; }

    virtual void OnSpawn();
    virtual void OnUpdate( CBotCmd* &cmd );

public:
    virtual bool HasDestination() { return m_vecDestination.IsValid(); }
    virtual bool HasPath() { return m_Path.IsValid();  }
    virtual bool IsDisabled() { return m_bDisabled; }
    virtual void SetDisabled( bool value ) { m_bDisabled = value; }

    virtual const Vector &GetDestination() { return m_vecDestination; }
    virtual const Vector &GetNextSpot() { return m_vecNextSpot; }
    virtual float GetDistanceLeft() { return ( !HasDestination() ) ? -1.0f : GetAbsOrigin().DistTo(m_vecDestination); }

    virtual bool ShouldRepath( const Vector &vecGoal );

    virtual int GetPriority() { return m_iDestinationPriority; }
    virtual float GetDistanceTolerance();

    virtual void CheckPath();
    virtual void ComputePath();

    virtual void SetDestination( const Vector &vecDestination, int priority = PRIORITY_VERY_LOW );
    virtual void SetDestination( CBaseEntity *pEntity, int priority = PRIORITY_VERY_LOW );
    virtual void SetDestination( CNavArea *pArea, int priority = PRIORITY_VERY_LOW );

    virtual void Stop();
    virtual void Wiggle();

public:
    // CImprovLocomotor
    virtual const Vector &GetCentroid() const;
    virtual const Vector &GetFeet() const;
    virtual const Vector &GetEyes() const;
    virtual float GetMoveAngle() const;

    virtual CNavArea *GetLastKnownArea() const;
    virtual bool GetSimpleGroundHeightWithFloor( const Vector &pos, float *height, Vector *normal = NULL );

    virtual void Crouch();
    virtual void StandUp();
    virtual bool IsCrouching() const;

    virtual void Jump();
    virtual bool IsJumping() const;

    virtual void Run();
    virtual void Walk();
    virtual bool IsRunning() const;

    virtual void StartLadder( const CNavLadder *ladder, NavTraverseType how, const Vector &approachPos, const Vector &departPos );
    virtual bool TraverseLadder( const CNavLadder *ladder, NavTraverseType how, const Vector &approachPos, const Vector &departPos, float deltaT );
    virtual bool IsUsingLadder() const;
    virtual bool IsTraversingLadder() const { return m_bTraversingLadder; }

    virtual void TrackPath( const Vector &pathGoal, float deltaT );
    virtual void OnMoveToSuccess( const Vector &goal );
    virtual void OnMoveToFailure( const Vector &goal, MoveToFailureType reason );

protected:
    bool m_bTraversingLadder;
    bool m_bJumping;
    bool m_bDisabled;

    CNavPath m_Path;
    CNavPathFollower m_Navigation;

    Vector m_vecDestination;
    Vector m_vecNextSpot;
    int m_iDestinationPriority;

    NavRelativeDirType m_WiggleDirection;
    CountdownTimer m_WiggleTimer;

    friend class CBot;
};

//================================================================================
// Componente para la navegación/movimiento por medio de teletransportación
//================================================================================
class CNavigationTeleportComponent : public CNavigationComponent
{
public:
    virtual bool ShouldTeleport( const Vector &vecGoal );
    virtual void TrackPath( const Vector &pathGoal, float deltaT );
};

//================================================================================
// Componente para seguir a alguién
//================================================================================
class CFollowComponent : public CBotComponent
{
public:
    virtual int GetID() const { return FOLLOW; }
    
    virtual void OnUpdate( CBotCmd* &cmd );

public:
    virtual CBaseEntity *GetFollowing() { return m_nFollowingEntity.Get(); }
    virtual bool IsFollowingSomeone() { return (GetFollowing() != NULL); }
    virtual bool IsFollowingPlayer();

    virtual bool IsPaused()
    {
        return !m_bFollow;
    }

    virtual void Start( CBaseEntity *pEntity, bool bFollow = true );
	virtual void Start( const char *pEntityName, bool bFollow = true );
    virtual void Stop();
    virtual void Pause() { m_bFollow = false; }
    virtual void Resume() { m_bFollow = true; }

protected:
    EHANDLE m_nFollowingEntity;
    bool m_bFollow;
};

//================================================================================
// Componente para procesar enemigos/amigos
//================================================================================
class CFriendsComponent : public CBotComponent
{
public:
    virtual int GetID() const { return FRIENDS; }

    CFriendsComponent();

    virtual void OnSpawn();
    virtual void OnUpdate( CBotCmd* &cmd );

public:
    int GetEnemiesCount() { return m_iEnemies; }
    int GetNearbyEnemiesCount() { return m_iNearbyEnemies; }

    virtual bool Enabled() { return m_bEnabled; }
    virtual void Stop();
    virtual void Resume();

	virtual void UpdateMemory();
    virtual void UpdateEnemy();
    virtual void UpdateNewEnemy();

    virtual void MaintainEnemy();
    virtual void LookAtEnemy();

    virtual float GetEnemyDistance();

    virtual CBaseEntity *SelectIdealEnemy();
    virtual CBaseEntity *GetIdealEnemy() { return m_nIdealEnemy.Get(); }

    virtual CBaseEntity *GetEnemy();
    virtual void SetEnemy( CBaseEntity *pEnemy, bool bUpdate = false );

    //virtual void GetEnemyParts( CBaseEntity *pEnemy, HitboxPositions &info );

    virtual CEnemyMemory *GetEnemyMemory( int index );
    virtual CEnemyMemory *GetEnemyMemory( CBaseEntity *pEnemy = NULL );
    virtual CEnemyMemory *UpdateEnemyMemory( CBaseEntity *pEnemy, const Vector vecLocation, float duration = -1, CBaseEntity *reported = NULL );

protected:
    bool m_bEnabled;

    EHANDLE m_nEnemy;
    EHANDLE m_nIdealEnemy;

    CUtlMap<int, CEnemyMemory *> m_nEnemyMemory;

    int m_iEnemies;
    int m_iNearbyEnemies;
    int m_iNearbyFriends;

    friend class CBot;
};

//================================================================================
// Componente para atacar
//================================================================================
class CAttackComponent : public CBotComponent
{
public:
    virtual int GetID() const { return ATTACK; }
    virtual void OnUpdate( CBotCmd* &cmd );

public:
    virtual bool IsUsingFiregun();
    virtual bool CanShot() { return (!m_ShotRateTimer.HasStarted() || m_ShotRateTimer.IsElapsed()); }
	virtual bool CanDuckShot();

    virtual void FiregunAttack();
    virtual void MeleeWeaponAttack();

protected:
    CountdownTimer m_ShotRateTimer;
};

//================================================================================
// Componente para estrategias (seguir a alguién, ocultarse, etc)
//================================================================================
class CStrategiesComponent : public CBotComponent
{
public:
    virtual int GetID() const { return STRATEGIES; }
};

#endif // BOT_COMPONENTS_H