//==== Woots 2017. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "in_attribute_system.h"

#include "dbhandler.h"
#include "players_system.h"

#include "filesystem.h"

#include "KeyValues.h"
#include "fmtstr.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define ATTRIBUTES_FILE "scripts/attributes.txt"
#define MODIFIERS_FILE "scripts/attributes_modifiers.txt"

CAttributeSystem g_Manager;
CAttributeSystem *TheAttributeSystem = &g_Manager;

//================================================================================
// Constructor
//================================================================================
CAttributeSystem::CAttributeSystem() : CAutoGameSystem("CAttributeSystem")
{
}

//================================================================================
// Inicialización
//================================================================================
void CAttributeSystem::PostInit()
{
	//SetDefLessFunc( m_AttributesList );
	//SetDefLessFunc( m_ModifiersList );

    Assert( ThePlayersDatabase != NULL );

    if ( ThePlayersDatabase == NULL )
        return;

	Load();
}

//================================================================================
// Carga el archivo de atributos y modificadores
//================================================================================
void CAttributeSystem::Load() 
{
	// Atributos
	LoadAttributes();

	// Modificadores
	LoadModifiers();
}

//================================================================================
// Carga el archivo de atributos y guarda la información
//================================================================================
void CAttributeSystem::LoadAttributes()
{
    // Limpiamos la lista, por si estamos recargando
    m_AttributesList.Purge();

    // Consulta para leer los atributos registrados
    dbReadResult *results = ThePlayersDatabase->ReadMultiple( "SELECT name,value,rate,amount,max,min FROM attributes" );

    // Por cada fila, leemos las 6 columnas
    FOR_EACH_RESULT( it, results, 6 )
    {
        AttributeInfo info;
        Q_strncpy( info.name, COLUMN( results, 0 ).text, sizeof( info.name ) );
        info.value = COLUMN( results, 1 ).floating;
        info.rate = COLUMN( results, 2 ).floating;
        info.amount = COLUMN( results, 3 ).floating;
        info.max = COLUMN( results, 4 ).floating;
        info.min = COLUMN( results, 5 ).floating;

        // Lo agregamos a la lista
        DevMsg( "[CAttributeSystem] Atributo: %s. (%.2f) (%.2f) (%.2f)\n", info.name, info.value, info.rate, info.amount );
        m_AttributesList.AddToTail( info );
    }

    FOR_EACH_VEC( m_AttributesList, it )
    {
        const char *lol = m_AttributesList.Element( it ).name;
        DevMsg( "[CAttributeSystem] -- Atributo: %s.\n", lol );
    }

    // Liberamos la memoria usada
    results->Purge();
    delete results;

    // Hemos terminado
    DevMsg( "[CAttributeSystem] Se han cargado %i atributos.\n", m_AttributesList.Count() );
}

