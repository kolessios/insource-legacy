//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#include "cbase.h"
#include "bots\bot.h"

#ifdef INSOURCE_DLL
#include "in_gamerules.h"
#include "in_utils.h"
#else
#include "bots\in_utils.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar bot_primary_attack;

//================================================================================
// Logging System
// Only for the current file, this should never be in a header.
//================================================================================

#define Msg(...) Log_Msg(LOG_BOTS, __VA_ARGS__)
#define Warning(...) Log_Warning(LOG_BOTS, __VA_ARGS__)

//================================================================================
// Sets a condition
//================================================================================
void CBot::SetCondition(BCOND condition)
{
	m_nConditions.Set(condition);
}

//================================================================================
// Clear all conditions
//================================================================================
void CBot::ClearConditions()
{
	m_nConditions.ClearAll();
}

//================================================================================
// Forgets a condition
//================================================================================
void CBot::ClearCondition(BCOND condition)
{
	m_nConditions.Clear(condition);
}

//================================================================================
// Returns if the bot is in a condition
//================================================================================
bool CBot::HasCondition(BCOND condition) const
{
	if (!IsConditionsAllowed()) {
		Assert(!"Attempt to verify a condition before gathering!");
		return false;
	}

	return m_nConditions.IsBitSet(condition);
}

//================================================================================
// Add a component to the list
//================================================================================
void CBot::AddComponent(IBotComponent *pComponent)
{
	if (!pComponent)
		return;

	pComponent->Reset();

	if (pComponent->IsSchedule()) {
		IBotSchedule *pSchedule = dynamic_cast<IBotSchedule *>(pComponent);
		Assert(pSchedule);

		if (pSchedule) {
			m_nSchedules.InsertOrReplace(pSchedule->GetID(), pSchedule);
		}
	}
	else {
		m_nComponents.InsertOrReplace(pComponent->GetID(), pComponent);
	}
}

//================================================================================
// Create the components that the bot will have
//================================================================================
void CBot::SetUpComponents()
{
	// Basic components
	// Each custom bot decide which ones to remove and add.
	ADD_COMPONENT(CBotVision);
	ADD_COMPONENT(CBotFollow);
	ADD_COMPONENT(CBotLocomotion);
	ADD_COMPONENT(CBotMemory);
	ADD_COMPONENT(CBotAttack);
	ADD_COMPONENT(CBotDecision); // This component is mandatory!
}

//================================================================================
// Create the schedules that the bot will have
//================================================================================
void CBot::SetUpSchedules()
{
	// Basic schedules
	// Each custom bot decide which ones to remove and add.
	ADD_COMPONENT(CHuntEnemySchedule);
	ADD_COMPONENT(CReloadSchedule);
	ADD_COMPONENT(CCoverSchedule);
	ADD_COMPONENT(CHideSchedule);
	ADD_COMPONENT(CChangeWeaponSchedule);
	ADD_COMPONENT(CHideAndHealSchedule);
	ADD_COMPONENT(CHideAndReloadSchedule);
	ADD_COMPONENT(CMoveAsideSchedule);
	ADD_COMPONENT(CCallBackupSchedule);
	ADD_COMPONENT(CDefendSpawnSchedule);
	//ADD_COMPONENT( CInvestigateLocationSchedule ); // TODO: Finish

#ifdef INSOURCE_DLL
	ADD_COMPONENT(CHelpDejectedFriendSchedule);
#endif
}

//================================================================================
// Returns the [IBotSchedule] registered for this ID
//================================================================================
IBotSchedule *CBot::GetSchedule(int schedule)
{
	int index = m_nSchedules.Find(schedule);

	if (m_nSchedules.IsValidIndex(index)) {
		return m_nSchedules.Element(index);
	}

	return NULL;
}

//================================================================================
// Returns the active schedule ID.
//================================================================================
int CBot::GetActiveScheduleID()
{
	if (!GetActiveSchedule()) {
		return SCHEDULE_NONE;
	}

	return GetActiveSchedule()->GetID();
}

//================================================================================
// Returns the ideal schedule for the bot (With more desire)
//================================================================================
int CBot::SelectIdealSchedule()
{
	VPROF_BUDGET("CBot::SelectIdealSchedule", VPROF_BUDGETGROUP_BOTS);

	if (!GetHost()->IsAlive())
		return SCHEDULE_NONE;

	float desire = BOT_DESIRE_NONE;
	IBotSchedule *pIdeal = NULL;

	FOR_EACH_MAP(m_nSchedules, it)
	{
		IBotSchedule *pSchedule = m_nSchedules[it];

		// Compute the level of desire in this frame.
		pSchedule->ComputeDesire();

		// This schedule has a greater desire!
		if (pSchedule->GetComputedDesire() > desire) {
			pIdeal = pSchedule;
			desire = pSchedule->GetComputedDesire();
		}
	}

	if (pIdeal && desire > BOT_DESIRE_NONE) {
		return pIdeal->GetID();
	}

	return SCHEDULE_NONE;
}

