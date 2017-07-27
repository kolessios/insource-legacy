//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "c_ap_player.h"

#include "dlight.h"
#include "c_effects.h"
#include "iefx.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
// Comandos
//================================================================================

//================================================================================
// Información y Red
//================================================================================

#undef CAP_Player

IMPLEMENT_CLIENTCLASS_DT( C_AP_Player, DT_AP_Player, CAP_Player )
END_RECV_TABLE()

//================================================================================
// Destructor
//================================================================================
C_AP_Player::~C_AP_Player()
{
	// Música
	DestroyMusic();

    // Luces
    DestroyDejectedLight();
    DestroyMuzzleLight();
}

//================================================================================
//================================================================================
void C_AP_Player::UpdatePoseParams()
{
    // Movimiento de las manos en primera persona
    /*if ( GetViewModel() && GetActiveWeapon() )
	{
		float flMoveX = GetPoseParameter( LookupPoseParameter("move_x") );
		GetViewModel()->SetPoseParameter( GetViewModel()->LookupPoseParameter("move_x"), flMoveX );
	}*/
}

//================================================================================
// Como el pensamiento, pero para el jugador local y los demás.
//================================================================================
bool C_AP_Player::Simulate()
{
	try {
        if ( IsSurvivor() || IsSoldier() ) {
            // Luz de incapacitación
            UpdateDejectedLight();

            // MuzzleFlash
            if ( GetFlashlight( MUZZLEFLASH ) )
                GetFlashlight( MUZZLEFLASH )->Think();

            // Actualizamos la música
            UpdateMusic();
        }
	}
	catch( ... ) {
		Warning("[C_AP_PlayerSurvivor::Simulate] EXCEPTION!");
	}

    // BaseClass
    return BaseClass::Simulate();
}

//================================================================================
//================================================================================
void C_AP_Player::PostDataUpdate( DataUpdateType_t updateType )
{
    if ( updateType == DATA_UPDATE_CREATED )
    {
		// Creamos la música
		CreateMusic();

        // Luz de incapacitación
        CreateDejectedLight();

        // Luz realista al disparar
        CreateMuzzleLight();
    }

    // Base!
    BaseClass::PostDataUpdate( updateType );
}


//================================================================================
// Prepara y configura la música
//================================================================================
void C_AP_Player::CreateMusic()
{
	CSoundInstance *pSound;
	CSoundInstance *pTag;

	// Creamos un nuevo administrador de sonido
	m_nMusicManager = new CSoundManager();

	PREPARE_SOUND_WITH_TAG( CHANNEL_1, PLAYER_SOUND(DEATH_MUSIC, true),		TAG_PLAYER_SOUND("Tag.Player.Death") );
	PREPARE_SOUND_WITH_TAG( CHANNEL_1, PLAYER_SOUND(DEJECTED_MUSIC, true),	TAG_PLAYER_SOUND("Tag.Player.PuddleOfYou") );
	PREPARE_SOUND_WITH_TAG( CHANNEL_1, PLAYER_SOUND(DYING_MUSIC, false),	TAG_PLAYER_SOUND("Tag.Player.SoCold") );

	PREPARE_SOUND_WITH_TAG( CHANNEL_2, PLAYER_SOUND(CLINGING1_MUSIC, true), TAG_PLAYER_SOUND("Tag.Player.Clinging1") );
	PREPARE_SOUND_WITH_TAG( CHANNEL_2, PLAYER_SOUND(CLINGING2_MUSIC, true), TAG_PLAYER_SOUND("Tag.Player.Clinging2") );
	PREPARE_SOUND_WITH_TAG( CHANNEL_2, PLAYER_SOUND(CLINGING3_MUSIC, true), TAG_PLAYER_SOUND("Tag.Player.Clinging3") );
	PREPARE_SOUND_WITH_TAG( CHANNEL_2, PLAYER_SOUND(CLINGING4_MUSIC, true), TAG_PLAYER_SOUND("Tag.Player.Clinging4") );
}

//================================================================================
// Destruye todos los sonidos
//================================================================================
void C_AP_Player::DestroyMusic()
{
	delete m_nMusicManager;
}

