//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"

#include "bot.h"
#include "in_utils.h"

#include "in_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar bot_primary_attack;

//================================================================================
// Establece una condición
//================================================================================
void CBot::SetCondition( BCOND condition )
{
    m_nConditions.Set( condition );
}

//================================================================================
// Limpia/olvida una condición
//================================================================================
void CBot::ClearCondition( BCOND condition )
{
    m_nConditions.Clear( condition );
}

//================================================================================
// Devuelve si el bot se encuentra en una condición
//================================================================================
bool CBot::HasCondition( BCOND condition )
{
    return (m_nConditions.IsBitSet( condition ));
}

//================================================================================
// Crea los componentes que tendrá el Bot
//================================================================================
void CBot::SetupComponents()
{
    // Componentes básicos
    // Dependiendo del Bot se pueden crear o quitar
    // en esta función

    ADD_ALIAS_COMPONENT( m_AimComponent, CAimComponent );
    ADD_ALIAS_COMPONENT( m_FollowComponent, CFollowComponent );
    ADD_ALIAS_COMPONENT( m_NavigationComponent, CNavigationComponent );
    ADD_ALIAS_COMPONENT( m_FriendShipComponent, CFriendsComponent );
    ADD_ALIAS_COMPONENT( m_AttackComponent, CAttackComponent );
}

//================================================================================
// Agrega un componente a la lista
//================================================================================
void CBot::AddComponent( CBotComponent *pComponent )
{
    pComponent->SetAI( this );
    pComponent->OnSpawn();

    m_nComponents.InsertOrReplace( pComponent->GetID(), pComponent );
}

//================================================================================
// Crea los componentes que tendrá el Bot
//================================================================================
void CBot::SetupSchedules()
{
    // Conjunto de tareas básicas
    // Dependiendo del Bot se pueden crear o quitar
    // en esta función

    ADD_SCHEDULE( CInvestigateSoundSchedule );
    ADD_SCHEDULE( CInvestigateLocationSchedule );
    ADD_SCHEDULE( CHuntEnemySchedule );
    ADD_SCHEDULE( CReloadSchedule );
    ADD_SCHEDULE( CCoverSchedule );
    ADD_SCHEDULE( CHideSchedule );
    ADD_SCHEDULE( CChangeWeaponSchedule );
    ADD_SCHEDULE( CHideAndHealSchedule );
    ADD_SCHEDULE( CHideAndReloadSchedule );
    ADD_SCHEDULE( CHelpDejectedFriendSchedule );
    ADD_SCHEDULE( CMoveAsideSchedule );
    ADD_SCHEDULE( CCallBackupSchedule );
    ADD_SCHEDULE( CDefendSpawnSchedule );
}

//================================================================================
// Agrega un componente a la lista
//================================================================================
void CBot::AddSchedule( CBotSchedule *pSchedule )
{
    pSchedule->SetAI( this );
    m_nSchedules.InsertOrReplace( pSchedule->GetID(), pSchedule );
}

//================================================================================
// Devuelve el [CBotSchedule] registrado para esta ID
//================================================================================
CBotSchedule *CBot::GetSchedule( int schedule )
{
    int index = m_nSchedules.Find( schedule );

    if ( m_nSchedules.IsValidIndex(index) )
        return m_nSchedules.Element( index );

    return NULL;
}

//================================================================================
// Devuelve la ID del conjunto de tareas activo.
//================================================================================
int CBot::GetActiveScheduleID()
{
    if ( !GetActiveSchedule() )
        return SCHEDULE_NONE;

    return GetActiveSchedule()->GetID();
}

