//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#include "cbase.h"
#include "bots\bot_utils.h"

#include "bots\bot_defs.h"
#include "bots\bot.h"

#ifdef INSOURCE_DLL
#include "in_utils.h"
#include "in_player.h"
#include "in_gamerules.h"
#else
#include "bots\in_utils.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
//================================================================================
CEntityMemory::CEntityMemory(IBot *pBot, CBaseEntity *pEntity, CBaseEntity *pInformer)
{
	m_hEntity = pEntity;
	m_hInformer = pInformer;
	m_vecLastPosition.Invalidate();
	m_bVisible = false;
	m_LastVisible.Invalidate();
	m_LastUpdate.Invalidate();
	m_pBot = pBot;
}

//================================================================================
// Returns the amount of time left before this memory is considered lost.
//================================================================================
float CEntityMemory::GetTimeLeft()
{
	return m_pBot->GetMemory()->GetMemoryDuration() - GetElapsedTimeSinceUpdate();
}

//================================================================================
// Returns if the memory should be considered as lost, ie:
// The entity does not exist, has died, is to be eliminated or 
// we have not received any update of it during the time limit of our memory.
//================================================================================
bool CEntityMemory::IsLost()
{
	if (GetEntity() == NULL || GetEntity()->IsMarkedForDeletion() || !GetEntity()->IsAlive())
		return true;

	return (GetTimeLeft() <= 0);
}

//================================================================================
//================================================================================
bool CEntityMemory::IsHitboxVisible(HitboxType part)
{
	switch (part) {
		case HEAD:
			return (m_VisibleHitbox.head.IsValid());
			break;

		case CHEST:
		default:
			return (m_VisibleHitbox.chest.IsValid());
			break;

		case FEET:
			return (m_VisibleHitbox.feet.IsValid());
			break;
	}

	return false;
}

//================================================================================
//================================================================================
float CEntityMemory::GetDistance() const
{
	return m_pBot->GetHost()->GetAbsOrigin().DistTo(GetLastKnownPosition());
}

//================================================================================
//================================================================================
float CEntityMemory::GetDistanceSquare() const
{
	return m_pBot->GetHost()->GetAbsOrigin().DistToSqr(GetLastKnownPosition());
}

//================================================================================
//================================================================================
bool CEntityMemory::IsEnemy() const
{
	return m_pBot->GetDecision()->IsEnemy(GetEntity());
}

//================================================================================
//================================================================================
bool CEntityMemory::IsFriend() const
{
	return m_pBot->GetDecision()->IsFriend(GetEntity());
}

//================================================================================
// Returns whether the specified hitbox is visible and sets its value to [vecPosition].
// If the specified hitbox (favorite) is not visible, it will try another hitbox with a lower priority.
//================================================================================
bool CEntityMemory::GetVisibleHitboxPosition(Vector & vecPosition, HitboxType favorite)
{
	if (IsHitboxVisible(favorite)) {
		switch (favorite) {
			case HEAD:
			{
				vecPosition = m_VisibleHitbox.head;
				return true;
				break;
			}

			case CHEST:
			default:
			{
				vecPosition = m_VisibleHitbox.chest;
				return true;
				break;
			}

			case FEET:
			{
				vecPosition = m_VisibleHitbox.feet;
				return true;
				break;
			}
		}
	}

	if (favorite != CHEST && IsHitboxVisible(CHEST)) {
		vecPosition = m_VisibleHitbox.chest;
		return true;
	}

	if (favorite != HEAD && IsHitboxVisible(HEAD)) {
		vecPosition = m_VisibleHitbox.head;
		return true;
	}

	if (favorite != FEET && IsHitboxVisible(FEET)) {
		vecPosition = m_VisibleHitbox.feet;
		return true;
	}

	return false;
}

//================================================================================
// Update hitbox positions and visibility
// This function can be very expensive, it should only be called for the active enemy.
//================================================================================
void CEntityMemory::UpdateHitboxAndVisibility()
{
	VPROF_BUDGET("CEntityMemory::UpdateHitboxAndVisibility", VPROF_BUDGETGROUP_BOTS_EXPENSIVE);

	// For now let's assume that we do not know Hitbox and therefore we have no vision
	UpdateVisibility(false);

	m_Hitbox.Reset();
	m_VisibleHitbox.Reset();

	// We obtain the positions of the Hitbox
	Utils::GetHitboxPositions(GetEntity(), m_Hitbox);

	if (!m_Hitbox.IsValid())
		return;

	{
		VPROF_BUDGET("UpdateVisibility", VPROF_BUDGETGROUP_BOTS_EXPENSIVE);

		// Now let's check if we can see any of them
		if (m_pBot->GetDecision()->IsAbleToSee(m_Hitbox.head)) {
			m_VisibleHitbox.head = m_Hitbox.head;
			UpdateVisibility(true);
		}

		if (m_pBot->GetDecision()->IsAbleToSee(m_Hitbox.chest)) {
			m_VisibleHitbox.chest = m_Hitbox.chest;
			UpdateVisibility(true);
		}

		if (m_pBot->GetDecision()->IsAbleToSee(m_Hitbox.feet)) {
			m_VisibleHitbox.feet = m_Hitbox.feet;
			UpdateVisibility(true);
		}
	}

	// We update the ideal position
	GetVisibleHitboxPosition(m_vecIdealPosition, m_pBot->GetProfile()->GetFavoriteHitbox());
}