//================================================================================
//================================================================================
float C_AP_Player::SoundDesire( const char *soundName, int channel )
{
	return 0.0f;

	// NULL
	if ( FStrEq(soundName, DEATH_MUSIC) )
	{
		if ( !IsAlive() )
			return 1.0f;
	}

	if ( IsAlive() )
	{
		// CHANNEL_1
		if ( FStrEq(soundName, DEJECTED_MUSIC) )
		{
			if ( GetPlayerStatus() == PLAYER_STATUS_DEJECTED )
				return 0.5f;
		}

		
		{
			if ( GetPlayerStatus() == PLAYER_STATUS_DEJECTED && GetHealth() <= 6 )
				return 1.0f;
		}

		// CHANNEL_2
		if ( FStrEq(soundName, CLINGING1_MUSIC) )
		{
			if ( GetPlayerStatus() == PLAYER_STATUS_CLIMBING )
				return 0.1f;
		}

		if ( FStrEq(soundName, CLINGING2_MUSIC) )
		{
			if ( GetPlayerStatus() == PLAYER_STATUS_CLIMBING && (GetHealth() <= 65 || m_flClimbingHold <= 65) )
				return 0.2f;
		}

		if ( FStrEq(soundName, CLINGING3_MUSIC) )
		{
			if ( GetPlayerStatus() == PLAYER_STATUS_CLIMBING && (GetHealth() <= 35 || m_flClimbingHold <= 35) )
				return 0.3f;
		}

		if ( FStrEq(soundName, CLINGING4_MUSIC) )
		{
			if ( GetPlayerStatus() == PLAYER_STATUS_CLIMBING && (GetHealth() <= 10 || m_flClimbingHold <= 15) || GetPlayerStatus() == PLAYER_STATUS_FALLING )
				return 0.4f;
		}
	}

	return 0.0f;
}

//================================================================================
//================================================================================
void C_AP_Player::UpdateMusic()
{
	m_nMusicManager->Update();
}

//================================================================================
//================================================================================
void C_AP_Player::OnSoundPlay( const char *soundName )
{
}

//================================================================================
//================================================================================
bool C_AP_Player::CreateDejectedLight()
{
    if ( GetFlashlight(DEJECTED) )
        return true;

    GetFlashlight(DEJECTED) = new CFlashlightEffect( index, "effects/flashlight001_infected" );
    Assert( GetFlashlight(DEJECTED) );

    // Ha ocurrido un problema
    if ( !GetFlashlight(DEJECTED) )
    {
        Warning("Ha ocurrido un problema al crear la linterna de incapacitación de %s \n", GetPlayerName());
        return false;
    }

    CFlashlightEffect *pLight = GetFlashlight(DEJECTED);

    // Configuración de la linterna
    pLight->Init();
    pLight->SetShadows( true );
    pLight->SetFOV( 65.0f );
    pLight->SetFar( GetFlashlightFarZ() );
    pLight->SetColor( 127, 0, 0, 255 );
    pLight->SetBrightScale( 6.0f );
    return true;
}

//================================================================================
//================================================================================
void C_AP_Player::DestroyDejectedLight()
{
    // Apagamos la linterna
    if ( GetFlashlight(DEJECTED) )
    {
        GetFlashlight(DEJECTED)->TurnOff();
        GetFlashlight(DEJECTED) = NULL;
    }
}