//================================================================================
// Devuelve el conjunto de tareas ideal para el Bot (Con más deseo)
//================================================================================
int CBot::SelectIdealSchedule()
{
    if ( !GetHost()->IsAlive() ) {
        return SCHEDULE_NONE;
    }

    float desire = BOT_DESIRE_NONE;
    CBotSchedule *pIdeal = GetActiveSchedule();

    // Tenemos un conjunto activo
    // Tomamos el nivel de deseo inicial de ella
    if ( pIdeal )
        desire = pIdeal->GetRealDesire();

    FOR_EACH_MAP( m_nSchedules, it )
    {
        CBotSchedule *pSchedule = m_nSchedules[it];

        // Este conjunto tiene un mayor deseo
        if ( pSchedule->GetRealDesire() > desire ) {
            pIdeal = pSchedule;
            desire = pSchedule->GetDesire();
        }
    }

    // Un conjunto ideal y con ganas
    if ( pIdeal && desire > BOT_DESIRE_NONE )
        return pIdeal->GetID();

    return SCHEDULE_NONE;
}

//================================================================================
// Actualiza el conjunto de tareas actual
//================================================================================
void CBot::UpdateSchedule()
{
    VPROF_BUDGET( "UpdateSchedule", VPROF_BUDGETGROUP_BOTS );

    int idealSchedule = TranslateSchedule( SelectIdealSchedule() );

    if ( idealSchedule == SCHEDULE_NONE ) {
        if ( GetActiveSchedule() ) {
            GetActiveSchedule()->OnEnd();
            m_nActiveSchedule = NULL;
        }
        
        return;
    }

    // Transformamos la ID a un [CBotSchedule]
    CBotSchedule *pSchedule = GetSchedule( idealSchedule );

    // Oops! Al parecer olvidamos incorporarlo en [SetupSchedules]
    if ( !pSchedule ) {
        Assert( !"GetSchedule == NULL" );
        return;
    }

    if ( GetActiveSchedule() ) {
        if ( GetActiveSchedule() == pSchedule ) {
            GetActiveSchedule()->Think();
            return;
        }
        else {
            GetActiveSchedule()->OnEnd();
            m_nActiveSchedule = NULL;
        }
    }

    // Empezamos el conjunto
    m_nActiveSchedule = pSchedule;
    m_nActiveSchedule->OnStart();
    m_nActiveSchedule->Think();
}

//================================================================================
// Marca una tarea como completada
//================================================================================
void CBot::TaskComplete() 
{
	if ( !GetActiveSchedule() )
		return;

	GetActiveSchedule()->TaskComplete();
}

//================================================================================
// Marca como fallida un conjunto de tareas
//================================================================================
void CBot::TaskFail( const char *pWhy ) 
{
	if ( !GetActiveSchedule() )
		return;

	GetActiveSchedule()->Fail( pWhy );
}

//================================================================================
// Devuelve si podemos perseguir a nuestro enemigo
//================================================================================
bool CBot::ShouldHuntEnemy()
{
    CEnemyMemory *memory = GetEnemyMemory();

    if ( !memory )
        return false;

    if ( !CanMove() )
        return false;

    if ( HasCondition( BCOND_LOW_HEALTH ) )
        return false;

    if ( HasCondition( BCOND_DEJECTED ) )
        return false;

    if ( HasCondition( BCOND_INDEFENSE ) )
        return false;

    if ( GetNearbyEnemiesCount() >= 3 )
        return false;

    if ( GetSquad() && GetSquad()->GetStrategie() == COWARDS )
        return false;

    float distance = GetEnemyDistance();
    const float tolerance = 650.0f;

    if ( distance >= tolerance ) {
        if ( IsDangerousEnemy() ) {
            if ( IsFollowingSomeone() && !IsFollowingPaused() )
                return false;
        }

        // Estamos en modo defensivo y el enemigo esta lejos
        if ( GetTacticalMode() == TACTICAL_MODE_DEFENSIVE ) {
            // Si alguién ha reportado, podemos ir a la posición de nuestro amigo, de otra forma
            // nos quedamos defendiendo.
            if ( !memory->ReportedBy() )
                return false;
        }
    }

    return true;
}

