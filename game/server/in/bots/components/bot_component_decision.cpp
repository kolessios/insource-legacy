//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#include "cbase.h"
#include "bots\bot.h"

#include "players_system.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar bot_locomotion_allow_teleport;
extern ConVar bot_locomotion_hiddden_teleport;
extern ConVar bot_locomotion_allow_wiggle;
extern ConVar bot_debug_jump;
extern ConVar bot_primary_attack;
extern ConVar bot_dont_attack;

//================================================================================
//================================================================================
bool CBotDecision::ShouldLookInterestingSpot() const
{
    return m_IntestingAimTimer.IsElapsed();
}

//================================================================================
//================================================================================
bool CBotDecision::ShouldLookRandomSpot() const
{
    return m_RandomAimTimer.IsElapsed();
}

//================================================================================
//================================================================================
bool CBotDecision::ShouldLookSquadMember() const
{
    if ( !GetVision()->IsVisionTimeExpired() )
        return false;

    if ( !GetBot()->GetSquad() )
        return false;

    if ( GetBot()->GetSquad()->GetCount() == 1 )
        return false;

    return true;
}

//================================================================================
//================================================================================
bool CBotDecision::ShouldLookThreat() const
{
    CEntityMemory *memory = GetBot()->GetPrimaryThreat();
    
    if ( !memory )
        return false;

    // Let's not bother looking at unimportant enemies unless they are close to me.
    if ( !IsDangerousEnemy() && !HasCondition( BCOND_ENEMY_NEAR ) ) {
        return false;
    }

    // We have lost sight of our enemy for a while, let us look elsewhere
    if ( !GetSkill()->IsEasy() ) {
        if ( HasCondition( BCOND_ENEMY_OCCLUDED ) && HasCondition( BCOND_ENEMY_LOST ) ) {
            return false;
        }
    }

    return true;
}

//================================================================================
//================================================================================
bool CBotDecision::ShouldFollow() const
{
    if ( !GetLocomotion() )
        return false;

    if ( !GetFollow()->IsFollowing() )
        return false;

    if ( !GetFollow()->IsEnabled() )
        return false;

    return true;
}


//================================================================================
//================================================================================
bool CBotDecision::ShouldUpdateNavigation() const
{
    if ( !CanMove() )
        return false;

    return true;
}

//================================================================================
//================================================================================
bool CBotDecision::ShouldTeleport( const Vector &vecGoal ) const
{
    // Solamente si nadie nos esta viendo
    if ( bot_locomotion_hiddden_teleport.GetBool() ) {
        if ( ThePlayersSystem->IsVisible( GetHost() ) )
            return false;

        if ( ThePlayersSystem->IsVisible( vecGoal ) )
            return false;
    }

    Vector vUpBit = GetHost()->GetAbsOrigin();
    vUpBit.z += 1;

    trace_t tr;
    UTIL_TraceHull( vecGoal, vUpBit, GetHost()->WorldAlignMins(), GetHost()->WorldAlignMaxs(),
                    MASK_SOLID, GetHost(), COLLISION_GROUP_NONE, &tr );

    if ( tr.startsolid || tr.fraction < 1.0 )
        return false;

    return bot_locomotion_allow_teleport.GetBool();
}

//================================================================================
// It returns if we should move randomly to try to untie us.
//================================================================================
bool CBotDecision::ShouldWiggle() const
{
    if ( !GetLocomotion() )
        return false;

    if ( !GetLocomotion()->IsStuck() )
        return false;

    if ( GetLocomotion()->GetStuckDuration() < 1.0f )
        return false;

    return bot_locomotion_allow_wiggle.GetBool();
}

//================================================================================
// Returns if we should run
// +speed
//================================================================================
bool CBotDecision::ShouldRun() const
{
    if ( !GetLocomotion() )
        return false;

    if ( !GetHost()->IsOnGround() || GetLocomotion()->IsJumping() )
        return false;

    if ( GetLocomotion()->IsUsingLadder() )
        return false;

    if ( GetLocomotion()->IsRunning() )
        return true;

    if ( GetBot()->GetActiveScheduleID() == SCHEDULE_NONE ) {
        if ( GetLocomotion()->HasDestination() ) {
            const float range = 130.0f;
            float flDistance = GetLocomotion()->GetDistanceToDestination();

            if ( flDistance > range )
                return true;
        }
    }

    return false;
}

