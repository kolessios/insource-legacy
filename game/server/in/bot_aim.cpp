//==== Woots 2017. http://creativecommons.org/licenses/by/2.5/mx/ ===========//
//
// Funciones de acceso rápido y personalización sobre el sistema de apuntado
//
//=============================================================================//

#include "cbase.h"
#include "bot.h"

#include "in_utils.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
// Devuelve si estamos apuntando a alguna parte.
//================================================================================
bool CBot::IsAiming()
{
    if ( !Aim() )
        return false;

    return Aim()->IsAiming();
}

//================================================================================
// Devuelve si el punto al que queremos apuntar esta en el rango de nuestra visión
//================================================================================
bool CBot::IsAimingReady()
{
	if ( !Aim() )
		return false;

	return Aim()->IsAimingReady();
}

//================================================================================
// Devuelve la prioridad del punto al que queremos apuntar
//================================================================================
int CBot::GetLookPriority() 
{
	if ( !Aim() )
		return PRIORITY_INVALID;

	return Aim()->GetPriority();
}

//================================================================================
// Devuelve la velocidad del mouse al apuntar
//================================================================================
AimSpeed CBot::GetAimSpeed() 
{
	if ( !Aim() )
		return AIM_SPEED_LOW;

	return Aim()->GetAimSpeed();
}

//================================================================================
// Devuelve la entidad a la que estamos apuntando
//================================================================================
CBaseEntity * CBot::GetLookTarget() 
{
	if ( !Aim() )
		return NULL;

	return Aim()->GetLookTarget();
}

//================================================================================
// Devuelve la posición a donde estamos apuntando
//================================================================================
const Vector &CBot::GetLookingAt() 
{
	if ( !Aim() )
		return vec3_invalid;

	return Aim()->GetLookingAt();
}

//================================================================================
// Restablece el apuntado
//================================================================================
void CBot::ClearLook() 
{
	if ( !Aim() )
		return;

	return Aim()->Clear();
}

//================================================================================
// Hace que el Bot apunte a la entidad especificada
//================================================================================
void CBot::LookAt( const char *pDesc, CBaseEntity *pEntity, int priority, float duration ) 
{
	if ( !Aim() )
		return;

	return Aim()->LookAt( pDesc, pEntity, priority, duration );
}

//================================================================================
// Hace que el Bot apunte a la entidad especificada
//================================================================================
void CBot::LookAt( const char *pDesc, CBaseEntity *pEntity, const Vector &vecLook, int priority, float duration ) 
{
	if ( !Aim() )
		return;

	return Aim()->LookAt( pDesc, pEntity, vecLook, priority, duration );
}

//================================================================================
// Hace que el Bot apunte a la posición especificada
//================================================================================
void CBot::LookAt( const char *pDesc, const Vector &vecLook, int priority, float duration ) 
{
	if ( !Aim() )
		return;

	return Aim()->LookAt( pDesc, vecLook, priority, duration );
}

//================================================================================
// Devuelve si es posible apuntar a un lugar interesante
//================================================================================
bool CBot::ShouldLookInterestingSpot() 
{
	if ( !Aim() )
		return false;

    return Aim()->m_nIntestingAimTimer.IsElapsed();
}

//================================================================================
// Devuelve si es posible apuntar a un lugar al azar
//================================================================================
bool CBot::ShouldLookRandomSpot() 
{
	if ( !Aim() )
		return false;

    return Aim()->m_nRandomAimTimer.IsElapsed();
}

//================================================================================
// Devuelve si es posible apuntar a un miembro de nuestro escuadron
//================================================================================
bool CBot::ShouldLookSquadMember() 
{
	if ( !Aim() )
		return false;

    // Hasta que terminemos de mirar lo que estabamos mirando
    if ( !Aim()->IsAimingTimeExpired() )
        return false;

    // No podemos mirarnos a nosotros mismos
    if ( GetSquad()->GetCount() == 1 )
        return false;

    return true;
}

//================================================================================
// Devuelve si deberíamos apuntar a nuestro enemigo (o su última posición conocida)
//================================================================================
bool CBot::ShouldLookEnemy()
{
    if ( !GetEnemy() )
        return false;

    CEnemyMemory *memory = GetEnemyMemory();
    Assert( memory );

    // No nos agobiemos mirando a enemigos sin importancia a menos que esten cerca de mi
    if ( !IsDangerousEnemy() && !HasCondition( BCOND_ENEMY_NEAR ) )
        return false;

    // Si hemos perdido visión de nuestro enemigo por un tiempo y además estamos
    // a una distancia considerable sin poder ver su última posición, entonces miramos
    // otros lugares donde podrían estar más enemigos
    if ( !GetSkill()->IsEasy() ) {
        if ( HasCondition( BCOND_ENEMY_OCCLUDED ) ) {
            if ( HasCondition( BCOND_ENEMY_FAR ) || HasCondition( BCOND_ENEMY_LAST_POSITION_OCCLUDED ) )
                return false;
        }
    }

    return true;
}
