//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "bot_controller.h"

#include "in_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
// Constructor
//================================================================================
CBotController::CBotController()
{
    SetPlayer( NULL );
}

//================================================================================
// Constructor
//================================================================================
CBotController::CBotController( CBaseInPlayer *pCharacter )
{
    SetPlayer( pCharacter );
}

//================================================================================
//================================================================================
void CBotController::SetPlayer( CBaseInPlayer *pCharacter )
{
    m_pPlayer = pCharacter;
}

//================================================================================
//================================================================================
const char *CBotController::GetName()
{
    return GetPlayer()->GetPlayerInfo()->GetName();
}

//================================================================================
//================================================================================
const char *CBotController::GetName()
{
    return GetPlayer()->GetPlayerInfo()->GetName();
}