//================================================================================
// Devuelve si podemos investigar un sonido de peligro
//================================================================================
bool CBot::ShouldInvestigateSound()
{
    if ( !CanMove() )
        return false;

    if ( HasCondition(BCOND_DEJECTED) )
        return false;

	if ( HasCondition(BCOND_INDEFENSE) )
		return false;

    if ( HasCondition( BCOND_HEAR_MOVE_AWAY ) )
        return false;

    if ( !HasCondition( BCOND_HEAR_COMBAT ) && !HasCondition( BCOND_HEAR_ENEMY ) && !HasCondition( BCOND_HEAR_DANGER ) )
        return false;

    if ( IsCombating() || GetEnemy() )
        return false;

    if ( GetSquad() && GetSquad()->GetStrategie() == COWARDS )
        return false;

	if ( IsFollowingSomeone() )
		return false;

    CSound *pSound = GetHost()->GetBestSound( SOUND_COMBAT | SOUND_PLAYER );

    if ( !pSound )
        return false;

    float distance = GetAbsOrigin().DistTo( pSound->GetSoundOrigin() );
    const float tolerance = 600.0f;

    // Estamos en modo defensivo, si el sonido
    // esta muy lejos mejor nos quedamos aquí
    if ( GetTacticalMode() == TACTICAL_MODE_DEFENSIVE && distance >= tolerance )
        return false;

    return true;
}

//================================================================================
// Devuelve si el Bot debería ocultarse para ciertas acciones (recargar, curarse)
//================================================================================
bool CBot::ShouldHide() 
{
    if ( !CanMove() )
        return false;

    if ( HasCondition(BCOND_DEJECTED) )
        return false;

	if ( IsDangerousEnemy() )
		return true;

	if ( HasCondition(BCOND_INDEFENSE) )
		return true;

	return false;
}

//================================================================================
// Devuelve si podemos tomar el arma especificada
//================================================================================
bool CBot::ShouldGrabWeapon(CBaseWeapon *pWeapon)
{
    if ( !pWeapon )
        return false;

    if ( pWeapon->GetOwner() )
        return false;

    if ( HasCondition(BCOND_DEJECTED) )
        return false;

    const float nearDistance = 500.0f;

    if ( GetEnemy() ) {
        // Hay un enemigo cerca
        if ( Utils::IsSpotOccupied(pWeapon->GetAbsOrigin(), NULL, nearDistance, GetEnemy()->GetTeamNumber()) )
            return false;

        // El enemigo puede atacarme fácilmente si voy
        if ( Utils::IsCrossingLineOfFire(pWeapon->GetAbsOrigin(), GetEnemy()->EyePosition(), GetHost(), GetHost()->GetTeamNumber()) )
            return false;
    }

    if ( !pWeapon->IsMeleeWeapon() ) {
        // Tenemos poca munición
        // Puede que el arma no sea mejor, pero al menos tiene munción
        if ( HasCondition(BCOND_EMPTY_PRIMARY_AMMO) || HasCondition(BCOND_LOW_PRIMARY_AMMO) ) {
            if ( pWeapon->HasAnyAmmo() )
                return true;
        }
    }

    if ( TheGameRules->FShouldSwitchWeapon(GetHost(), pWeapon) )
        return true;

    // Somos un aliado del jugador, procuremos no tomar las armas cercanas a un jugador humano
    if ( GetHost()->Classify() == CLASS_PLAYER_ALLY || GetHost()->Classify() == CLASS_PLAYER_ALLY_VITAL ) {
        if ( Utils::IsSpotOccupiedByClass(pWeapon->GetAbsOrigin(), CLASS_PLAYER, NULL, nearDistance) )
            return false;
    }

    return false;
}

//================================================================================
// Devuelve si podemos usar el arma en caso de ser necesario
//================================================================================
bool CBot::ShouldSwitchToWeapon( CBaseWeapon *pWeapon ) 
{
	if ( !pWeapon )
		return false;

	return GetHost()->Weapon_CanSwitchTo( pWeapon );
}

//================================================================================
// Devuelve si podemos ayudar a un amigo incapacitado
//================================================================================
bool CBot::ShouldHelpDejectedFriend(CPlayer *pDejected)
{
    if ( !pDejected->IsDejected() )
        return false;

    if ( m_nDejectedFriend.Get() ) {
        // Debemos ayudarlo!
        if ( m_nDejectedFriend.Get() == pDejected )
            return true;

        // Yo estoy tratando de ayudar a alguién más
        if ( m_nDejectedFriend.Get()->IsDejected() )
            return false;
    }

    if ( pDejected->IsBeingHelped() )
        return false;

    // @TODO: ¿Verificar que otros Bots ya esten tratando de ayudarlo?

    return true;
}

