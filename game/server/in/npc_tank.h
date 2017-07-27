//==== Woots 2017. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef NPC_TANK_H
#define NPC_TANK_H

#ifdef _WIN32
#pragma once
#endif

#include "npc_base_infected.h"

//================================================================================
// Representa un Jefe en Apocalypse
//================================================================================
class CNPC_Tank : public CBaseInfected
{
public:
    DECLARE_CLASS( CNPC_Tank, CBaseInfected );
    DECLARE_DATADESC();
    DEFINE_CUSTOM_AI;

	// Información
    virtual int GetNPCHealth();
    virtual const char *GetNPCModel();
    virtual float GetMeleeDistance();
    virtual float GetMeleeDamage();
}

#endif