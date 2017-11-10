//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "in_gamerules.h"

#include "takedamageinfo.h"
#include "effect_dispatch_data.h"
#include "movevars_shared.h"
#include "gamevars_shared.h"
#include "engine/IEngineSound.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"
#include "engine/ivdebugoverlay.h"
#include "obstacle_pushaway.h"
#include "props_shared.h"
#include "debugoverlay_shared.h"

#include "baseentity_shared.h"
#include "ammodef.h"
#include "decals.h"
#include "util_shared.h"

#include "in_ammodef.h"
#include "weapon_base.h"
#include "shot_manipulator.h"
#include "ai_debug_shared.h"

#include "bullet.h"
#include "in_buttons.h"

#ifdef CLIENT_DLL
#include "c_in_player.h"
#include "prediction.h"
#include "clientmode_in.h"
#include "vgui_controls/AnimationController.h"

#define CRecipientFilter C_RecipientFilter
#define CPlayer C_Player
#else
#include "in_player.h"
#include "rumble_shared.h"
#include "soundent.h"
#include "player_lagcompensation.h"
#include "IEffects.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void DispatchEffect( const char *pName, const CEffectData &data );

//================================================================================
// Macros
//================================================================================

#ifndef CLIENT_DLL
#define CHASE_CAM_DISTANCE		96.0f
#define WALL_OFFSET				6.0f
#endif

//================================================================================
// Comandos
//================================================================================

// Velocidad al caminar
DECLARE_NOTIFY_CMD( sv_player_walk_speed, "80.0", "Velocidad al caminar." )

// Velocidad normal
DECLARE_NOTIFY_CMD( sv_player_speed, "200.0", "Velocidad normal." )

// Velocidad al correr
DECLARE_NOTIFY_CMD( sv_player_sprint_speed, "230.0", "Velocidad al correr." )

DECLARE_NOTIFY_CMD( sv_firstperson_ragdoll, "0", "" );
DECLARE_NOTIFY_CMD( sv_player_view_from_eyes, "0", "" );

DECLARE_SERVER_CMD( sv_player_tests_hull_mins_x, "0", "" );
DECLARE_SERVER_CMD( sv_player_tests_hull_mins_y, "0", "" );

DECLARE_SERVER_CMD( sv_player_tests_hull_maxs_x, "0", "" );
DECLARE_SERVER_CMD( sv_player_tests_hull_maxs_y, "0", "" );

//================================================================================
//================================================================================


//================================================================================
// Crea el sistema de animación
//================================================================================
void CPlayer::CreateAnimationSystem()
{
    // Información predeterminada
    MultiPlayerMovementData_t data;
    data.Init();

    data.m_flBodyYawRate = 120.0f;
    data.m_flRunSpeed = 110.0f;
    data.m_flWalkSpeed = 1.0f;

    m_pAnimationSystem = CreatePlayerAnimationSystem( this, data );
}

//================================================================================
// Traduce una actividad a otra
//================================================================================
Activity CPlayer::TranslateActivity( Activity actBase )
{
    // Tenemos un arma
    if ( GetActiveWeapon() ) {
        Activity weapActivity = GetActiveWeapon()->ActivityOverride( actBase, false );

        if ( weapActivity != actBase )
            return weapActivity;
    }

    return actBase;
}

//================================================================================
// Devuelve la posición de donde deben salir las balas
//================================================================================
Vector CPlayer::Weapon_ShootPosition()
{
    return BaseClass::Weapon_ShootPosition();
    /*
    CBaseViewModel *vm = GetViewModel();

    if ( !vm )
        return BaseClass::Weapon_ShootPosition();

    int iAttachment = vm->LookupAttachment("muzzle");

    if ( iAttachment == -1 )
        return BaseClass::Weapon_ShootPosition();

    Vector vecOrigin;
    
    if ( !vm->GetAttachment( iAttachment, vecOrigin ) )
        return BaseClass::Weapon_ShootPosition();

    return vecOrigin;
    */
}

