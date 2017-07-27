//==== Woots 2017. http://creativecommons.org/licenses/by/2.5/mx/ ===========//
//
// Conjunto de tareas, aquí es donde esta la I.A. de los Bots, cada conjunto
// de tareas se activa según el nivel de deseo que devuelve la función
// GetDesire(), el conjunto con más deseo se activará y empezará a realizar cada
// una de las tareas que se le fue asignada.
//
// Si la función ShouldForceFinish() devuelve false entonces cualquier otro conjunto
// que tenga más deseo terminara y reemplazara la activa.
//
// El funcionamiento predeterminado de cada tarea se encuentra en 
// el archivo bot_schedules.cpp. Se puede sobreescribir el funcionamiento
// de una tarea al devolver true en las funciones StartTask y RunTask de CBot
//
//=============================================================================//

#ifndef BOT_SCHEDULES_H
#define BOT_SCHEDULES_H

#ifdef _WIN32
#pragma once
#endif

//================================================================================
// Información acerca de una tarea, se conforma de la tarea que se debe ejecutar
// y un valor que puede ser un Vector, un flotante, un string, etc.
//================================================================================
struct BotTaskInfo_t
{
	BotTaskInfo_t( int iTask )
	{
		task = iTask;

		vecValue.Invalidate();
		flValue	 = 0;
		iszValue = NULL_STRING;
		pszValue = NULL;
	}

    BotTaskInfo_t( int iTask, int value )
    {
        task = iTask;
        iValue = value;
        flValue = (float)value;

        vecValue.Invalidate();
        iszValue = NULL_STRING;
        pszValue = NULL;
    }

	BotTaskInfo_t( int iTask, Vector value )
	{
		task = iTask;
		vecValue = value;

		flValue	 = 0;
		iszValue = NULL_STRING;
		pszValue = NULL;
	}

	BotTaskInfo_t( int iTask, float value )
	{
		task = iTask;
		flValue = value;
        iValue = (int)value;

		vecValue.Invalidate();
		iszValue = NULL_STRING;
		pszValue = NULL;
	}

	BotTaskInfo_t( int iTask, const char *value )
	{
		task = iTask;
		iszValue = MAKE_STRING(value);

		vecValue.Invalidate();
		flValue	 = 0;
		pszValue = NULL;
	}

	BotTaskInfo_t( int iTask, CBaseEntity *value )
	{
		task = iTask;
		pszValue = value;

		vecValue.Invalidate();
		flValue	 = 0;
		iszValue = NULL_STRING;
	}

	int task;

	Vector vecValue;
	float flValue;
    int iValue;
	string_t iszValue;
	EHANDLE pszValue;
};

//================================================================================
// Macros
//================================================================================

#define ADD_TASK( task, value ) m_Tasks.AddToTail( new BotTaskInfo_t(task, value) );

#define ADD_SCHEDULE( classname ) AddSchedule( new classname() );
#define ADD_INTERRUPT( condition ) m_Interrupts.AddToTail( condition );

#define DECLARE_SCHEDULE( id ) virtual int GetID() const { return id; } \
    virtual void Setup();

