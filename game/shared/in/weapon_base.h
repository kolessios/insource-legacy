//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef WEAPON_BASE_H
#define WEAPON_BASE_H

#ifdef _WIN32
#pragma once
#endif

#include "weapon_info.h"


#ifdef CLIENT_DLL
class C_Player;

#define CPlayer C_Player
#define CBaseWeapon C_BaseWeapon
#define CBaseWeaponSniper C_BaseWeaponSniper
#define CBaseWeaponShotgun C_BaseWeaponShotgun

#define CWeaponCubemap C_WeaponCubemap

#define CWeaponAK47 C_WeaponAK47
#define CWeaponM16 C_WeaponM16
#define CWeaponSMG C_WeaponSMG
#define CWeaponP220 C_WeaponP220
#define CWeaponCombatShotgun C_WeaponCombatShotgun
#define CWeaponSniperRifle C_WeaponSniperRifle

#ifdef APOCALYPSE
//#define CWeaponAK47 C_WeaponAK47
//#define CWeaponM4A1 C_WeaponM4A1
#endif
#else
class CPlayer;
#endif

//================================================================================
// Clase base para crear un arma de fuego
//================================================================================
class CBaseWeapon : public CBaseCombatWeapon
{
public:
    DECLARE_CLASS( CBaseWeapon, CBaseCombatWeapon );
    DECLARE_NETWORKCLASS();
    DECLARE_PREDICTABLE();

    CBaseWeapon();

    #ifndef CLIENT_DLL
        DECLARE_DATADESC();

        virtual int ObjectCaps();
    #else
        virtual bool ShouldPredict();
        virtual void OnDataChanged( DataUpdateType_t type );

        virtual IClientModelRenderable*	GetClientModelRenderable() { return NULL;  }
        virtual bool ShouldDraw();
        virtual int	DrawModel( int flags, const RenderableInstance_t &instance );

        virtual void AddViewmodelBob( CBaseViewModel *, Vector &, QAngle & );
        virtual float CalcViewmodelBob();

        virtual bool GetShootPosition( Vector &, QAngle & );
        virtual bool OnFireEvent( CBaseViewModel *, const Vector &, const QAngle &, int event, const char * );
    #endif
    
    // Predicción
    virtual bool IsPredicted() const { return true; }

	// ¿Estamos recargando?
    virtual bool IsReloading() { return m_bInReload; }

    // Tipo de arma
    virtual bool HasSuppressor()
    {
        return false;
    }
	virtual bool IsShotgun() { return false; }
    virtual bool IsSniper() { return false; }

	// Principales
    virtual void Spawn();

	virtual void Equip( CBaseCombatCharacter *pOwner );
	virtual void Drop( const Vector &vecVelocity );

    virtual CBaseAnimating *GetIdealAnimating();
    
    // Configuración
    const CWeaponInfo &GetWeaponInfo() const;
    CPlayer *GetPlayerOwner() const;

	virtual int GetAddonAttachment();
	virtual float GetVerticalPunch();
    virtual float GetSpreadPerShot();
    virtual float GetFireRate();
    virtual float GetWeaponFOV();

    virtual WeaponSound_t GetWeaponSound( const FireBulletsInfo_t & );
        
    // Acciones
    virtual void ItemPostFrame();

    virtual bool CanPrimaryAttack();
    virtual bool CanSecondaryAttack();

    virtual void PrimaryAttack();
    virtual void SecondaryAttack() { };

    virtual void WeaponIdle();

    virtual bool Reload();
    virtual void AddViewKick();

    virtual void DefaultTouch( CBaseEntity *pOther );
    virtual bool DefaultDeploy( char *szViewModel, char *szWeaponModel, int iActivity, char *szAnimExt );
	virtual bool Holster( CBaseCombatWeapon *pSwitchingTo = NULL );

    virtual void SetNextAttack( float time = 0.0f );
    virtual void SetNextPrimaryAttack( float time = 0.0f );
    virtual void SetNextSecondaryAttack( float time = 0.0f );
    virtual void SetNextIdleTime( float time = 0.0f );

	virtual void HideThink();
    virtual void ShowThink();

    // Sonidos
    virtual void WeaponSound( WeaponSound_t, float = 0.0f );
    virtual void StartGroupingSounds();
    virtual void EndGroupingSounds();

    // Efectos
    virtual const char *GetTracerType() { return "weapon_tracers"; }
    virtual void MakeTracer( const Vector &vecTracerSrc, const trace_t &tr, int iTracerType, bool isFirstTracer = false );
    virtual int GetTracerAttachment();
    virtual bool DispatchMuzzleEffect( const char *, bool );

    virtual void ShowHitmarker( CBaseEntity *pVictim );

    // Animaciones
	virtual bool IsLayerAnim( int iActivity );
	virtual bool SetIdealActivity( Activity ideal );

    virtual Activity GetPrimaryAttackActivity() { return ACT_VM_PRIMARYATTACK;    }
    virtual Activity GetIdleActivity() { return ACT_VM_IDLE; }
    //virtual Activity GetDeployActivity() { return ACT_VM_DEPLOY; }
    virtual Activity GetDrawActivity() { return ACT_VM_DRAW; }
    virtual Activity GetReloadActivity() { return ACT_VM_RELOAD; }
    virtual Activity GetHolsterActivity() { return ACT_VM_HOLSTER; }
    virtual Activity GetReloadLoopActivity() { return ACT_VM_RELOAD_LOOP; }
    virtual Activity GetReloadEndActivity() { return ACT_VM_RELOAD_END; }

protected:
    bool m_bPrimaryAttackReleased;
	int m_iAddonAttachment;

private:
    CBaseWeapon( const CBaseWeapon & );
};