//================================================================================
// Updates the current schedule
//================================================================================
void CBot::UpdateSchedule()
{
	VPROF_BUDGET("CBot::UpdateSchedule", VPROF_BUDGETGROUP_BOTS);

	// Maybe an custom A.I. want to change a schedule.
	int idealSchedule = TranslateSchedule(SelectIdealSchedule());

	if (idealSchedule == SCHEDULE_NONE) {
		if (GetActiveSchedule()) {
			GetActiveSchedule()->Finish();
			m_nActiveSchedule = NULL;
		}

		return;
	}

	IBotSchedule *pSchedule = GetSchedule(idealSchedule);
	AssertMsg(pSchedule, "GetSchedule == NULL");

	if (pSchedule == NULL)
		return;

	if (GetActiveSchedule()) {
		if (GetActiveSchedule() == pSchedule) {
			GetActiveSchedule()->Update();
			return;
		}
		else {
			GetActiveSchedule()->Finish();
		}
	}

	m_nActiveSchedule = pSchedule;
	m_nActiveSchedule->Start();
	m_nActiveSchedule->Update();
}

//================================================================================
// Mark a task as complete
// TODO: Real life scenario where this is used?
//================================================================================
void CBot::TaskComplete()
{
	if (!GetActiveSchedule())
		return;

	GetActiveSchedule()->TaskComplete();
}

//================================================================================
// Mark as failed a schedule
// TODO: Real life scenario where this is used?
//================================================================================
void CBot::TaskFail(const char *pWhy)
{
	if (!GetActiveSchedule())
		return;

	GetActiveSchedule()->Fail(pWhy);
}


//================================================================================
// Gets new conditions from environment and statistics
// Conditions that do not require information about components (vision/smell/hearing)
//================================================================================
void CBot::GatherConditions()
{
	VPROF_BUDGET("CBot::GatherConditions", VPROF_BUDGETGROUP_BOTS);

	// Hard coded!
	if (GetMemory()) {
		GetMemory()->Update();
	}

	GatherHealthConditions();
	GatherWeaponConditions();
	GatherEnemyConditions();
	GatherAttackConditions();
	GatherLocomotionConditions();
	GatherOtherConditions();
}

//================================================================================
// Obtains new conditions related to health and damage
//================================================================================
void CBot::GatherHealthConditions()
{
	VPROF_BUDGET("GatherHealthConditions", VPROF_BUDGETGROUP_BOTS);

	if (GetDecision()->IsLowHealth()) {
		SetCondition(BCOND_LOW_HEALTH);
	}

#ifdef INSOURCE_DLL
	if (GetHost()->IsDejected()) {
		SetCondition(BCOND_DEJECTED);
	}

	if (GetHost()->GetLastDamageTimer().IsGreaterThen(3.0f)) {
		m_iRepeatedDamageTimes = 0;
		m_flDamageAccumulated = 0.0f;
	}
#endif

	if (m_iRepeatedDamageTimes == 0)
		return;

	if (m_iRepeatedDamageTimes <= 2) {
		SetCondition(BCOND_LIGHT_DAMAGE);
	}
	else {
		SetCondition(BCOND_REPEATED_DAMAGE);
	}

	if (m_flDamageAccumulated >= 30.0f) {
		SetCondition(BCOND_HEAVY_DAMAGE);
	}
}