//================================================================================
// Returns if we should walk (slow walking)
// +walk
//================================================================================
bool CBotDecision::ShouldSneak() const
{
    if ( !GetLocomotion() )
        return false;

    if ( GetLocomotion()->IsSneaking() )
        return true;

    return false;
}

//================================================================================
// Returns if we should crouch
// +duck
//================================================================================
bool CBotDecision::ShouldCrouch() const
{
    if ( !GetLocomotion() )
        return false;

    // Always crouch jump
    // TODO: Scenarios where crouch jump should not be done?
    if ( GetLocomotion()->IsJumping() )
        return true;

    if ( GetLocomotion()->IsCrouching() )
        return true;

    return false;
}

//================================================================================
// Returns if we should jump
// +jump
//================================================================================
bool CBotDecision::ShouldJump() const
{
    VPROF_BUDGET( "ShouldJump", VPROF_BUDGETGROUP_BOTS );

    if ( !GetLocomotion() )
        return false;

    if ( !GetHost()->IsOnGround() || GetLocomotion()->IsJumping() )
        return false;

    if ( GetLocomotion()->IsUsingLadder() )
        return false;

    if ( GetLocomotion()->HasDestination() ) {
        // TODO: All the code below creates traces in each frame to check if there is something blocking us and we need to jump.
        // Obviously I think this form is very bizarre and there are better ways to do it.

        // Height to check if something is blocking us
        Vector vecFeetBlocked = GetLocomotion()->GetFeet();
        vecFeetBlocked.z += StepHeight;

        // Height to see if we can jump
        Vector vecFeetClear = GetLocomotion()->GetFeet();
        vecFeetClear.z += JumpCrouchHeight + 5.0f;

        // We create a new vector with the value of the next position but at the height of the player's feet
        Vector vecNextSpot = GetLocomotion()->GetNextSpot();
        vecNextSpot.z = GetLocomotion()->GetFeet().z;

        // Sacamos el angulo hacia el destino
        Vector vec2Angles = vecNextSpot - GetLocomotion()->GetFeet();
        QAngle angles;
        VectorAngles( vec2Angles, angles );

        // Sacamos el vector que indica "enfrente"
        Vector vecForward;
        AngleVectors( angles, &vecForward );

        // Trazamos dos líneas para verificar que podemos hacer el salto
        trace_t blocked;
        UTIL_TraceLine( vecFeetBlocked, vecFeetBlocked + 30.0f * vecForward, MASK_SOLID, GetHost(), COLLISION_GROUP_NONE, &blocked );

        trace_t clear;
        UTIL_TraceLine( vecFeetClear, vecFeetClear + 30.0f * vecForward, MASK_SOLID, GetHost(), COLLISION_GROUP_NONE, &clear );

        if ( bot_debug_jump.GetBool() && GetBot()->ShouldShowDebug() ) {
            NDebugOverlay::Line( vecFeetBlocked, vecFeetBlocked + 30.0f * vecForward, 0, 0, 255, true, 0.1f );
            NDebugOverlay::Line( vecFeetClear, vecFeetClear + 30.0f * vecForward, 0, 0, 255, true, 0.1f );
        }

        // Algo nos esta bloqueando
        if ( blocked.fraction < 1.0 && clear.fraction == 1.0 ) {
            if ( blocked.m_pEnt ) {
                if ( blocked.m_pEnt->IsPlayer() ) {
                    CPlayer *pPlayer = ToInPlayer( blocked.m_pEnt );
                    Assert( pPlayer );

                    // Si esta incapacitada pasamos por encima de el (like a boss)
                    if ( !pPlayer->IsDejected() )
                        return false;
                }
                else if ( blocked.m_pEnt->MyCombatCharacterPointer() ) {
                    return false;
                }
            }

            if ( bot_debug_jump.GetBool() && GetBot()->ShouldShowDebug() ) {
                NDebugOverlay::EntityBounds( GetHost(), 0, 255, 0, 5.0f, 0.1f );
            }

            return true;
        }
    }

    return false;
}


