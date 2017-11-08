//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#include "cbase.h"
#include "c_scp_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
// Commands
//================================================================================

//================================================================================
// Data and network
//================================================================================

LINK_ENTITY_TO_CLASS( player, C_SCP_Player );

IMPLEMENT_CLIENTCLASS_DT( C_SCP_Player, DT_SCP_Player, CSCP_Player )
END_RECV_TABLE()

BEGIN_PREDICTION_DATA( C_SCP_Player )
END_PREDICTION_DATA()

//================================================================================
// Constructor
//================================================================================
C_SCP_Player::C_SCP_Player()
{
    // TODO
    m_pMovementSound = new CSoundInstance( "SCP173.Movement", this, this );
}

//================================================================================
//================================================================================
const QAngle& C_SCP_Player::GetRenderAngles()
{
    if ( GetPlayerClass() == PLAYER_CLASS_SCP_173 )
    {
        // SCP-173 es una estatua sin animaciones ni nada complejo
        // Arreglamos los angulos hacia donde miramos y evitamos que el modelo se mueva hacia abajo/arriba.
        QAngle *angles = new QAngle;
        VectorCopy( GetAbsAngles(), *angles );
        angles->y -= 270.0f;
        angles->x = 0;

        return *angles;
    }
    
    return BaseClass::GetRenderAngles();
}
