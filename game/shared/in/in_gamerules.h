//==== Woots 2017. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef IN_GAMERULES_H
#define IN_GAMERULES_H

#pragma once

#include "gamerules.h"
#include "voice_gamemgr.h"

#include "in_view_vectors.h"
#include "in_shareddefs.h"

#ifdef CLIENT_DLL
    class C_Player;

    #define CInGameRules C_InGameRules
    #define CInGameRulesProxy C_InGameRulesProxy
    #define CPlayer C_Player
#else
    class CPlayer;
    class CSoundManager;
#endif

//================================================================================
// Macros
//================================================================================

#define IS_SKILL_EASY TheGameRules->IsSkillLevel(SKILL_EASY)
#define IS_SKILL_MEDIUM TheGameRules->IsSkillLevel(SKILL_MEDIUM)
#define IS_SKILL_HARD TheGameRules->IsSkillLevel(SKILL_HARD)
#define IS_SKILL_VERY_HARD TheGameRules->IsSkillLevel(SKILL_VERY_HARD)
#define IS_SKILL_ULTRA_HARD TheGameRules->IsSkillLevel(SKILL_ULTRA_HARD)
#define IS_SKILL_HARDEST TheGameRules->IsSkillLevel(SKILL_HARDEST)
#define GetGameDifficulty() TheGameRules->GetSkillLevel()

#ifndef CLIENT_DLL
//================================================================================
// Administrador para el chat de voz
//================================================================================
class CVoiceGameMgrHelper : public IVoiceGameMgrHelper
{
    //DECLARE_CLASS( CVoiceGameMgrHelper, IVoiceGameMgrHelper );

public:
    virtual bool CanPlayerHearPlayer( CBasePlayer *, CBasePlayer *, bool & );
};
#endif

//================================================================================
// Permite el envio de información de las Reglas de Juego
// entre servidor y cliente
//================================================================================
class CInGameRulesProxy : public CGameRulesProxy
{
    DECLARE_CLASS( CInGameRulesProxy, CGameRulesProxy );
    DECLARE_NETWORKCLASS();

public:
};

//================================================================================
// Base de las reglas del juego.
//================================================================================
class CInGameRules : public CGameRules
{
    DECLARE_CLASS( CInGameRules, CGameRules );
    DECLARE_NETWORKCLASS_NOBASE();

public:

    // Definiciones
    virtual bool IsMultiplayer() { return false; }
    virtual bool IsTeamplay() { return false; }
    virtual bool IsDeathmatch() { return false; }
    virtual bool IsCoOp() { return false; }
    virtual bool HasDirector() { return false; }

    // Modo de juego
    virtual int GetGameMode() { return GAME_MODE_NONE; }
    virtual bool IsGameMode( int iMode ) { return (GetGameMode() == iMode); }

    // Modo de Spawn
    virtual int GetSpawnMode() { return SPAWN_MODE_RANDOM; } 
    virtual bool IsSpawnMode( int iMode ) { return (GetSpawnMode() == iMode); }

    // Principales
    CInGameRules();
    ~CInGameRules();

    virtual const char *GetGameDescription();

    virtual bool ShouldCollide( int, int );

    // Cámara
    const CViewVectors *GetViewVectors() const;
    const CInViewVectors *GetInViewVectors() const;

    // Damage query implementations.
    virtual bool Damage_IsTimeBased( int iDmgType );            // Damage types that are time-based.
    virtual bool Damage_ShouldGibCorpse( int iDmgType );        // Damage types that gib the corpse.
    virtual bool Damage_ShowOnHUD( int iDmgType );            // Damage types that have client HUD art.
    virtual bool Damage_NoPhysicsForce( int iDmgType );        // Damage types that don't have to supply a physics force & position.
    virtual bool Damage_ShouldNotBleed( int iDmgType );        // Damage types that don't make the player bleed.
    virtual bool Damage_CausesSlowness( const CTakeDamageInfo &info );