//================================================================================
// Returns if we can hunt our enemy
//================================================================================
bool CBotDecision::ShouldHuntThreat() const
{
    if ( !CanMove() )
        return false;

    if ( !GetSkill()->IsEasiest() ) {
        if ( HasCondition( BCOND_LOW_HEALTH ) )
            return false;

        if ( HasCondition( BCOND_HELPLESS ) )
            return false;

        // There are several more dangerous enemies, we should not go
        if ( GetDataMemoryInt( "NearbyDangerousThreats" ) >= 3 )
            return false;
    }

    if ( !GetMemory() )
        return false;

    CEntityMemory *memory = GetBot()->GetPrimaryThreat();

    if ( !memory )
        return false;

    if ( GetBot()->GetSquad() && GetBot()->GetSquad()->GetStrategie() == COWARDS )
        return false;

    float distance = GetMemory()->GetPrimaryThreatDistance();
    const float tolerance = 700.0f;

    if ( distance >= tolerance ) {
        if ( IsDangerousEnemy() && GetFollow() ) {
            if ( GetFollow()->IsFollowing() && GetFollow()->IsEnabled() ) {
                return false;
            }
        }

        // We are in defensive mode and the enemy is far
        if ( GetBot()->GetTacticalMode() == TACTICAL_MODE_DEFENSIVE ) {
            // If someone has reported, we can go to the position of our friend,
            // otherwise we remained defending.
            if ( memory->GetInformer() == NULL ) {
                return false;
            }
        }
    }

    return true;
}

//================================================================================
// Returns if we can investigate the source of a danger sound
//================================================================================
bool CBotDecision::ShouldInvestigateSound() const
{
    if ( !CanMove() )
        return false;

    if ( HasCondition( BCOND_HELPLESS ) )
        return false;

    if ( HasCondition( BCOND_HEAR_MOVE_AWAY ) )
        return false;

    if ( !HasCondition( BCOND_HEAR_COMBAT ) && !HasCondition( BCOND_HEAR_ENEMY ) && !HasCondition( BCOND_HEAR_DANGER ) )
        return false;

    if ( IsCombating() || GetBot()->GetEnemy() )
        return false;

    if ( GetBot()->GetSquad() && GetBot()->GetSquad()->GetStrategie() == COWARDS )
        return false;

    if ( GetFollow() && GetFollow()->IsFollowing() && GetFollow()->IsEnabled() )
        return false;

    CSound *pSound = GetHost()->GetBestSound( SOUND_COMBAT | SOUND_PLAYER );

    if ( !pSound )
        return false;

    // We are in defensive mode, if the sound
    // it's far better we stayed here
    if ( GetBot()->GetTacticalMode() == TACTICAL_MODE_DEFENSIVE ) {
        float distance = GetHost()->GetAbsOrigin().DistTo( pSound->GetSoundOrigin() );
        const float tolerance = 600.0f;

        if ( distance >= tolerance )
            return false;
    }

    return true;
}

//================================================================================
// Returns whether the bot should be hidden for certain actions (reload, heal)
//================================================================================
bool CBotDecision::ShouldCover() const
{
    if ( !CanMove() )
        return false;

    if ( IsDangerousEnemy() )
        return true;

    if ( HasCondition( BCOND_HELPLESS ) )
        return true;

    return false;
}

