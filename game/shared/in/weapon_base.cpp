//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "weapon_base.h"

#include "in_buttons.h"
#include "in_shareddefs.h"

#include "in_gamerules.h"

#include "datacache/imdlcache.h"
#include "tier0/vprof.h"

#ifndef CLIENT_DLL
    #include "in_player.h"
    #include "player_lagcompensation.h"
    #include "soundent.h"
    #include "bots\squad.h"
    #include "bots\bot.h"
#else
    #include "fx_impact.h"
    #include "c_in_player.h"
    #include "sdk_input.h"
    #include "cl_animevent.h"
    #include "c_te_legacytempents.h"
    #include "viewrender.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
// Comandos
//================================================================================

DECLARE_NOTIFY_COMMAND( sv_weapon_shot_is_bullet, "0", "" )

DECLARE_REPLICATED_CHEAT_COMMAND( sv_weapon_silence, "0", "" ) // TODO: Remove
DECLARE_REPLICATED_CHEAT_COMMAND( sv_weapon_infinite_ammo, "0", "Indica si las armas no gastan munición al disparar" )
DECLARE_REPLICATED_COMMAND( sv_weapon_serverside_hitmarker, "0", "" )

DECLARE_NOTIFY_COMMAND( sv_weapon_auto_reload, "0", "Indica si se debe recargar el arma al acabarse la munición del clip" )
DECLARE_NOTIFY_COMMAND( sv_weapon_auto_fire, "0", "Indica si se puede dejar presionado el botón de ataque para disparar balas continuamente" )
DECLARE_NOTIFY_COMMAND( sv_weapon_equip_touch, "0", "Indica si las armas se pueden recoger al tocarlas" )

#define HIDEWEAPON_THINK_CONTEXT			"BaseCombatWeapon_HideThink"
#define SHOWWEAPON_THINK_CONTEXT			"BaseCombatWeapon_ShowThink"

//================================================================================
// Información y Red
//================================================================================

IMPLEMENT_NETWORKCLASS_ALIASED( BaseWeapon, DT_BaseWeapon )

BEGIN_NETWORK_TABLE( CBaseWeapon, DT_BaseWeapon )
#ifndef CLIENT_DLL
    SendPropExclude( "DT_BaseAnimating", "m_nNewSequenceParity" ),
    SendPropExclude( "DT_BaseAnimating", "m_nResetEventsParity" ),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CBaseWeapon )
END_PREDICTION_DATA()
#else
BEGIN_DATADESC( CBaseWeapon )
    DEFINE_THINKFUNC( ShowThink )
END_DATADESC()
#endif

#ifndef CLIENT_DLL
int CBaseWeapon::ObjectCaps()
{
	int caps = BaseClass::ObjectCaps();

	if ( !IsFollowingEntity() )
		caps |= FCAP_IMPULSE_USE;

	return caps;
}
#else
//================================================================================
//================================================================================
bool CBaseWeapon::ShouldPredict()
{
    if ( GetOwner() == C_BasePlayer::GetLocalPlayer() )
        return true;

    return BaseClass::ShouldPredict();
}

//================================================================================
//================================================================================
void CBaseWeapon::OnDataChanged( DataUpdateType_t type ) 
{
    BaseClass::OnDataChanged( type );

    if ( GetPredictable() && !ShouldPredict() ) {
        ShutdownPredictable();
    }
}

//================================================================================
//================================================================================
bool CBaseWeapon::ShouldDraw()
{
    if ( GetPlayerOwner() ) {
        if ( GetPlayerOwner()->ShouldDraw() )
            return true;
    }

    return BaseClass::ShouldDraw();
}

//================================================================================
//================================================================================
int CBaseWeapon::DrawModel( int flags, const RenderableInstance_t & instance )
{
    if ( IsEffectActive( EF_NODRAW ) )
        return 0;

    // El dueño es un jugador
    if ( GetPlayerOwner() && CurrentViewID() == VIEW_MAIN ) {
        if ( !GetPlayerOwner()->ShouldDraw() )
            return 0;

        if ( GetSplitScreenViewPlayer() == GetPlayerOwner() ) {
            if ( GetPlayerOwner()->IsFirstPerson() ) {
                if ( !GetPlayerOwner()->GetViewEntity() )
                    return 0;
            }
        }
    }

    return BaseClass::DrawModel( flags, instance );
}

//================================================================================
//================================================================================
void CBaseWeapon::AddViewmodelBob( CBaseViewModel *viewmodel, Vector &origin, QAngle &angles )
{
}

//================================================================================
//================================================================================
float CBaseWeapon::CalcViewmodelBob()
{
    return 0.0f;
}