    virtual int Damage_GetTimeBased();
    virtual int Damage_GetShouldGibCorpse();
    virtual int Damage_GetShowOnHud();
    virtual int Damage_GetNoPhysicsForce();
    virtual int Damage_GetShouldNotBleed();
    virtual int Damage_GetCausesSlowness();

    virtual bool CanPushEntity( CBaseEntity *pPushingEntity, CBaseEntity *pEntity );

#ifndef CLIENT_DLL

    // Dificultad
    virtual int	GetSkillLevel() { return g_iDifficultyLevel; }
    virtual void RefreshSkillData( bool forceUpdate );
    virtual void OnSkillLevelChanged( int iNewLevel );
    virtual void SetSkillLevel( int iLevel );

    // Director
	virtual bool Director_AdaptativeSkill();

    virtual void Director_PreUpdate() {}
    virtual void Director_Update();
    virtual void Director_PostUpdate() {}

    virtual void Director_CreateMusic( CSoundManager *pManager );
    virtual int Director_MusicDesire( const char *soundname, int channel );
    virtual void Director_OnMusicPlay( const char *soundname );
    virtual void Director_OnMusicStop( const char *soundname );

    // Servidor
    virtual void StartServer();
    virtual void LoadServer();

    virtual void Precache();
    virtual void Think();

    virtual void RestartMap();

    // Conexión del cliente
    virtual bool ClientConnected( edict_t *, const char *, const char *, char *, int  );
    virtual void ClientDisconnected( edict_t * );
    virtual bool ClientCommand( CBaseEntity *, const CCommand & );
    virtual void ClientDestroy( CBasePlayer * );

    // Relaciones
    virtual void InitDefaultAIRelationships();
    virtual int PlayerRelationship( CBaseEntity *, CBaseEntity * );

    virtual bool FShouldSwitchWeapon( CBasePlayer *, CBaseCombatWeapon * );
    
    // Daño
    virtual bool AllowDamage( CBaseEntity *pVictim, const CTakeDamageInfo &info );
    
    virtual bool CanFriendlyfire( CBasePlayer *pVictim, CBasePlayer *pAttacker );
    virtual bool CanFriendlyfire();

    virtual bool CanBotsFriendlyfire( CBasePlayer *pVictim, CBasePlayer *pAttacker );
    virtual bool CanBotsFriendlyfire();

    virtual void AdjustDamage( CBaseEntity *pVictim, CTakeDamageInfo &info );
    virtual float AdjustPlayerDamageInflicted( float damage ) { return damage; }
    virtual void AdjustPlayerDamageTaken( CPlayer *pVictim, CTakeDamageInfo &info );
    virtual void AdjustPlayerDamageTaken( CTakeDamageInfo *info ) { }
    virtual void AdjustPlayerDamageHitGroup( CPlayer *pVictim, CTakeDamageInfo &info, int hitGroup );
    
    virtual float FlPlayerFallDamage( CBasePlayer * );