//================================================================================
// Returns if we can take the specified weapon
//================================================================================
bool CBotDecision::ShouldGrabWeapon( CBaseWeapon *pWeapon ) const
{
    if ( !pWeapon )
        return false;

    if ( pWeapon->GetOwner() )
        return false;

    if ( HasCondition( BCOND_DEJECTED ) )
        return false;

    const float nearDistance = 500.0f;

    if ( GetBot()->GetEnemy() ) {
        // There is an enemy nearby
        if ( Utils::IsSpotOccupied( pWeapon->GetAbsOrigin(), NULL, nearDistance, GetBot()->GetEnemy()->GetTeamNumber() ) )
            return false;

        // The enemy can attack me easily if I go
        if ( Utils::IsCrossingLineOfFire( pWeapon->GetAbsOrigin(), GetBot()->GetEnemy()->EyePosition(), GetHost(), GetHost()->GetTeamNumber() ) )
            return false;
    }

    if ( !pWeapon->IsMeleeWeapon() ) {
        // We have little ammunition
        // The weapon may not be better, but at least it has ammunition.
        if ( HasCondition( BCOND_EMPTY_PRIMARY_AMMO ) || HasCondition( BCOND_LOW_PRIMARY_AMMO ) ) {
            if ( pWeapon->HasAnyAmmo() )
                return true;
        }
    }

    if ( TheGameRules->FShouldSwitchWeapon( GetHost(), pWeapon ) )
        return true;

    // We are an player ally, we try not to take the weapons close to a human player
    // TODO: Apply this even if we are on the enemy team with enemy human players. (Teamplay)
    if ( GetHost()->Classify() == CLASS_PLAYER_ALLY || GetHost()->Classify() == CLASS_PLAYER_ALLY_VITAL ) {
        if ( Utils::IsSpotOccupiedByClass( pWeapon->GetAbsOrigin(), CLASS_PLAYER, NULL, nearDistance ) )
            return false;
    }

    return false;
}

//================================================================================
// Returns if we can change the weapon if necessary
//================================================================================
bool CBotDecision::ShouldSwitchToWeapon( CBaseWeapon *pWeapon ) const
{
    if ( !pWeapon )
        return false;

    return GetHost()->Weapon_CanSwitchTo( pWeapon );
}

//================================================================================
//================================================================================
bool CBotDecision::ShouldHelpFriends() const
{
    if ( GetSkill()->IsEasiest() )
        return false;

    if ( !CanMove() )
        return false;

    if ( HasCondition( BCOND_HELPLESS ) )
        return false;

    if ( IsCombating() )
        return false;

    if ( GetBot()->GetSquad() ) {
        if ( GetBot()->GetSquad()->GetStrategie() == COWARDS )
            return false;

        if ( GetBot()->GetSquad()->GetActiveCount() <= 2 && GetBot()->GetSquad()->GetStrategie() == LAST_CALL_FOR_BACKUP )
            return false;
    }

    return true;
}

//================================================================================
// Returns if we can help a dejected friend
//================================================================================
bool CBotDecision::ShouldHelpDejectedFriend( CPlayer *pDejected ) const
{
#ifdef INSOURCE_DLL
    if ( !pDejected->IsDejected() )
        return false;

    CPlayer *pHelping = ToInPlayer( GetDataMemoryEntity( "DejectedFriend" ) );

    if ( pHelping ) {
        // We must help him!
        if ( pHelping == pDejected )
            return true;

        // I am trying to help someone else
        if ( pHelping->IsDejected() )
            return false;
    }

    if ( pDejected->IsBeingHelped() )
        return false;

    // TODO: Check other bots that are already trying to help
    return true;
#else
    // TODO: Implement Dejected System?
    return false;
#endif
}

//================================================================================
// Returns if bot is low health and must be hidden
//================================================================================
bool CBotDecision::IsLowHealth() const
{
    int lowHealth = 30;

    if ( GetSkill()->GetLevel() >= SKILL_HARD ) {
        lowHealth += 10;
    }

    if ( GetGameDifficulty() >= SKILL_HARD ) {
        lowHealth += 10;
    }

    if ( GetGameDifficulty() >= SKILL_ULTRA_HARD ) {
        lowHealth += 10;
    }

    if ( GetHost()->GetHealth() <= lowHealth ) {
        return true;
    }

    return false;
}

//================================================================================
// Returns if the bot can move
//================================================================================
bool CBotDecision::CanMove() const
{
    if ( HasCondition( BCOND_DEJECTED ) )
        return false;

    if ( !GetLocomotion() )
        return false;

    if ( GetLocomotion()->IsDisabled() )
        return false;

#ifdef INSOURCE_DLL
    if ( GetHost()->IsMovementDisabled() )
        return false;
#endif

    return true;
}

