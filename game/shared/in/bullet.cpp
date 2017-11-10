//==== Woots 2017. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "bullet.h"

#include "ammodef.h"


#include "bullet_manager.h"
#include "shot_manipulator.h"

#include "effect_dispatch_data.h"
#include "decals.h"

#include "in_gamerules.h"

#ifndef CLIENT_DLL
	#include "in_player.h"
	#include "waterbullet.h"
	#include "player_lagcompensation.h"
    #include "te_effect_dispatch.h"
#else
	#include "c_in_player.h"
    #include "c_te_effect_dispatch.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


//================================================================================
// Iván: Al parecer las balas realistas son afectadas gravemente por la latencia
//================================================================================

//================================================================================
// Comandos
//================================================================================

DECLARE_SERVER_CMD( sv_realistic_bullet, "0", "" )
DECLARE_SERVER_CMD( sv_showimpacts, "0", "Shows client (red) and server (blue) bullet impact point" )
DECLARE_SERVER_CMD( sv_showimpacts_penetration, "0", "Shows client (red) and server (blue) bullet impact point" )

float g_DebugDuration = 4.0f; 

//================================================================================
// Macros
//================================================================================

#define BulletLog( origin ) debugoverlay->AddTextOverlay( origin, 0, g_DebugDuration, UTIL_VarArgs("BULLET #%i", m_iShot) );\
    debugoverlay->AddTextOverlay( origin, 1, g_DebugDuration, UTIL_VarArgs("CURRENT DISTANCE: %.2f", m_flCurrentDistance) );\
    debugoverlay->AddTextOverlay( origin, 2, g_DebugDuration, UTIL_VarArgs("LEFT DISTANCE: %.2f", m_flDistanceLeft) );\
    debugoverlay->AddTextOverlay( origin, 3, g_DebugDuration, UTIL_VarArgs("SPREAD: %.5f", m_BulletsInfo.m_vecSpread.x) );\
    debugoverlay->AddTextOverlay( origin, 5, g_DebugDuration, UTIL_VarArgs("DAMAGE: %.2f", m_DamageInfo.GetDamage()) );

//================================================================================
// Constructor
//================================================================================
CBullet::CBullet( const FireBulletsInfo_t bulletsInfo )
{
	Init( 0, bulletsInfo, NULL );
}

//================================================================================
// Constructor
//================================================================================
CBullet::CBullet( const FireBulletsInfo_t bulletsInfo, CBaseEntity *pOwner )
{
	Init( 0, bulletsInfo, pOwner );
}

//================================================================================
// Constructor
//================================================================================
CBullet::CBullet( int shot, const FireBulletsInfo_t info, CBaseEntity *pOwner ) 
{
	Init( shot, info, pOwner );
}

//================================================================================
// Inicializa la información de la bala
//================================================================================
void CBullet::Init( int shot, const FireBulletsInfo_t bulletsInfo, CBaseEntity *pOwner )
{
    m_iShot = shot;
    m_nOwner = pOwner;
    m_nWeapon = (CBaseWeapon *)bulletsInfo.m_pAttacker;
    m_bRealistic = sv_realistic_bullet.GetBool();
    m_BulletsInfo = bulletsInfo;

    if ( m_nOwner ) {
        if ( m_nOwner->MyCombatCharacterPointer() && !m_nWeapon )
            m_nWeapon = (CBaseWeapon *)m_nOwner->MyCombatCharacterPointer()->GetActiveWeapon();
    }

    // Si fue disparada de un arma, usamos su información
    if ( m_nWeapon ) {
        m_flMaxDistance = m_nWeapon->GetWeaponInfo().m_flMaxDistance;
        m_flMaxPenetrationDistance = m_nWeapon->GetWeaponInfo().m_flPenetrationMaxDistance;
        m_iMaxPenetrationLayers = m_nWeapon->GetWeaponInfo().m_iPenetrationNumLayers;
    }

    if ( m_bRealistic ) {
        TheBulletManager->Add( this );
    }
}