//================================================================================
// Devuelve si el bot tiene baja salud y debe ocultarse
//================================================================================
bool CBot::IsLowHealth() 
{
    int lowHealth = 30;

    if ( GetSkill()->GetLevel() >= SKILL_HARD )
        lowHealth += 10;

    if ( GetDifficulty() >= SKILL_HARD )
        lowHealth += 10;

    if ( GetDifficulty() >= SKILL_ULTRA_HARD )
        lowHealth += 10;

    if ( GetHealth() <= lowHealth )
        return true;

    return false;
}

//================================================================================
// Devuelve si el bot puede moverse a un destino
//================================================================================
bool CBot::CanMove() 
{
    if ( !Navigation() )
        return false;

    if ( Navigation()->IsDisabled() )
        return false;

    if ( GetHost()->IsMovementDisabled() )
        return false;

    if ( HasCondition( BCOND_DEJECTED ) )
        return false;

    return true;
}

//================================================================================
// Cambia el arma actual del bot por la mejor que tenga
//================================================================================
void CBot::SwitchToBestWeapon()
{
    CBaseWeapon *pCurrent = GetHost()->GetBaseWeapon();

    if ( !pCurrent )
        return;

    // Mejores armas
    CBaseWeapon *pPistol = NULL;
    CBaseWeapon *pSniper = NULL;
    CBaseWeapon *pShotgun = NULL;
    CBaseWeapon *pMachineGun = NULL;
    CBaseWeapon *pShortRange = NULL;

    // Chequeo para saber que armas tengo
    for ( int i = 0; i < GetHost()->WeaponCount(); i++ ) {
        CBaseWeapon *pWeapon = (CBaseWeapon *)GetHost()->GetWeapon(i);

        if ( !ShouldSwitchToWeapon(pWeapon) )
            continue;

        // @TODO: ¿Otra forma de detectar una pistola?
        if ( pWeapon->ClassMatches("pistol") ) {
            pPistol = pWeapon;
        }

        // El mejor francotirador
        else if ( pWeapon->IsSniper() ) {
            if ( !pSniper || pWeapon->GetWeight() > pSniper->GetWeight() )
                pSniper = pWeapon;
        }

        // La mejor escopeta
        else if ( pWeapon->IsShotgun() ) {
            if ( !pShotgun || pWeapon->GetWeight() > pShotgun->GetWeight() )
                pShotgun = pWeapon;
        }

        // El mejor rifle
        else {
            if ( !pMachineGun || pWeapon->GetWeight() > pMachineGun->GetWeight() )
                pMachineGun = pWeapon;
        }
    }

    // La mejor arma de corto alcance
    {
        if ( pShotgun )
            pShortRange = pShotgun;
        else if ( pMachineGun )
            pShortRange = pMachineGun;
        else if ( pPistol )
            pShortRange = pPistol;
    }

    float closeRange = 1000.0f;

    // Un enemigo peligroso
    if ( IsDangerousEnemy() )
        closeRange = 500.0f;

    // Estamos usando una snipa!
    if ( pCurrent->IsSniper() && pShortRange && GetEnemy() ) {
        // Mi enemigo esta cerca, cambiamos a un arma de corto alcance
        if ( Friends()->GetEnemyDistance() <= closeRange ) {
            GetHost()->Weapon_Switch(pShortRange);
            return;
        }
    }

    // No estamos usando una snipa, pero tenemos una
    if ( !pCurrent->IsSniper() && pSniper && GetEnemy() ) {
        // Mi enemigo se ha alejado, cambiamos a la snipa
        if ( Friends()->GetEnemyDistance() > closeRange ) {
            GetHost()->Weapon_Switch(pSniper);
            return;
        }
    }

    // Miramos todas nuestras armas
    /*for ( int i = 0 ; i < GetHost()->WeaponCount() ; i++ )
    {
        CBaseWeapon *pWeapon = (CBaseWeapon *)GetHost()->GetWeapon(i);

        if ( !pWeapon )
            continue;

        if ( pWeapon == pCurrent )
            continue;

        // @TODO: Considerar cambiar a otra arma si el arma actual tarda mucho en recargar a lo CS

        // No tenemos munición en el arma principal
        if ( HasCondition(BCOND_EMPTY_PRIMARY_AMMO) && HasCondition(BCOND_EMPTY_CLIP1_AMMO) )
        {
            if ( pWeapon->UsesClipsForAmmo1() && pWeapon->Clip1() > 0 )
            {
                // Cambiamos
                GetHost()->Weapon_Switch( pWeapon );
                return;
            }
        }
    }*/

    if ( IsIdle() || !pCurrent->HasAnyAmmo() ) {
        CBaseWeapon *pBest = (CBaseWeapon *)TheGameRules->GetNextBestWeapon(GetHost(), NULL);

        if ( pBest == pCurrent )
            return;

        GetHost()->Weapon_Switch(pBest);
    }
}