//================================================================================
// Devuelve la posición y dirección de donde provienen las bas
// [Clientside] Solo para efectos visuales
//================================================================================
bool CBaseWeapon::GetShootPosition( Vector &vOrigin, QAngle &vAngles )
{
    C_BaseCombatCharacter *pEnt = ToBaseCombatCharacter( GetOwner() );
    C_Player *pPlayer = GetPlayerOwner();

    // Tenemos dueño, obtenemos los angulos para la dirección
    if ( pEnt ) {
        if ( pPlayer && pPlayer->IsLocalPlayer() )
            vAngles = pPlayer->EyeAngles();
        else
            vAngles = pEnt->GetRenderAngles();
    }
    else {
        vAngles.Init();
    }

    CBaseAnimating *pOwner = GetIdealAnimating();
    int iAttachment = pOwner->LookupAttachment( "muzzle" );

    // Obtenemos la posición de comienzo
    if ( pOwner->GetAttachment(iAttachment, vOrigin) )
        return true;

    vOrigin = ( pPlayer ) ? pPlayer->Weapon_ShootPosition() : GetRenderOrigin();
    return false;
}

//================================================================================
// El modelo ha lanzado un evento
//================================================================================
bool CBaseWeapon::OnFireEvent( CBaseViewModel *pViewModel, const Vector& origin, const QAngle& angles, int event, const char *options )
{
    switch ( event ) {
        // Reproducir sonido
        case AE_CL_PLAYSOUND:
        case CL_EVENT_SOUND:
        {
            CBaseEntity *pOwner = GetOwner();

            if ( !pOwner )
                pOwner = this;

            CBroadcastRecipientFilter filter;
            EmitSound( filter, pOwner->entindex(), options, &pOwner->GetAbsOrigin() );

            return true;
        }
        break;

        // MuzzleFlash
        case AE_MUZZLEFLASH:
        {
            bool bIsFirstPerson = false;

            // Jugador en primera persona
            if ( GetPlayerOwner() && GetPlayerOwner()->IsFirstPerson() ) {
                bIsFirstPerson = true;
            }

            DispatchMuzzleEffect( options, bIsFirstPerson );
            return true;
        }
        break;
    }

    DevMsg("[CBaseWeapon::OnFireEvent] %i \n", event);
    return BaseClass::OnFireEvent( pViewModel, origin, angles, event, options );
}
#endif

//================================================================================
// Constructor
//================================================================================
CBaseWeapon::CBaseWeapon()
{
    SetPredictionEligible( true );
    AddSolidFlags( FSOLID_TRIGGER );

    RegisterThinkContext( SHOWWEAPON_THINK_CONTEXT );
    SetContextThink( &CBaseWeapon::ShowThink, 0, SHOWWEAPON_THINK_CONTEXT );

	RegisterThinkContext( HIDEWEAPON_THINK_CONTEXT );
    SetContextThink( &CBaseWeapon::HideThink, 0, HIDEWEAPON_THINK_CONTEXT );
}

//================================================================================
//================================================================================
void CBaseWeapon::Spawn()
{
    BaseClass::Spawn();

	m_iAddonAttachment = -1;
    AddFlag( FL_OBJECT ); 
}

//================================================================================
// Alguién se esta equipando el arma
//================================================================================
void CBaseWeapon::Equip( CBaseCombatCharacter *pOwner )
{
    BaseClass::Equip( pOwner );

    // La configuración del arma nos indica que esta se puede colocar en un 
    // attachment del dueño cuando se encuentre inactiva.
    if ( GetWeaponInfo().m_nAddonAttachment && strlen( GetWeaponInfo().m_nAddonAttachment ) > 2 ) {
        m_iAddonAttachment = pOwner->LookupAttachment( GetWeaponInfo().m_nAddonAttachment );

        if ( m_iAddonAttachment > 0 ) {
            SetParent( pOwner, m_iAddonAttachment );

            // Iván: Esto es MUY necesario
            SetLocalOrigin( vec3_origin );
            SetLocalAngles( vec3_angle );

            // Dejamos de hacer merge al modelo del jugador
            RemoveEffects( EF_BONEMERGE );
        }
    }
}

//================================================================================
//================================================================================
void CBaseWeapon::Drop( const Vector &vecVelocity ) 
{
	BaseClass::Drop( vecVelocity );
	m_iAddonAttachment = -1;
}

//================================================================================
// Devuelve el [CBaseAnimating] ideal para los efectos
//================================================================================
CBaseAnimating *CBaseWeapon::GetIdealAnimating()
{
    CPlayer *pPlayer = GetPlayerOwner();

    if ( pPlayer ) {
#ifdef CLIENT_DLL
        // Jugador local en primera persona
        if ( pPlayer->IsLocalPlayer() && pPlayer->ShouldUseViewModel() )
            return pPlayer->GetViewModel();
#endif
    }

    if ( GetOwner() )
        return GetOwner();

    return this;
}