//================================================================================
// Prepara y configura la bala antes de ser disparada
//================================================================================
void CBullet::Prepare()
{
    int damageType = GetAmmoDef()->DamageType( m_BulletsInfo.m_iAmmoType );

    m_iPenetrations = 0;

    m_vecOrigin = m_BulletsInfo.m_vecSrc;  
    m_flCurrentDistance = 0.0f;
    m_flDistanceLeft = m_flMaxDistance;
    m_flSpeed = GetAmmoDef()->GetAmmoOfIndex( m_BulletsInfo.m_iAmmoType )->bulletSpeed;

    // Información de daño de la bala
    m_DamageInfo.SetAmmoType( m_BulletsInfo.m_iAmmoType );
    m_DamageInfo.SetAttacker( GetOwner() ); // Jugador
    m_DamageInfo.SetDamage( m_BulletsInfo.m_flDamage );
    m_DamageInfo.SetDamageType( damageType );
    m_DamageInfo.SetInflictor( m_BulletsInfo.m_pAttacker ); // Arma
    m_DamageInfo.SetWeapon( GetWeapon() );

    if ( GetOwner() && GetOwner()->IsPlayer() ) {
        m_DamageInfo.AdjustPlayerDamageInflictedForSkillLevel();
    }

    CShotManipulator manipulator( m_BulletsInfo.m_vecDirShooting );

    // Calculamos la dirección de la bala con el Manipulator
    // La primera bala debe ir recto, las demás tendrán disperción
    if ( m_iShot == 0 && m_BulletsInfo.m_iShots > 1 || m_BulletsInfo.m_vecSpread == vec3_origin )
        m_vecShotDir = manipulator.GetShotDirection();
    else
        m_vecShotDir = manipulator.ApplySpread( m_BulletsInfo.m_vecSpread );

    // Inicio y fin de la bala
    m_vecShotStart = m_vecShotInitial = m_BulletsInfo.m_vecSrc;
    m_vecShotEnd = m_vecShotStart + m_flMaxDistance * m_vecShotDir;
}

//================================================================================
// Simula la trayección de la bala en el mundo
// Devuelve false si la bala debería ser eliminada del mundo
//================================================================================
bool CBullet::Simulate()
{
    if ( !m_bRealistic )
        return false;

    // Ya no estas en el mapa
    //if ( !IsInWorld() )
        //return false;

    if ( !IsFinite( m_flSpeed ) || m_flSpeed <= 0 )
        return false;

    if ( g_MultiDamage.GetTarget() != NULL )
        ApplyMultiDamage();

    ClearMultiDamage();
    g_MultiDamage.SetDamageType( GetAmmoDef()->DamageType( m_BulletsInfo.m_iAmmoType ) );

    // Entre más caemos, la gravedad nos hace más rápidos
    m_flSpeed += 0.1f * m_vecShotDir.z;

    // Donde debería terminar la bala antes de aumentar más la velocidad
    m_vecShotEnd = m_vecShotStart + m_vecShotDir * m_flSpeed;

    // Vamos disminuyendo la altura por la gravedad
    m_vecShotDir.z -= 0.001f / m_flSpeed;

    bool alive = HandleFire();

    ApplyMultiDamage();

    if ( !alive )
        return false;

    // Nueva posición de la bala
    if ( !m_bIgnorePositionUpdate ) {
        m_vecOrigin += m_vecShotDir * m_flSpeed;
        m_vecShotStart = m_vecOrigin;
    }
    else {
        m_bIgnorePositionUpdate = false;
    }

    return true;
}

//================================================================================
// Dispara la bala
//================================================================================
void CBullet::Fire( int seed ) 
{
    RandomSeed( seed );
	Fire();
}

//================================================================================
// Dispara la bala
//================================================================================
void CBullet::Fire()
{
    Prepare();

    if ( !m_bRealistic ) {
        HandleFire();
    }
}

