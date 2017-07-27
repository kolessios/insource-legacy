//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "in_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar sv_gamemode;

//================================================================================
// Instala las reglas del juego del modo actual
//================================================================================
void InstallGameRules()
{
	CreateGameRulesObject( "CFacilitiesGameRules" );
}