//================================================================================
// Devuelve la información del arma
//================================================================================
const CWeaponInfo &CBaseWeapon::GetWeaponInfo() const
{
    const FileWeaponInfo_t *pWeaponInfo = &GetWpnData();
    const CWeaponInfo *pInfo = static_cast< const CWeaponInfo* >( pWeaponInfo );
    return *pInfo;
}

//================================================================================
// Devuelve nuestro Jugador dueño
//================================================================================
CPlayer *CBaseWeapon::GetPlayerOwner() const
{
    return ToInPlayer( GetOwner() );
}

//================================================================================
// Devuelve el ID del attachment donde debe colocarse el arma cuando esta
// inactivo.
//================================================================================
int CBaseWeapon::GetAddonAttachment() 
{
	return m_iAddonAttachment;
}

//================================================================================
// Devuelve el poder de retroceso
//================================================================================
float CBaseWeapon::GetVerticalPunch() 
{
	return GetWeaponInfo().m_flVerticalPunch;
}

//================================================================================
// Devuelve el nivel de distribución de balas
//================================================================================
float CBaseWeapon::GetSpreadPerShot()
{
    float spread = GetWeaponInfo().m_flSpreadPerShot;

    // Nos esta usando un Jugador
    // Cambiamos la distribución dependiendo de ciertas acciones
    if ( GetPlayerOwner() ) {
        CPlayer *pPlayer = GetPlayerOwner();

        // Agachado
        if ( pPlayer->IsCrouching() ) {
            spread *= GetWeaponInfo().m_flCrouchSpread;
        }

#ifndef CLIENT_DLL
        // Disperción aumentada para bots
        if ( pPlayer->IsBot() && TheGameRules->GetSkillLevel() < SKILL_HARDEST ) {
            if ( pPlayer->Classify() != CLASS_PLAYER_ALLY && pPlayer->Classify() != CLASS_PLAYER_ALLY_VITAL ) {
                spread *= GetWeaponInfo().m_flBotSpread;
            }
        }
#endif

        // Saltando
        if ( !pPlayer->IsOnGround() ) {
            spread *= GetWeaponInfo().m_flJumpSpread;
        }

        // Incapacitado
        if ( pPlayer->IsDejected() ) {
            spread *= GetWeaponInfo().m_flDejectedSpread;
        }

        // Moviendose
        // @TODO: Client-side
#ifndef CLIENT_DLL
        if ( pPlayer->IsMoving() ) {
            spread *= GetWeaponInfo().m_flMovingSpread;
        }
#endif
    }

    spread = clamp( spread, VECTOR_CONE_PRECALCULATED.x, VECTOR_CONE_20DEGREES.x );
    return spread;
}

//================================================================================
// Devuelve la velocidad de disparo
//================================================================================
float CBaseWeapon::GetFireRate()
{
    float flShotsPerSecond = GetWeaponInfo().m_flFireRate;
    return ( 1 / flShotsPerSecond );
}

//================================================================================
// Devuelve el FOV del arma
//================================================================================
float CBaseWeapon::GetWeaponFOV()
{
    return GetWeaponInfo().m_flWeaponFOV;
}

//================================================================================
// Devuelve el tipo de sonido que debemos reproducir al disparar
//================================================================================
WeaponSound_t CBaseWeapon::GetWeaponSound( const FireBulletsInfo_t &info ) 
{
#ifdef CLIENT_DLL
    // InSource usa SINGLE para el sonido de disparo del jugador local,
    // es decir, aquel sonido que se escucha justo enfrente del jugador
    // Los disparos del jugador local solo se procesan en cliente.
    return SINGLE;
#else
    // InSource usa SINGLE_NPC para el sonido de disparo de otro jugador,
    // este sonido debe tener 2 canales (Izquierda: Disparo cercano, Derecha: Disparo lejano)
    // https://developer.valvesoftware.com/wiki/Soundscript#Distance_variance_in_Source
    // Los disparos de otros jugadores solo se procesan en servidor.
    return SINGLE_NPC;
#endif
}

//================================================================================
//================================================================================
bool CBaseWeapon::CanPrimaryAttack()
{
    CPlayer *pPlayer = GetPlayerOwner();

    if ( !pPlayer )
        return false;

    // Aún no
    if ( m_flNextPrimaryAttack >= gpGlobals->curtime )
        return false;

    if ( pPlayer->GetNextAttack() >= gpGlobals->curtime )
        return false;

    // Debes hacer cada disparo manual.
    if ( !sv_weapon_auto_fire.GetBool() && !m_bPrimaryAttackReleased )
        return false;

    // Es un arma de fuego
    if ( !IsMeleeWeapon() ) {
        // Munición actual
        int ammo = Clip1();

        // No ocupa clips para la munición. ¿granadas?
        if ( !UsesClipsForAmmo1() )
            ammo = pPlayer->GetAmmoCount( GetPrimaryAmmoType() );

        // Ya no tenemos munición en el arma o estamos bajo el agua
        // Reproducimos el sonido de "vacio"
        if ( ammo <= 0 || pPlayer->GetWaterLevel() == 3 && !m_bFiresUnderwater ) {
            // Recarga automatica 
            //HandleFireOnEmpty();
            //Deploy();

            // Vacio!
            WeaponSound( EMPTY );
            SendWeaponAnim( ACT_VM_DRYFIRE );
            SetNextPrimaryAttack( 0.5 );

            // Empezamos a recargar
            if ( ammo <= 0 && sv_weapon_auto_reload.GetBool() )
                Reload();

            return false;
        }
    }

    return true;
}

