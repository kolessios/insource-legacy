//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#include "cbase.h"
#include "ent_host.h"

#include "ent\ent_player.h"
#include "ent\ent_character.h"

#include "bots\bot.h"
#include "bots\squad_manager.h"

#include "in_gamerules.h"
#include "in_attribute_system.h"
#include "in_playeranimstate_proxy.h"
#include "predicted_viewmodel.h"

#include "iservervehicle.h"
#include "in_utils.h"
#include "alienfx\LFX2.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ISoundEmitterSystemBase *soundemitterbase;
extern bool IsInCommentaryMode( void );
extern float DamageForce( const Vector &size, float damage );

CEntPlayer *CEntHost::GetPlayer() const
{
    return dynamic_cast<CEntPlayer *>(m_pHost);
}

CEntCharacter * CEntHost::GetCharacter() const
{
    return dynamic_cast<CEntCharacter *>(m_pHost);
}

bool CEntHost::IsIdle() const
{
    if ( GetBotController() ) {
        return GetBotController()->IsIdle();
    }

    // Si no nos estan atacando ni estamos en combate
    return (!IsUnderAttack() && !IsOnCombat());
}

bool CEntHost::IsAlerted() const
{
    if ( GetBotController() ) {
        return GetBotController()->IsAlerted();
    }

    // Nos estan atacando o estamos en combate!
    return (IsUnderAttack() || IsOnCombat());
}

bool CEntHost::IsUnderAttack() const
{
    return m_bUnderAttack;
}

bool CEntHost::IsOnCombat() const
{
    return m_bOnCombat;
}

bool CEntHost::IsOnGround() const
{
    return (GetHost()->GetFlags() & FL_ONGROUND) ? true : false;
}

bool CEntHost::IsOnGodMode() const
{
    return (GetHost()->GetFlags() & FL_GODMODE) ? true : false;
}

bool CEntHost::IsOnBuddhaMode() const
{
    return (GetHost()->m_debugOverlays & OVERLAY_BUDDHA_MODE) ? true : false;
}

int CEntHost::GetButtons() const
{
    if ( IsPlayer() ) {
        return GetPlayer()->m_nButtons;
    }
    else {
        return GetCharacter()->GetButtons();
    }
}

bool CEntHost::IsButtonPressing( int btn ) const
{
    if ( IsPlayer() ) {
        return (GetPlayer()->m_nButtons & btn) ? true : false;
    }
    else {
        return GetCharacter()->IsButtonPressing( btn );
    }
}

bool CEntHost::IsButtonPressed( int btn ) const
{
    if ( IsPlayer() ) {
        return (GetPlayer()->m_afButtonPressed & btn) ? true : false;
    }
    else {
        return GetCharacter()->IsButtonPressed( btn );
    }
}

bool CEntHost::IsButtonReleased( int btn ) const
{
    if ( IsPlayer() ) {
        return (GetPlayer()->m_afButtonReleased & btn) ? true : false;
    }
    else {
        return GetCharacter()->IsButtonReleased( btn );
    }
}

void CEntHost::DisableButtons( int nButtons )
{
    if ( IsPlayer() ) {
        GetPlayer()->m_afButtonDisabled |= nButtons;
    }
    else {
        GetCharacter()->DisableButtons( nButtons );
    }
}

void CEntHost::EnableButtons( int nButtons )
{
    if ( IsPlayer() ) {
        GetPlayer()->m_afButtonDisabled &= ~nButtons;
    }
    else {
        GetCharacter()->EnableButtons( nButtons );
    }
}

void CEntHost::EnableAllButtons()
{
    if ( IsPlayer() ) {
        GetPlayer()->m_afButtonDisabled = 0;
    }
    else {
        GetCharacter()->EnableAllButtons();
    }
}

void CEntHost::ForceButtons( int nButtons )
{
    if ( IsPlayer() ) {
        GetPlayer()->m_afButtonForced |= nButtons;
    }
    else {
        GetCharacter()->ForceButtons( nButtons );
    }
}

void CEntHost::UnforceButtons( int nButtons )
{
    if ( IsPlayer() ) {
        GetPlayer()->m_afButtonForced &= ~nButtons;
    }
    else {
        GetCharacter()->UnforceButtons( nButtons );
    }
}

void CEntHost::InitialSpawn()
{
    SetState( PLAYER_STATE_NONE );

    CreateAnimationSystem();
    CreateSenses();
}