//================================================================================
// Gets new conditions related to current weapon
//================================================================================
void CBot::GatherWeaponConditions()
{
	VPROF_BUDGET("GatherWeaponConditions", VPROF_BUDGETGROUP_BOTS);

	CBaseWeapon *pWeapon = GetHost()->GetActiveBaseWeapon();

	if (pWeapon == NULL) {
		SetCondition(BCOND_HELPLESS);
		return;
	}

	// Primary ammunition
	{
		int ammo = 0;
		int totalAmmo = GetHost()->GetAmmoCount(pWeapon->GetPrimaryAmmoType());
		int totalRange = 30;

		if (pWeapon->UsesClipsForAmmo1()) {
			ammo = pWeapon->Clip1();
			int maxAmmo = pWeapon->GetMaxClip1();
			totalRange = (maxAmmo * 0.5);

			if (ammo == 0)
				SetCondition(BCOND_EMPTY_CLIP1_AMMO);
			else if (ammo < totalRange)
				SetCondition(BCOND_LOW_CLIP1_AMMO);
		}

		if (totalAmmo == 0)
			SetCondition(BCOND_EMPTY_PRIMARY_AMMO);
		else if (totalAmmo < totalRange)
			SetCondition(BCOND_LOW_PRIMARY_AMMO);
	}

	// Secondary ammunition
	{
		int ammo = 0;
		int totalAmmo = GetHost()->GetAmmoCount(pWeapon->GetSecondaryAmmoType());
		int totalRange = 15;

		if (pWeapon->UsesClipsForAmmo2()) {
			ammo = pWeapon->Clip2();
			int maxAmmo = pWeapon->GetMaxClip2();
			totalRange = (maxAmmo * 0.5);

			if (ammo == 0)
				SetCondition(BCOND_EMPTY_CLIP2_AMMO);
			else if (ammo < totalRange)
				SetCondition(BCOND_LOW_CLIP2_AMMO);
		}

		if (totalAmmo == 0)
			SetCondition(BCOND_EMPTY_SECONDARY_AMMO);
		else if (totalAmmo < totalRange)
			SetCondition(BCOND_LOW_SECONDARY_AMMO);
	}

	UnblockConditions();

	// You have no ammunition of any kind, you are defenseless
	if (HasCondition(BCOND_EMPTY_PRIMARY_AMMO) &&
		HasCondition(BCOND_EMPTY_CLIP1_AMMO) &&
		HasCondition(BCOND_EMPTY_SECONDARY_AMMO) &&
		HasCondition(BCOND_EMPTY_CLIP2_AMMO)) {
		SetCondition(BCOND_HELPLESS);
	}

	if (GetMemory()) {
		CBaseWeapon *pWeapon = ToBaseWeapon(GetDataMemoryEntity("VisibleWeapon"));

		if (GetDecision()->ShouldGrabWeapon(pWeapon)) {
			GetMemory()->UpdateDataMemory("BestWeapon", pWeapon, 20.0f);
			SetCondition(BCOND_BETTER_WEAPON_AVAILABLE);
		}
	}

	BlockConditions();
}

//================================================================================
// Gets new conditions related to the current enemy
//================================================================================
void CBot::GatherEnemyConditions()
{
	VPROF_BUDGET("GatherEnemyConditions", VPROF_BUDGETGROUP_BOTS);

	CEntityMemory *memory = GetPrimaryThreat();

	if (memory == NULL) {
		SetCondition(BCOND_WITHOUT_ENEMY);
		return;
	}

	if (!IsCombating()) {
		Alert();
	}

	// List of distances to establish useful conditions.
	// TODO: No hard-coded
	const float distances[] = {
		100.0f, // Too near
		300.0f, // Near
		800.0f, // Far
		1300.0f // Too far!
	};

	float enemyDistance = memory->GetDistance();

	if (enemyDistance <= distances[0])
		SetCondition(BCOND_ENEMY_TOO_NEAR);
	if (enemyDistance <= distances[1])
		SetCondition(BCOND_ENEMY_NEAR);
	if (enemyDistance >= distances[2])
		SetCondition(BCOND_ENEMY_FAR);
	if (enemyDistance >= distances[3])
		SetCondition(BCOND_ENEMY_TOO_FAR);

	if (!memory->IsVisible()) {
		// We have no vision of the enemy
		SetCondition(BCOND_ENEMY_LOST);
	}
	else {
		SetCondition(BCOND_SEE_ENEMY);

		CBaseEntity *pBlockedBy = NULL;

		// No direct line of sight with our enemy
		// TODO
		/*if ( !GetDecision()->IsLineOfSightClear( memory->GetEntity(), &pBlockedBy ) ) {
			SetCondition( BCOND_ENEMY_OCCLUDED );

			// A friend is in front of us!
			if ( GetDecision()->IsFriend( pBlockedBy ) ) {
				SetCondition( BCOND_ENEMY_OCCLUDED_BY_FRIEND );
			}
		}*/
	}

	if (GetDecision()->IsAbleToSee(memory->GetLastKnownPosition())) {
		SetCondition(BCOND_ENEMY_LAST_POSITION_VISIBLE);
	}

	if (!memory->GetEntity()->IsAlive()) {
		if (GetProfile()->IsEasy()) {
			if (GetEnemy()->m_lifeState == LIFE_DEAD) {
				SetCondition(BCOND_ENEMY_DEAD);
			}
		}
		else {
			SetCondition(BCOND_ENEMY_DEAD);
		}
	}
}

