//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef CHARACTER_REFENCE_H
#define CHARACTER_REFENCE_H

#ifdef _WIN32
#pragma once
#endif

//#include "iplayerinfo.h"
#include "basecombatcharacter.h"

class CBotController : public IBotController
{
public:
    CBotController();
    CBotController( CBaseInPlayer *pPlayer );

    // Player
    virtual CBaseInPlayer *GetPlayer() { m_pPlayer; }
    virtual void SetPlayer( CBaseInPlayer *pPlayer );

    // IBotController
    virtual void SetAbsOrigin( Vector & vec );
    virtual void SetAbsAngles( QAngle & ang );
    virtual void SetLocalOrigin( const Vector& origin );
    virtual const Vector GetLocalOrigin( void );
    virtual void SetLocalAngles( const QAngle& angles );
    virtual const QAngle GetLocalAngles( void );

    // IPlayerInfo
    virtual const char *GetName();
    virtual int GetUserID();

protected:
    CBaseInPlayer *m_pPlayer;
};

#endif // CHARACTER_REFENCE_H