    virtual bool FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker, const CTakeDamageInfo &info );
    virtual bool FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker );

    virtual bool FPlayerCanShieldHandleDamage( CPlayer *pPlayer, const CTakeDamageInfo &info );
    virtual float FPlayerGetDamageHandledByShield( CPlayer * pPlayer, const CTakeDamageInfo &info );
    virtual float PlayerShieldPause( CPlayer * pPlayer );

    virtual bool FPlayerCanDejected( CBasePlayer *, const CTakeDamageInfo & );
    virtual bool FCanPlayDeathSound( const CTakeDamageInfo & );

    virtual bool FPlayerCanRespawn( CBasePlayer * );
    virtual bool FPlayerCanRespawnNow( CPlayer * );
    virtual bool FPlayerCanGoSpectate( CBasePlayer *pPlayer );
    virtual float FlPlayerSpawnTime( CBasePlayer * );

    virtual void PlayerThink( CBasePlayer *pPlayer );
    virtual void PlayerSpawn( CBasePlayer *pPlayer );
    virtual void PlayerKilled( CBasePlayer *pPlayer, const CTakeDamageInfo &info );

    virtual void InitHUD( CBasePlayer *pPlayer );

    // Autoaim
    virtual bool AllowAutoTargetCrosshair();
	virtual bool ShouldAutoAim( CBasePlayer *pPlayer, edict_t *target ) { return true; }
	virtual float GetAutoAimScale( CBasePlayer *pPlayer ) { return 1.0f; }
	virtual int	GetAutoAimMode();

    virtual int IPointsForKill( CBasePlayer *, CBasePlayer * );

    virtual void DeathNotice( CBasePlayer *pVictim, const CTakeDamageInfo &info ) { }
    virtual bool UseSuicidePenalty() { return true; }
    virtual float FlHealthChargerRechargeTime() { return 0.0f; }

    virtual bool CanPlayDeathAnim( CBaseEntity *, const CTakeDamageInfo & );
    virtual bool CanHavePlayerItem( CBasePlayer *, CBaseCombatWeapon * );
    virtual bool CanHaveItem( CBasePlayer *, CItem * );

    virtual void PlayerGotItem( CBasePlayer *, CItem * ) { }
    virtual void PlayerGotAmmo( CBaseCombatCharacter *, char *, int ) { }

    virtual int WeaponShouldRespawn( CBaseCombatWeapon *pWeapon ) { return GR_WEAPON_RESPAWN_NO; }
    virtual float FlWeaponRespawnTime( CBaseCombatWeapon *pWeapon ) { return 0.0f; }
    virtual float FlWeaponTryRespawn( CBaseCombatWeapon *pWeapon ) { return 0.0f; }
    virtual Vector VecWeaponRespawnSpot( CBaseCombatWeapon *pWeapon ) { return vec3_origin; }

    virtual int ItemShouldRespawn( CItem * ) { return GR_ITEM_RESPAWN_NO; }
    virtual float FlItemRespawnTime( CItem *pItem ) { return 0.0f; }
    virtual Vector VecItemRespawnSpot( CItem *pItem ) { return vec3_origin; }
    virtual QAngle VecItemRespawnAngles( CItem *pItem ) { return vec3_angle; }

    virtual bool IsAllowedToSpawn( CBaseEntity *pEntity ) { return true; }

    virtual int DeadPlayerWeapons( CBasePlayer *pPlayer ) { return GR_PLR_DROP_GUN_ACTIVE; }
    virtual int DeadPlayerAmmo( CBasePlayer *pPlayer ) { return GR_PLR_DROP_AMMO_ACTIVE; }

    virtual CBaseCombatWeapon *GetNextBestWeapon( CBaseCombatCharacter *, CBaseCombatWeapon * );
    virtual CBaseEntity *GetPlayerSpawnSpot( CBasePlayer * );

    virtual const char *GetTeamID( CBaseEntity *pEntity ) { return ""; }
    virtual bool PlayerCanHearChat( CBasePlayer *, CBasePlayer * );
    virtual bool PlayFootstepSounds( CBasePlayer * );

    virtual bool FAllowFlashlight();
    virtual bool FAllowNPCs();

    virtual bool CanUseSpawnPoint( CBaseEntity * );
    virtual void UseSpawnPoint( CBaseEntity * );
    virtual void FreeSpawnPoint( CBaseEntity * );
#else
	virtual const char *GetBackgroundMovie();
#endif

protected:
    #ifndef CLIENT_DLL
        CUtlVector<int> m_nSpawnSlots;
    #endif
};

extern CInGameRules *TheGameRules;

#define VEC_DEJECTED_HULL_MIN TheGameRules->GetInViewVectors()->m_vDejectedHullMin
#define VEC_DEJECTED_HULL_MAX TheGameRules->GetInViewVectors()->m_vDejectedHullMax
#define VEC_DEJECTED_VIEWHEIGHT TheGameRules->GetInViewVectors()->m_vDejectedView

#endif // IN_GAMERULES_H