//================================================================================
// Obtiene nuevas condiciones relacionadas al ataque
//================================================================================
void CBot::GatherAttackConditions()
{
	VPROF_BUDGET("GatherAttackConditions", VPROF_BUDGETGROUP_BOTS);

	UnblockConditions();

	BCOND condition = GetDecision()->ShouldRangeAttack1();

	if (condition != BCOND_NONE)
		SetCondition(condition);

	condition = GetDecision()->ShouldRangeAttack2();

	if (condition != BCOND_NONE)
		SetCondition(condition);

	condition = GetDecision()->ShouldMeleeAttack1();

	if (condition != BCOND_NONE)
		SetCondition(condition);

	condition = GetDecision()->ShouldMeleeAttack2();

	if (condition != BCOND_NONE)
		SetCondition(condition);

	BlockConditions();
}

//================================================================================
//================================================================================
void CBot::GatherLocomotionConditions()
{
	VPROF_BUDGET("GatherLocomotionConditions", VPROF_BUDGETGROUP_BOTS);

	if (!GetLocomotion())
		return;

	if (GetLocomotion()->HasDestination()) {
		if (GetLocomotion()->IsUnreachable()) {
			SetCondition(BCOND_GOAL_UNREACHABLE);
		}
	}
}

//================================================================================
// Gets new conditions
//================================================================================
void CBot::GatherOtherConditions()
{
	VPROF_BUDGET("GatherOtherConditions", VPROF_BUDGETGROUP_BOTS);

	// We want to always know if any of our friends is down
	// Slow!
	/*if ( GetDecision()->ShouldKnownDejectedFriends() ) {
		CPlayer *pDejected = GetDecision()->GetClosestDejectedFriend();

		if ( pDejected ) {
			GetMemory()->UpdateEntityMemory(pDejected, pDejected->GetAbsOrigin());
			GetMemory()->UpdateDataMemory("DejectedFriend", pDejected, 30.0f);
			SetCondition(BCOND_SEE_DEJECTED_FRIEND);
		}
	}*/
}

//================================================================================
//================================================================================
CSquad *CBot::GetSquad()
{
	return GetHost()->GetSquad();
}

//================================================================================
//================================================================================
void CBot::SetSquad(CSquad *pSquad)
{
	GetHost()->SetSquad(pSquad);
}

//================================================================================
//================================================================================
void CBot::SetSquad(const char *name)
{
	GetHost()->SetSquad(name);
}

//================================================================================
//================================================================================
bool CBot::IsSquadLeader()
{
	if (!GetSquad())
		return false;

	return (GetSquad()->GetLeader() == GetHost());
}

//================================================================================
//================================================================================
CPlayer * CBot::GetSquadLeader()
{
	if (!GetSquad())
		return NULL;

	return GetSquad()->GetLeader();
}

//================================================================================
//================================================================================
IBot * CBot::GetSquadLeaderAI()
{
	if (!GetSquad())
		return NULL;

	if (!GetSquad()->GetLeader())
		return NULL;

	return GetSquad()->GetLeader()->GetBotController();
}

//================================================================================
// Un miembro de nuestro escuadron ha recibido daño
//================================================================================
void CBot::OnMemberTakeDamage(CPlayer *pMember, const CTakeDamageInfo & info)
{
	// Estoy a la defensiva, pero mi escuadron necesita ayuda
   // if ( ShouldHelpFriend() )
	{
		// TODO
	}

	// Han atacado a un jugador normal, reportemos a sus amigos Bots
	if (!pMember->IsBot() && info.GetAttacker()) {
		OnMemberReportEnemy(pMember, info.GetAttacker());
	}
}

//================================================================================
//================================================================================
void CBot::OnMemberDeath(CPlayer *pMember, const CTakeDamageInfo & info)
{
	if (!GetMemory() || !GetVision())
		return;

	CEntityMemory *memory = GetMemory()->GetEntityMemory(pMember);

	if (!memory)
		return;

	// ¡Amigo! ¡Noo! :'(
	if (!GetProfile()->IsHardest() && memory->IsVisible()) {
		GetVision()->LookAt("Squad Member Death", pMember->GetAbsOrigin(), PRIORITY_VERY_HIGH, RandomFloat(0.3f, 1.5f));
	}
}

//================================================================================
//================================================================================
void CBot::OnMemberReportEnemy(CPlayer *pMember, CBaseEntity *pEnemy)
{
	if (!GetMemory())
		return;

	// Posición estimada del enemigo
	Vector vecEstimatedPosition = pEnemy->WorldSpaceCenter();
	const float errorDistance = 100.0f;

	// Cuando un amigo nos reporta la posición de un enemigo siempre debe haber un margen de error,
	// un humano no puede saber la posición exacta hasta verlo con sus propios ojos.
	vecEstimatedPosition.x += RandomFloat(-errorDistance, errorDistance);
	vecEstimatedPosition.y += RandomFloat(-errorDistance, errorDistance);

	// Actualizamos nuestra memoria
	GetMemory()->UpdateEntityMemory(pEnemy, vecEstimatedPosition, pMember);
}
