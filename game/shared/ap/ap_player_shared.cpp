//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#include "cbase.h"
#include "in_gamerules.h"

#ifdef CLIENT_DLL
	#include "c_ap_player.h"

	#define CAP_PlayerSurvivor C_AP_PlayerSurvivor
#else
	#include "ap_player.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar sv_player_speed;
extern ConVar sv_player_walk_speed;
extern ConVar sv_player_sprint_speed;

//================================================================================
// Crea el sistema de animación
//================================================================================
void CAP_Player::CreateAnimationSystem()
{
    MultiPlayerMovementData_t data;
    data.Init();

    data.m_nAimPitchPoseName = "body_pitch";
    data.m_nAimYawPoseName = "body_yaw";
    data.m_flBodyYawRate = 120.0f;
    data.m_flRunSpeed = 190.0f;
    data.m_flWalkSpeed = 1.0f;

    m_pAnimationSystem = CreatePlayerAnimationSystem( this, data );
}

//====================================================================
// Traduce una actividad a otra
//====================================================================
Activity CAP_Player::TranslateActivity( Activity actBase )
{
    int health = GetHealth();

    // Animaciones que tienen prioridad sobre la del arma
    {
        if ( GetPlayerStatus() == PLAYER_STATUS_DEJECTED ) {
            return ACT_IDLE_INCAP_PISTOL;
        }

        if ( GetPlayerStatus() == PLAYER_STATUS_CLIMBING ) {
            if ( health < 30 )
                return ACT_TERROR_LEDGE_HANG_DANGLE;
            else if ( health < 60 )
                return ACT_TERROR_LEDGE_HANG_WEAK;
            else
                return ACT_TERROR_LEDGE_HANG_FIRM;
        }

        if ( GetPlayerStatus() == PLAYER_STATUS_FALLING ) {
            return ACT_TERROR_FALL;
        }
    }

    // Usamos la animación del arma
    if ( GetActiveWeapon() ) {
        Activity weapActivity = GetActiveWeapon()->ActivityOverride( actBase, false );

        if ( weapActivity != actBase )
            return weapActivity;
    }

    // Otras animaciones / sin arma
    switch ( actBase ) {
        case ACT_MP_STAND_IDLE:
        case ACT_MP_IDLE_CALM:
		case ACT_MP_SWIM_IDLE:
        {
            if ( IsSurvivor() || IsSoldier() ) {
                return ACT_IDLE_CALM_PISTOL;
            }

            return ACT_IDLE;
            break;
        }

        case ACT_MP_IDLE_INJURED:
        {
            if ( IsSurvivor() || IsSoldier() ) {
                return ACT_IDLE_INJURED_PISTOL;
            }
            break;
        }

        case ACT_MP_RUN:
        case ACT_MP_RUN_CALM:
		case ACT_MP_SWIM:
        {
            if ( IsSurvivor() || IsSoldier() ) {
                return ACT_RUN_CALM_PISTOL;
            }

            return ACT_RUN;
            break;
        }

        case ACT_MP_RUN_INJURED:
        {
            if ( IsSurvivor() || IsSoldier() ) {
                return ACT_RUN_INJURED_PISTOL;
            }
            break;
        }

        case ACT_MP_WALK:
        case ACT_MP_WALK_CALM:
        {
            if ( IsSurvivor() || IsSoldier() ) {
                return ACT_WALK_CALM_PISTOL;
            }

            return ACT_WALK;
            break;
        }

        case ACT_MP_WALK_INJURED:
        {
            if ( IsSurvivor() || IsSoldier() ) {
                return ACT_WALK_INJURED_PISTOL;
            }
        }

        case ACT_MP_CROUCH_IDLE:
        {
            if ( IsSurvivor() || IsSoldier() ) {
                return ACT_CROUCHIDLE_PISTOL;
            }

            return ACT_CROUCHIDLE;
            break;
        }

        case ACT_MP_CROUCHWALK:
        {
            if ( IsSurvivor() || IsSoldier() ) {
                return ACT_RUN_CROUCH_PISTOL;
            }

            return ACT_RUN_CROUCH;
            break;
        }

        case ACT_MP_JUMP:
        {
            if ( IsSurvivor() || IsSoldier() ) {
                return ACT_JUMP_RIFLE;
            }

            return ACT_JUMP;
            break;
        }

        case ACT_DIERAGDOLL:
        {
            if ( IsSurvivor() || IsSoldier() ) {
                if ( GetPlayerStatus() == PLAYER_STATUS_DEJECTED )
                    return ACT_DIE_INCAP;

                //if ( RandomInt(0, 10) == 0 )
                //return ACT_TERROR_DIE_FROM_STAND;

                return ACT_DIE_STANDING;
            }

            return ACT_DIESIMPLE;
            break;
        }

        case ACT_MP_GESTURE_FLINCH_HEAD:
        case ACT_MP_GESTURE_FLINCH_STOMACH:
        case ACT_MP_GESTURE_FLINCH_CHEST:
        case ACT_MP_GESTURE_FLINCH_LEFTARM:
        case ACT_MP_GESTURE_FLINCH_RIGHTARM:
        case ACT_MP_GESTURE_FLINCH:
        {
            if ( GetPlayerStatus() == PLAYER_STATUS_CLIMBING )
                return ACT_TERROR_FLINCH_LEDGE;

            return ACT_TERROR_FLINCH;
            break;
        }
    }

    return actBase;
}

//================================================================================
// Devuelve la velocidad inicial del jugador
//================================================================================
float CAP_Player::GetSpeed()
{
    if ( IsSoldier() ) {
        // Cloacker
        if ( GetPlayerClass() == PLAYER_CLASS_SOLDIER_LEVEL2 ) {
            return (sv_player_sprint_speed.GetFloat() + 60.0f);
        }

        // Bulldozer
        if ( GetPlayerClass() == PLAYER_CLASS_SOLDIER_LEVEL3 ) {
            if ( IsSprinting() || IsCrouching() )
                return sv_player_speed.GetFloat();
            else
                return sv_player_walk_speed.GetFloat();
        }
    }

    return BaseClass::GetSpeed();
}