//================================================================================
// Carga el archivo de modificadores y guarda la información
//================================================================================
void CAttributeSystem::LoadModifiers() 
{
    // Limpiamos la lista, por si estamos recargando
    m_ModifiersList.Purge();

    // Consulta para leer los atributos registrados
    dbReadResult *results = ThePlayersDatabase->ReadMultiple("SELECT name,affects,value,rate,rate_absolute,amount,amount_absolute,max,min,duration FROM attributes_modifiers");

    // Por cada fila, leemos las 10 columnas
    FOR_EACH_RESULT( it, results, 10 )
    {
        const char *attrName = COLUMN( results, 0 ).text;

        AttributeInfo info;
        Q_strncpy( info.name, attrName, sizeof( info.name ) );
        Q_strncpy( info.affects, COLUMN(results, 1).text, sizeof(info.affects) );
		info.value	         = COLUMN(results, 2).floating;
		info.rate	         = COLUMN(results, 3).floating;
        info.rate_absolute   = (COLUMN(results, 4).integer == 1);
		info.amount	         = COLUMN(results, 5).floating;
        info.amount_absolute = (COLUMN(results, 6).integer == 1);
		info.max	         = COLUMN(results, 7).floating;
		info.min	         = COLUMN(results, 8).floating;
        info.duration        = COLUMN(results, 9).floating;

        // Lo agregamos a la lista
        m_ModifiersList.AddToTail( info );
		DevMsg("[CAttributeSystem] Modificador: %s. (%.2f) (%.2f) (%.2f)\n", info.name, info.value, info.rate, info.amount);
    }

    // Liberamos la memoria usada
    results->Purge();
    delete results;

    // Hemos terminado
    DevMsg("[CAttributeSystem] Se han cargado %i modificadores.\n", m_ModifiersList.Count());

    /*
	m_ModifiersList.Purge();

	KeyValues *pFile = new KeyValues("AttributesModifiers");
    KeyValues::AutoDelete autoDelete( pFile );

	// Leemos el archivo
    pFile->LoadFromFile( filesystem, MODIFIERS_FILE, NULL );
    
	KeyValues *kAttribute;

	// Attributo
    for ( kAttribute = pFile->GetFirstTrueSubKey(); kAttribute; kAttribute = kAttribute->GetNextTrueSubKey() )
    {
		KeyValues *kData;
		AttributeInfo info;

		// Predeterminado
		info.name			= kAttribute->GetName();
		info.value			= 0.0f;
		info.rate			= 0.0f;
		info.amount			= 0.0f;
		info.max			= 0.0f;
		info.min			= 0.0f;
		info.duration		= 0.0f;
		info.amount_absolute = false;
		info.rate_absolute	= false;

		Q_strncpy( info.affects, "", 100 );
		        
		// Configuración
        for ( kData = kAttribute->GetFirstSubKey(); kData; kData = kData->GetNextKey() )
        {
			if ( FStrEq(kData->GetName(), "value") )
			{
				info.value = kData->GetFloat( (const char *)NULL, 0.0f );
			}

			if ( FStrEq(kData->GetName(), "rate") )
			{
				info.rate = kData->GetFloat( (const char *)NULL, 0.0f );
			}

			if ( FStrEq(kData->GetName(), "amount") )
			{
				info.amount = kData->GetFloat( (const char *)NULL, 0.0f );
			}

			if ( FStrEq(kData->GetName(), "max") )
			{
				info.max = kData->GetFloat( (const char *)NULL, 0.0f );
			}

			if ( FStrEq(kData->GetName(), "min") )
			{
				info.min = kData->GetFloat( (const char *)NULL, 0.0f );
			}

			if ( FStrEq(kData->GetName(), "amount_absolute") )
			{
				info.amount_absolute = kData->GetBool( (const char *)NULL, false );
			}

			if ( FStrEq(kData->GetName(), "rate_absolute") )
			{
				info.rate_absolute = kData->GetBool( (const char *)NULL, false );
			}

			if ( FStrEq(kData->GetName(), "duration") )
			{
				info.duration = kData->GetFloat( (const char *)NULL, 0.0f );
			}

			if ( FStrEq(kData->GetName(), "affects") )
			{
				Q_strncpy( info.affects, kData->GetString((const char *)NULL, ""), 100 );
			}
		}

		// Lo agregamos a la lista
		m_ModifiersList.InsertOrReplace( kAttribute->GetName(), info );
		DevMsg(2, "[CAttributeSystem] Modificador: %s -> %s.\n", info.name, info.affects);
	}

	DevMsg("[CAttributeSystem] Se han cargado %i modificadores.\n", m_ModifiersList.Count());
    */
}

//================================================================================
// Devuelve si se ha podido encontrar y establecer la información del atributo
//================================================================================
bool CAttributeSystem::GetAttributeInfo( const char *name, AttributeInfo &info ) 
{
    FOR_EACH_VEC( m_AttributesList, it )
    {
        AttributeInfo in = m_AttributesList.Element( it );

        if ( FStrEq( in.name, name ) )
        {
            info = in;
            return true;
        }
    }

    return false;
}

//================================================================================
// Devuelve si se ha podido encontrar y establecer la información del modificador
//================================================================================
bool CAttributeSystem::GetModifierInfo( const char *name, AttributeInfo &info ) 
{
    FOR_EACH_VEC( m_ModifiersList, it )
    {
        AttributeInfo in = m_ModifiersList.Element( it );

        if ( FStrEq( in.name, name ) )
        {
            info = in;
            return true;
        }
    }

    return false;
}

//================================================================================
// Crea y devuelve un nuevo objeto [CAttribute] con el nombre especificado
//================================================================================
CAttribute *CAttributeSystem::GetAttribute( const char *name ) 
{
	AttributeInfo info;

	// No se ha encontrado este atributo
	if ( !GetAttributeInfo(name, info) )
		return NULL;

	return new CAttribute( info );
}

//================================================================================
//================================================================================
CON_COMMAND( sv_attributes_reload, "Reload the attributes configuration" )
{
	TheAttributeSystem->Load();
}

CON_COMMAND( sv_attributes_test, "" )
{
    CAttribute *pAttribute = TheAttributeSystem->GetAttribute( "stamina" );
    Assert( pAttribute );

    if ( !pAttribute ) return;

    DevMsg("ID: %s \n", pAttribute->GetID());

    delete pAttribute;
}

CON_COMMAND( sv_keyvalues_test, "" )
{
	KeyValues *pFile = new KeyValues("KeyValuesTest");
    KeyValues::AutoDelete autoDelete( pFile );

	// Leemos el archivo
    pFile->LoadFromFile( filesystem, "scripts/keyvalues_test.txt", NULL );
    
	KeyValues *kSection;

	// Attributo
    for ( kSection = pFile->GetFirstTrueSubKey(); kSection; kSection = kSection->GetNextTrueSubKey() )
    {
		DevMsg("%s\n", kSection->GetName());
		DevMsg("----\n");

		KeyValues *kData;
		        
		// Configuración
        for ( kData = kSection->GetFirstSubKey(); kData; kData = kData->GetNextKey() )
        {
			DevMsg("- GetName: %s\n", kData->GetName());
			DevMsg("- GetFloat: %.2f\n", kData->GetFloat());
			DevMsg("- GetString: %s\n", kData->GetString());
			DevMsg("----\n");
		}
	}
}