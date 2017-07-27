//==== Woots 2017. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef ENV_DAYNIGHT_CYCLE_H
#define ENV_DAYNIGHT_CYCLE_H

#ifdef _WIN32
#pragma once
#endif

class CSun;
class CDeferredLightGlobal;

class CEnvDayNightCycle : public CPointEntity
{
public:
    DECLARE_CLASS( CEnvDayNightCycle, CPointEntity );

    CEnvDayNightCycle();

    virtual void Activate();
    virtual void Think();

    //virtual CDeferredLightGlobal *GlobalLight() { return m_pLightGlobal; }
    virtual CSun *Sun() { return m_pSun; }
protected:
    //CDeferredLightGlobal *m_pLightGlobal;
    CSun *m_pSun;

    int m_iHour;
    int m_iMinute;
    int m_iSecond;
    int m_iDay;
};

#endif // ENV_DAYNIGHT_CYCLE_H