void CEntHost::Spawn()
{
    m_iDejectedTimes = 0;
    m_flHelpProgress = 0.0f;
    m_flClimbingHold = 0.0f;
    m_bUnderAttack = false;
    m_bOnCombat = false;

    m_Components.PurgeAndDeleteElements();
    m_Attributes.PurgeAndDeleteElements();

    m_LastDamageTimer.Invalidate();

    if ( GetState() == PLAYER_STATE_NONE ) {
        SetState( PLAYER_STATE_WELCOME );
    }
    else {
        SetState( PLAYER_STATE_ACTIVE );
    }

    CreateComponents();
    CreateAttributes();

    if ( GetBotController() ) {
        GetBotController()->Spawn();
    }

    SetSquad( (CSquad *)NULL );
    SetStatus( PLAYER_STATUS_NONE );
}

void CEntHost::Precache()
{
    GetHost()->PrecacheModel( GetHostModel() );

    GetHost()->PrecacheScriptSound( "Player.Death" );
    GetHost()->PrecacheScriptSound( "Player.Pain" );
}

void CEntHost::PreThink()
{
    if ( IsAlive() && IsActive() ) {
        PreUpdateAttributes();
        UpdateMovementType();
        UpdateSpeed();
    }
}

void CEntHost::Think()
{
}

void CEntHost::PostThink()
{
    m_bIsBot = (GetBotController()) ? true : false;

    if ( IsAlive() && IsActive() ) {
        UpdateComponents();
        UpdateAttributes();

        m_bOnCombat = (m_CombatTimer.HasStarted() && m_CombatTimer.IsLessThen( 10.0f ));
        m_bUnderAttack = (m_UnderAttackTimer.HasStarted() && m_UnderAttackTimer.IsLessThen( 5.0f ));

        if ( m_bIsBot && !m_bOnCombat ) {
            m_bOnCombat = GetBotController()->IsAlerted() || GetBotController()->IsCombating();
        }
    }

    if ( GetAnimationSystem() ) {
        GetAnimationSystem()->Update();
    }
}

const char * CEntHost::GetHostModel()
{
    return "models/player.mdl";
}

void CEntHost::SetUpModel()
{
}

gender_t CEntHost::GetHostGender()
{
    return GENDER_NONE;
}

void CEntHost::CreateViewModel( int index )
{
    Assert( IsPlayer() );
    Assert( index >= 0 && index < MAX_VIEWMODELS );

    if ( !IsPlayer() )
        return;

    if ( GetPlayer()->GetViewModel( index ) )
        return;

    CPredictedViewModel *vm = (CPredictedViewModel *)CreateEntityByName( "predicted_viewmodel" );

    if ( vm ) {
        vm->SetAbsOrigin( GetAbsOrigin() );
        vm->SetOwner( GetPlayer() );
        vm->SetIndex( index );

        DispatchSpawn( vm );
        vm->FollowEntity( GetPlayer(), false );
        vm->AddEffects( EF_NODRAW );

        GetPlayer()->m_hViewModel.Set( index, vm );
    }
}

void CEntHost::SetUpHands()
{
}

void CEntHost::CreateHands( int index, int parent )
{
    Assert( IsPlayer() );
    Assert( index >= 1 && index < MAX_VIEWMODELS );

    if ( !IsPlayer() )
        return;

    if ( !GetPlayer()->GetViewModel( parent ) )
        return;

    if ( !GetHandsModel( index ) )
        return;

    CBaseViewModel *hands = GetPlayer()->GetViewModel( index );
    CBaseViewModel *vm = GetPlayer()->GetViewModel( parent );

    // Ya existe, actualizamos el modelo
    if ( hands ) {
        hands->SetModel( GetHandsModel( index ) );
        return;
    }

    hands = (CBaseViewModel *)CreateEntityByName( "predicted_viewmodel" );

    if ( hands ) {
        hands->SetAbsOrigin( GetAbsOrigin() );
        hands->SetOwner( GetPlayer() );
        hands->SetIndex( index );

        DispatchSpawn( hands );

        hands->SetModel( GetHandsModel( index ) );
        hands->SetOwnerViewModel( vm );
        hands->SetOwnerEntity( GetPlayer() );
        hands->AddEffects( EF_NODRAW );

        GetPlayer()->m_hViewModel.Set( index, hands );
    }
}