inline CBaseWeapon *ToBaseWeapon( CBaseEntity *pEntity )
{
    if ( !pEntity )
        return NULL;

    return static_cast<CBaseWeapon *>( pEntity );
}

//================================================================================
// Clase base para crear un francotirador
//================================================================================
class CBaseWeaponSniper : public CBaseWeapon
{
public:
    DECLARE_CLASS( CBaseWeaponSniper, CBaseWeapon );
    DECLARE_NETWORKCLASS();
    DECLARE_PREDICTABLE();

    CBaseWeaponSniper();

    // Tipo de arma
    virtual bool IsSniper() { return true; }
    virtual bool IsWeaponZoomed() { return m_bInZoom; }

    // Configuración
    virtual float GetSpreadPerShot();

    // Acciones
    virtual void ItemPostFrame();

    virtual bool CanSecondaryAttack();
    virtual void SecondaryAttack();

    virtual void ToggleZoom();
    virtual void EnableZoom();
    virtual void DisableZoom();

protected:
    CNetworkVar( bool, m_bInZoom );

private:
    CBaseWeaponSniper( const CBaseWeaponSniper & );
};

//================================================================================
// Clase base para crear una escopeta
//================================================================================
class CBaseWeaponShotgun : public CBaseWeapon
{
public:
    DECLARE_CLASS( CBaseWeaponShotgun, CBaseWeapon );
    DECLARE_NETWORKCLASS();
    DECLARE_PREDICTABLE();

	CBaseWeaponShotgun();

	// Tipo de arma
	virtual bool IsShotgun() { return false; }

	// Recarga
	virtual bool Reload();
	virtual void FinishReload();

private:
    CBaseWeaponShotgun( const CBaseWeaponShotgun & );
};

//================================================================================
// Macros
//================================================================================

#define IMPLEMENT_DEFAULT_ACTTABLE(classname, anim) acttable_t C##classname::m_acttable[] = {\
        { ACT_MP_STAND_IDLE,                    ACT_IDLE_##anim,                false },\
        { ACT_MP_IDLE_CALM,                        ACT_IDLE_CALM_##anim,            false },\
        { ACT_MP_IDLE_INJURED,                    ACT_IDLE_INJURED_##anim,        false },\
        { ACT_MP_RUN,                            ACT_RUN_##anim,                    false },\
        { ACT_MP_RUN_CALM,                        ACT_RUN_CALM_##anim,            false },\
        { ACT_MP_RUN_INJURED,                    ACT_RUN_INJURED_##anim,            false },\
        { ACT_MP_WALK,                            ACT_WALK_##anim,                false },\
        { ACT_MP_WALK_CALM,                        ACT_WALK_CALM_##anim,            false },\
        { ACT_MP_WALK_INJURED,                    ACT_WALK_INJURED_##anim,        false },\
        { ACT_MP_CROUCH_IDLE,                    ACT_CROUCHIDLE_##anim,            false },\
        { ACT_MP_CROUCHWALK,                    ACT_RUN_CROUCH_##anim,            false },\
        { ACT_MP_ATTACK_STAND_PRIMARYFIRE,        ACT_PRIMARYATTACK_##anim,        false },\
        { ACT_MP_ATTACK_CROUCH_PRIMARYFIRE,        ACT_PRIMARYATTACK_##anim,        false },\
        { ACT_MP_RELOAD_STAND,                    ACT_RELOAD_##anim,                false },\
        { ACT_MP_RELOAD_CROUCH,                    ACT_RELOAD_##anim,                false },\
        { ACT_MP_DEPLOY,                        ACT_DEPLOY_##anim,                false },\
    };\
    IMPLEMENT_ACTTABLE( C##classname );

#undef DECLARE_WEAPON
#define DECLARE_WEAPON( classname, name, baseclass, anim ) class C##classname : public baseclass {\
public:\
    DECLARE_CLASS( C##classname, baseclass );\
    DECLARE_NETWORKCLASS();\
    DECLARE_PREDICTABLE();\
    DECLARE_ACTTABLE();\
    C##classname() { }\
private:\
    C##classname( const C##classname & );\
};\
    IMPLEMENT_NETWORKCLASS_ALIASED( classname, DT_##classname )\
    BEGIN_NETWORK_TABLE( C##classname, DT_##classname )\
    END_NETWORK_TABLE()\
    BEGIN_PREDICTION_DATA( C##classname )\
    END_PREDICTION_DATA()\
    LINK_ENTITY_TO_CLASS( weapon_##name, C##classname );\
    PRECACHE_WEAPON_REGISTER( weapon_##name ); \
    IMPLEMENT_DEFAULT_ACTTABLE( classname, anim );

/*
#undef DECLARE_WEAPON
#define DECLARE_WEAPON( classname, name, anim ) DECLARE_BASE_WEAPON( classname, name, CBaseWeapon, anim )

#undef DECLARE_WEAPON_SNIPER
#define DECLARE_WEAPON_SNIPER( classname, name, anim ) DECLARE_BASE_WEAPON( classname, name, CBaseWeaponSniper, anim )

#undef DECLARE_WEAPON_SHOTGUN
#define DECLARE_WEAPON_SHOTGUN( classname, name, anim ) DECLARE_BASE_WEAPON( classname, name, CBaseWeaponShotgun, anim )
*/

#endif // WEAPON_BASE_H