//================================================================================
//================================================================================
void C_AP_Player::UpdateDejectedLight()
{
    CFlashlightEffect *pLight = GetFlashlight(DEJECTED);

    if ( !pLight )
        return;

    // Obtenemos el jugador local o el jugador a quien esta viendo en modo espectador
    C_Player *pPlayer = GetActivePlayer();

    // Destruimos la linterna
    if ( !pPlayer || (pPlayer->IsAlive() && !pPlayer->IsDejected()) || pPlayer->IsObserver() )
    {
        pLight->TurnOff();
        return;
    }

    // Linterna de otro jugador
    if ( !IsLocalPlayer() )
    {
        // Si estamos viendo a este jugador en modo espectador, nuestra linterna actuara como el de el
        if ( !IsLocalPlayerWatchingMe(OBS_MODE_CHASE) && !IsLocalPlayerWatchingMe(OBS_MODE_IN_EYE) )
        {
            pLight->TurnOff();
            return;
        }
    }

    // Apuntamos la luz a su cadaver
	CBaseAnimating *pRagdoll = pPlayer->GetRagdoll();

    // Obtenemos la posición de la linterna
    Vector vecPosition, vecForward, vecRight, vecUp;

    // Altura y un poco de movimiento lateral para añadir un efecto drámatico
    vecPosition = pRagdoll->GetAbsOrigin();
    vecPosition.z += 100.0f;
    vecPosition.x += 60.0f;
    vecPosition.y += 60.0f;

    // Angulos hacia el cadaver
    Vector vecLook = pRagdoll->GetAbsOrigin() - vecPosition;
    QAngle anglesLook;
    VectorAngles( vecLook, anglesLook );

    AngleVectors( anglesLook, &vecForward, &vecRight, &vecUp  );

    // Actualizamos la linterna
    pLight->Update( vecPosition, vecForward, vecRight, vecUp );
    pLight->TurnOn();
}

//================================================================================
//================================================================================
bool C_AP_Player::CreateMuzzleLight()
{
    if ( GetFlashlight(MUZZLEFLASH) )
        return true;

    GetFlashlight(MUZZLEFLASH) = new CFlashlightEffect( index, "effects/muzzleflash_light" );
    Assert( GetFlashlight(MUZZLEFLASH) );

    // Ha ocurrido un problema
    if ( !GetFlashlight(MUZZLEFLASH) )
    {
        Warning("Ha ocurrido un problema al crear la luz de disparo de %s \n", GetPlayerName());
        return false;
    }

    CFlashlightEffect *pLight = GetFlashlight(MUZZLEFLASH);

    // Configuración de la linterna
    pLight->Init();
    pLight->SetShadows( true, false, 0, 4.0f );
    pLight->SetFar( GetFlashlightFarZ() + 500.0f );
    pLight->SetColor( 224, 209, 108, 455 );
    return true;
}

//================================================================================
//================================================================================
void C_AP_Player::DestroyMuzzleLight()
{
    // Apagamos la linterna
    if ( GetFlashlight(MUZZLEFLASH) )
    {
        GetFlashlight(MUZZLEFLASH)->TurnOff();
        GetFlashlight(MUZZLEFLASH) = NULL;
    }
}

extern ConVar muzzleflash_light;

//================================================================================
//================================================================================
void C_AP_Player::ShowMuzzleFlashlight()
{
    ConVarRef sv_muzzleflashlight_realistic( "sv_muzzleflashlight_realistic" );
    CFlashlightEffect *pLight = GetFlashlight( MUZZLEFLASH );

    // Obtenemos la ubicación para la linterna
    Vector vecPosition, vecForward, vecRight, vecUp;
    GetFlashlightPosition( this, vecPosition, vecForward, vecRight, vecUp, true, "muzzle_flash" );
    //GetFlashlightPosition( this, vecPosition, vecForward, vecRight, vecUp, true, "anim_attachment_LH" );

    // Luz de disparo realista
    if ( sv_muzzleflashlight_realistic.GetBool() && pLight ) {
        pLight->SetBrightScale( RandomFloat( 4.0f, 8.0f ) );
        pLight->SetFOV( RandomFloat( 80.0f, 100.0f ) );
        pLight->SetDie( 0.066f );
        pLight->TurnOn();
        pLight->Update( vecPosition, vecForward, vecRight, vecUp );
    }

    // Muzzleflash Simple
    if ( muzzleflash_light.GetBool() ) {
        // Make an elight
        dlight_t *el = effects->CL_AllocDlight( index );
        el->origin = vecPosition;
        el->radius = RandomFloat( 145.0f, 316.0f );
        el->decay = 0.05f;
        el->die = gpGlobals->curtime + 0.001f;
        el->color.r = 224;
        el->color.g = 209;
        el->color.b = 108;
        el->color.exponent = 5;
    }
}

