//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#include "cbase.h"
#include "ent_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void CEntPlayer::InitialSpawn()
{
    BaseClass::InitialSpawn();

    m_takedamage = DAMAGE_YES;
    pl.deadflag = false;
    m_lifeState = LIFE_ALIVE;

    GetEntHost()->InitialSpawn();
}

void CEntPlayer::Spawn()
{
    BaseClass::Spawn();

    m_nButtons = 0;
    m_iRespawnFrames = 0;
    m_Local.m_iHideHUD = 0;

    AddFlag( FL_AIMTARGET );

    GetEntHost()->Spawn();

    SetThink( &CEntPlayer::PlayerThink );
    SetNextThink( gpGlobals->curtime + 0.1f );
}

void CEntPlayer::Precache()
{
    BaseClass::Precache();

    PrecacheModel( "models/player.mdl" );
    PrecacheModel( "sprites/light_glow01.vmt" );
    PrecacheModel( "sprites/spotlight01_proxyfade.vmt" );
    PrecacheModel( "sprites/glow_test02.vmt" );
    PrecacheModel( "sprites/light_glow03.vmt" );
    PrecacheModel( "sprites/glow01.vmt" );

    PrecacheScriptSound( "Player.FlashlightOn" );
    PrecacheScriptSound( "Player.FlashlightOff" );

    GetEntHost()->Precache();
}

void CEntPlayer::PreThink()
{
    if ( IsInAVehicle() ) {
        UpdateClientData();
        CheckTimeBasedDamage();

        CheckSuitUpdate();
        WaterMove();

        return;
    }

    QAngle vOldAngles = GetLocalAngles();
    QAngle vTempAngles = GetLocalAngles();
    vTempAngles = EyeAngles();

    if ( vTempAngles[PITCH] > 180.0f ) {
        vTempAngles[PITCH] -= 360.0f;
    }

    SetLocalAngles( vTempAngles );

    GetEntHost()->PreThink();

    BaseClass::PreThink();
    SetLocalAngles( vOldAngles );
}

void CEntPlayer::PlayerThink()
{
    GetEntHost()->Think();
}

void CEntPlayer::PostThink()
{
    BaseClass::PostThink();

    if ( IsAlive() ) {
        ProcessSceneEvents();
        DoBodyLean();
    }

    GetEntHost()->PostThink();

    m_angEyeAngles = EyeAngles();

    QAngle angles = GetLocalAngles();
    angles[PITCH] = 0;
    SetLocalAngles( angles );
}

bool CEntPlayer::BecomeRagdollOnClient( const Vector & force )
{
    if ( !CanBecomeRagdoll() )
        return false;

    // Have to do this dance because m_vecForce is a network vector
    // and can't be sent to ClampRagdollForce as a Vector *
    Vector vecClampedForce;
    ClampRagdollForce( force, &vecClampedForce );
    m_vecForce = vecClampedForce;

    // Remove our flame entity if it's attached to us
    /*CEntityFlame *pFireChild = dynamic_cast<CEntityFlame *>( GetEffectEntity() );
    if ( pFireChild )
    {
    pFireChild->SetThink( &CBaseEntity::SUB_Remove );
    pFireChild->SetNextThink( gpGlobals->curtime + 0.1f );
    }*/

    AddFlag( FL_TRANSRAGDOLL );
    m_bClientSideRagdoll = true;
    return true;
}

void CEntPlayer::CreateRagdollEntity()
{
    GetEntHost()->CreateRagdoll();
}

void CEntPlayer::SetAnimation( PLAYER_ANIM anim )
{
    if ( anim == PLAYER_WALK || anim == PLAYER_IDLE ) return;

    if ( anim == PLAYER_RELOAD ) {
        GetEntHost()->DoAnimationEvent( PLAYERANIMEVENT_RELOAD, 0, true );
    }
    else if ( anim == PLAYER_JUMP ) {
        // TODO: Por alguna razón esto no se ejecuta en cliente
        GetEntHost()->DoAnimationEvent( PLAYERANIMEVENT_JUMP, 0, false );
    }
    else {
        Assert( false );
        Warning("CEntPlayer::SetAnimation Obsolete! \n");
    }
}