//================================================================================
//================================================================================
bool CBotDecision::IsUsingFiregun() const
{
    CBaseWeapon *pWeapon = GetHost()->GetActiveBaseWeapon();

    if ( !pWeapon )
        return false;

    return !pWeapon->IsMeleeWeapon();
}

//================================================================================
//================================================================================
bool CBotDecision::CanAttack() const
{
    // Returns if Bot has the ability to attack
    if ( bot_dont_attack.GetBool() )
        return false;

    return true;
}

//================================================================================
//================================================================================
bool CBotDecision::CanCrouchAttack() const
{
    if ( GetSkill()->IsEasiest() )
        return false;

    if ( HasCondition( BCOND_DEJECTED ) )
        return false;

    CEntityMemory *memory = GetBot()->GetPrimaryThreat();

    if ( !memory )
        return false;

    Vector vecEyePosition = GetHost()->GetAbsOrigin();
    vecEyePosition.z += VEC_DUCK_VIEW.z;

    CBulletsTraceFilter traceFilter( COLLISION_GROUP_NONE );
    traceFilter.SetPassEntity( GetHost() );

    Vector vecForward;
    GetHost()->GetVectors( &vecForward, NULL, NULL );

    trace_t tr;
    UTIL_TraceLine( vecEyePosition, vecEyePosition + vecForward * 3000.0f, MASK_SHOT, &traceFilter, &tr );

    if ( tr.m_pEnt == memory->GetEntity() ) {
        //NDebugOverlay::Line( tr.startpos, tr.endpos, 0, 255, 0, true, 0.5f );
        return true;
    }

    //NDebugOverlay::Line( tr.startpos, tr.endpos, 255, 0, 0, true, 0.5f );
    return false;
}


//================================================================================
//================================================================================
bool CBotDecision::IsEnemyLowPriority() const
{
    CBaseEntity *pEnemy = GetBot()->GetEnemy();
    CEntityMemory *pIdeal = GetMemory()->GetIdealThreat();

    if ( !pEnemy )
        return true;

    if ( HasCondition( BCOND_ENEMY_DEAD ) )
        return true;

    if ( pEnemy->IsPlayer() ) {
        CPlayer *pPlayer = ToInPlayer( pEnemy );
        Assert( pPlayer );

        if ( pPlayer->IsDejected() )
            return true;
    }

    if ( pIdeal ) {
        if ( pIdeal->GetDistance() <= 300.0f )
            return true;

        if ( pEnemy->IsPlayer() && !pIdeal->GetEntity()->IsPlayer() )
            return false;
    }

    if ( IsPrimaryThreatLost() )
        return true;

    return false;
}

//================================================================================
//================================================================================
bool CBotDecision::IsBetterEnemy( CBaseEntity * pIdeal, CBaseEntity * pPrevious ) const
{
    if ( !pPrevious )
        return true;

    if ( !GetSkill()->IsEasiest() ) {
        if ( GetHost()->IsAbleToSee( pIdeal ) && !GetHost()->IsAbleToSee( pPrevious ) )
            return true;
    }

    int previousPriority = GetHost()->IRelationPriority( pPrevious );
    int idealPriority = GetHost()->IRelationPriority( pIdeal );

    float flPreviousDistance = pPrevious->GetAbsOrigin().DistTo( GetHost()->GetAbsOrigin() );
    float flIdealDistance = pIdeal->GetAbsOrigin().DistTo( GetHost()->GetAbsOrigin() );

    // The enemy to compare is further away
    if ( flIdealDistance > flPreviousDistance ) {
        // But has more priority
        if ( idealPriority > previousPriority )
            return true;

        return false;
    }

    // Has more priority
    if ( idealPriority > previousPriority )
        return true;

    // We certainly give priority to enemies very close
    if ( flIdealDistance <= 200.0f )
        return true;

    // Is more dangerous!
    if ( IsDangerousEnemy( pIdeal ) && !IsDangerousEnemy( pPrevious ) )
        return true;

    // It is the enemy of some member of the squadron
    //if ( GetBot()->GetSquad() && GetBot()->GetSquad()->IsSquadEnemy( pIdeal, GetHost() ) )
        //return true;

    return false;
}