//================================================================================
// Devuelve la posición de donde deben salir las balas
//================================================================================
Vector CPlayer::Weapon_ShootDirection()
{
    return GetAutoaimVector( AUTOAIM_SCALE_DEFAULT );

    /*
    CBaseViewModel *vm = GetViewModel();

    // Sin arma activa
    if ( !vm )
    return GetAutoaimVector( AUTOAIM_SCALE_DEFAULT );

    int iAttachment = vm->LookupAttachment("muzzle_flash");

    if ( iAttachment == -1 )
    return GetAutoaimVector( AUTOAIM_SCALE_DEFAULT );

    Vector vecOrigin, vecDir;
    QAngle angles;
    vm->GetAttachment( iAttachment, vecOrigin, angles );

    AngleVectors( angles, &vecDir, NULL, NULL );
    return vecDir;
    */
}

//================================================================================
// Lanza disparos desde el arma del jugador
// NOTA: En clientside solo funciona para el jugador local.
//================================================================================
void CPlayer::FireBullets( const FireBulletsInfo_t &info )
{
    int iDamageType = GetAmmoDef()->DamageType( info.m_iAmmoType );

#ifndef CLIENT_DLL
    if ( GetActiveWeapon() ) {
        int iRumbleEffect = GetActiveWeapon()->GetRumbleEffect();

        // El arma debe envíar un temblor al mando al disparar
        if ( iRumbleEffect != RUMBLE_INVALID )
            RumbleEffect( iRumbleEffect, 0, RUMBLE_FLAG_RESTART );
    }
#endif

    // Make sure we don't have a dangling damage target from a recursive call
    if ( g_MultiDamage.GetTarget() != NULL ) {
        ApplyMultiDamage();
    }

    ClearMultiDamage();
    g_MultiDamage.SetDamageType( iDamageType );

    int seed = CBaseEntity::GetPredictionRandomSeed() & 255;

#ifndef CLIENT_DLL
    lagcompensation->StartLagCompensation( this, LAG_COMPENSATE_HITBOXES );
#endif

    for ( int iShot = 0; iShot < info.m_iShots; iShot++ ) {
        CBullet *pBullet = new CBullet( iShot, info, this );
        pBullet->Fire( seed );
        seed++;
    }

#ifndef CLIENT_DLL
    lagcompensation->FinishLagCompensation( this );
#endif

    OnFireBullets( info );

    ApplyMultiDamage();
}

//================================================================================
//================================================================================
void CPlayer::OnFireBullets( const FireBulletsInfo_t & info )
{
#ifndef CLIENT_DLL
    m_CombatTimer.Start();
    AddAttributeModifier( "stress_firegun" );
#endif
}

//====================================================================
//====================================================================
bool CPlayer::ShouldDrawUnderwaterBulletBubbles()
{
    return (GetWaterLevel() == 3);
}

//================================================================================
// Cámara desde los ojos del modelo
//================================================================================
bool CPlayer::GetEyesView( CBaseAnimating *pEntity, Vector& eyeOrigin, QAngle& eyeAngles, int secureDistance )
{
    if ( !pEntity )
        return false;

    int attachment = pEntity->LookupAttachment( "eyes" );

    if ( attachment == -1 )
        return false;

    // Obtenemos la ubicación de los ojos
    pEntity->GetAttachment( attachment, eyeOrigin, eyeAngles );

    // Obtenemos la dirección "enfrente" de los ojos
    Vector vecForward;
    AngleVectors( eyeAngles, &vecForward );

    // Verificamos que la visión de los ojos no choque con algún objeto solido
    trace_t tr;
    UTIL_TraceLine( eyeOrigin, eyeOrigin + secureDistance * vecForward, MASK_ALL, this, COLLISION_GROUP_NONE, &tr );

    // La vista no choca, todo bien
    if ( tr.fraction == 1.0f || (tr.endpos.DistTo( eyeOrigin ) > 30) )
        return true;

    return false;
}

