//==== InfoSmart 2015. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "in_ammodef.h"
#include "in_gamerules.h"
#include "ammodef.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
// Comandos
//================================================================================

DECLARE_SERVER_CMD( sk_max_sniper_ammo, "999", "" );
DECLARE_SERVER_CMD( sk_max_rifle_ammo, "999", "" );
DECLARE_SERVER_CMD( sk_max_pistol_ammo, "999", "" );
DECLARE_SERVER_CMD( sk_max_shotgun_ammo, "999", "" );

//================================================================================
//================================================================================

// shared ammo definition
// JAY: Trying to make a more physical bullet response
#define BULLET_MASS_GRAINS_TO_LB(grains)    (0.002285*(grains)/16.0f)
#define BULLET_MASS_GRAINS_TO_KG(grains)    lbs2kg(BULLET_MASS_GRAINS_TO_LB(grains))

// exaggerate all of the forces, but use real numbers to keep them consistent
#define BULLET_IMPULSE_EXAGGERATION            1    

// convert a velocity in ft/sec and a mass in grains to an impulse in kg in/s
#define BULLET_IMPULSE(grains, ftpersec)    ((ftpersec)*12*BULLET_MASS_GRAINS_TO_KG(grains)*BULLET_IMPULSE_EXAGGERATION)

#define BULLET_SPEED(ftpersec) 0.12*ftpersec

//================================================================================
//================================================================================
CAmmoDef* GetAmmoDef()
{
    static CAmmoDef def;
    static bool bInitted = false;

    if ( !bInitted )
    {
        bInitted = true;

        def.AddAmmoType( "AMMO_TYPE_SNIPERRIFLE", DMG_BULLET,   TRACER_LINE_AND_WHIZ,   0,  0,  "sk_max_sniper_ammo",   BULLET_SPEED(3825), BULLET_IMPULSE(100, 1025),  0 );
        def.AddAmmoType("AMMO_TYPE_ASSAULTRIFLE", DMG_BULLET,	TRACER_LINE_AND_WHIZ,	0,	0,	"sk_max_rifle_ammo",	BULLET_SPEED(1225), BULLET_IMPULSE(100, 1025),	0 );        
		def.AddAmmoType("AMMO_TYPE_PISTOL",       DMG_BULLET,   TRACER_LINE_AND_WHIZ,   0,  0,	"sk_max_pistol_ammo",   BULLET_SPEED(1225), BULLET_IMPULSE(200, 1125),  0 );
        def.AddAmmoType("AMMO_TYPE_SHOTGUN",      DMG_BULLET,   TRACER_LINE_AND_WHIZ,   0,  0,  "sk_max_shotgun_ammo",  BULLET_SPEED(1200), BULLET_IMPULSE(300, 1225),  0 );
    }

    return &def;
}