//================================================================================
// Returns if the specified entity is dangerous to us
//================================================================================
bool CBotDecision::IsDangerousEnemy( CBaseEntity *pEnemy ) const
{
    if ( !pEnemy )
        pEnemy = GetBot()->GetEnemy();

    if ( !pEnemy )
        return false;

    if ( pEnemy->IsPlayer() ) {
        CPlayer *pPlayer = ToInPlayer( pEnemy );

        if ( pPlayer->IsDejected() )
            return false;

        if ( pPlayer->IsMovementDisabled() )
            return false;

        if ( pPlayer->IsBot() ) {
            if ( pPlayer->GetAI()->HasCondition( BCOND_HELPLESS ) )
                return false;

            if ( !pPlayer->GetAI()->GetDecision()->CanMove() )
                return false;
        }

        return true;
    }

    // Each bot must implement its own criteria.
    return false;
}

//================================================================================
// Returns if we have lost sight of our current enemy
//================================================================================
bool CBotDecision::IsPrimaryThreatLost() const
{
    return (HasCondition( BCOND_ENEMY_OCCLUDED ));
}

//================================================================================
// It returns if we should perform our tasks more carefully.
//================================================================================
bool CBotDecision::ShouldMustBeCareful() const
{
    if ( GetSkill()->IsEasiest() )
        return false;

    if ( GetDataMemoryInt("NearbyDangerousThreats") >= 2 )
        return true;

    if ( IsDangerousEnemy() )
        return true;

    return false;
}

//================================================================================
// We change our current weapon by the best we have according to the situation.
//================================================================================
void CBotDecision::SwitchToBestWeapon()
{
    CBaseWeapon *pCurrent = GetHost()->GetActiveBaseWeapon();

    if ( !pCurrent )
        return;

    // Best
    CBaseWeapon *pPistol = NULL;
    CBaseWeapon *pSniper = NULL;
    CBaseWeapon *pShotgun = NULL;
    CBaseWeapon *pMachineGun = NULL;
    CBaseWeapon *pShortRange = NULL;

    // Check to know what weapons I have
    for ( int i = 0; i < GetHost()->WeaponCount(); i++ ) {
        CBaseWeapon *pWeapon = (CBaseWeapon *)GetHost()->GetWeapon( i );

        if ( !ShouldSwitchToWeapon( pWeapon ) )
            continue;

        // TODO: Another way to detect a pistol?
        if ( pWeapon->ClassMatches( "pistol" ) ) {
            if ( !pPistol || pWeapon->GetWeight() > pPistol->GetWeight() ) {
                pPistol = pWeapon;
            }
        }

        // The best sniper
        else if ( pWeapon->IsSniper() ) {
            if ( !pSniper || pWeapon->GetWeight() > pSniper->GetWeight() ) {
                pSniper = pWeapon;
            }
        }

        // The best shotgun
        else if ( pWeapon->IsShotgun() ) {
            if ( !pShotgun || pWeapon->GetWeight() > pShotgun->GetWeight() ) {
                pShotgun = pWeapon;
            }
        }

        // The best rifle
        else {
            if ( !pMachineGun || pWeapon->GetWeight() > pMachineGun->GetWeight() ) {
                pMachineGun = pWeapon;
            }
        }
    }

    // The best short-range weapon
    {
        if ( pShotgun ) {
            pShortRange = pShotgun;
        }
        else if ( pMachineGun ) {
            pShortRange = pMachineGun;
        }
        else if ( pPistol ) {
            pShortRange = pPistol;
        }
    }

    float closeRange = 400.0f;

    if ( IsDangerousEnemy() ) {
        closeRange = 800.0f;
    }

    if ( GetMemory() ) {
        // We're using a sniper gun!
        if ( pCurrent->IsSniper() && pShortRange && GetBot()->GetEnemy() ) {
            // My enemy is close, we change to a short range weapon
            if ( GetMemory()->GetPrimaryThreatDistance() <= closeRange ) {
                GetHost()->Weapon_Switch( pShortRange );
                return;
            }
        }

        // We are not using a sniper gun, but we have one
        if ( !pCurrent->IsSniper() && pSniper && GetBot()->GetEnemy() ) {
            // My enemy has moved away, it will be better to switch to sniper gun
            // TODO: This is not the best...
            if ( GetMemory()->GetPrimaryThreatDistance() > closeRange ) {
                GetHost()->Weapon_Switch( pSniper );
                return;
            }
        }
    }

    // We always change to our best available weapon while 
    // we are doing nothing or we run out of ammunition.
    if ( IsIdle() || !pCurrent->HasAnyAmmo() ) {
        CBaseWeapon *pBest = (CBaseWeapon *)TheGameRules->GetNextBestWeapon( GetHost(), NULL );

        if ( pBest == pCurrent ) {
            return;
        }

        GetHost()->Weapon_Switch( pBest );
    }
}