//================================================================================
// Calcula la posición de la camara del Jugador
//================================================================================
void CPlayer::CalcPlayerView( Vector& eyeOrigin, QAngle& eyeAngles, float& fov )
{
    try {
        if ( !IsAlive() ) {
            CBaseAnimating *pRagdoll = GetRagdoll();

            if ( sv_firstperson_ragdoll.GetBool() ) {
                if ( GetEyesView( pRagdoll, eyeOrigin, eyeAngles, fov ) )
                    return;
            }

            Vector vecRagdollPosition = pRagdoll->GetAbsOrigin();
            vecRagdollPosition.z += VEC_DEAD_VIEWHEIGHT.z;

            // TODO ?
            BaseClass::CalcPlayerView( eyeOrigin, eyeAngles, fov );

            //
            eyeOrigin = vecRagdollPosition;

            Vector vecForward;
            AngleVectors( eyeAngles, &vecForward );
            VectorNormalize( vecForward );

            VectorMA( vecRagdollPosition, -50.0F, vecForward, eyeOrigin );

            Vector WALL_MIN( -WALL_OFFSET, -WALL_OFFSET, -WALL_OFFSET );
            Vector WALL_MAX( WALL_OFFSET, WALL_OFFSET, WALL_OFFSET );

            trace_t trace; // clip against world

#ifdef CLIENT_DLL
            CBaseEntity::EnableAbsRecomputations( false );
#endif
            UTIL_TraceHull( vecRagdollPosition, eyeOrigin, WALL_MIN, WALL_MAX, MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &trace );
#ifdef CLIENT_DLL
            CBaseEntity::EnableAbsRecomputations( true );
#endif

            if ( trace.fraction < 1.0 )
                eyeOrigin = trace.endpos;

            return;
        }
    }
    catch ( ... ) { }

    if ( sv_player_view_from_eyes.GetBool() ) {
        if ( GetEyesView( this, eyeOrigin, eyeAngles, fov ) )
            return;
    }

    BaseClass::CalcPlayerView( eyeOrigin, eyeAngles, fov );
}

//================================================================================
// Devuelve la velocidad inicial del jugador
//================================================================================
float CPlayer::GetSpeed()
{
    // Velocidad normal
    float flSpeed = sv_player_speed.GetFloat();

    // Estamos corriendo o agachados
    // Ivan: Si estamos agachados usamos la velocidad
    // al correr ya que en el código de Valve se hace la disminución
    if ( IsSneaking() ) {
        flSpeed = sv_player_walk_speed.GetFloat();
    }
    else if ( IsSprinting() || IsCrouching() ) {
        flSpeed = sv_player_sprint_speed.GetFloat();
    }

    return flSpeed;
}

//================================================================================
// Aplica modificadores a la velocidad máxima a la que puede ir el jugador
//================================================================================
void CPlayer::SpeedModifier( float &speed )
{
    // TODO: Clientside
#ifndef CLIENT_DLL
    // Hemos recibido daño que nos hace lentos
    if ( m_SlowDamageTimer.HasStarted() && m_SlowDamageTimer.IsLessThen( 0.5f ) ) {
        speed -= 110.0f;
        speed = MAX( 80.0f, speed );
        return;
    }
#endif

    // Disminuimos la velocidad según nuestra salud
    if ( GetHealth() <= 50 )
        speed -= 40.0f;
    else if ( GetHealth() <= 30 )
        speed -= 50.0f;
    else if ( GetHealth() <= 15 )
        speed -= 60.0f;

    // Minímo: 80
    speed = MAX( 80.0f, speed );
}

//================================================================================
// Actualiza la velocidad máxima del Jugador
//================================================================================
void CPlayer::UpdateSpeed()
{
    float flSpeed = GetSpeed();

    SpeedModifier( flSpeed );
    SetMaxSpeed( flSpeed );
}

//================================================================================
// Devuelve si el jugador puede correr
//================================================================================
bool CPlayer::CanSprint()
{
#ifdef APOCALYPSE
    // No tenemos aguante
    if ( GetStamina() <= 5.0f )
        return false;
#endif

    return true;
}