//================================================================================
//================================================================================
bool CBaseWeapon::CanSecondaryAttack()
{
    CPlayer *pPlayer = GetPlayerOwner();

    if ( !pPlayer )
        return false;

    // Aún no
    if ( m_flNextSecondaryAttack >= gpGlobals->curtime )
        return false;

    // Munición actual
    int ammo = m_iClip2;

    // No ocupa clips para la munición. ¿granadas?
    if ( !UsesClipsForAmmo1() )
        ammo = pPlayer->GetAmmoCount( m_iSecondaryAmmoType );

    // Ya no tenemos munición en el arma o estamos bajo el agua
    // Reproducimos el sonido de "vacio"
    if ( ammo <= 0 || pPlayer->GetWaterLevel() == 3 && !m_bFiresUnderwater ) {
        // Recarga automatica 
        //HandleFireOnEmpty();

        // Vacio!
        WeaponSound( EMPTY );
        SendWeaponAnim( ACT_VM_DRYFIRE );
        SetNextSecondaryAttack( 0.5 );

        return false;
    }

    return true;
}

//================================================================================
// Se ejecuta en cada frame, procesa el input del jugador
// NOTA: En clientside solo funciona para el jugador local.
//================================================================================
void CBaseWeapon::ItemPostFrame()
{
    CPlayer *pPlayer = GetPlayerOwner();

    if ( !pPlayer )
        return;

    // Duración que mantenemos el clic presionado.
    m_fFireDuration = (pPlayer->IsButtonPressing( IN_ATTACK )) ? (m_fFireDuration + gpGlobals->frametime) : 0.0f;

    // Checar si hemos terminado de recargar.
    if ( UsesClipsForAmmo1() )
        CheckReload();

    // Disparo secundario
    if ( pPlayer->IsButtonPressed( IN_ATTACK2 ) && CanSecondaryAttack() )
        SecondaryAttack();

    // Disparo primario
    if ( pPlayer->IsButtonPressing( IN_ATTACK ) ) {
        if ( CanPrimaryAttack() ) {
            if ( pPlayer->IsButtonPressed( IN_ATTACK ) && pPlayer->IsButtonReleased( IN_ATTACK2 ) )
                SetNextPrimaryAttack();

            PrimaryAttack();

            // Estamos presionando el botón de ataque.
            m_bPrimaryAttackReleased = false;
        }
        else {
            m_fFireDuration = 0.0f;
        }
    }
    else {
        m_bPrimaryAttackReleased = true;
    }

    if ( pPlayer->IsButtonPressing( IN_RELOAD ) && UsesClipsForAmmo1() && !IsReloading() ) {
        Reload();
        m_fFireDuration = 0.0f;
    }

    // No estamos atacando ni recargando.
    if ( !(pPlayer->IsButtonPressing( IN_RELOAD )) && !pPlayer->IsButtonPressing( IN_ATTACK ) && !pPlayer->IsButtonPressing( IN_ATTACK2 ) ) {
        // No estamos recargando.
        if ( !IsReloading() ) {
            // Animación de no estar haciendo nada.
            WeaponIdle();
        }
    }
}