//================================================================================
// Maneja el disparo, hace las penetraciones necesarias
// Devuelve false si la bala debería ser eliminada (al terminar su vida)
//================================================================================
bool CBullet::HandleFire()
{
    trace_t tr;

    // Skip multiple entities when tracing
    CBulletsTraceFilter filter( COLLISION_GROUP_NONE );
    filter.SetPassEntity( GetOwner() );
    filter.AddEntityToIgnore( m_BulletsInfo.m_pAdditionalIgnoreEnt );

#ifndef CLIENT_DLL
    if ( GetPlayerOwner() ) {
        CPlayer *pPlayer = ToInPlayer( m_nOwner );

        if ( pPlayer->IsInAVehicle() )
            filter.AddEntityToIgnore( pPlayer->GetVehicleEntity() );
    }
#endif

    while ( m_flDistanceLeft > 0 && m_iPenetrations < m_iMaxPenetrationLayers && m_DamageInfo.GetDamage() > 0 ) {
        // Trazamos la línea de disparo
        UTIL_TraceLine( m_vecShotStart, m_vecShotEnd, MASK_SHOT, &filter, &tr );

        // Manejamos cualquier impacto con el agua
        HandleWaterImpact( filter );

        // Efecto: Tracer
        // @TODO: Hacer esto incluso sin un arma especificada
        if ( GetWeapon() && !m_bRealistic ) {
            GetWeapon()->MakeTracer( tr.startpos, tr, GetAmmoDef()->TracerType( m_BulletsInfo.m_iAmmoType ), (m_iPenetrations == 0) );
        }

        m_flCurrentDistance = m_vecShotInitial.DistTo( tr.endpos );
        m_flDistanceLeft = (m_flMaxDistance - m_flCurrentDistance);

        // Información de depuración: Salida de bala            
        if ( sv_showimpacts.GetBool() ) {
#ifndef CLIENT_DLL
            debugoverlay->AddBoxOverlay( tr.startpos, Vector( -2, -2, -2 ), Vector( 2, 2, 2 ), QAngle( 0, 0, 0 ), 255, 0, 0, 150, g_DebugDuration );
            DebugDrawLine( tr.startpos, tr.endpos, 255, 0, 0, true, g_DebugDuration );

            if ( sv_showimpacts_penetration.GetBool() && !m_bRealistic ) {
                BulletLog( tr.startpos );
            }
#else
            debugoverlay->AddBoxOverlay( tr.startpos, Vector( -2, -2, -2 ), Vector( 2, 2, 2 ), QAngle( 0, 0, 0 ), 0, 0, 255, 150, g_DebugDuration );
            DebugDrawLine( tr.startpos, tr.endpos, 0, 0, 255, true, g_DebugDuration );
#endif
        }

        // ¿Hemos empezado en algo solido?
        if ( tr.startsolid )
            return false;

        // No hemos impactado con nada
        if ( tr.fraction == 1.0f )
            return true;

        if ( m_bRealistic ) {
            // Bala realista: Pase lo que pase, la simulación se hara cargo
            return HandleShotImpact( tr );
        }
        else {
            // La fuerza de la bala termina aquí
            if ( !HandleShotImpact( tr ) )
                return false;
        }
    }

    if ( sv_showimpacts.GetBool() ) {
        if ( m_flDistanceLeft <= 0 )
            debugoverlay->AddTextOverlay( m_vecOrigin, 7, g_DebugDuration, UTIL_VarArgs( "LA BALA SE QUEDO SIN DISTANCIA!" ) );

        if ( m_iPenetrations >= m_iMaxPenetrationLayers )
            debugoverlay->AddTextOverlay( m_vecOrigin, 8, g_DebugDuration, UTIL_VarArgs( "PENETRATION!: %i / %i", m_iPenetrations, m_iMaxPenetrationLayers ) );

        if ( m_DamageInfo.GetDamage() <= 0 )
            debugoverlay->AddTextOverlay( m_vecOrigin, 9, g_DebugDuration, UTIL_VarArgs( "DAMAGE!: %.2F", m_DamageInfo.GetDamage() ) );

    }

    // Nos hemos quedado sin distancia que recorrer...
    // Ya no podemos seguir penetrando paredes...
    // La bala se ha quedado sin daño...
    return false;
}

