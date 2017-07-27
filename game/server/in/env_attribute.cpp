//==== Woots 2017. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "env_attribute.h"

#include "in_player.h"
#include "in_utils.h"
#include "in_attribute_system.h"
#include "eventqueue.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
// Información
//================================================================================

LINK_ENTITY_TO_CLASS( env_attribute, CEnvAttribute );

BEGIN_DATADESC( CEnvAttribute )
	DEFINE_KEYFIELD( m_nModifierName, FIELD_STRING, "ModifierName" ),
	DEFINE_KEYFIELD( m_flRadius, FIELD_FLOAT, "Radius" ),
	DEFINE_KEYFIELD( m_iTeam, FIELD_INTEGER, "Team" ),
	DEFINE_KEYFIELD( m_bDisabled, FIELD_BOOLEAN, "StartDisabled" ),
END_DATADESC()

//================================================================================
// Creación
//================================================================================
void CEnvAttribute::Spawn()
{
	AttributeInfo dummy;
	if ( !TheAttributeSystem->GetModifierInfo(STRING(m_nModifierName), dummy) )
	{
		Warning("El modificador de atributo: %s no existe.\n", STRING(m_nModifierName));
		Assert( !"El modificador de atributo no existe." );

		UTIL_Remove( this );
		return;
	}

	SetNextThink( gpGlobals->curtime + 0.1f );
}

//================================================================================
// Pensamiento
//================================================================================
void CEnvAttribute::Think() 
{
	// Desactivado
	if ( m_bDisabled )
	{
		SetNextThink( gpGlobals->curtime + 0.1f );
		return;
	}

	// Aplicamos el modificador
	Utils::AddAttributeModifier( STRING(m_nModifierName), m_flRadius, GetAbsOrigin(), m_iTeam );

	// Volvemos a pensar
	SetNextThink( gpGlobals->curtime + 0.1f );
}