//================================================================================
// Devuelve si el jugador puede caminar
//================================================================================
bool CPlayer::CanSneak()
{
    // Agachados ya estamos a una velocidad muy baja
    if ( IsCrouching() )
        return false;

    return true;
}

//================================================================================
// Actualiza la detección al correr
//================================================================================
void CPlayer::UpdateMovementType()
{
    // Nos permitimos correr
    if ( CanSprint() ) {
        // Solo si presionamos el boton
        if ( IsButtonPressing( IN_SPEED ) ) {
            StartSprint();

#ifndef CLIENT_DLL
            AddAttributeModifier( "running" );
#endif
        }
        else if ( IsSprinting() ) {
            StopSprint();
        }
    }
    else if ( IsSprinting() ) {
        StopSprint();
    }

    // Nos permitimos caminar 
    if ( CanSneak() ) {
        // Solo si presionamos el boton
        if ( IsButtonPressing( IN_WALK ) ) {
            StartSneaking();
        }
        else {
            StopSneaking();
        }
    }
    else if ( IsSneaking() ) {
        StopSneaking();
    }
}

//================================================================================
//================================================================================
const Vector CPlayer::GetPlayerMins() const
{
    if ( sv_player_tests_hull_mins_x.GetFloat() > 0 )
        return Vector( sv_player_tests_hull_mins_x.GetFloat(), sv_player_tests_hull_mins_y.GetFloat(), VEC_DEJECTED_HULL_MIN.z );

    if ( IsObserver() )
        return VEC_OBS_HULL_MIN;

    if ( GetPlayerStatus() == PLAYER_STATUS_DEJECTED )
        return VEC_DEJECTED_HULL_MIN;

    if ( IsCrouching() )
        return VEC_DUCK_HULL_MIN;

    return VEC_HULL_MIN;
}

//================================================================================
//================================================================================
const Vector CPlayer::GetPlayerMaxs() const
{
    if ( sv_player_tests_hull_maxs_x.GetFloat() > 0 )
        return Vector( sv_player_tests_hull_maxs_x.GetFloat(), sv_player_tests_hull_maxs_y.GetFloat(), VEC_DEJECTED_HULL_MAX.z );

    if ( IsObserver() )
        return VEC_OBS_HULL_MAX;

    if ( GetPlayerStatus() == PLAYER_STATUS_DEJECTED )
        return VEC_DEJECTED_HULL_MAX;

    if ( IsCrouching() )
        return VEC_DUCK_HULL_MAX;

    return VEC_HULL_MAX;
}


//================================================================================
//================================================================================
void CPlayer::UpdateCollisionBounds()
{
    SetCollisionBounds( GetPlayerMins(), GetPlayerMaxs() );
}

//================================================================================
//================================================================================
bool CPlayer::ShouldBleed( const CTakeDamageInfo & info, int hitgroup )
{
    int blood = BloodColor();

    if ( blood == DONT_BLEED )
        return false;

    if ( hitgroup == HITGROUP_GEAR )
        return false;

#ifndef CLIENT_DLL
    if ( !CanTakeDamage( info ) )
        return false;

    float handledByShield = TheGameRules->FPlayerGetDamageHandledByShield( this, info );

    if ( handledByShield >= info.GetDamage() )
        return false;
#endif

    return true;
}