//================================================================================
// Ataque primario
// NOTA: En clientside solo funciona para el jugador local.
//================================================================================
void CBaseWeapon::PrimaryAttack()
{
    // Usamos clips para la munición pero nos hemos quedado sin balas
    if ( UsesClipsForAmmo1() && !m_iClip1 ) {
        if ( sv_weapon_auto_reload.GetBool() )
            Reload();

        return;
    }

    CPlayer *pPlayer = GetPlayerOwner();

    if ( !pPlayer )
        return;

    float spread = GetSpreadPerShot();
    int spentBullets = 1;

    // Información de las balas
    FireBulletsInfo_t info;
    info.m_iShots = GetWeaponInfo().m_iBullets;
    info.m_flDistance = GetWeaponInfo().m_flMaxDistance;
    info.m_iTracerFreq = 1;
    info.m_flDamage = info.m_flPlayerDamage = GetWeaponInfo().m_iDamage;
    info.m_pAttacker = this;
    info.m_nFlags = FIRE_BULLETS_FIRST_SHOT_ACCURATE;
    info.m_iAmmoType = m_iPrimaryAmmoType;
    info.m_vecSrc = pPlayer->Weapon_ShootPosition();
    info.m_vecDirShooting = pPlayer->Weapon_ShootDirection();
    info.m_vecSpread = Vector( spread, spread, spread );
    info.m_bPrimaryAttack = true;

    VECTOR_CONE_5DEGREES;

    // Quitamos la munición usada de nuestra reserva
    if ( !sv_weapon_infinite_ammo.GetBool() ) {
        if ( UsesClipsForAmmo1() ) {
            // Cada bala disparada debe ser una gastada
            // Esto hace que las escopetas que disparan 4 balas, gasten en verdad 4 balas del clip
            if ( sv_weapon_shot_is_bullet.GetBool() ) {
                info.m_iShots = MIN( info.m_iShots, m_iClip1 );
                spentBullets = info.m_iShots;
            }
            else {
                spentBullets = MIN( spentBullets, m_iClip1 );
            }

            m_iClip1 -= spentBullets;
        }
        else {
            info.m_iShots = MIN( info.m_iShots, pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) );
            pPlayer->RemoveAmmo( spentBullets, m_iPrimaryAmmoType );
        }
    }

    // Animación de disparo en viewmodel
    SendWeaponAnim( GetPrimaryAttackActivity() );
    pPlayer->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY, 0, true );

    WeaponSound( GetWeaponSound( info ) );
    pPlayer->FireBullets( info );

    AddViewKick();
    SetWeaponIdleTime( gpGlobals->curtime + SequenceDuration() );
    SetNextPrimaryAttack( GetFireRate() );
    SetNextSecondaryAttack( SequenceDuration() );
}

//================================================================================
//================================================================================
void CBaseWeapon::WeaponIdle()
{
    if ( HasWeaponIdleTimeElapsed() ) {
        SendWeaponAnim( GetIdleActivity() );
    }
}

//================================================================================
// Recarga el arma
//================================================================================
bool CBaseWeapon::Reload()
{
    return DefaultReload( GetMaxClip1(), GetMaxClip2(), GetReloadActivity() );
}

//================================================================================
// Aplica el retroceso del arma a la vista del jugador
//================================================================================
void CBaseWeapon::AddViewKick()
{
    CPlayer *pPlayer = GetPlayerOwner();
        
    if ( !pPlayer )
        return;

	// @TODO: Hacer que los Bots tomen en consideración esto al apuntar
	if ( pPlayer->IsBot() )
		return;

    float flPunch = GetVerticalPunch();
    QAngle anglePunch;

    anglePunch.x = -flPunch;
    anglePunch.y = RandomFloat(0.1f, 0.5f);
    anglePunch.z = RandomFloat(0.1f, 0.5f);

    pPlayer->ViewPunch( anglePunch );
}

//================================================================================
// Lo que sucede cuando una entidad toca el arma
//================================================================================
void CBaseWeapon::DefaultTouch( CBaseEntity *pOther )
{
    // No podemos obtener armas al tocarlas
    if ( !sv_weapon_equip_touch.GetBool() )
        return;

    BaseClass::DefaultTouch( pOther );
}

//================================================================================
// Esta es nuestra nueva arma activa, la mostramos
//================================================================================
bool CBaseWeapon::DefaultDeploy( char *szViewModel, char *szWeaponModel, int iActivity, char *szAnimExt )
{
    MDLCACHE_CRITICAL_SECTION();

    if ( !HasAnyAmmo() && AllowsAutoSwitchFrom() )
        return false;

    CPlayer *pOwner = GetPlayerOwner();

	if ( !pOwner )
		return false;

	// Dead men deploy no weapons
    if ( !pOwner->IsAlive() )
        return false;
    
    // Tercera persona: Animación para sacar el arma
    pOwner->SetAnimationExtension( szAnimExt );
    pOwner->DoAnimationEvent( PLAYERANIMEVENT_DEPLOY, 0, true );

#ifndef CLIENT_DLL
    pOwner->SetDistLook( GetWeaponInfo().m_flMaxDistance );
#endif

    // Primera persona: Animación para sacar el arma
    SetViewModel();
    SendWeaponAnim( iActivity );

	float flSequenceDuration = 0.0f;

    if ( GetActivity() == iActivity ) {
        flSequenceDuration = SequenceDuration();
    }

    // Can't shoot again until we've finished deploying
    SetNextAttack( flSequenceDuration );

	// Mostramos
    ShowThink();

    /*
    TODO: Owner Deploy Animation Handle
    if ( GetActivity() == iActivity ) {
    #ifdef APOCALYPSE
    flSequenceDuration = 0.3f;
    #else
    flSequenceDuration = (SequenceDuration() / 2);
    #endif
    }

    if ( flSequenceDuration > 0 )
    {
    // Si no tenemos un attachment, entonces ocultamos el modelo
    // en el mundo, la función ShowThink se encargará de mostrarlo
    if ( GetAddonAttachment() <= 0 )
    AddEffects( EF_NODRAW );
    }

    SetNextThink( gpGlobals->curtime + flSequenceDuration, SHOWWEAPON_THINK_CONTEXT );
    */

    return true;
}

