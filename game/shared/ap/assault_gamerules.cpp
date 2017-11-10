//==== Woots 2017. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "assault_gamerules.h"

#ifndef CLIENT_DLL
    #include "ap_player.h"
    #include "in_utils.h"
    #include "bots\bot.h"
    #include "players_system.h"
    #include "director.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

REGISTER_GAMERULES_CLASS( CAssaultGameRules );

//================================================================================
// Comandos
//================================================================================

#ifndef CLIENT_DLL
DECLARE_SERVER_CMD( sk_soldier_head, "2.0", "" )
DECLARE_SERVER_CMD( sk_soldier_chest, "1.3", "" )
DECLARE_SERVER_CMD( sk_soldier_stomach, "1.0", "" )
DECLARE_SERVER_CMD( sk_soldier_arm, "0.8", "" )
DECLARE_SERVER_CMD( sk_soldier_leg, "0.6", "" )

// Escudo
DECLARE_NOTIFY_CMD( sk_soldier_shield_pause, "5.0", "Tiempo en segundos que la regeneracion de escudo es pausada al tomar dano." )
#endif

//================================================================================
//================================================================================
CAssaultGameRules::CAssaultGameRules()
{
}

#ifndef CLIENT_DLL
//================================================================================
//================================================================================
void CAssaultGameRules::PlayerThink( CBasePlayer *pBasePlayer )
{
    CAP_Player *pPlayer = ToApPlayer( pBasePlayer );
    Assert( pPlayer );

    if ( pPlayer->Classify() == CLASS_PLAYER_ALLY_VITAL ) {
        pPlayer->AddAttributeModifier( "shield_bot" );
    }
    else if ( pPlayer->Classify() == CLASS_PLAYER ) {
        pPlayer->AddAttributeModifier( "shield_level6" ); // TODO
    }
    else if ( pPlayer->IsSoldier() ) {
        // Bulldozer
        if ( pPlayer->GetPlayerClass() == PLAYER_CLASS_SOLDIER_LEVEL3 ) {
            switch ( pPlayer->GetDifficultyLevel() ) {
                case SKILL_EASY:
                    pPlayer->AddAttributeModifier( "shield_level5" );
                    break;

                case SKILL_MEDIUM:
                    pPlayer->AddAttributeModifier( "shield_level6" );
                    break;

                case SKILL_HARD:
                    pPlayer->AddAttributeModifier( "shield_level7" );
                    break;

                case SKILL_VERY_HARD:
                    pPlayer->AddAttributeModifier( "shield_level8" );
                    Utils::AddAttributeModifier( "shield_level4", 2000.0f, pPlayer->GetAbsOrigin(), pPlayer->GetTeamNumber() );
                    break;

                case SKILL_ULTRA_HARD:
                    pPlayer->AddAttributeModifier( "shield_level10" );
                    Utils::AddAttributeModifier( "shield_level5", 2000.0f, pPlayer->GetAbsOrigin(), pPlayer->GetTeamNumber() );
                    break;
            }
        }
    }

    if ( pPlayer->GetBotController() ) {
        if ( pPlayer->IsSurvivor() && pPlayer->GetBotController()->GetFollow() ) {
            CPlayer *pLeader = ToInPlayer( pPlayer->GetBotController()->GetFollow()->GetEntity() );

            if ( !pLeader || pLeader->IsBot() ) {
                if ( TheDirector->GetStatus() == STATUS_FINALE || TheDirector->GetStatus() == STATUS_PANIC ) {
                    CPlayer *pNear = ThePlayersSystem->GetNear( pPlayer->GetAbsOrigin(), pPlayer, TEAM_HUMANS, true );
                    pPlayer->GetBotController()->GetFollow()->Start( pNear );
                }
            }
        }
    }
}

//================================================================================
//================================================================================
void CAssaultGameRules::AdjustPlayerDamageTaken( CPlayer * pBaseVictim, CTakeDamageInfo & info )
{
    BaseClass::AdjustPlayerDamageTaken( pBaseVictim, info );

    if ( info.GetDamage() <= 0 )
        return;

    CAP_Player *pVictim = ToApPlayer( pBaseVictim );
    Assert( pVictim );

    // En el modo asalto los supervivientes toman mucho menos daño de las balas
    /*if ( pVictim->IsSurvivor() && (info.GetDamageType() & DMG_BULLET) != 0 ) {
        info.ScaleDamage( sk_assault_survivors_scale_damage_taken.GetFloat() );
    }*/
}

//====================================================================
// Ajusta el daño recibido en un lugar especifico del cuerpo
//====================================================================
void CAssaultGameRules::AdjustPlayerDamageHitGroup( CPlayer * pBaseVictim, CTakeDamageInfo & info, int hitGroup )
{
    CAP_Player *pVictim = ToApPlayer( pBaseVictim );
    Assert( pVictim );

    if ( !pVictim->IsSoldier() ) {
        BaseClass::AdjustPlayerDamageHitGroup( pVictim, info, hitGroup );
        return;
    }

    switch ( hitGroup ) {
        case HITGROUP_GEAR:
            info.SetDamage( 0.0f );
            break;
        case HITGROUP_HEAD:
            info.ScaleDamage( sk_soldier_head.GetFloat() );
            break;
        case HITGROUP_CHEST:
            info.ScaleDamage( sk_soldier_chest.GetFloat() );
            break;
        case HITGROUP_STOMACH:
        default:
            info.ScaleDamage( sk_soldier_stomach.GetFloat() );
            break;
        case HITGROUP_LEFTARM:
        case HITGROUP_RIGHTARM:
            info.ScaleDamage( sk_soldier_arm.GetFloat() );
            break;
        case HITGROUP_LEFTLEG:
        case HITGROUP_RIGHTLEG:
            info.ScaleDamage( sk_soldier_leg.GetFloat() );
            break;
    }
}

//================================================================================
//================================================================================
float CAssaultGameRules::PlayerShieldPause( CPlayer * pBasePlayer )
{
    CAP_Player *pPlayer = ToApPlayer( pBasePlayer );
    Assert( pPlayer );

    if ( pPlayer->IsSoldier() ) {
        return sk_soldier_shield_pause.GetFloat();
    }

    return BaseClass::PlayerShieldPause( pBasePlayer );
}
#endif