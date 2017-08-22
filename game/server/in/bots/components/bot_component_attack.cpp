//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#include "cbase.h"
#include "bot.h"

#include "bot_manager.h"

#include "in_utils.h"
#include "in_buttons.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
// Called on each frame to update component operation.
//================================================================================
void CBotAttack::Update()
{
    VPROF_BUDGET( "CBotAttack::Update", VPROF_BUDGETGROUP_BOTS );

    if (
        HasCondition( BCOND_CAN_RANGE_ATTACK1 ) || HasCondition( BCOND_CAN_MELEE_ATTACK1 ) ||
        HasCondition( BCOND_CAN_RANGE_ATTACK2 ) || HasCondition( BCOND_CAN_MELEE_ATTACK2 )
        ) {
        if ( GetDecision()->IsUsingFiregun() ) {
            FiregunAttack();
        }
        else {
            MeleeWeaponAttack();
        }
    }
}

//================================================================================
// Activates the attack of a firearm
//================================================================================
void CBotAttack::FiregunAttack()
{
    CBaseWeapon *pWeapon = GetHost()->GetActiveBaseWeapon();
    Assert( pWeapon );

    


    if ( GetDecision()->CanCrouchShot() ) {
        float flDistance = GetMemory()->GetPrimaryThreatDistance();

        // Check to see if we can crouch for accuracy
        if ( GetLocomotion() ) {
            if ( GetDecision()->ShouldCrouchShot() && flDistance >= 140.0f ) {
                GetLocomotion()->Crouch();
            }
            else {
                GetLocomotion()->StandUp();
            }
        }
    }

    // Podemos disparar
    if ( HasCondition( BCOND_CAN_RANGE_ATTACK1 ) && CanShot() ) {
        GetBot()->Combat();

        // Atacamos
        InjectButton( IN_ATTACK );
        
    }

    // Podemos usar el disparo secundario
    // TODO
    if ( HasCondition( BCOND_CAN_RANGE_ATTACK2 ) ) {
        GetBot()->Combat();
        InjectButton( IN_ATTACK2 );
    }
}

//================================================================================
//================================================================================
void CBotAttack::MeleeWeaponAttack()
{
    if ( HasCondition( BCOND_CAN_MELEE_ATTACK1 ) ) {
        GetBot()->Combat();
        InjectButton( IN_ATTACK );
    }

    if ( HasCondition( BCOND_CAN_MELEE_ATTACK2 ) ) {
        GetBot()->Combat();
        InjectButton( IN_ATTACK2 );
    }
}