//================================================================================
//================================================================================
void CPlayer::TraceAttack( const CTakeDamageInfo & info, const Vector & vecDir, trace_t * ptr )
{
    if ( m_takedamage == DAMAGE_NO )
        return;

    AddMultiDamage( info, this );

    Vector vecOrigin = ptr->endpos - vecDir * 4;

#ifndef CLIENT_DLL
    int blood = BloodColor();
    SetLastHitGroup( ptr->hitgroup );

    // @TODO: Shared code
    if ( ShouldBleed( info, ptr->hitgroup ) ) {
        SpawnBlood( vecOrigin, vecDir, blood, info.GetDamage() );
        TraceBleed( info.GetDamage(), vecDir, ptr, info.GetDamageType() );
    }
    else if ( ptr->hitgroup == HITGROUP_GEAR ) {
        g_pEffects->Sparks( vecOrigin, 1, 1, &vecDir );
        UTIL_Smoke( vecOrigin, random->RandomFloat( 5, 10 ), 15.0f );
    }
    else {
        if ( random->RandomFloat( 0, 5 ) == 5 ) {
            g_pEffects->Sparks( vecOrigin, 1, 1, &vecDir );
            UTIL_Smoke( vecOrigin, random->RandomFloat( 5, 10 ), 15.0f );
        }
    }
#endif
}