//================================================================================
// Devuelve si hay una cobertura cercana a la posición del bot 
// y establece la posición de la cobertura.
//================================================================================
bool CBot::GetNearestCover( Vector *vecPosition ) 
{
	// Buscamos un lugar seguro para ocultarnos
    CSpotCriteria criteria;
    criteria.SetMaxRange( 600.0f );
    criteria.UseNearest( true );
    criteria.OutOfVisibility( true );
    criteria.AvoidTeam( GetEnemy() );

	return Utils::FindCoverPosition( vecPosition, GetHost(), criteria );
}

//================================================================================
// Devuelve si hay una cobertura cercana a la posición del bot
//================================================================================
bool CBot::GetNearestCover() 
{
	Vector vecDummy;
	return GetNearestCover(&vecDummy);
}

//================================================================================
// Devuelve si el bot se encuentra en una posición de cobertura
//================================================================================
bool CBot::IsInCoverPosition()
{
	Vector vecHidingSpot;
	const float tolerance = 75.0f;

	if ( !GetNearestCover(&vecHidingSpot) )
		return false;

	if ( GetAbsOrigin().DistTo(vecHidingSpot) > tolerance )
		return false;

	return true;
}

//================================================================================
// Obtiene nuevas condiciones a partir del entorno y las estadísticas
//================================================================================
void CBot::SelectConditions()
{
    VPROF_BUDGET("SelectConditions", VPROF_BUDGETGROUP_BOTS);

    ClearCondition( BCOND_TASK_FAILED );
    ClearCondition( BCOND_TASK_DONE );
    ClearCondition( BCOND_SCHEDULE_FAILED );
    ClearCondition( BCOND_SCHEDULE_DONE );

    // Condiciones de salud
    SelectHealthConditions();

    // Condiciones de las armas
    SelectWeaponConditions();

    // Condiciones del enemigo
    SelectEnemyConditions();

    // Condiciones de ataque
    SelectAttackConditions();
}

//================================================================================
// Obtiene nuevas condiciones relacionadas a la salud y el daño
//================================================================================
void CBot::SelectHealthConditions()
{
    VPROF_BUDGET("SelectHealthConditions", VPROF_BUDGETGROUP_BOTS);

    ClearCondition( BCOND_LOW_HEALTH );
    ClearCondition( BCOND_LIGHT_DAMAGE );
    ClearCondition( BCOND_HEAVY_DAMAGE );
    ClearCondition( BCOND_REPEATED_DAMAGE );
    ClearCondition( BCOND_DEJECTED );

    if ( IsLowHealth() )
        SetCondition( BCOND_LOW_HEALTH );

    if ( GetHost()->IsDejected() )
        SetCondition( BCOND_DEJECTED );

    if ( GetHost()->GetLastDamageTimer().IsGreaterThen( 3.0f ) ) {
        m_iRepeatedDamageTimes = 0;
        m_flDamageAccumulated = 0.0f;
    }

    if ( m_iRepeatedDamageTimes == 0 )
        return;

    if ( m_iRepeatedDamageTimes <= 2 ) {
        SetCondition( BCOND_LIGHT_DAMAGE );
    }
    else {
        SetCondition( BCOND_REPEATED_DAMAGE );
    }

     if ( m_flDamageAccumulated >= 30.0f )
         SetCondition( BCOND_HEAVY_DAMAGE );
}