//================================================================================
// ¡La bala ha impactado!
//================================================================================
bool CBullet::HandleShotImpact( trace_t &tr )
{
    //int damageType = GetAmmoDef()->DamageType( m_BulletsInfo.m_iAmmoType );

    // Marca y sonido de impacto
    DoImpactEffect( tr );

#ifndef CLIENT_DLL
    // Información de depuración: Impacto de bala
    if ( sv_showimpacts.GetBool() ) {
        debugoverlay->AddBoxOverlay( tr.endpos, Vector( -2, -2, -2 ), Vector( 2, 2, 2 ), QAngle( 0, 0, 0 ), 0, 255, 0, 150, g_DebugDuration );

        if ( sv_showimpacts_penetration.GetBool() ) {
            BulletLog( tr.endpos );
        }
    }

    if ( GetOwner() )
        GetOwner()->TraceAttackToTriggers( m_DamageInfo, tr.startpos, tr.endpos, m_vecShotDir );
#endif

    // Le hemos dado a una entidad
    if ( tr.m_pEnt && !tr.m_pEnt->IsWorld() ) {
        HandleEntityImpact( tr.m_pEnt, tr );
    }

    // Obtenemos el material de la superficie que impactamos y con ello
    // simulamos la fuerza para detener la bala
    surfacedata_t *surfaceData = physprops->GetSurfaceData( tr.surface.surfaceProps );

    // TODO: Mejor código para esto
    if ( surfaceData->game.material == CHAR_TEX_WOOD )
        m_flDistanceLeft -= 500.0f;
    else if ( surfaceData->game.material == CHAR_TEX_GRATE )
        m_flDistanceLeft -= 100.0f;
    else if ( surfaceData->game.material == CHAR_TEX_TILE )
        m_flDistanceLeft -= 200.0f;
    else if ( surfaceData->game.material == CHAR_TEX_COMPUTER )
        m_flDistanceLeft -= 50.0f;
    else if ( surfaceData->game.material == CHAR_TEX_GLASS )
        m_flDistanceLeft -= 200.0f;
    else if ( surfaceData->game.material == CHAR_TEX_PLASTIC )
        m_flDistanceLeft -= 200.0f;
    else if ( surfaceData->game.material == CHAR_TEX_VENT || surfaceData->game.material == CHAR_TEX_METAL )
        m_flDistanceLeft -= 700.0f;
    else if ( surfaceData->game.material == CHAR_TEX_BLOODYFLESH || surfaceData->game.material == CHAR_TEX_FLESH )
        m_flDistanceLeft -= 600.0f;
    
#ifndef CLIENT_DLL
    // Información de depuración: Reducción de fuerza
    if ( sv_showimpacts.GetBool() && sv_showimpacts_penetration.GetBool() ) {
        debugoverlay->AddTextOverlay( tr.endpos, 6, g_DebugDuration, UTIL_VarArgs( "MATERIAL: %i", surfaceData->game.material ) );
        debugoverlay->AddTextOverlay( tr.endpos, 7, g_DebugDuration, UTIL_VarArgs( "NEW DISTANCE LEFT: %.2f", m_flDistanceLeft ) );
    }
#endif

    ++m_iPenetrations;

    // La bala ha terminado en una pared/objeto
    // Intentamos salir... (penetrar)

    // Posición estimada de salida, según la distancia que puede recorrer la bala al penetrar
    Vector vecExit = tr.endpos + m_flMaxPenetrationDistance * m_vecShotDir;

    // Trazo de la penetración
    trace_t penetration_tr;
    UTIL_TraceLine( vecExit, tr.endpos, MASK_SHOT, m_nOwner, COLLISION_GROUP_NONE, &penetration_tr );

    // La bala no pudo salir
    if ( penetration_tr.startsolid ) {
#ifndef CLIENT_DLL
        // Trazo de penetración incompleta   
        //if ( sv_showimpacts.GetBool() )
            //DebugDrawLine( tr.endpos, vecExit, 255, 64, 0, true, g_DebugDuration );
#endif

        return false;
    }

    // Menos poder de penetración para la próxima
    m_flMaxPenetrationDistance -= 3.0f;

    // La bala ahora comienza en la salida de la penetración
    m_vecShotStart = m_vecOrigin = penetration_tr.endpos;

    // Ignoramos la actualización de la posición en la próxima simulación
    m_bIgnorePositionUpdate = true;

#ifndef CLIENT_DLL
    // Trazo de penetración        
    if ( sv_showimpacts.GetBool() ) {
        DebugDrawLine( tr.endpos, penetration_tr.endpos, 242, 245, 169, true, g_DebugDuration );
        debugoverlay->AddBoxOverlay( m_vecShotStart, Vector( -2, -2, -2 ), Vector( 2, 2, 2 ), QAngle( 0, 0, 0 ), 242, 245, 169, 150, g_DebugDuration );
    }
#endif

    // Entre más impactos tenga menor será el daño
    // @TODO: Escalar daño según la distancia
    m_DamageInfo.ScaleDamage( 0.9f );
    return true;
}

//================================================================================
// La bala ha impactado a la entidad especificada
//================================================================================
void CBullet::HandleEntityImpact( CBaseEntity *pVictim, trace_t &tr )
{
    // Calculamos la fuerza de la bala
    CalculateBulletDamageForce( &m_DamageInfo, m_BulletsInfo.m_iAmmoType, m_vecShotDir, tr.endpos );
    m_DamageInfo.ScaleDamageForce( m_BulletsInfo.m_flDamageForceScale );

#ifndef CLIENT_DLL
    TheGameRules->AdjustDamage( pVictim, m_DamageInfo );
#endif

    // Ouch! Sangre
    //IPredictionSystem::SuppressHostEvents( GetOwner() );
    pVictim->DispatchTraceAttack( m_DamageInfo, m_vecShotDir, &tr );
    //IPredictionSystem::SuppressHostEvents( NULL );

    if ( GetWeapon() ) {
        GetWeapon()->ShowHitmarker( pVictim );
    }

#ifndef CLIENT_DLL
    if ( sv_showimpacts.GetBool() )
        pVictim->DrawBBoxOverlay( g_DebugDuration );
#endif
}

