//==== Woots 2017. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "weapon_base.h"

#include "in_buttons.h"


#include "datacache/imdlcache.h"
#include "tier0/vprof.h"

#ifndef CLIENT_DLL
    #include "in_player.h"
    #include "player_lagcompensation.h"
    #include "soundent.h"
    #include "bots\squad.h"
#else
    #include "fx_impact.h"
    #include "c_in_player.h"
    #include "sdk_input.h"
    #include "cl_animevent.h"
    #include "c_te_legacytempents.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
// Comandos
//================================================================================

//================================================================================
// Información y Red
//================================================================================

IMPLEMENT_NETWORKCLASS_ALIASED( BaseWeaponShotgun, DT_BaseWeaponShotgun )

BEGIN_NETWORK_TABLE( CBaseWeaponShotgun, DT_BaseWeaponShotgun )
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CBaseWeaponShotgun )
END_PREDICTION_DATA()
#endif

//================================================================================
//================================================================================
CBaseWeaponShotgun::CBaseWeaponShotgun()
{
	m_bReloadsSingly = true;
}

//================================================================================
//================================================================================
bool CBaseWeaponShotgun::Reload() 
{
	// Ya estamos recargando, usamos la animación en loop
	if ( IsReloading() )
	{
		return DefaultReload( GetMaxClip1(), GetMaxClip2(), GetReloadLoopActivity() );
	}
	else
	{
		return DefaultReload( GetMaxClip1(), GetMaxClip2(), GetReloadActivity() );
	}
}

//================================================================================
// Hemos terminado de recargar
//================================================================================
void CBaseWeaponShotgun::FinishReload() 
{
	CPlayer *pPlayer = GetPlayerOwner();
        
    // Solo Jugadores
    if ( !pPlayer )
        return;

	if ( IsReloading() )
	{
		SendWeaponAnim( GetReloadEndActivity() );
		pPlayer->DoAnimationEvent( PLAYERANIMEVENT_RELOAD_END, 0, true );

		// Tony; update our weapon idle time
		MDLCACHE_CRITICAL_SECTION();
		SetWeaponIdleTime( gpGlobals->curtime + SequenceDuration() );
		SetNextAttack( SequenceDuration() );
	}

	BaseClass::FinishReload();
}