//================================================================================
// Ocultamos esta arma para cambiarla por [pSwitchingTo]
//================================================================================
bool CBaseWeapon::Holster( CBaseCombatWeapon *pSwitchingTo ) 
{
    MDLCACHE_CRITICAL_SECTION();

	CPlayer *pOwner = GetPlayerOwner();

	if ( !pOwner )
		return false;

	// Dead men deploy no weapons
    if ( !pOwner->IsAlive() )
        return false;

    // Cancelamos la recarga y el pensamiento
    m_bInReload = false;
    SetThink( NULL );

    // Primera persona: Animación para ocultar el arma
	SendWeaponAnim( GetHolsterActivity() );

	float flSequenceDuration = 0.0f;

    if ( GetActivity() == GetHolsterActivity() ) {
        flSequenceDuration = SequenceDuration();
    }

	// Can't shoot again until we've finished deploying
    SetNextAttack( flSequenceDuration );

    // Ocultamos
    HideThink();

    // TODO: Implement Holster Wait Animation
    /*if ( flSequenceDuration > 0 )
    {
    SetWeaponVisible( true );
    //RemoveEffects( EF_NODRAW );
    }*/

	// Ocultamos el arma cuando la animación haya terminado
    /*if ( flSequenceDuration == 0.0f ) {
        HideThink();
    }
    else {
        SetNextThink( gpGlobals->curtime + flSequenceDuration, HIDEWEAPON_THINK_CONTEXT );
    }*/

	return true;
}

//================================================================================
//================================================================================
void CBaseWeapon::SetNextAttack( float time )
{
    SetNextPrimaryAttack( time );
    SetNextSecondaryAttack( time );

    if ( GetPlayerOwner() ) {
        GetPlayerOwner()->SetNextAttack( gpGlobals->curtime + time );
    }
}

//================================================================================
//================================================================================
void CBaseWeapon::SetNextPrimaryAttack( float time )
{
    m_flNextPrimaryAttack = gpGlobals->curtime + time;
}

//================================================================================
//================================================================================
void CBaseWeapon::SetNextSecondaryAttack( float time )
{
    m_flNextSecondaryAttack = gpGlobals->curtime + time;
}

//================================================================================
//================================================================================
void CBaseWeapon::SetNextIdleTime( float time )
{
    SetWeaponIdleTime( gpGlobals->curtime + time );
}

//================================================================================
// Oculta el arma
//================================================================================
void CBaseWeapon::HideThink()
{
    // Ocultamos el arma
    if ( GetOwner() ) {
        // Tenemos un lugar donde colocar el arma
        // Colocamos el arma en el attachment
        if ( GetAddonAttachment() > 0 ) {
            // Agregamos el attachment
            SetParent( GetOwner(), GetAddonAttachment() );

            // Iván: Esto es MUY necesario
            SetLocalOrigin( vec3_origin );
            SetLocalAngles( vec3_angle );

            // Dejamos de hacer merge, de esta forma el
            // attachment se verá correctamente.
            RemoveEffects( EF_BONEMERGE );
        }
        else {
            // Ocultamos el arma y el viewmodel
            SetWeaponVisible( false );
        }
    }
}

//================================================================================
// Muestra el arma
//================================================================================
void CBaseWeapon::ShowThink()
{
    // Mostramos el arma (Tercera persona)
    if ( GetOwner() && GetOwner()->GetActiveWeapon() == this ) {
        // Teniamos un lugar donde colocar el arma
        // Volvemos a colocar el arma en las manos del jugador
        if ( GetAddonAttachment() > 0 ) {
            // Quitamos cualquier attachment
            SetParent( GetOwner(), NULL );
            SetLocalOrigin( vec3_origin );
            SetLocalAngles( vec3_angle );

            // Nos aseguramos que haga merge con el jugador
            AddEffects( EF_BONEMERGE );
        }

        // Mostramos el arma y el viewmodel
        SetWeaponVisible( true );
    }
}