//================================================================================
// Verifica si la bala impacto con el agua y si es así crea los efectos
// Devuelve true si hemos impactado con agua
//================================================================================
bool CBullet::HandleWaterImpact( ITraceFilter &filter ) 
{
	trace_t tr;

	// Trazamos la misma línea que el disparo normal, pero esta vez nos detenemos
	// al tocar agua
    UTIL_TraceLine( m_vecShotStart, m_vecShotEnd, (MASK_SHOT|CONTENTS_WATER|CONTENTS_SLIME), &filter, &tr );

	// No hemos impactado con nada
	if ( tr.fraction == 1.0f )
		return false;

	// See if this is the point we entered
	if ( ( enginetrace->GetPointContents(tr.endpos - Vector(0,0,0.1f), MASK_WATER ) & (CONTENTS_WATER|CONTENTS_SLIME) ) == 0 )
		return false;

#ifndef CLIENT_DLL
	// Impacto del agua
	if ( ShouldDrawWaterImpact() )
	{
		int	nMinSplashSize = GetAmmoDef()->MinSplashSize( m_BulletsInfo.m_iAmmoType );
		int	nMaxSplashSize = GetAmmoDef()->MaxSplashSize( m_BulletsInfo.m_iAmmoType );

		CEffectData	data;
 		data.m_vOrigin = tr.endpos;
		data.m_vNormal = tr.plane.normal;
		data.m_flScale = random->RandomFloat( nMinSplashSize, nMaxSplashSize );\

		if ( tr.contents & CONTENTS_SLIME )
			data.m_fFlags |= FX_WATER_IN_SLIME;
		
		DispatchEffect( "gunshotsplash", data );
	}


	if ( ShouldDrawUnderwaterBulletBubbles() )
	{
		CWaterBullet *pWaterBullet = ( CWaterBullet * )CreateEntityByName( "waterbullet" );

		if ( pWaterBullet )
		{
			pWaterBullet->Spawn( tr.endpos, m_vecShotDir );
					 
			CEffectData tracerData;
			tracerData.m_vStart = tr.endpos;
			tracerData.m_vOrigin = tr.endpos + m_vecShotDir * 800.0f;
			tracerData.m_fFlags = TRACER_TYPE_WATERBULLET;
			DispatchEffect( "TracerSound", tracerData );
		}
	}
#endif

	return true;
}


//================================================================================
// Crea un sprite de impacto
//================================================================================
void CBullet::DoImpactEffect( trace_t &tr ) 
{
	int damageType = GetAmmoDef()->DamageType( m_BulletsInfo.m_iAmmoType );

    IPredictionSystem::SuppressHostEvents( GetOwner() );

	// Marca de impacto de salida
    if ( GetOwner() ) {        
        GetOwner()->DoImpactEffect( tr, damageType );
    }
    else {
        UTIL_ImpactTrace( &tr, damageType );
    }

    // Impacto de disparo a los cadaveres
    // @TODO: ¿Esto funciona?
    CEffectData data;
    data.m_vStart = tr.startpos;
    data.m_vOrigin = tr.endpos;
    data.m_nDamageType = damageType;
    DispatchEffect( "RagdollImpact", data );

    IPredictionSystem::SuppressHostEvents( NULL );

#ifndef CLIENT_DLL
    if ( GetWeapon() && GetWeapon()->IsSniper() ) {
        CSoundEnt::InsertSound( SOUND_DANGER | SOUND_CONTEXT_BULLET_IMPACT | SOUND_CONTEXT_FROM_SNIPER, tr.endpos, SOUNDENT_VOLUME_EMPTY, 0.2f, m_nOwner, SOUNDENT_CHANNEL_REPEATING );
    }
    else {
        CSoundEnt::InsertSound( SOUND_DANGER | SOUND_CONTEXT_BULLET_IMPACT, tr.endpos, SOUNDENT_VOLUME_EMPTY, 0.2f, m_nOwner, SOUNDENT_CHANNEL_REPEATING );
    }
#endif
}

//================================================================================
// Devuelve si la bala sigue dentro de los limites del mapa
//================================================================================
bool CBullet::IsInWorld() 
{
	if (m_vecOrigin.x >= MAX_COORD_INTEGER) return false;
	if (m_vecOrigin.y >= MAX_COORD_INTEGER) return false;
	if (m_vecOrigin.z >= MAX_COORD_INTEGER) return false;
	if (m_vecOrigin.x <= MIN_COORD_INTEGER) return false;
	if (m_vecOrigin.y <= MIN_COORD_INTEGER) return false;
	if (m_vecOrigin.z <= MIN_COORD_INTEGER) return false;
	return false;
}

//================================================================================
//================================================================================
CBasePlayer * CBullet::GetPlayerOwner() 
{
	return ToInPlayer( m_nOwner );
}