//================================================================================
// Returns if there is a cover spot close to the bot position
//================================================================================
bool CBotDecision::GetNearestCover( float radius, Vector *vecCoverSpot ) const
{
    CSpotCriteria criteria;
    criteria.SetMaxRange( radius );
    criteria.UseNearest( true );
    criteria.OutOfVisibility( true );
    criteria.AvoidTeam( GetBot()->GetEnemy() );
    criteria.SetTacticalMode( GetBot()->GetTacticalMode() );

    if ( GetHost()->GetActiveBaseWeapon() && GetHost()->GetActiveBaseWeapon()->IsSniper() ) {
        criteria.IsSniper( true );
    }

    return Utils::FindCoverPosition( vecCoverSpot, GetHost(), criteria );
}

//================================================================================
// Returns if the bot is in a coverage position
//================================================================================
bool CBotDecision::IsInCoverPosition() const
{
    Vector vecCoverSpot;
    const float tolerance = 75.0f;

    if ( !GetNearestCover( GET_COVER_RADIUS, &vecCoverSpot ) )
        return false;

    if ( GetHost()->GetAbsOrigin().DistTo( vecCoverSpot ) > tolerance )
        return false;

    return true;
}

//================================================================================
//================================================================================
BCOND CBotDecision::ShouldRangeAttack1()
{
    if ( !CanAttack() )
        return BCOND_NONE;

    if ( bot_primary_attack.GetBool() )
        return BCOND_CAN_RANGE_ATTACK1;

    if ( !CanShoot() )
        return BCOND_NONE;

    if ( !GetMemory() )
        return BCOND_NONE;

    CEntityMemory *memory = GetMemory()->GetPrimaryThreat();

    // TODO: A way to support attacks without an active enemy
    if ( !memory )
        return BCOND_NONE;

    CBaseWeapon *pWeapon = GetHost()->GetActiveBaseWeapon();

    // TODO: A way to support attacks without a [CBaseWeapon]
    if ( !pWeapon || pWeapon->IsMeleeWeapon() )
        return BCOND_NONE;

    float flDistance = memory->GetDistance();

    // TODO: The code commented below was an attempt to make the Bot continue firing 
    // for a few seconds at the enemy's last known position, but a more intelligent way is needed.

    /*if ( IsPrimaryThreatLost() ) {
    if ( !pWeapon || pWeapon->IsMeleeWeapon() )
    return BCOND_NONE;

    if ( GetSkill()->GetLevel() <= SKILL_MEDIUM )
    return BCOND_NONE;

    if ( GetBot()->GetActiveSchedule() != NULL )
    return BCOND_NONE;

    if ( GetVision() ) {
    if ( GetVision()->GetAimTarget() != memory->GetEntity() || !GetVision()->IsAimReady() )
    return BCOND_NOT_FACING_ATTACK;
    }

    if ( HasCondition( BCOND_ENEMY_LOST ) )
    return BCOND_NONE;
    }*/

    if ( HasCondition( BCOND_EMPTY_CLIP1_AMMO ) )
        return BCOND_NONE;

    if ( !pWeapon->CanPrimaryAttack() )
        return BCOND_NONE;

    //if ( !GetSkill()->IsEasy() && GetBot()->GetEnemy()->m_lifeState == LIFE_DYING )
        //return BCOND_NONE;

    if ( flDistance > pWeapon->GetWeaponInfo().m_flIdealDistance )
        return BCOND_TOO_FAR_TO_ATTACK;

    // We draw a line pretending to be the bullets
    CBulletsTraceFilter traceFilter( COLLISION_GROUP_NONE );
    traceFilter.SetPassEntity( GetHost() );

    Vector vecForward, vecOrigin( GetHost()->EyePosition() );
    GetHost()->GetVectors( &vecForward, NULL, NULL );

    trace_t tr;
    UTIL_TraceLine( vecOrigin, vecOrigin + vecForward * pWeapon->GetWeaponInfo().m_flMaxDistance, MASK_SHOT, &traceFilter, &tr );

    // We have hit an entity
    if ( tr.m_pEnt ) {
        // Friend!
        if ( TheGameRules->PlayerRelationship( GetHost(), tr.m_pEnt ) == GR_ALLY )
            return BCOND_BLOCKED_BY_FRIEND;

        // We have not hit our enemy, so we must make the verification that our aim is already on it.
        if ( tr.m_pEnt != memory->GetEntity() ) {
            if ( GetVision() ) {
                if ( GetVision()->GetAimTarget() != memory->GetEntity() || !GetVision()->IsAimReady() )
                    return BCOND_NOT_FACING_ATTACK;
            }
        }
    }

    // A better way to do this and move it to a better place.
    float fireRate = pWeapon->GetFireRate();
    float minRate = fireRate + GetSkill()->GetMinAttackRate();
    float maxRate = fireRate + GetSkill()->GetMaxAttackRate();
    m_ShotRateTimer.Start( RandomFloat( minRate, maxRate ) );

    return BCOND_CAN_RANGE_ATTACK1;
}