//================================================================================
//================================================================================
void C_AP_Player::CalcPlayerView( Vector &eyeOrigin, QAngle &eyeAngles, float &fov )
{
    try
    {
        // Hemos muerto
        if ( !IsAlive() && GetRagdoll() )
        {
            CBaseAnimating *pRagdoll = GetRagdoll();

            // Ubicación, origen y destino de la luz.
            Vector vecPosition, vecForward, vecRight, vecUp;

            vecPosition = pRagdoll->GetAbsOrigin();
            vecPosition.z += 40.0f;

            float timedeath = gpGlobals->curtime - GetDeathTime();
            vecPosition.z += 15.0f * clamp( timedeath, 0, 5.0f );

            //DevMsg( "%.2f - %.2f - %.2f\n", GetDeathTime(), (gpGlobals->curtime - GetDeathTime()), (15.0f * timedeath) );

            // Solo durante uno segundos después de llegar al máximo, luego dejamos
            // que el jugador controle la cámara
            if ( timedeath < 8.0f || gpGlobals->curtime == timedeath )
            {
                Vector vecLook = pRagdoll->GetAbsOrigin() - vecPosition;
                QAngle anglesLook;
                VectorAngles( vecLook, anglesLook );

                AngleVectors( anglesLook, &vecForward, &vecRight, &vecUp );

                eyeOrigin = vecPosition;
                eyeAngles = anglesLook;
                return;
            }
        }
    }
    catch( ... ) { }

    // Estamos cayeeendoooo!!!
    if ( GetPlayerStatus() == PLAYER_STATUS_FALLING )
    {
        // Pongamos al jugador en la perspectiva del personaje
        if ( GetEyesView(this, eyeOrigin, eyeAngles, fov) )
            return;
    }

    // Tenemos un arma activa
    /*if ( GetActiveWeapon() )
    {
        CBaseViewModel *vm = GetViewModel( 0 );

        if ( vm )
        {
            int attachment = vm->LookupAttachment( "attach_camera" );

            // Usamos el attachment de la cámara
            if ( attachment > 0 )
            {
                vm->GetAttachment( attachment, eyeOrigin, eyeAngles );
                NDebugOverlay::Box( eyeOrigin, Vector( -5, -5, -5 ), Vector( 5, 5, 5 ), 0, 255, 0, 5.0f, 0.1f );
                return;
            }
        }
    }*/

    BaseClass::CalcPlayerView( eyeOrigin, eyeAngles, fov );
}

//================================================================================
//================================================================================
void C_AP_Player::DoPostProcessingEffects( PostProcessParameters_t &params )
{
    C_Player *pPlayer = GetActivePlayer();

    if ( !pPlayer )
        return;

    // Somos nosotros pero ya estamos en modo espectador, no queremos efectos
    if ( pPlayer == this ) {
        float timedeath = gpGlobals->curtime - GetDeathTime();

        if ( IsObserver() && timedeath >= 8.0f )
            return;
    }

    if ( pPlayer->IsAlive() ) {
        if ( pPlayer->IsDejected() ) {
            float health = (float)pPlayer->GetHealth();
            float seed = (5.0f + health);

            float flVignette = (health / seed);
            flVignette = MAX( flVignette, 0.6f );

            params.m_flParameters[PPPN_VIGNETTE_START] = 0.1f;
            params.m_flParameters[PPPN_VIGNETTE_END] = flVignette;
            params.m_flParameters[PPPN_VIGNETTE_BLUR_STRENGTH] = 1.0f;
        }
        else {
            float local_stress = pPlayer->GetLocalStress();

            float flVignette = ((160 - local_stress) / 100);
            flVignette = MAX( flVignette, 0.6f ); // 0.1 = Cerrado en el centro

            if ( local_stress > 0 ) {
                params.m_flParameters[PPPN_VIGNETTE_START] = 0.1f;
                params.m_flParameters[PPPN_VIGNETTE_END] = flVignette;
                params.m_flParameters[PPPN_VIGNETTE_BLUR_STRENGTH] = 0.3f;
            }
        }

        DoStressContrastEffect( params );
    }
    else {
        params.m_flParameters[PPPN_VIGNETTE_START] = 0.1f;
        params.m_flParameters[PPPN_VIGNETTE_END] = 1.0f;
        params.m_flParameters[PPPN_VIGNETTE_BLUR_STRENGTH] = 1.0f;
    }
}
