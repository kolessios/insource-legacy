//==== Woots 2017. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "info_weapon_spawn.h"

#include "in_player.h"
#include "in_gamerules.h"
#include "players_manager.h"

#include "director.h"

#include "nav.h"
#include "nav_area.h"
#include "nav_mesh.h"

#include "eventqueue.h"
#include "dbhandler.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

dbHandler *g_WeaponDatabase = NULL;

DECLARE_NOTIFY_COMMAND( sv_weapon_spawn_distance, "3500", "" );
DECLARE_NOTIFY_COMMAND( sv_weapon_spawn_think, "2", "" );

//================================================================================
// Puntero a la base de datos
//================================================================================
static dbHandler *database()
{
	if ( !g_WeaponDatabase )
	{
		g_WeaponDatabase = new dbHandler();
		g_WeaponDatabase->Initialise("weapons.db");
	}

	return g_WeaponDatabase;
}

//================================================================================
// Información y Red
//================================================================================

LINK_ENTITY_TO_CLASS( info_weapon_spawn, CWeaponSpawn );

BEGIN_DATADESC( CWeaponSpawn )
	DEFINE_KEYFIELD( m_iTier, FIELD_INTEGER, "Tier" ),
	DEFINE_KEYFIELD( m_iAmount, FIELD_INTEGER, "Amount" ),
	DEFINE_KEYFIELD( m_iOnlyInSkill, FIELD_INTEGER, "OnlySkill" ),

	// Inputs
    DEFINE_INPUTFUNC( FIELD_VOID, "Spawn", InputSpawn ),
    DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
    DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
    DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),

	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetTier", InputSetTier ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetAmount", InputSetTier ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Fill", InputFill ),

	// Outputs
    DEFINE_OUTPUT( m_OnSpawn, "OnSpawn" ),
    DEFINE_OUTPUT( m_OnDepleted, "OnDepleted" ),
END_DATADESC()

//================================================================================
// Destructor
//================================================================================
CWeaponSpawn::~CWeaponSpawn()
{
	// Sucio...
	if ( g_WeaponDatabase )
		delete g_WeaponDatabase;
}

//================================================================================
//================================================================================
int CWeaponSpawn::ObjectCaps() 
{
	// Estatico, debemos usarlo para obtener el arma
	if ( m_iAmount == 0 )
		return ( BaseClass::ObjectCaps() | FCAP_CONTINUOUS_USE ) & ~FCAP_ACROSS_TRANSITION;

	return BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION;
}

//================================================================================
// Creación en el mundo
//================================================================================
void CWeaponSpawn::Spawn() 
{
	m_bDisabled = false;
	m_nWeapon = NULL;
	m_iActualAmount = m_iAmount;

	// No somos solidos
    SetSolid( SOLID_NONE );

	// Estatico
	if ( IsStatic() )
	{
		m_pWeaponClass = GetWeaponClass();

		// Creamos un arma de este tipo temporalmente.
		// Solo la usaremos para obtener el modelo
		CBaseEntity *pWeapon = CreateEntityByName( m_pWeaponClass );

		if ( !pWeapon || !CanSpawn() )
		{
			UTIL_Remove( this );
		}

		DispatchSpawn( pWeapon );

		// Establecemos el modelo
		SetModel( STRING(pWeapon->GetModelName()) );

		// Eliminamos el arma temporal
		UTIL_Remove( pWeapon );
		return;
	}

	// Creamos la arma ahora mismo!
    if ( HasSpawnFlags(SF_SPAWN_IMMEDIATELY) )
        g_EventQueue.AddEvent( this, "Spawn", 1.0f, this, this );

	// Pensamiento
	SetThink( &CWeaponSpawn::Think );
	SetNextThink( gpGlobals->curtime + 2.0f );
}