//================================================================================
//================================================================================
BCOND CBotDecision::ShouldRangeAttack2()
{
    if ( bot_dont_attack.GetBool() )
        return BCOND_NONE;

    if ( !GetMemory() )
        return BCOND_NONE;

    CEntityMemory *memory = GetMemory()->GetPrimaryThreat();

    // TODO: A way to support attacks without an active enemy
    if ( !memory )
        return BCOND_NONE;

    CBaseWeapon *pWeapon = GetHost()->GetActiveBaseWeapon();

    // TODO: A way to support attacks without a [CBaseWeapon]
    if ( !pWeapon || pWeapon->IsMeleeWeapon() )
        return BCOND_NONE;

    // Snipa!
    if ( pWeapon->IsSniper() ) {
        if ( !GetSkill()->IsEasiest() ) {
            // Zoom!
            if ( IsCombating() && !pWeapon->IsWeaponZoomed() ) {
                return BCOND_CAN_RANGE_ATTACK2;
            }

            // We removed the zoom
            else if ( pWeapon->IsWeaponZoomed() ) {
                return BCOND_CAN_RANGE_ATTACK2;
            }
        }
    }

    // TODO: Each weapon can have its own behavior in the secondary attack
    return BCOND_NONE;
}

//================================================================================
//================================================================================
BCOND CBotDecision::ShouldMeleeAttack1()
{
    if ( bot_dont_attack.GetBool() )
        return BCOND_NONE;

    if ( bot_primary_attack.GetBool() )
        return BCOND_CAN_MELEE_ATTACK1;

    if ( !GetMemory() )
        return BCOND_NONE;

    CEntityMemory *memory = GetMemory()->GetPrimaryThreat();

    // TODO: A way to support attacks without an active enemy
    if ( !memory )
        return BCOND_NONE;

    CBaseWeapon *pWeapon = GetHost()->GetActiveBaseWeapon();

    // TODO: A way to support attacks without a [CBaseWeapon]
    if ( !pWeapon || !pWeapon->IsMeleeWeapon() )
        return BCOND_NONE;

    float flDistance = memory->GetDistance();

    // TODO: Criteria to attack with a melee weapon
    if ( flDistance > 120.0f )
        return BCOND_TOO_FAR_TO_ATTACK;

    // TODO
    return BCOND_NONE;
}

//================================================================================
//================================================================================
BCOND CBotDecision::ShouldMeleeAttack2()
{
    // TODO
    return BCOND_NONE;
}
