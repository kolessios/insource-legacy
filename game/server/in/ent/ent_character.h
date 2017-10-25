//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#ifndef ENT_CHARACTER_H
#define ENT_CHARACTER_H

#ifdef _WIN32
#pragma once
#endif

#include "basecombatcharacter.h"
#include "ent/ent_host.h"

//================================================================================
//================================================================================
class CEntCharacter : public CBaseCombatCharacter
{
public:
    DECLARE_CLASS( CEntCharacter, CBaseCombatCharacter );
    DECLARE_SERVERCLASS();
    DECLARE_DATADESC();

    //
    CEntHost *GetEntHost() {
        return m_pHost;
    }

    void SetEntHost( CEntHost *host ) {
        if ( m_pHost ) {
            delete m_pHost;
            m_pHost = NULL;
        }

        m_pHost = host;
    }

protected:
    CEntHost *m_pHost;
};

#endif