const char * CEntHost::GetHandsModel( int handsindex ) const
{
    return NULL;
}

CBaseEntity * CEntHost::GetEnemy() const
{
    if ( GetBotController() ) {
        GetBotController()->GetEnemy();
    }

    return NULL;
}

CBaseWeapon * CEntHost::GetActiveWeapon() const
{
    return dynamic_cast<CBaseWeapon *>( GetHost()->GetActiveWeapon() );
}

CBaseEntity * CEntHost::GiveNamedItem( const char * szName, int iSubType, bool removeIfNotCarried )
{
    // If I already own this type don't create one
    if ( GetHost()->Weapon_OwnsThisType( szName, iSubType ) )
        return NULL;

    CBaseEntity *pEntity = CreateEntityByName( szName );

    if ( !pEntity ) {
        Msg( "NULL Ent in GiveNamedItem!\n" );
        return NULL;
    }

    pEntity->SetLocalOrigin( GetLocalOrigin() );
    pEntity->AddSpawnFlags( SF_NORESPAWN );

    CBaseCombatWeapon *pWeapon = dynamic_cast<CBaseCombatWeapon *>(pEntity);

    DispatchSpawn( pEntity );

    if ( pWeapon ) {
        pWeapon->SetSubType( iSubType );
        GetHost()->Weapon_Equip( pWeapon );
    }
    else {
        if ( pEntity && !(pEntity->IsMarkedForDeletion()) ) {
            pEntity->Touch( GetHost() );
        }
    }

    return pEntity;
}

CEntComponent * CEntHost::GetComponent( int id )
{
    FOR_EACH_VEC( m_Components, key )
    {
        CEntComponent *pComponent = m_Components.Element( key );

        if ( !pComponent )
            continue;

        if ( pComponent->GetID() == id )
            return pComponent;
    }

    return NULL;
}

void CEntHost::AddComponent( int id )
{
    CEntComponent *pComponent = NULL;

    switch ( id ) {
        case PLAYER_COMPONENT_HEALTH:
            pComponent = new CPlayerHealthComponent();
            break;

        case PLAYER_COMPONENT_EFFECTS:
            pComponent = new CPlayerEffectsComponent();
            break;

        case PLAYER_COMPONENT_DEJECTED:
            pComponent = new CPlayerDejectedComponent();
            break;
    }

    AssertMsg( pComponent, "AddComponent(id) not handled!" );

    if ( !pComponent ) {
        return;
    }

    AddComponent( pComponent );
}

void CEntHost::AddComponent( CEntComponent * component )
{
    if ( GetComponent( pComponent->GetID() ) )
        return;

    pComponent->SetParent( this );
    pComponent->Init();

    m_Components.AddToTail( pComponent );
}

void CEntHost::CreateComponents()
{
}

void CEntHost::UpdateComponents()
{
    FOR_EACH_VEC( m_Components, it )
    {
        m_Components[it]->Update();
    }
}

void CEntHost::AddAttribute( const char * name )
{
    if ( GetAttribute( name ) )
        return;

    CAttribute *pAttribute = TheAttributeSystem->GetAttribute( name );
    AssertMsg1( pAttribute, "The attribute %s does not exist!", name );

    if ( !pAttribute ) {
        return;
    }

    AddAttribute( pAttribute );
}

void CEntHost::AddAttribute( CAttribute * attribute )
{
    if ( !attribute )
        return;

    m_Attributes.AddToTail( attribute );
}

void CEntHost::CreateAttributes()
{
}



void CEntHost::PreUpdateAttributes()
{
    FOR_EACH_VEC( m_Attributes, it )
    {
        m_Attributes[it]->PreUpdate();
    }
}

void CEntHost::UpdateAttributes()
{
    FOR_EACH_VEC( m_Attributes, it )
    {
        m_Attributes[it]->Update();
    }
}

CAttribute * CEntHost::GetAttribute( const char * name )
{
    FOR_EACH_VEC( m_Attributes, it )
    {
        CAttribute *pAttribute = m_Attributes[it];

        if ( FStrEq( pAttribute->GetID(), name ) )
            return pAttribute;
    }

    return NULL;
}

void CEntHost::AddAttributeModifier( const char * name )
{
    AttributeInfo info;

    // El modificador no existe
    if ( !TheAttributeSystem->GetModifierInfo( name, info ) )
        return;

    CAttribute *pAttribute = GetAttribute( info.affects );

    if ( !pAttribute )
        return;

    pAttribute->AddModifier( info );
}