#define BEGIN_SETUP_SCHEDULE( classname ) typedef classname CurrentSchedule; \
    void CurrentSchedule::Setup() {

#define BEGIN_SCHEDULE( classname ) void classname::Setup() {

#define END_SCHEDULE() }

//================================================================================
// Base para crear un conjunto de tareas
//================================================================================
class CBotSchedule : public CBotComponent
{
public:
    CBotSchedule()
    {
		m_nActiveTask = NULL;
		m_bFailed = false;
        m_bStarted = false;
        m_bFinished = false;
    }

    virtual int GetID() const = 0;

public:
    // Principales
    virtual void Fail( const char *pWhy );

	virtual float GetDesire() { return BOT_DESIRE_NONE; }
	virtual float GetRealDesire();

    virtual void OnStart();
	virtual void OnEnd();

    virtual bool HasFinished()
    {
        return m_bFinished;
    }

    virtual bool HasStarted()
    {
        return m_bStarted;
    }

    virtual bool HasTasks()
    {
        return m_Tasks.Count() > 0;
    }

	virtual bool ShouldForceFinish() { return false; }

    virtual void Think();
	virtual bool ShouldInterrupted();

    virtual bool IsWaitFinished() { return m_WaitTimer.IsElapsed(); }
    virtual void Wait( float seconds );

    // Tareas
    virtual BotTaskInfo_t *GetActiveTask() { return m_nActiveTask; }
    virtual const char *GetActiveTaskName();

    virtual void Setup() { }

    virtual void TaskStart();
    virtual void TaskRun();
    virtual void TaskComplete();

    // Helpers
	virtual bool CanMove();

protected:
    CBot *m_nBot;
    BotTaskInfo_t *m_nActiveTask;

	float m_flLastDesire;
	bool m_bFailed;

    bool m_bStarted;
    bool m_bFinished;

    CUtlVector<BotTaskInfo_t *> m_Tasks;
    CUtlVector<BCOND> m_Interrupts;
    
    CountdownTimer m_WaitTimer;
    IntervalTimer m_StartTimer;

protected:
	Vector m_vecSavedLocation;
	Vector m_vecLocation;
    float m_flDistanceTolerance;
};

//================================================================================
// Investigar un sonido y regresar a donde estabamos si no había nada
//================================================================================
class CInvestigateSoundSchedule : public CBotSchedule
{
	DECLARE_CLASS_GAMEROOT( CInvestigateSoundSchedule, CBotSchedule );
    DECLARE_SCHEDULE( SCHEDULE_INVESTIGATE_SOUND );

public:
	virtual bool ShouldForceFinish() { return true; }

	virtual float GetDesire();
	virtual void SetLocation( const Vector location ) { m_vecSoundPosition = location; }

	virtual void TaskStart();

protected:
    Vector m_vecSoundPosition;
};

//================================================================================
// Investigar una ubicación y regresar a donde estabamos si no había nada
//================================================================================
class CInvestigateLocationSchedule : public CBotSchedule
{
public:
	DECLARE_CLASS_GAMEROOT( CInvestigateLocationSchedule, CBotSchedule );
    DECLARE_SCHEDULE( SCHEDULE_INVESTIGATE_LOCATION );

public:
	virtual bool ShouldForceFinish() { return true; }

	virtual float GetDesire();
    virtual void SetLocation( const Vector location ) { m_vecLocation = location; }

protected:
    Vector m_vecLocation;
};

//================================================================================
// Perseguir a nuestro enemigo
//================================================================================
class CHuntEnemySchedule : public CBotSchedule
{
public:
	DECLARE_CLASS_GAMEROOT( CHuntEnemySchedule, CBotSchedule );
    DECLARE_SCHEDULE( SCHEDULE_HUNT_ENEMY );

public:
	virtual float GetDesire();
};

//================================================================================
// Recargar
//================================================================================
class CReloadSchedule : public CBotSchedule
{
public:
	DECLARE_CLASS_GAMEROOT( CReloadSchedule, CBotSchedule );
    DECLARE_SCHEDULE( SCHEDULE_RELOAD );

public:
	virtual float GetDesire();
};

//================================================================================
// Cubrirse
//================================================================================
class CCoverSchedule : public CBotSchedule
{
public:
	DECLARE_CLASS_GAMEROOT( CCoverSchedule, CBotSchedule );
    DECLARE_SCHEDULE( SCHEDULE_COVER );

public:
	virtual float GetDesire();
	virtual bool ShouldForceFinish() { return true; }
};

//================================================================================
// Ocultarse, como cubrirse pero sin volver a nuestra ubicación original
//================================================================================
class CHideSchedule : public CBotSchedule
{
public:
	DECLARE_CLASS_GAMEROOT( CCoverSchedule, CBotSchedule );
    DECLARE_SCHEDULE( SCHEDULE_HIDE );

public:
	virtual float GetDesire();
	virtual bool ShouldForceFinish() { return true; }
};

//================================================================================
// Tomar el arma en el suelo que estamos viendo.
//================================================================================
class CChangeWeaponSchedule : public CBotSchedule
{
public:
	DECLARE_CLASS_GAMEROOT( CChangeWeaponSchedule, CBotSchedule );
    DECLARE_SCHEDULE( SCHEDULE_CHANGE_WEAPON );

	enum
	{
		TASK_GRAB_WEAPON = BCUSTOM_TASK
	};

public:
	virtual float GetDesire();

    virtual void TaskStart();
	virtual void TaskRun();
};

//================================================================================
// Ocultarnos y después curarnos
//================================================================================
class CHideAndHealSchedule : public CBotSchedule
{
public:
	DECLARE_CLASS_GAMEROOT( CHideAndHealSchedule, CBotSchedule );
    DECLARE_SCHEDULE( SCHEDULE_HIDE_AND_HEAL );

public:
	virtual float GetDesire();
	virtual bool ShouldForceFinish() { return true; }
};

//================================================================================
// Ocultarnos y recargar
//================================================================================
class CHideAndReloadSchedule : public CBotSchedule
{
public:
	DECLARE_CLASS_GAMEROOT( CHideAndReloadSchedule, CBotSchedule );
    DECLARE_SCHEDULE( SCHEDULE_HIDE_AND_RELOAD );

public:
	virtual bool ShouldForceFinish() { return true; }
	virtual float GetDesire();

	virtual void TaskRun();
};

//================================================================================
// Ayudar a un amigo incapacitado
//================================================================================
class CHelpDejectedFriendSchedule : public CBotSchedule
{
public:
	DECLARE_CLASS_GAMEROOT( CHelpDejectedFriendSchedule, CBotSchedule );
    DECLARE_SCHEDULE( SCHEDULE_HELP_DEJECTED_FRIEND );

    enum
    {
        BTASK_HELP = BCUSTOM_TASK,
    };

public:
	virtual bool ShouldHelp();

	virtual float GetDesire();
    virtual void TaskRun();
};

//================================================================================
// Moverse a un lado
//================================================================================
class CMoveAsideSchedule : public CBotSchedule
{
public:
	DECLARE_CLASS_GAMEROOT( CMoveAsideSchedule, CBotSchedule );
    DECLARE_SCHEDULE( SCHEDULE_MOVE_ASIDE );

public:
	virtual float GetDesire();
	virtual void OnStart();


protected:
	CountdownTimer m_nMoveAsideTimer;
};

//================================================================================
// Retirarse y llamar refuerzos
//================================================================================
class CCallBackupSchedule : public CBotSchedule
{
public:
	DECLARE_CLASS_GAMEROOT( CCallBackupSchedule, CBotSchedule );
    DECLARE_SCHEDULE( SCHEDULE_CALL_FOR_BACKUP );

public:
};

//================================================================================
// Defender el punto de aparición
//================================================================================
class CDefendSpawnSchedule : public CBotSchedule
{
public:
    DECLARE_CLASS_GAMEROOT( CDefendSpawnSchedule, CBotSchedule );
    DECLARE_SCHEDULE( SCHEDULE_DEFEND_SPAWN );

public:
    virtual float GetDesire();
};

#endif // BOT_SCHEDULES_H