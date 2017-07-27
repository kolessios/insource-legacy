//==== Woots 2017. http://creativecommons.org/licenses/by/2.5/mx/ ===========//
//
// Funciones de acceso rápido y personalización sobre el sistema de seguimiento
//
//=============================================================================//

#include "cbase.h"
#include "bot.h"

#include "in_utils.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
//================================================================================
CBaseEntity *CBot::GetFollowing()
{
	if ( !Follow() )
		return NULL;

	return Follow()->GetFollowing();
}

//================================================================================
//================================================================================
bool CBot::IsFollowingSomeone() 
{
	if ( !Follow() )
		return false;

	return Follow()->IsFollowingSomeone();
}

//================================================================================
//================================================================================
bool CBot::IsFollowingPlayer() 
{
	if ( !Follow() )
		return false;

	return Follow()->IsFollowingPlayer();
}

//================================================================================
//================================================================================
bool CBot::IsFollowingPaused()
{
    if ( !Follow() )
        return false;

    return Follow()->IsPaused();
}

//================================================================================
//================================================================================
bool CBot::ShouldFollow() 
{
    if ( !Navigation() )
        return false;

    if ( !IsFollowingSomeone() )
        return false;

    if ( IsFollowingPaused() )
        return false;

	return true;
}

//================================================================================
//================================================================================
void CBot::StartFollow( CBaseEntity *pEntity, bool bFollow ) 
{
	if ( !Follow() )
		return;

	return Follow()->Start( pEntity, bFollow );
}

//================================================================================
//================================================================================
void CBot::StartFollow( const char *pEntityName, bool bFollow ) 
{
	if ( !Follow() )
		return;

	return Follow()->Start( pEntityName, bFollow );
}

//================================================================================
//================================================================================
void CBot::StopFollow() 
{
	if ( !Follow() )
		return;

	return Follow()->Stop();
}

//================================================================================
//================================================================================
void CBot::PauseFollow() 
{
	if ( !Follow() )
		return;

	return Follow()->Pause();
}

//================================================================================
//================================================================================
void CBot::ResumeFollow() 
{
	if ( !Follow() )
		return;

	return Follow()->Resume();
}