//================================================================================
// Emite el sonido al realizar el ataque primario
//================================================================================
void CBaseWeapon::WeaponSound( WeaponSound_t sound, float soundtime )
{
    // Nota: Esta función se llama en cliente
    // solo para los disparos hechos por el jugador local

    if ( sv_weapon_silence.GetBool() )
        return;

    const char *shootsound = GetShootSound( sound );

    if ( !shootsound || !shootsound[0] )
        return;

    CBaseEntity *pOwner = GetOwner();

    if ( !pOwner )
        pOwner = this;

    // SPECIAL1 se utilizará para los disparos lejanos
    if ( sound == SPECIAL1 ) {
        // Solo el servidor podrá transmitir este sonido
        // se encargará de detectar quienes estan lo suficientemente lejos
#ifndef CLIENT_DLL
        CPASOutAttenuationFilter filter( pOwner, shootsound );
        filter.UsePredictionRules();

        if ( !te->CanPredict() )
            return;

        // Emitimos el sonido para aquellos lejanos pero no tanto...
        EmitSound( filter, pOwner->entindex(), shootsound, &pOwner->GetAbsOrigin(), soundtime );
#endif
    }
    else {
        CBroadcastRecipientFilter filter;
        filter.UsePredictionRules();

        if ( !te->CanPredict() )
            return;

        // Emitimos el sonido para todos aquellos que puedan escucharlo
        EmitSound( filter, pOwner->entindex(), shootsound, &pOwner->GetAbsOrigin(), soundtime );
    }

#ifndef CLIENT_DLL
    if ( sound == SINGLE || sound == SINGLE_NPC ) {
        float flVolume = SOUNDENT_VOLUME_EMPTY;

        if ( !HasSuppressor() ) {
            // TODO: Pistolas
            flVolume = (IsShotgun()) ? SOUNDENT_VOLUME_SHOTGUN : SOUNDENT_VOLUME_MACHINEGUN;
        }

        CSoundEnt::InsertSound( SOUND_COMBAT | SOUND_CONTEXT_GUNFIRE, pOwner->GetAbsOrigin(), flVolume, 0.3f, pOwner, SOUNDENT_CHANNEL_WEAPON );
    }
    else if ( sound == EMPTY && !HasSuppressor() ) {
        CSoundEnt::InsertSound( SOUND_COMBAT | SOUND_CONTEXT_GUNFIRE, pOwner->GetAbsOrigin(), SOUNDENT_VOLUME_EMPTY, 0.3f, pOwner, SOUNDENT_CHANNEL_WEAPON );
    }
#endif
}

//================================================================================
//================================================================================
void ImpactSoundGroup( const char *pSoundName, const Vector &vEndPos )
{
    #ifdef CLIENT_DLL
    CLocalPlayerFilter filter;
    C_BaseEntity::EmitSound( filter, NULL, pSoundName, &vEndPos );

    /*int j = g_GroupedSounds.AddToTail();
    g_GroupedSounds[j].m_SoundName    = pSoundName;
    g_GroupedSounds[j].m_vPos        = vEndPos;*/
    #endif
}

//================================================================================
//================================================================================
void CBaseWeapon::StartGroupingSounds()
{
    #ifdef CLIENT_DLL
    SetImpactSoundRoute( ImpactSoundGroup );
    #endif
}

//================================================================================
//================================================================================
void CBaseWeapon::EndGroupingSounds()
{
    #ifdef CLIENT_DLL
    //g_GroupedSounds.Purge();
    SetImpactSoundRoute( NULL );
    #endif
}

//================================================================================
// Crea un efecto de trayección de la bala y los sonidos a los jugadores 
// que pasen cerca
//================================================================================
void CBaseWeapon::MakeTracer( const Vector &vecTracerSrc, const trace_t &tr, int iTracerType, bool isFirstTracer )
{
    if ( !GetOwner() ) {
        Assert( false );
        BaseClass::MakeTracer( vecTracerSrc, tr, iTracerType );
        return;
    }

    const char *tracerParticle = GetTracerType();

    if ( !tracerParticle ) {
        AssertMsg( 0, "pszTracerParticle == NULL" );
        return;
    }

    Vector vecStart = vecTracerSrc;

    CBaseAnimating *pRenderable = GetIdealAnimating();
    int iEntIndex = pRenderable->entindex();
    int tracerAttachment = -1;

    if ( isFirstTracer ) {
        tracerAttachment = GetTracerAttachment();

        // Sobreescribimos la ubicación de donde debe comenzar el tracer
        // usando el attachment del arma.
        if ( tracerAttachment >= 0 ) {
            pRenderable->GetAttachment( tracerAttachment, vecStart );
        }
    }

#ifdef CLIENT_DLL
    {
        //NDebugOverlay::Box( vecStart, Vector( -1, -1, -1 ), Vector( 1, 1, 1 ), 255, 0, 0, 30.0f, 1.0f );
        //NDebugOverlay::Line( vecStart, tr.endpos, 255, 0, 0, true, 1.0f );
    }
#endif
    
    // Excluimos al propietario del arma ya que el hará los efectos en clientside
    IPredictionSystem::SuppressHostEvents( GetOwner() );
    UTIL_ParticleTracer( tracerParticle, vecStart, tr.endpos, iEntIndex, tracerAttachment, true );
    IPredictionSystem::SuppressHostEvents( NULL );
}

//================================================================================
// Devuelve el attachment de donde deben provenir las balas
//================================================================================
int CBaseWeapon::GetTracerAttachment()
{
    CBaseAnimating *pRenderable = GetIdealAnimating();

    if ( !pRenderable )
        return -1;

    return pRenderable->LookupAttachment( "muzzle_flash" );
}

