//==== Woots 2017. ===========//

#include "cbase.h"
#include "in_player_component.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
// Constructor
//================================================================================
CPlayerComponent::CPlayerComponent() : BaseClass()
{
    Init();
}

//================================================================================
// Constructor
//================================================================================
CPlayerComponent::CPlayerComponent( CBasePlayer *pParent ) : BaseClass()
{
	SetParent( pParent );
    Init();
}