void CEntHost::SetSquad( CSquad * pSquad )
{
    if ( m_pSquad ) {
        m_pSquad->RemoveMember( this );
    }

    m_pSquad = pSquad;

    if ( m_pSquad ) {
        m_pSquad->AddMember( this );
    }
}

void CEntHost::SetSquad( const char * name )
{
    CSquad *pSquad = TheSquads->GetOrCreateSquad( name );
    SetSquad( pSquad );
}

void CEntHost::OnNewLeader( CPlayer * pMember )
{
}

void CEntHost::OnMemberTakeDamage( CPlayer * pMember, const CTakeDamageInfo & info )
{
    if ( GetBotController() ) {
        GetBotController()->OnMemberTakeDamage( pMember, info );
    }
}

void CEntHost::OnMemberDeath( CPlayer * pMember, const CTakeDamageInfo & info )
{
    if ( GetBotController() ) {
        GetBotController()->OnMemberDeath( pMember, info );
    }
}

void CEntHost::OnMemberReportEnemy( CPlayer * pMember, CBaseEntity * pEnemy )
{
    if ( GetBotController() ) {
        GetBotController()->OnMemberReportEnemy( pMember, pEnemy );
    }
}

void CEntHost::CreateSenses()
{
    m_pSenses = new CAI_Senses;
    m_pSenses->SetOuter( GetHost() );
}

void CEntHost::SetDistLook( float flDistLook )
{
    if ( GetSenses() ) {
        GetSenses()->SetDistLook( flDistLook );
    }
}

int CEntHost::GetSoundInterests()
{
    return SOUND_DANGER | SOUND_COMBAT | SOUND_PLAYER | SOUND_CARCASS | SOUND_MEAT | SOUND_GARBAGE;
}

int CEntHost::GetSoundPriority( CSound * pSound )
{
    if ( pSound->IsSoundType( SOUND_COMBAT ) ) {
        return SOUND_PRIORITY_HIGH;
    }

    if ( pSound->IsSoundType( SOUND_DANGER ) ) {
        if ( pSound->IsSoundType( SOUND_CONTEXT_FROM_SNIPER | SOUND_CONTEXT_EXPLOSION ) ) {
            return SOUND_PRIORITY_HIGHEST;
        }
        else if ( pSound->IsSoundType( SOUND_CONTEXT_GUNFIRE | SOUND_CONTEXT_BULLET_IMPACT ) ) {
            return SOUND_PRIORITY_VERY_HIGH;
        }

        return SOUND_PRIORITY_HIGH;
    }

    if ( pSound->IsSoundType( SOUND_CARCASS | SOUND_MEAT | SOUND_GARBAGE ) ) {
        return SOUND_PRIORITY_VERY_LOW;
    }

    return SOUND_PRIORITY_NORMAL;
}

bool CEntHost::QueryHearSound( CSound * pSound )
{
    CBaseEntity *pOwner = pSound->m_hOwner.Get();

    if ( pOwner == GetHost() )
        return false;

    if ( pSound->IsSoundType( SOUND_PLAYER ) && !pOwner ) {
        return false;
    }

    if ( pSound->IsSoundType( SOUND_CONTEXT_PLAYER_ALLIES_ONLY ) ) {
        if ( Classify() != CLASS_PLAYER_ALLY && Classify() != CLASS_PLAYER_ALLY_VITAL ) {
            return false;
        }
    }

    if ( pOwner ) {
        // Solo escuchemos sonidos provocados por nuestros aliados si son de combate.
        if ( TheGameRules->PlayerRelationship( GetHost(), pOwner ) == GR_ALLY ) {
            if ( pSound->IsSoundType( SOUND_COMBAT ) && !pSound->IsSoundType( SOUND_CONTEXT_GUNFIRE ) ) {
                return true;
            }

            return false;
        }
    }

    if ( ShouldIgnoreSound( pSound ) ) {
        return false;
    }

    return true;
}

bool CEntHost::QuerySeeEntity( CBaseEntity * pEntity, bool bOnlyHateOrFearIfNPC )
{
    if ( bOnlyHateOrFearIfNPC ) {
        Disposition_t disposition = GetHost()->IRelationType( pEntity );
        return (disposition == D_HT || disposition == D_FR);
    }

    return true;
}