//====================================================================
// Disparo de una bala desde el arma del Jugador
//====================================================================
/*
void CPlayer::FireBullet(
Vector vecSrc,                // shooting postion
const QAngle &shootAngles,  //shooting angle
float vecSpread,                // spread vector
int iDamage,                // base damage
int iBulletType,            // ammo type
CBaseEntity *pevAttacker,     // shooter
bool bDoEffects,            // create impact effect ?
float x,                        // spread x factor
float y                        // spread y factor
)
{
// Daño y distancia viajada actual de la bala
float fCurrentDamage    = iDamage;
float flCurrentDistance = 0.0;

// Máxima distancia a la que puede viajar
float flDistanceLeft = 7000;

// Tipo de daño
int iDamageType = DMG_BULLET | DMG_NEVERGIB;

// Establecemos las direcciones hacia donde puede ir la bala
Vector vecDirShooting, vecRight, vecUp;
AngleVectors( shootAngles, &vecDirShooting, &vecRight, &vecUp );

// No hay atacante, por lógica, somos nosotros
if ( !pevAttacker )
pevAttacker = this;

// Agregamos la dispersión de balas
Vector vecDir = vecDirShooting +
x * vecSpread * vecRight +
y * vecSpread * vecUp;

VectorNormalize( vecDir );

trace_t tr;
int i = 0;


while ( flDistanceLeft > 0.0f )
{
++i;

// Penetramos algo, empezamos donde termino
if ( i > 1 )
{
vecSrc = tr.endpos + vecDir * 20;

// Trace temporal para crear el hoyo de salida
trace_t tmptr;
UTIL_TraceLine( vecSrc, tr.endpos, MASK_SOLID|CONTENTS_DEBRIS|CONTENTS_HITBOX, this, COLLISION_GROUP_NONE, &tmptr );
UTIL_ImpactTrace( &tmptr, iDamageType );
}

// Ubicación donde debe terminar la bala
Vector vecEnd = vecSrc + vecDir * flDistanceLeft;


// Trazamos una línea para ver donde terminara la bala
UTIL_TraceLine( vecSrc, vecEnd, MASK_SOLID|CONTENTS_DEBRIS|CONTENTS_HITBOX, this, COLLISION_GROUP_NONE, &tr );

// No le hemos dado a nada
if ( tr.fraction == 1.0f || tr.startsolid )
return;

// Distancia recorrida de la bala
flCurrentDistance += tr.fraction * flDistanceLeft;

// Disminuimos la distancia restante
flDistanceLeft -= flCurrentDistance;

// Entre más distancia recorra, menor será el daño
fCurrentDamage *= pow( 0.85f, (flCurrentDistance / 500));

// Información de depuración
if ( sv_showimpacts.GetBool() )
{
#ifdef CLIENT_DLL
// Cuadro donde termina la bala
debugoverlay->AddBoxOverlay( tr.endpos, Vector(-2,-2,-2), Vector(2,2,2), QAngle( 0, 0, 0), 255,0,0,127, 6 );

// Cuadro donde empieza la bala
debugoverlay->AddBoxOverlay( vecSrc, Vector(-2,-2,-2), Vector(2,2,2), QAngle( 0, 0, 0), 216,224,132,255, 6 );

// Línea de trayección y mensaje con información
DebugDrawLine( tr.startpos, tr.endpos, 255, 0, 0, true, 6 );
DevMsg("[FireBullet][%s] %i pasada: Distancia recorrida: %f - Distancia restante: %f - Dano de bala: %f \n", GetPlayerName(), i, flCurrentDistance, flDistanceLeft, fCurrentDamage);

if ( tr.m_pEnt && tr.m_pEnt->IsPlayer() )
{
C_BasePlayer *player = ToBasePlayer( tr.m_pEnt );
player->DrawClientHitboxes( 4, true );
}
#else
// draw blue server impact markers
NDebugOverlay::Box( tr.endpos, Vector(-2,-2,-2), Vector(2,2,2), 0,0,255,127, 6 );

if ( tr.m_pEnt && tr.m_pEnt->IsPlayer() )
{
CBasePlayer *player = ToBasePlayer( tr.m_pEnt );
player->DrawServerHitboxes( 4, true );
}
#endif
}



// Efectos especiales
if ( bDoEffects )
{
// La bala termino en algo liquido
if ( enginetrace->GetPointContents(tr.endpos) & (CONTENTS_WATER|CONTENTS_SLIME) )
{
trace_t waterTrace;
UTIL_TraceLine( vecSrc, tr.endpos, (MASK_SHOT|CONTENTS_WATER|CONTENTS_SLIME), this, COLLISION_GROUP_NONE, &waterTrace );

// Efecto de burbujas bajo el agua
if ( waterTrace.allsolid != 1 )
{
CEffectData    data;
data.m_vOrigin = waterTrace.endpos;
data.m_vNormal = waterTrace.plane.normal;
data.m_flScale = random->RandomFloat( 8, 12 );

if ( waterTrace.contents & CONTENTS_SLIME )
data.m_fFlags |= FX_WATER_IN_SLIME;

DispatchEffect( "gunshotsplash", data );
}

// Sin penetración
break;
}
else
{
// No debemos dibujar hoyos de bala en estas texturas
if ( !( tr.surface.flags & (SURF_SKY|SURF_NODRAW|SURF_HINT|SURF_SKIP) ) )
{
UTIL_ImpactTrace( &tr, iDamageType );
}
}
}

#ifdef GAME_DLL
ClearMultiDamage();

// Daño de la bala
CTakeDamageInfo info( pevAttacker, pevAttacker, fCurrentDamage, iDamageType );
CalculateBulletDamageForce( &info, iBulletType, vecDir, tr.endpos );

tr.m_pEnt->DispatchTraceAttack( info, vecDir, &tr );
TraceAttackToTriggers( info, tr.startpos, tr.endpos, vecDir );

ApplyMultiDamage();
#endif

surfacedata_t *data = physprops->GetSurfaceData( tr.surface.surfaceProps );

// Según el tipo de material que penetremos disminuiremos la distancia faltante,
// la bala viajara menos y hará menor daño.

if ( data->game.material == CHAR_TEX_WOOD )
flDistanceLeft -= 500.0f;

else if ( data->game.material == CHAR_TEX_GRATE )
flDistanceLeft -= 100.0f;

else if ( data->game.material == CHAR_TEX_TILE )
flDistanceLeft -= 200.0f;

else if ( data->game.material == CHAR_TEX_COMPUTER )
flDistanceLeft -= 50.0f;

else if ( data->game.material == CHAR_TEX_GLASS )
flDistanceLeft -= 200.0f;

else if ( data->game.material == CHAR_TEX_PLASTIC )
flDistanceLeft -= 200.0f;

else if ( data->game.material == CHAR_TEX_VENT || data->game.material == CHAR_TEX_METAL )
flDistanceLeft -= 700.0f;

else if ( data->game.material == CHAR_TEX_BLOODYFLESH || data->game.material == CHAR_TEX_FLESH )
flDistanceLeft -= 600.0f;

else
break;
}
}
*/