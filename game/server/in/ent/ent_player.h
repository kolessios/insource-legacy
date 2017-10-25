//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#ifndef ENT_PLAYER_H
#define ENT_PLAYER_H

#ifdef _WIN32
#pragma once
#endif

#include "player.h"
#include "ent/ent_host.h"

#include "ai_speech.h"
#include "ai_senses.h"

//================================================================================
//================================================================================
class CEntPlayer : public CBasePlayer
{
public:
    DECLARE_CLASS( CPlayer, CBasePlayer );
    DECLARE_SERVERCLASS();
    DECLARE_DATADESC();

public:
    CEntPlayer();
    ~CEntPlayer();

    //
    CEntHost *GetEntHost() const {
        return m_pHost;
    }

    void SetEntHost( CEntHost *host ) {
        if ( m_pHost ) {
            delete m_pHost;
            m_pHost = NULL;
        }

        m_pHost = host;
    }

    //
    virtual Class_T Classify() {
        return GetEntHost()->Classify();
    }

    //
    virtual void InitialSpawn();
    virtual void Spawn();
    virtual void Precache();

    //
    virtual void PreThink();
    virtual void PlayerThink();
    virtual void PostThink();

    //
    virtual CBaseEntity *GetEnemy() const {
        return GetEntHost()->GetEnemy();
    }

    //
    virtual bool BecomeRagdollOnClient( const Vector &force );
    virtual void CreateRagdollEntity();

    //
    virtual void SetAnimation( PLAYER_ANIM anim );

    //
    virtual void SetFlashlightEnabled( bool enabled ) {
        GetEntHost()->SetFlashlightEnabled( enabled );
    }

    virtual int FlashlightIsOn() {
        return GetEntHost()->FlashlightIsOn();
    }

    virtual void FlashlightTurnOn() {
        GetEntHost()->FlashlightTurnOn();
    }

    virtual void FlashlightTurnOff() {
        GetEntHost()->FlashlightTurnOff();
    }

    virtual bool IsIlluminatedByFlashlight( CBaseEntity *pEntity, float *flReturnDot ) {
        GetEntHost()->IsIlluminatedByFlashlight( pEntity, flReturnDot );
    }

    //
    virtual bool ShouldBleed( const CTakeDamageInfo &info, int hitgroup ) {
        return GetEntHost()->ShouldBleed( info, hitgroup );
    }

    virtual void TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr ) {
        return GetEntHost()->TraceAttack( info, vecDir, ptr );
    }

    virtual void DamageEffect( const CTakeDamageInfo &inputInfo ) {
        GetEntHost()->DamageEffect( inputInfo );
    }

    virtual bool CanTakeDamage( const CTakeDamageInfo &inputInfo ) {
        return GetEntHost()->CanTakeDamage( inputInfo );
    }

    virtual int OnTakeDamage( const CTakeDamageInfo &inputInfo ) {
        return GetEntHost()->OnTakeDamage( inputInfo );
    }

    virtual int OnTakeDamage_Alive( CTakeDamageInfo &inputInfo ) {
        return GetEntHost()->OnTakeDamage_Alive( inputInfo );
    }

    virtual void ApplyDamage( const CTakeDamageInfo &inputInfo ) {
        GetEntHost()->ApplyDamage( inputInfo );
    }

    virtual void Event_Killed( const CTakeDamageInfo &info ) {
        GetEntHost()->Event_Killed( info );
    }

    virtual bool Event_Gibbed( const CTakeDamageInfo &info ) {
        return GetEntHost()->Event_Gibbed( info );
    }

    virtual void Event_Dying( const CTakeDamageInfo &info ) {
        GetEntHost()->Event_Dying( info );
    }

    //
    virtual void CreateViewModel( int viewmodelindex = 0 ) {
        GetEntHost()->CreateViewModel( viewmodelindex );
    }

    //
    virtual bool GetEyesView( CBaseAnimating *pEntity, Vector& eyeOrigin, QAngle& eyeAngles, int secureDistance ) {
        return GetEntHost()->GetEyesView( pEntity, eyeOrigin, eyeAngles, secureDistance );
    }

    virtual void CalcPlayerView( Vector& eyeOrigin, QAngle& eyeAngles, float& fov ) {
        GetEntHost()->CalcPlayerView( eyeOrigin, eyeAngles, fov );
    }

    // Collision Bounds
    virtual const Vector GetPlayerMins() const {
        return GetEntHost()->GetMins();
    }

    virtual const Vector GetPlayerMaxs() const {
        return GetEntHost()->GetMaxs();
    }

    // Utils
    virtual bool ShouldAutoaim();

    virtual void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value ) {
        GetEntHost()->Use( pActivator, pCaller, useType, value );
    }

    virtual void Touch( CBaseEntity *pOther ) {
        GetEntHost()->Touch( pOther );
    }

    virtual int ObjectCaps() {
        return GetEntHost()->ObjectCaps();
    }

protected:
    CEntHost *m_pHost;

    CNetworkQAngle( m_angEyeAngles );
};

#endif // ENT_PLAYER_H