void CEntHost::OnLooked( int iDistance )
{
    if ( GetBotController() ) {
        GetBotController()->OnLooked( iDistance );
    }
}

void CEntHost::OnListened()
{
    if ( GetBotController() ) {
        GetBotController()->OnListened();
    }
}

CSound * CEntHost::GetLoudestSoundOfType( int iType )
{
    return CSoundEnt::GetLoudestSoundOfType( iType, EarPosition() );
}

bool CEntHost::SoundIsVisible( CSound * pSound )
{
    return IsAbleToSee( pSound->GetSoundReactOrigin(), CBaseCombatCharacter::DISREGARD_FOV );
}

CSound * CEntHost::GetBestSound( int validTypes )
{
    CSound *pResult = GetSenses()->GetClosestSound( false, validTypes );

    if ( pResult == NULL ) {
        DevMsg( "NULL Return from GetBestSound\n" );
    }

    return pResult;
}

CSound * CEntHost::GetBestScent( void )
{
    CSound *pResult = GetSenses()->GetClosestSound( true );

    if ( pResult == NULL ) {
        DevMsg( "NULL Return from GetBestScent\n" );
    }

    return pResult;
}

void CEntHost::HandleAnimEvent( animevent_t * event )
{
    if ( event->Event() == AE_PLAYER_FOOTSTEP_LEFT || event->Event() == AE_PLAYER_FOOTSTEP_RIGHT ) {
        FootstepSound();
        return;
    }
}

void CEntHost::DoAnimationEvent( PlayerAnimEvent_t nEvent, int data, bool bPredicted )
{
    if ( !GetAnimationSystem() )
        return;

    // Hemos disparado, entramos en combate!
    // @TODO: Si el jugador no tiene animación de disparo esto
    // jamás será llamado, necesitamos un mejor lugar.
    if ( nEvent == PLAYERANIMEVENT_ATTACK_PRIMARY ) {
        m_CombatTimer.Start();
        AddAttributeModifier( "stress_firegun" );
    }

    // Procesamos la animación en el servidor
    // y después la enviamos al cliente con una entidad temporal.
    GetAnimationSystem()->DoAnimationEvent( nEvent, data );
    SendPlayerAnimation( GetHost(), nEvent, data, bPredicted );
}

void CEntHost::FootstepSound()
{
    float flVolume = (IsSprinting()) ? RandomFloat( 300.0f, 400.0f ) : RandomFloat( 100.0f, 200.0f );

    if ( IsSneaking() ) {
        flVolume = RandomFloat( 30.0f, 60.0f );
    }

    CSoundEnt::InsertSound( SOUND_PLAYER | SOUND_CONTEXT_FOOTSTEP, GetAbsOrigin(), flVolume, 0.2f, GetHost(), SOUNDENT_CHANNEL_FOOTSTEP );
}

void CEntHost::PainSound( const CTakeDamageInfo &info )
{
    GetHost()->EmitSound( "Player.Pain" );
}

void CEntHost::DeathSound( const CTakeDamageInfo &info )
{
    if ( !TheGameRules->FCanPlayDeathSound( info ) )
        return;

    GetHost()->EmitSound( "Player.Death" );
    CSoundEnt::InsertSound( SOUND_CARCASS, GetAbsOrigin(), 1024.0f, 5.0f, GetHost() );
}

void CEntHost::SetFlashlightEnabled( bool enabled )
{
    m_bFlashlightEnabled = enabled;
}

int CEntHost::FlashlightIsOn()
{
    return IsEffectActive( EF_DIMLIGHT );
}

void CEntHost::FlashlightTurnOn()
{
    if ( FlashlightIsOn() )
        return;

    if ( !TheGameRules->FAllowFlashlight() || !m_bFlashlightEnabled )
        return;

    AddEffects( EF_DIMLIGHT );
    GetHost()->EmitSound( "Player.FlashlightOn" );
}

void CEntHost::FlashlightTurnOff()
{
    if ( !FlashlightIsOn() || !m_bFlashlightEnabled )
        return;

    RemoveEffects( EF_DIMLIGHT );
    GetHost()->EmitSound( "Player.FlashlightOff" );
}

bool CEntHost::IsIlluminatedByFlashlight( CBaseEntity * pEntity, float * flReturnDot )
{
    return false;
}

