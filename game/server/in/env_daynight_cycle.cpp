//==== InfoSmart 2015. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "env_daynight_cycle.h"


#include "sun.h"
//#include "deferred/deferred_shared_common.h"
//#include "deferred/CDefLightGlobal.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
// Información y Red
//================================================================================

LINK_ENTITY_TO_CLASS( env_daynight_cycle, CEnvDayNightCycle );

DECLARE_SERVER_CMD( sv_daycycle_rate, "0.1", "" );
DECLARE_SERVER_CMD( sv_daycycle_seconds_rate, "30", "" );

//================================================================================
// Constructor
//================================================================================
CEnvDayNightCycle::CEnvDayNightCycle()
{
    m_iHour        = 0;
    m_iMinute    = 0;
    m_iSecond    = 0;
    m_iDay        = 0;
}

//================================================================================
// [Evento] La entidad ha sido activada
//================================================================================
void CEnvDayNightCycle::Activate()
{
    BaseClass::Activate();

    // Establecemos las entidades de luz global y sol
    //m_pLightGlobal    = ( CDeferredLightGlobal* )gEntList.FindEntityByClassname( NULL, "light_deferred_global" );
    m_pSun            = ( CSun * )gEntList.FindEntityByClassname( NULL, "env_sun" );
        
    // ¡No hay luz global en este mapa!
    //if ( !GlobalLight() ) {
        SetNextThink( NULL );
        return;
    //}

    // Comenzamos a pensar
    SetNextThink( gpGlobals->curtime );
}

//================================================================================
// [Evento] Pensamiento
//================================================================================
void CEnvDayNightCycle::Think()
{
    ConVarRef sv_skyname("sv_skyname");
    m_iSecond += sv_daycycle_seconds_rate.GetInt();

    // Un minuto más
    if ( m_iSecond > 59 )
    {
        ++m_iMinute;
        m_iSecond = 0;
    }
        
    // Una hora más
    if ( m_iMinute > 59 )
    {
        ++m_iHour;
        m_iMinute = 0;
    }

    // Un día más
    if ( m_iHour > 23 )
    {
        ++m_iDay;
        m_iHour = 0;
    }

    // Calculamos el nuevo Pitch
    float magic        = 0.00219907407;
    float total        = m_iSecond + (m_iMinute*60) + (m_iHour*3600);
    float pitch        = total * magic;

    DevMsg(2, "[CEnvDayNightCycle] %i:%i:%i -> %f -> %f \n", m_iHour, m_iMinute, m_iSecond, total, pitch);

    // Establecemos el nuevo Pitch
    /*QAngle gAngle    = GlobalLight()->GetAbsAngles();
    QAngle gNow        = gAngle;
    gNow[PITCH]        = pitch;

    // Pitch fuera del mapa, desactivamos las sombras
    if ( pitch < 10 || pitch > 179 )
    {
        //GlobalLight()->SetShadowsDisabled( true );
        sv_skyname.SetValue("sky_day01_09");
    }
    else
    {
        //GlobalLight()->SetShadowsDisabled( false );
        sv_skyname.SetValue("sky_day01_01");
    }

    // Establecemos los nuevos angulos de la luz global y el sol
    GlobalLight()->SetAbsAngles( gNow );

    if ( Sun() ) {
        Sun()->SetAbsAngles( gNow );
    }*/

    // Volvemos a pensar en...
    SetNextThink( gpGlobals->curtime + sv_daycycle_rate.GetFloat() );
}