//================================================================================
// Obtiene nuevas condiciones relacionadas al arma actual
//================================================================================
void CBot::SelectWeaponConditions()
{
    VPROF_BUDGET("SelectWeaponConditions", VPROF_BUDGETGROUP_BOTS);

    // Limpiamos condiciones
    ClearCondition(BCOND_EMPTY_PRIMARY_AMMO);
    ClearCondition(BCOND_LOW_PRIMARY_AMMO);
    ClearCondition(BCOND_EMPTY_CLIP1_AMMO);
    ClearCondition(BCOND_LOW_CLIP1_AMMO);
    ClearCondition(BCOND_EMPTY_SECONDARY_AMMO);
    ClearCondition(BCOND_LOW_SECONDARY_AMMO);
    ClearCondition(BCOND_EMPTY_CLIP2_AMMO);
    ClearCondition(BCOND_LOW_CLIP2_AMMO);
    ClearCondition(BCOND_INDEFENSE);

    // Cambiamos a la mejor arma para esta situación
    // @TODO: Esto esta bien colocarlo aquí?
    SwitchToBestWeapon();

    CBaseWeapon *pWeapon = GetHost()->GetBaseWeapon();

    if ( !pWeapon ) {
        SetCondition(BCOND_INDEFENSE);
        return;
    }

    // Munición primaria
    {
        int ammo = 0;
        int totalAmmo = GetHost()->GetAmmoCount(pWeapon->GetPrimaryAmmoType());
        int totalRange = 30;

        // Usa clips/cartuchos
        if ( pWeapon->UsesClipsForAmmo1() ) {
            ammo = pWeapon->Clip1();
            int maxAmmo = pWeapon->GetMaxClip1();
            totalRange = (maxAmmo * 0.5);

            // Sin munición en el Clip1
            if ( ammo == 0 )
                SetCondition(BCOND_EMPTY_CLIP1_AMMO);
            else if ( ammo < totalRange )
                SetCondition(BCOND_LOW_CLIP1_AMMO);
        }

        // Sin munición primaria
        if ( totalAmmo == 0 )
            SetCondition(BCOND_EMPTY_PRIMARY_AMMO);
        else if ( totalAmmo < totalRange )
            SetCondition(BCOND_LOW_PRIMARY_AMMO);
    }

    // Munición secundaria
    {
        int ammo = 0;
        int totalAmmo = GetHost()->GetAmmoCount(pWeapon->GetSecondaryAmmoType());
        int totalRange = 15;

        // Usa clips/cartuchos
        if ( pWeapon->UsesClipsForAmmo2() ) {
            ammo = pWeapon->Clip2();
            int maxAmmo = pWeapon->GetMaxClip2();
            totalRange = (maxAmmo * 0.5);

            // Sin munición en el Clip2
            if ( ammo == 0 )
                SetCondition(BCOND_EMPTY_CLIP2_AMMO);
            else if ( ammo < totalRange )
                SetCondition(BCOND_LOW_CLIP2_AMMO);
        }

        // Sin munición secundaria
        if ( totalAmmo == 0 )
            SetCondition(BCOND_EMPTY_SECONDARY_AMMO);
        else if ( totalAmmo < totalRange )
            SetCondition(BCOND_LOW_SECONDARY_AMMO);
    }

    // No tienes munición de ningun tipo, estas indefenso
    if ( HasCondition(BCOND_EMPTY_PRIMARY_AMMO) &&
        HasCondition(BCOND_EMPTY_CLIP1_AMMO) &&
        HasCondition(BCOND_EMPTY_SECONDARY_AMMO) &&
        HasCondition(BCOND_EMPTY_CLIP2_AMMO) )
        SetCondition(BCOND_INDEFENSE);
}