void CEntHost::DamageEffect( const CTakeDamageInfo & info )
{
    float flDamage = info.GetDamage();
    int fDamageType = info.GetDamageType();

    color32 red = { 128,0,0,150 };
    color32 blue = { 0,0,128,100 };
    color32 yellow = { 215,223,1,100 };
    color32 green = { 33,97,0,100 };
    color32 white = { 255,255,255,20 };

    color32 backgroundColor = red;
    int fxColor = LFX_RED | LFX_FULL_BRIGHTNESS;
    int flags = FFADE_IN;
    bool bScreenFade = true;

    // Ahogo
    if ( fDamageType & DMG_DROWN ) {
        backgroundColor = blue;
        fxColor = LFX_BLUE | LFX_FULL_BRIGHTNESS;
    }

    // Rasguño
    else if ( fDamageType & DMG_SLASH ) {
        // If slash damage shoot some blood
        SpawnBlood( EyePosition(), g_vecAttackDir, BloodColor(), flDamage );
    }

    // Plasma
    else if ( fDamageType & DMG_PLASMA ) {
        backgroundColor = blue;
        flags = FFADE_MODULATE;
        fxColor = LFX_BLUE | LFX_FULL_BRIGHTNESS;

        // Very small screen shake
        if ( IsAlive() ) {
            GetHost()->ViewPunch( QAngle( random->RandomInt( -0.1, 0.1 ), random->RandomInt( -0.1, 0.1 ), random->RandomInt( -0.1, 0.1 ) ) );
        }

        GetHost()->EmitSound( "Player.PlasmaDamage" );
    }

    // Choque electrico
    else if ( fDamageType & (DMG_SHOCK | DMG_ENERGYBEAM) ) {
        backgroundColor = yellow;
        flags = FFADE_MODULATE;
        fxColor = LFX_YELLOW | LFX_FULL_BRIGHTNESS;

        g_pEffects->Sparks( info.GetDamagePosition(), 2, 2 );
        UTIL_Smoke( info.GetDamagePosition(), random->RandomInt( 10, 15 ), 10 );
    }

    // Veneno
    else if ( fDamageType & (DMG_PARALYZE | DMG_NERVEGAS | DMG_POISON | DMG_RADIATION | DMG_ACID) ) {
        backgroundColor = green;
        fxColor = LFX_GREEN | LFX_FULL_BRIGHTNESS;
    }

    // Sonic
    else if ( fDamageType & DMG_SONIC ) {
        GetHost()->EmitSound( "Player.SonicDamage" );
    }

    // Bala
    else if ( fDamageType & DMG_BULLET ) {
        GetHost()->EmitSound( "Flesh.BulletImpact" );
    }

    // Explosión
    else if ( fDamageType & DMG_BLAST ) {
        if ( IsAlive() ) {
            GetPlayer()->OnDamagedByExplosion( info );
        }
    }

    if ( bScreenFade && IsAlive() ) {
        if ( info.GetDamage() <= 0 ) {
            backgroundColor = white;
        }

#ifdef APOCALYPSE
        if ( IsDejected() ) {
            backgroundColor.a = 50;
        }
#endif

        UTIL_ScreenFade( GetHost(), backgroundColor, 0.4f, 0.1f, flags );
        Utils::AlienFX_SetColor( GetPlayer(), LFX_ALL, fxColor, 0.5f );
    }

    CSoundEnt::InsertSound( SOUND_PLAYER | SOUND_COMBAT | SOUND_CONTEXT_INJURY, GetAbsOrigin(), PLAYER_SOUND_RADIUS, 0.3f, this, SOUNDENT_CHANNEL_INJURY );
}

bool CEntHost::CanTakeDamage( const CTakeDamageInfo & info )
{
    CTakeDamageInfo dinfo = info;
    IServerVehicle *pVehicle = GetVehicle();

    if ( pVehicle ) {
        if ( !pVehicle->PassengerShouldReceiveDamage( dinfo ) )
            return false;
    }

    if ( IsInCommentaryMode() ) {
        if ( !GetPlayer()->ShouldTakeDamageInCommentaryMode( info ) )
            return false;
    }

    if ( IsOnGodMode() )
        return false;

    if ( info.GetDamage() <= 0.0f )
        return false;

    if ( GetHost()->m_takedamage != DAMAGE_YES )
        return false;

    if ( !TheGameRules->FPlayerCanTakeDamage( GetPlayer(), info.GetAttacker(), info ) )
        return false;

    return true;
}

