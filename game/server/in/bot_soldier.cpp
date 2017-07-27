//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "bot_soldier.h"

#include "in_utils.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
//================================================================================
void CBotSoldier::Spawn() 
{
	m_vecCover.Invalidate();
	BaseClass::Spawn();
}

//================================================================================
//================================================================================
void CBotSoldier::Process( CBotCmd *&cmd ) 
{
	UpdateNearestCover();

	BaseClass::Process( cmd );
}

//================================================================================
//================================================================================
void CBotSoldier::UpdateNearestCover() 
{
	// Buscamos un lugar seguro para ocultarnos
    CSpotCriteria criteria;
    criteria.SetMaxRange( 500.0f );
    criteria.UseNearest( true );
    criteria.OutOfVisibility( true );
    criteria.AvoidTeam( GetEnemy() );

	// Buscamos cerca de nuestro líder
	if ( Follow() && Follow()->IsFollowingSomeone() )
	{
		criteria.SetMaxRange( 700.0f );
		criteria.SetOrigin( Follow()->GetFollowing()->GetAbsOrigin() );
	}

	Utils::FindCoverPosition( &m_vecCover, GetHost(), criteria );
}

//================================================================================
//================================================================================
bool CBotSoldier::GetNearestCover( Vector &vecPosition ) 
{
	if ( m_vecCover.IsValid() )
	{
		vecPosition = m_vecCover;
		return true;
	}

	vecPosition.Invalidate();
	return false;
}

//================================================================================
//================================================================================
bool CBotSoldier::GetNearestCover() 
{
	if ( m_vecCover.IsValid() )
		return true;

	return false;
}

//================================================================================
//================================================================================
bool CBotSoldier::IsInCoverPosition()
{
	Vector vecPosition;
	const float tolerance = 75.0f;

	if ( !GetNearestCover(vecPosition) )
		return false;

	if ( GetAbsOrigin().DistTo(vecPosition) > tolerance )
		return false;

	return true;
}

//================================================================================
//================================================================================
bool CBotSoldier::ShouldFollow() 
{
	bool result = BaseClass::ShouldFollow();

	// Nope...
	if ( !result )
		return false;

	CBaseEntity *pEntity = GetFollowing();

	// Estamos en combate (y en una cobertura)
	if ( IsAlerted() || IsCombating() )
	{
		const float tolerance = 800.0f;

		// Nuestro líder aún esta cerca
		if ( GetAbsOrigin().DistTo(pEntity->GetAbsOrigin()) <= tolerance )
			return false;
	}

	return true;
}

//================================================================================
// Crea los componentes que tendrá el Bot
//================================================================================
void CBotSoldier::SetupComponents() 
{
	BaseClass::SetupComponents();

	//ADD_ALIAS_COMPONENT( m_FollowComponent, CSoldierFollowComponent );
}

//================================================================================
//================================================================================
void CBotSoldier::SetupSchedules() 
{
	ADD_SCHEDULE( CInvestigateSoundSchedule );
    ADD_SCHEDULE( CInvestigateLocationSchedule );
    ADD_SCHEDULE( CHuntEnemySchedule );
    ADD_SCHEDULE( CReloadSchedule );
    ADD_SCHEDULE( CChangeWeaponSchedule );
    ADD_SCHEDULE( CHideAndHealSchedule );
    ADD_SCHEDULE( CHideAndReloadSchedule );
    ADD_SCHEDULE( CHelpDejectedFriendSchedule );
    ADD_SCHEDULE( CMoveAsideSchedule );
    //ADD_SCHEDULE( CCallBackupSchedule );
	//ADD_SCHEDULE( CSoldierCoverSchedule );
    ADD_SCHEDULE( CDefendSpawnSchedule );
}