//================================================================================
// Obtiene nuevas condiciones relacionadas al enemigo actual
//================================================================================
void CBot::SelectEnemyConditions()
{
    VPROF_BUDGET( "SelectEnemyConditions", VPROF_BUDGETGROUP_BOTS );

    // Limpiamos condiciones
    ClearCondition( BCOND_ENEMY_LOST );
    ClearCondition( BCOND_ENEMY_OCCLUDED );
    ClearCondition( BCOND_ENEMY_LAST_POSITION_OCCLUDED );
    ClearCondition( BCOND_ENEMY_DEAD );
    ClearCondition( BCOND_ENEMY_UNREACHABLE );
    ClearCondition( BCOND_ENEMY_TOO_NEAR );
    ClearCondition( BCOND_ENEMY_NEAR );
    ClearCondition( BCOND_ENEMY_FAR );
    ClearCondition( BCOND_ENEMY_TOO_FAR );
    ClearCondition( BCOND_NEW_ENEMY );
    ClearCondition( BCOND_SEE_ENEMY );

    // No tenemos un enemigo
    if ( !GetEnemy() )
        return;

    CEnemyMemory *memory = Friends()->GetEnemyMemory();

    if ( !memory )
        return;

    const float distances[] = {
        100.0f,
        350.0f,
        800.0f,
        1300.0f
    };

    float enemyDistance = GetEnemyDistance();

    if ( enemyDistance <= distances[0] )
        SetCondition( BCOND_ENEMY_TOO_NEAR );
    if ( enemyDistance <= distances[1] )
        SetCondition( BCOND_ENEMY_NEAR );
    if ( enemyDistance >= distances[2] )
        SetCondition( BCOND_ENEMY_FAR );
    if ( enemyDistance >= distances[3] )
        SetCondition( BCOND_ENEMY_TOO_FAR );

    if ( !GetHost()->IsAbleToSee( memory->GetLastPosition() ) )
        SetCondition( BCOND_ENEMY_LAST_POSITION_OCCLUDED );

    // No tenemos visión de ningún hitbox del enemigo
    // @TODO: Que pasa con los enemigos que no tienen hitbox
    if ( !memory->IsAnyHitboxVisible( GetHost() ) ) {
        SetCondition( BCOND_ENEMY_OCCLUDED );

        if ( !m_occludedEnemyTimer.HasStarted() )
            m_occludedEnemyTimer.Start();
        else if ( m_occludedEnemyTimer.GetElapsedTime() > 3.0f )
            SetCondition( BCOND_ENEMY_LOST );
    }
    else {
        SetCondition( BCOND_SEE_ENEMY );
        m_occludedEnemyTimer.Invalidate();
    }

    if ( !GetEnemy()->IsAlive() ) {
        if ( GetSkill()->IsEasy() ) {
            // ¡Somos noobs! No sabemos reconocer una animación de muerte :(
            if ( GetEnemy()->m_lifeState == LIFE_DEAD )
                SetCondition( BCOND_ENEMY_DEAD );
        }
        else {
            SetCondition( BCOND_ENEMY_DEAD );
        }
    }
}

//================================================================================
// Obtiene nuevas condiciones relacionadas al ataque
//================================================================================
void CBot::SelectAttackConditions()
{
    VPROF_BUDGET("SelectAttackConditions", VPROF_BUDGETGROUP_BOTS);

    // Limpiamos condiciones
    ClearCondition( BCOND_TOO_CLOSE_TO_ATTACK );
    ClearCondition( BCOND_TOO_FAR_TO_ATTACK );
    ClearCondition( BCOND_NOT_FACING_ATTACK );
    ClearCondition( BCOND_BLOCKED_BY_FRIEND );

    ClearCondition( BCOND_CAN_RANGE_ATTACK1 );
    ClearCondition( BCOND_CAN_RANGE_ATTACK2 );
    ClearCondition( BCOND_CAN_MELEE_ATTACK1 );
    ClearCondition( BCOND_CAN_MELEE_ATTACK2 );

    BCOND condition = ShouldRangeAttack1();

    if ( condition != BCOND_NONE )
        SetCondition( condition );

    condition = ShouldRangeAttack2();

    if ( condition != BCOND_NONE )
        SetCondition( condition );

    condition = ShouldMeleeAttack1();

    if ( condition != BCOND_NONE )
        SetCondition( condition );

    condition = ShouldMeleeAttack2();

    if ( condition != BCOND_NONE )
        SetCondition( condition );
}