int CEntHost::OnTakeDamage( const CTakeDamageInfo & inputInfo )
{
    CTakeDamageInfo info = inputInfo;

    if ( !CanTakeDamage( info ) )
        return 0;

    int response = 1;

    // Información del último daño recibido
    // Los Bots necesitan esta información aunque al final no recibamos daño
    m_LastDamage = info;
    m_LastDamageTimer.Start();

    info.CopyDamageToBaseDamage();

    // Estas funciones deciden si debemos aplicar la reducción en la vida y los efectos al recibir daño
    switch ( m_lifeState ) {
        case LIFE_ALIVE:
        {
            response = OnTakeDamage_Alive( info );
            break;
        }

        case LIFE_DYING:
        {
            response = OnTakeDamage_Dying( info );
            break;
        }

        case LIFE_DEAD:
        {
            response = OnTakeDamage_Dead( info );
            break;
        }

        default:
        {
            Assert( 0 );
            false;
        }
    }

    if ( response == 0 )
        return 0;

    if ( IsAlive() ) {
        ApplyDamage( info );
    }

    return 1;
}

int CEntHost::OnTakeDamage_Alive( CTakeDamageInfo & info )
{
    if ( GetBotController() ) {
        GetBotController()->OnTakeDamage( info );
    }

    if ( GetSquad() ) {
        GetSquad()->ReportTakeDamage( GetHost(), info );
    }

    //TheGameRules->AdjustPlayerDamageTaken( GetPlayer(), info );

    if ( TheGameRules->Damage_MakeSlow( info ) ) {
        m_SlowDamageTimer.Start();
    }

    m_UnderAttackTimer.Start();
    m_CombatTimer.Start();

    if ( TheGameRules->FPlayerCanDejected( GetPlayer(), info ) ) {
        int damage = (int)round( info.GetDamage() );
        int newHealth = GetHost()->GetHealth() - damage;

        if ( newHealth <= 0 ) {
            if ( GetStatus() == PLAYER_STATUS_CLIMBING ) {
                SetStatus( PLAYER_STATUS_FALLING );
            }
            else {
                SetStatus( PLAYER_STATUS_DEJECTED );
            }

            return 0;
        }
    }

    if ( info.GetAttacker() && (info.GetDamageType() & DMG_BULLET) == 0 ) {
        Vector vecDir = info.GetAttacker()->WorldSpaceCenter() - Vector( 0, 0, 10 ) - WorldSpaceCenter();
        VectorNormalize( vecDir );

        if ( (GetMoveType() == MOVETYPE_WALK) && (!info.GetAttacker()->IsSolidFlagSet( FSOLID_TRIGGER )) ) {
            Vector force = vecDir * -DamageForce( WorldAlignSize(), info.GetBaseDamage() );
            if ( force.z > 250.0f ) {
                force.z = 250.0f;
            }

            ApplyAbsVelocityImpulse( force );
        }
    }

    return 1;
}

