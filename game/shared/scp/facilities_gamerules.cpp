//==== Woots 2017. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "scp_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar sv_gamemode;

//================================================================================
// Reglas de juego base para SCP: Source
//================================================================================
class CFacilitiesGameRules : public CSCPGameRules
{
public:
    DECLARE_CLASS( CFacilitiesGameRules, CSCPGameRules );

    CFacilitiesGameRules();

#ifndef CLIENT_DLL
#else
#endif
};

REGISTER_GAMERULES_CLASS( CFacilitiesGameRules );

//================================================================================
// Constructor
//================================================================================
CFacilitiesGameRules::CFacilitiesGameRules()
{
    TheGameRules = this;

    // Establecemos el modo de juego
    SetGameMode( sv_gamemode.GetInt() );
}