//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "bot.h"
#include "bot_manager.h"

#include "in_utils.h"

#include "nav.h"
#include "nav_mesh.h"

#include "in_buttons.h"
#include "basetypes.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
// Devuelve si el Bot puede teletransportarse al quedar atascado
//================================================================================
bool CNavigationTeleportComponent::ShouldTeleport( const Vector &vecGoal ) 
{
    return true;
}

//================================================================================
//================================================================================
void CNavigationTeleportComponent::TrackPath( const Vector &pathGoal, float deltaT )
{
    // Apuntamos al destino
    if ( GetBot()->Aim() ) 
    {
        Vector lookAt( pathGoal );
        lookAt.z = GetHost()->EyePosition().z;
        GetBot()->Aim()->LookAt( "Aiming Route", lookAt, PRIORITY_LOW, 0.5f );
    }

    // Próximo punto de destino
    m_vecNextSpot = pathGoal;

    // Teleport
    GetHost()->Teleport( &pathGoal, &GetHost()->GetAbsAngles(), NULL );
}