void CEntHost::ApplyDamage( const CTakeDamageInfo & info )
{
    switch ( GetHost()->LastHitGroup() ) {
        case HITGROUP_GENERIC:
            DoAnimationEvent( PLAYERANIMEVENT_FLINCH );
            break;

        case HITGROUP_HEAD:
            DoAnimationEvent( PLAYERANIMEVENT_FLINCH_HEAD );
            break;

        case HITGROUP_CHEST:
        case HITGROUP_STOMACH:
            DoAnimationEvent( PLAYERANIMEVENT_FLINCH_CHEST );
            break;

        case HITGROUP_LEFTARM:
            DoAnimationEvent( PLAYERANIMEVENT_FLINCH_LEFTARM );
            break;

        case HITGROUP_RIGHTARM:
            DoAnimationEvent( PLAYERANIMEVENT_FLINCH_RIGHTARM );
            break;

        case HITGROUP_LEFTLEG:
            DoAnimationEvent( PLAYERANIMEVENT_FLINCH_LEFTLEG );
            break;

        case HITGROUP_RIGHTLEG:
            DoAnimationEvent( PLAYERANIMEVENT_FLINCH_RIGHTLEG );
            break;
    }

    /*if ( info.GetDamage() > 0 && info.GetAttacker() ) {
        if ( info.GetAttacker()->IsPlayer() )
            DebugAddMessage( "Taking %.2f damage from %s", info.GetDamage(), info.GetAttacker()->GetPlayerName() );
        else
            DebugAddMessage( "Taking %.2f damage from %s", info.GetDamage(), info.GetAttacker()->GetClassname() );
    }*/

    if ( IsPlayer() ) {
        if ( GetPlayer()->IsNetClient() && !GetPlayer()->ShouldThrottleUserMessage( "Damage" ) ) {
            CUserAndObserversRecipientFilter user( GetPlayer() );
            user.MakeReliable();

            UserMessageBegin( user, "Damage" );
            WRITE_BYTE( info.GetBaseDamage() );
            WRITE_FLOAT( info.GetDamagePosition().x );
            WRITE_FLOAT( info.GetDamagePosition().y );
            WRITE_FLOAT( info.GetDamagePosition().z );
            MessageEnd();
        }
    }

    DamageEffect( info );

    // grab the vector of the incoming attack. ( pretend that the inflictor is a little lower than it really is, so the body will tend to fly upward a bit).
    Vector vecDir = vec3_origin;

    if ( info.GetInflictor() ) {
        vecDir = info.GetInflictor()->WorldSpaceCenter() - Vector( 0, 0, 10 ) - WorldSpaceCenter();
        VectorNormalize( vecDir );
    }

    g_vecAttackDir = vecDir;

    // add to the damage total for clients, which will be sent as a single
    // message at the end of the frame
    // todo: remove after combining shotgun blasts?
    if ( info.GetInflictor() && info.GetInflictor()->edict() ) {
        GetPlayer()->m_DmgOrigin = info.GetInflictor()->GetAbsOrigin();
    }

    GetPlayer()->m_DmgTake += (int)info.GetDamage();

    // Reset damage time countdown for each type of time based damage player just sustained
    for ( int i = 0; i < CDMG_TIMEBASED; i++ ) {
        // Make sure the damage type is really time-based.
        // This is kind of hacky but necessary until we setup DamageType as an enum.
        int iDamage = (DMG_PARALYZE << i);

        if ( (info.GetDamageType() & iDamage) && g_pGameRules->Damage_IsTimeBased( iDamage ) )
            GetPlayer()->m_rgbTimeBasedDamage[i] = 0;
    }

    GetPlayer()->m_bitsDamageType |= info.GetDamageType(); // Save this so we can report it to the client
    GetPlayer()->m_bitsHUDDamage = -1;  // make sure the damage bits get resent

    // Separate the fractional amount of damage from the whole
    float flFractionalDamage = info.GetDamage() - floor( info.GetDamage() );
    float flIntegerDamage = info.GetDamage() - flFractionalDamage;

    // Add fractional damage to the accumulator
    GetPlayer()->m_flDamageAccumulator += flFractionalDamage;

    // If the accumulator is holding a full point of damage, move that point
    // of damage into the damage we're about to inflict.
    if ( GetPlayer()->m_flDamageAccumulator >= 1.0 ) {
        flIntegerDamage += 1.0;
        GetPlayer()->m_flDamageAccumulator -= 1.0;
    }

    if ( flIntegerDamage <= 0 )
        return;

    PainSound( info );

    // Aplicamos la reducción de vida
    GetPlayer()->m_iHealth = GetPlayer()->GetHealth() - flIntegerDamage;
    GetPlayer()->m_iHealth = clamp( GetPlayer()->m_iHealth, 0, GetMaxHealth() );

    // fire global game event
    if ( IsPlayer() ) {
        IGameEvent * event = gameeventmanager->CreateEvent( "player_hurt", false );

        if ( event ) {
            event->SetInt( "userid", GetPlayer()->GetUserID() );
            event->SetInt( "health", GetPlayer()->GetHealth() );
            event->SetInt( "priority", 5 );	// HLTV event priority, not transmitted

            if ( info.GetAttacker() && info.GetAttacker()->IsPlayer() ) {
                CBasePlayer *player = ToBasePlayer( info.GetAttacker() );
                event->SetInt( "attacker", player->GetUserID() ); // hurt by other player
            }
            else {
                event->SetInt( "attacker", 0 ); // hurt by "world"
            }

            gameeventmanager->FireEvent( event );
        }
    }

    if ( m_iHealth < GetMaxHealth() ) {
        m_fTimeLastHurt = gpGlobals->curtime;
    }

    if ( m_iHealth == 0 ) {
        if ( InBuddhaMode() ) {
            m_iHealth = 1;
        }
        else {
            Event_Killed( info );
        }
    }
}
