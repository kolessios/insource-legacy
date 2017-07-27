//==== Woots 2017. http://creativecommons.org/licenses/by/2.5/mx/ ===========//
//
// Funciones de acceso rápido y personalización sobre el sistema de ataque
//
//=============================================================================//

#include "cbase.h"
#include "bot.h"

#include "in_utils.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
// Realiza un ataque con el arma de fuego
//================================================================================
void CBot::FiregunAttack()
{
	if ( !Attack() )
		return;

	Attack()->FiregunAttack();
}

//================================================================================
// Realiza un ataque cuerpo a cuerpo
//================================================================================
void CBot::MeleeWeaponAttack() 
{
	if ( !Attack() )
		return;

	Attack()->MeleeWeaponAttack();
}

//================================================================================
// Devuelve si estamos usando un arma de fuego
//================================================================================
bool CBot::IsUsingFiregun() 
{
	if ( !Attack() )
		return false;

	return Attack()->IsUsingFiregun();
}

//================================================================================
// Devuelve si podemos disparar ahora
//================================================================================
bool CBot::CanShot() 
{
	if ( !Attack() )
		return false;

	return Attack()->CanShot();
}

//================================================================================
// Devuelve si el Bot puede disparar agachado
//================================================================================
bool CBot::CanDuckShot() 
{
	if ( !Attack() )
		return false;

	return Attack()->CanDuckShot();
}