//================================================================================
//================================================================================
CSquad *CBot::GetSquad() 
{
    return GetHost()->GetSquad();
}

//================================================================================
//================================================================================
void CBot::SetSquad( CSquad *pSquad ) 
{
    GetHost()->SetSquad( pSquad );
}

//================================================================================
//================================================================================
void CBot::SetSquad( const char *name ) 
{
    GetHost()->SetSquad( name );
}

//================================================================================
//================================================================================
bool CBot::IsSquadLeader()
{
    if ( !GetSquad() )
        return false;

    return (GetSquad()->GetLeader() == GetHost());
}

//================================================================================
//================================================================================
CPlayer * CBot::GetSquadLeader()
{
    if ( !GetSquad() )
        return NULL;

    return GetSquad()->GetLeader();
}

//================================================================================
//================================================================================
CBot * CBot::GetSquadLeaderAI()
{
    if ( !GetSquad() )
        return NULL;

    if ( !GetSquad()->GetLeader() )
        return NULL;

    return GetSquad()->GetLeader()->GetAI();
}

//================================================================================
// Devuelve si el bot puede tomar acciones para ayudar a sus amigos
//================================================================================
bool CBot::ShouldHelpFriend()
{
    if ( GetSkill()->IsEasy() )
        return false;

    if ( HasCondition( BCOND_DEJECTED ) )
        return false;

    if ( HasCondition( BCOND_INDEFENSE ) )
        return false;

    if ( IsCombating() )
        return false;

    if ( !CanMove() )
        return false;

    if ( GetSquad() ) {
        if ( GetSquad()->GetStrategie() == COWARDS )
            return false;

        if ( GetSquad()->GetCount() <= 2 && GetSquad()->GetStrategie() == LAST_CALL_FOR_BACKUP )
            return false;
    }

    return true;
}

//================================================================================
// Un miembro de nuestro escuadron ha recibido daño
//================================================================================
void CBot::OnMemberTakeDamage( CPlayer *pMember, const CTakeDamageInfo & info ) 
{
    // Estoy a la defensiva, pero mi escuadron necesita ayuda
    if ( ShouldHelpFriend() )
    {
        // TODO
    }

    // Han atacado a un jugador normal, reportemos a sus amigos Bots
    if ( !pMember->IsBot() && info.GetAttacker() )
        OnMemberReportEnemy( pMember, info.GetAttacker() );
}

//================================================================================
//================================================================================
void CBot::OnMemberDeath( CPlayer *pMember, const CTakeDamageInfo & info ) 
{
    if ( !GetSkill()->IsHardest() && GetHost()->IsAbleToSee( pMember ) ) {
        // ¡Amigo! ¡Noo! :'(
        LookAt( "Squad Member Death", pMember->GetAbsOrigin(), PRIORITY_VERY_HIGH, RandomFloat( 0.3f, 1.5f ) );
    }
}

//================================================================================
//================================================================================
void CBot::OnMemberReportEnemy( CPlayer *pMember, CBaseEntity *pEnemy ) 
{
    // Posición estimada del enemigo
    Vector vecEstimatedPosition = pEnemy->WorldSpaceCenter();
    const float errorDistance = 100.0f;

    // Cuando un amigo nos reporta la posición de un enemigo siempre debe haber un margen de error,
    // un humano no puede saber la posición exacta hasta verlo con sus propios ojos.
    vecEstimatedPosition.x += RandomFloat( -errorDistance, errorDistance );
    vecEstimatedPosition.y += RandomFloat( -errorDistance, errorDistance );

    // Actualizamos nuestra memoria
    UpdateEnemyMemory( pEnemy, vecEstimatedPosition, -1.0f, pMember );
}