//================================================================================
// Pensamiento
//================================================================================
void CWeaponSpawn::Think() 
{
	// Volvemos a pensar en 2s
	SetNextThink( gpGlobals->curtime + sv_weapon_spawn_think.GetFloat() );

    if ( !engine->IsInEditMode()  )
    {
	    CNavArea *pArea = TheNavMesh->GetNavArea( GetAbsOrigin() );

	    // Marcamos esta area como la que tiene recursos
	    if ( pArea && !pArea->HasAttributes(NAV_MESH_RESOURCES) )
	    {
		    NavAttributeSetter setter( (NavAttributeType)NAV_MESH_RESOURCES );
		    setter( pArea );
	    }
    }

	// A veces no aparecera nada :P
	// Esto tiene sentido si sv_weapon_spawn_think es muy grande
	if ( IsDistanceHandled() )
	{
		if ( IS_SKILL_HARD || IS_SKILL_VERY_HARD )
		{
			if ( RandomInt(1, 10) >= 5 )
				return;
		}
		else
		{
			if ( RandomInt(1, 10) >= 8 )
				return;
		}
	}

	// Nos adaptamos a la dificultad
	if ( IsAdaptative() )
	{
		// Guardamos el tier original
		int tier = m_iTier;

		// Tier 3/4 = Mejores
		if ( m_iTier == 3 || m_iTier == 4 )
		{
			if ( IS_SKILL_VERY_HARD )
				m_iTier = RandomInt(1, 2);
			if ( IS_SKILL_HARD )
				m_iTier = RandomInt(2, 3);
		}
		else if ( m_iTier == 2 )
		{
			if ( IS_SKILL_VERY_HARD )
				m_iTier = RandomInt(1, 2);
			if ( IS_SKILL_EASY )
				m_iTier = RandomInt(2, 4);
		}
		else if ( m_iTier == 1 )
		{
			if ( IS_SKILL_EASY )
				m_iTier = RandomInt(1, 3);
		}

		// Creamos un arma
		SpawnWeapon();

		// Restauramos el tier
		m_iTier = tier;
	}
	else 
	{
		// Sencillamente tratamos de crear un arma
		SpawnWeapon();
	}
}

//================================================================================
// Devuelve si es un creador estatico, es decir, 
// que da armas en vez de crearlas en el mundo.
//================================================================================
bool CWeaponSpawn::IsStatic() 
{
	if ( HasSpawnFlags(SF_STATIC_SPAWNER) )
		return true;

	// Creador infinito
	//if ( m_iAmount == 0 )
		//return true;

	return false;
}

//================================================================================
// Devuelve si el creador se adapta a la dificultad del juego
//================================================================================
bool CWeaponSpawn::IsAdaptative() 
{
	#ifdef APOCALYPSE
	if ( TheGameRules->IsGameMode(GAME_MODE_NONE) || TheGameRules->IsGameMode(GAME_MODE_COOP) || TheGameRules->IsGameMode(GAME_MODE_ASSAULT) )
		return true;
	#endif

	return HasSpawnFlags(SF_ADAPTATIVE);
}

//================================================================================
// Devuelve si el arma solo debe crearse si hay un jugador cercano
//================================================================================
bool CWeaponSpawn::IsDistanceHandled() 
{
	#ifdef APOCALYPSE
	if ( TheGameRules->IsGameMode(GAME_MODE_NONE) || TheGameRules->IsGameMode(GAME_MODE_SURVIVAL) )
		return true;
	#endif

	return false;
}

//================================================================================
// Devuelve el tipo de arma que se creara
//================================================================================
const char *CWeaponSpawn::GetWeaponClass()
{
	return database()->ReadString("SELECT classname FROM registered WHERE tier = %i ORDER BY RANDOM() LIMIT 1", m_iTier);
}

//================================================================================
// Devuelve si podemos crear un arma
//================================================================================
bool CWeaponSpawn::CanSpawn() 
{
	ConVarRef ai_inhibit_spawners("ai_inhibit_spawners");
    
    // No podemos
    if ( ai_inhibit_spawners.GetBool() )
        return false;

	// Desactivado
    if ( m_bDisabled )
        return false;

	// Solo uno activo
    if ( HasSpawnFlags(SF_ONLY_ONE_ACTIVE) )
    {
        // Todavía existe
        if ( m_nWeapon.Get() )
            return false;
    }

	if ( m_iAmount > 0 )
	{
		// Ya no tenemos
		if ( m_iActualAmount == 0 )
			return false;
	}

	if ( m_iOnlyInSkill > 0 )
	{
		// No podemos aparecer en este nivel de dificultad
		if ( !TheGameRules->IsSkillLevel(m_iOnlyInSkill) )
			return false;
	}

	// Solo si hay un jugador cerca
	if ( IsDistanceHandled() )
	{
		float tolerance = sv_weapon_spawn_distance.GetFloat();
		float distance = 9999999.0f;

		// Buscamos el jugador más cercano
		ThePlayers->GetNear( GetAbsOrigin(), distance );

		// Todos los jugadores aún estan muy lejos
		if ( distance > tolerance )
			return false;
	}

	// Debe estar oculto de los jugadores
    if ( HasSpawnFlags(SF_HIDE_FROM_PLAYERS) )
    {
        if ( ThePlayers->IsEyesVisible(this) )
            return false;
    }

	if ( m_nWeapon.Get() )
	{
		float distance = GetAbsOrigin().DistTo( m_nWeapon->GetAbsOrigin() );

		// Nuestra última creación esta muy cerca
		if ( distance < 100.0f )
			return false;
	}

	return true;
}

