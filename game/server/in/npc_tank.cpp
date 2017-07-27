//==== Woots 2017. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "npc_tank.h"

#include "in_shareddefs.h"

#include "physics_prop_ragdoll.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
//================================================================================

DECLARE_NPC_HEALTH( CNPC_Tank, sk_tank_health, "300" )
DECLARE_NPC_MELEE_DAMAGE( CNPC_Tank, sk_tank_damage, "30" )
DECLARE_NPC_MELEE_DISTANCE( CNPC_Tank, sk_tank_melee_distance, "120" )

DECLARE_REPLICATED_COMMAND( sk_tank_vision_distance, "912", "Max Vision" )
DECLARE_REPLICATED_COMMAND( sk_tank_wander, "0", "" )

//================================================================================
// Información y Red
//================================================================================

LINK_ENTITY_TO_CLASS( npc_tank, CNPC_Tank );

BEGIN_DATADESC( CNPC_Tank )
END_DATADESC()