//================================================================================
//================================================================================
bool CBaseWeapon::DispatchMuzzleEffect( const char *options, bool isFirstPerson )
{
#ifdef CLIENT_DLL
    CBaseAnimating *pOwner = GetPlayerOwner()->GetViewModel();

    if ( !pOwner )
        return false;

    int iAttachment = pOwner->LookupAttachment( "muzzle_flash" );

    if ( iAttachment == -1 )
        return false;

    tempents->MuzzleFlash( MUZZLEFLASH_SMG1, pOwner->GetRefEHandle(), iAttachment, isFirstPerson );
    return true;
#else
    return false;
#endif
}

//================================================================================
//================================================================================
void CBaseWeapon::ShowHitmarker( CBaseEntity *pVictim )
{
    if ( !pVictim )
        return;

    if ( !pVictim->MyCombatCharacterPointer() )
        return;

    CPlayer *pOwner = GetPlayerOwner();

    if ( !pOwner )
        return;

#ifdef CLIENT_DLL
    if ( sv_weapon_serverside_hitmarker.GetBool() )
        return;

    if ( !pOwner->IsLocalPlayer() )
        return;

    engine->ClientCmd( "cl_show_hitmarker\n" );
#else
    CUserAndObserversRecipientFilter filter( pOwner );

    if ( !sv_weapon_serverside_hitmarker.GetBool() )
        filter.RemoveRecipient( pOwner );

    UserMessageBegin( filter, "ShowHitmarker" );
        WRITE_BYTE( 1 );
    MessageEnd();
#endif
}

//================================================================================
// Devuelve si la animación especificada en realidad es un layer
//================================================================================
bool CBaseWeapon::IsLayerAnim( int iActivity ) 
{
	switch( iActivity )
	{
		case ACT_VM_PRIMARYATTACK_LAYER:
		case ACT_VM_IDLE_LAYER:
		case ACT_VM_DEPLOY_LAYER:
		case ACT_VM_RELOAD_LAYER:
		case ACT_VM_MELEE_LAYER:
			return true;
	}

	return false;
}

//================================================================================
//================================================================================
bool CBaseWeapon::SetIdealActivity( Activity ideal )
{
	return BaseClass::SetIdealActivity( ideal );

	// Las animaciones del Viewmodel en Left 4 Dead son Layers (Gestures)
	// algo que solo esta implementado en los jugadores y en servidor para algunas entidades
	// @TODO: Implementar todo esto a cliente
#if 0
	// No es un layer, lo pasamos normal
	if ( !IsLayerAnim(ideal) )
		return BaseClass::SetIdealActivity( ideal );

	MDLCACHE_CRITICAL_SECTION();
	int	idealSequence = SelectWeightedSequence( ideal );

	if ( idealSequence == -1 )
		return false;

	//BaseClass::SetIdealActivity( GetIdleActivity() );

#if defined( CLIENT_DLL )
	if ( !IsPredicted() )
		return false;
#endif
	
	if ( idealSequence < 0 )
		return false;

	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	
	if ( pOwner == NULL )
		return false;
	
	CBaseViewModel *vm = pOwner->GetViewModel( m_nViewModelIndex );
	
	if ( vm == NULL )
		return false;

	SetViewModel();
	Assert( vm->ViewModelIndex() == m_nViewModelIndex );
	vm->SendViewModelMatchingSequenceLayer( idealSequence );

	//Set the next time the weapon will idle
	SetWeaponIdleTime( gpGlobals->curtime + vm->SequenceDuration() );
	return true;
#endif
}

#ifndef CLIENT_DLL
void DebugWeaponFireSounds()
{
    CBaseWeapon *pWeapon = NULL;

    do {
        pWeapon = ToBaseWeapon( gEntList.FindEntityByClassname( pWeapon, "weapon_*" ) );

        if ( !pWeapon )
            continue;

        pWeapon->WeaponSound( SINGLE );
        pWeapon->WeaponSound( SPECIAL1 );
        pWeapon->DoMuzzleFlash();
    } while( pWeapon != NULL );
}

void DebugWeaponReloadSounds()
{
    CBaseWeapon *pWeapon = NULL;

    do {
        pWeapon = ToBaseWeapon( gEntList.FindEntityByClassname( pWeapon, "weapon_*" ) );

        if ( !pWeapon )
            continue;

        pWeapon->WeaponSound( RELOAD );
    } while( pWeapon != NULL );
}

ConCommand debug_weapon_firesounds( "debug_weapon_firesounds", DebugWeaponFireSounds, "", FCVAR_CHEAT );
ConCommand debug_weapon_reloadsounds( "debug_weapon_reloadsounds", DebugWeaponReloadSounds, "", FCVAR_CHEAT );
#endif