//================================================================================
// Crea un arma en la posición del creador
//================================================================================
void CWeaponSpawn::SpawnWeapon() 
{
	// No podemos
	if ( !CanSpawn() )
		return;

	// Es un creador estatico
	if ( IsStatic() )
		return;

	// Un arma del tier, al azar
	const char *pClassname = GetWeaponClass();

	if ( !pClassname )
	{
		Assert( !"GetWeaponClass() == NULL" );
		return;
	}

	// Creamos la entidad
	CBaseWeapon *pWeapon = (CBaseWeapon *)CBaseEntity::CreateNoSpawn( pClassname, GetAbsOrigin(), GetAbsAngles() );

	if ( !pWeapon )
	{
		Assert( !"pWeapon == NULL in CWeaponSpawn::SpawnWeapon()" );
		return;
	}

	// Nombre para identificarlo como hijo del creador
	pWeapon->SetName( MAKE_STRING("@spawner_weapon") );

	// Mostramos y activamos
	DispatchSpawn( pWeapon );
	pWeapon->Activate();

	// Última creación
	m_nWeapon = pWeapon;

	// Hemos creado un arma
	m_OnSpawn.FireOutput( this, NULL );

	if ( m_iAmount > 0 )
	{
		// Uno menos
		--m_iActualAmount;

		// Nos hemos quedado sin armas!
		if ( m_iActualAmount == 0 )
			m_OnDepleted.FireOutput( this, NULL );
	}
}

//================================================================================
// Alguién ha usado la entidad
//================================================================================
void CWeaponSpawn::Use( CBaseEntity *pActivator, CBaseEntity * pCaller, USE_TYPE useType, float value ) 
{
	// No es un creador estatico
	if ( !IsStatic() )
		return;

	// No tenemos para dar
	if ( m_iAmount > 0 && m_iActualAmount == 0 )
		return;

	CPlayer *pPlayer = ToInPlayer( pActivator );

	if ( !pPlayer )
		return;

	// Le damos este tipo de arma
	pPlayer->GiveNamedItem( m_pWeaponClass );

	if ( m_iAmount > 0 )
	{
		// Uno menos
		--m_iActualAmount;

		// Nos hemos quedado sin armas!
		if ( m_iActualAmount == 0 )
		{
			m_OnDepleted.FireOutput( this, NULL );
			AddEffects( EF_NODRAW );
		}
	}
}


//================================================================================
//================================================================================
void CWeaponSpawn::InputSpawn( inputdata_t &inputdata )
{
    SpawnWeapon();
}

//================================================================================
//================================================================================
void CWeaponSpawn::InputEnable( inputdata_t &inputdata )
{
    m_bDisabled = false;
}

//================================================================================
//================================================================================
void CWeaponSpawn::InputDisable( inputdata_t &inputdata )
{
    m_bDisabled = true;
}

//================================================================================
//================================================================================
void CWeaponSpawn::InputToggle( inputdata_t &inputdata )
{
    m_bDisabled = !m_bDisabled;
}

void CWeaponSpawn::InputSetTier( inputdata_t &inputdata ) 
{
	m_iTier = inputdata.value.Int();
}

void CWeaponSpawn::InputSetAmount( inputdata_t &inputdata ) 
{
	m_iAmount = inputdata.value.Int();
	g_EventQueue.AddEvent( this, "Fill", 0.0f, this, this );
}

void CWeaponSpawn::InputFill( inputdata_t &inputdata ) 
{
	m_iActualAmount = m_iAmount;
	RemoveFlag( EF_NODRAW );
}