//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#include "cbase.h"
#include "in_player.h"

#include "nav.h"
#include "nav_area.h"

#include "bots\bot.h"
#include "bots\squad_manager.h"

#include "in_playeranimstate_proxy.h"
#include "in_gamerules.h"
#include "in_utils.h"

#include "mp_shareddefs.h"
#include "in_buttons.h"

#include "in_player_components_basic.h"
#include "in_attribute_system.h"

#include "director.h"

#include "obstacle_pushaway.h"
#include "predicted_viewmodel.h"
#include "physics_prop_ragdoll.h"
#include "shake.h"
#include "rumble_shared.h"
#include "soundent.h"
#include "team.h"
#include "fmtstr.h"
#include "props.h"

#include "engine/IEngineSound.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"

#include "vehicle_base.h"
#include "effects.h"
#include "IEffects.h"
#include "eventqueue.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
// Logging System
// Only for the current file, this should never be in a header.
//================================================================================

#define Msg(...) Log_Msg(LOG_PLAYER, __VA_ARGS__)
#define Warning(...) Log_Warning(LOG_PLAYER, __VA_ARGS__)

//================================================================================
//================================================================================

extern ISoundEmitterSystemBase *soundemitterbase;
extern bool IsInCommentaryMode(void);
extern float DamageForce(const Vector &size, float damage);

#define PUSHAWAY_THINK_CONTEXT	"PlayerPushawayThink"

//================================================================================
// Commands
//================================================================================

DECLARE_SERVER_CMD(sv_player_model, "models/player.mdl", "Default Player Model")
DECLARE_CHEAT_CMD(sv_player_debug, "0", "Shows information of the specified player")

// Lights
// This is found on the server since it indicates whether dynamic lights should be used (which are visible to all players)
DECLARE_NOTIFY_CMD(sv_muzzleflashlight_realistic, "1", "The muzzleflash is a dynamic light")
DECLARE_NOTIFY_CMD(sv_flashlight_realistic, "1", "The flashlight is a dynamic light")
DECLARE_NOTIFY_CMD(sv_flashlight_weapon, "1", "The flashlight must come from the weapon")

//================================================================================
// Data and Network
//================================================================================

extern void SendProxy_Origin(const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID);

// Local Players
BEGIN_SEND_TABLE_NOBASE(CPlayer, DT_BaseInLocalPlayerExclusive)
// send a hi-res origin to the local player for use in prediction
SendPropVector(SENDINFO(m_vecOrigin), -1, SPROP_NOSCALE | SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, SendProxy_Origin),
END_SEND_TABLE()

// Other Players
BEGIN_SEND_TABLE_NOBASE(CPlayer, DT_InBaseNonLocalPlayerExclusive)
// send a low-res origin to other players
SendPropVector(SENDINFO(m_vecOrigin), -1, SPROP_COORD_MP_LOWPRECISION | SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, SendProxy_Origin),
END_SEND_TABLE()

IMPLEMENT_SERVERCLASS_ST(CPlayer, DT_BaseInPlayer)
    // Excluimos el envio de información del sistema de animaciones.
    // Este será procesado en el cliente por el [GetAnimationSystem] 
    SendPropExclude("DT_BaseEntity", "m_angRotation"),
    SendPropExclude("DT_BaseEntity", "m_vecOrigin"),
    SendPropExclude("DT_BaseAnimating", "m_flPoseParameter"),
    SendPropExclude("DT_BaseAnimating", "m_flPlaybackRate"),
    SendPropExclude("DT_BaseAnimating", "m_nSequence"),
    SendPropExclude("DT_BaseAnimatingOverlay", "overlay_vars"),
    SendPropExclude("DT_ServerAnimationData", "m_flCycle"),
    SendPropExclude("DT_AnimTimeMustBeFirst", "m_flAnimTime"),

    SendPropBool(SENDINFO(m_bMovementDisabled)),
    SendPropBool(SENDINFO(m_bAimingDisabled)),
    SendPropInt(SENDINFO(m_iButtonsDisabled)),
    SendPropInt(SENDINFO(m_iButtonsForced)),

    SendPropInt(SENDINFO(m_iOldHealth)),

    SendPropBool(SENDINFO(m_bFlashlightEnabled)),
    SendPropBool(SENDINFO(m_bSprinting)),
    SendPropBool(SENDINFO(m_bSneaking)),

    SendPropBool(SENDINFO(m_bOnCombat)),
    SendPropBool(SENDINFO(m_bUnderAttack)),

    SendPropInt(SENDINFO(m_iPlayerStatus)),
    SendPropInt(SENDINFO(m_iPlayerState)),
    SendPropInt(SENDINFO(m_iPlayerClass)),

    SendPropInt(SENDINFO(m_iDejectedTimes)),

    SendPropFloat(SENDINFO(m_flHelpProgress), 32, 0, 0.0f, 100.0f),
    SendPropFloat(SENDINFO(m_flClimbingHold), 32, 0, 0.0f, 100.0f),

    SendPropInt(SENDINFO(m_iEyeAngleZ)),
    SendPropBool(SENDINFO(m_bIsBot)),

    SendPropAngle(SENDINFO_VECTORELEM(m_angEyeAngles, 0), 11, SPROP_CHANGES_OFTEN),
    SendPropAngle(SENDINFO_VECTORELEM(m_angEyeAngles, 1), 11, SPROP_CHANGES_OFTEN),

    // Data that only gets sent to the local player.
    SendPropDataTable("localdata", 0, &REFERENCE_SEND_TABLE(DT_BaseInLocalPlayerExclusive), SendProxy_SendLocalDataTable),

    // Data that gets sent to all other players
    SendPropDataTable("nonlocaldata", 0, &REFERENCE_SEND_TABLE(DT_InBaseNonLocalPlayerExclusive), SendProxy_SendNonLocalDataTable),
END_SEND_TABLE()

BEGIN_DATADESC(CPlayer)
    DEFINE_THINKFUNC(PushawayThink)
END_DATADESC()

//================================================================================
// Constructor
//================================================================================
CPlayer::CPlayer()
{
    // Animations are processed on the client side
    UseClientSideAnimation();

    m_angEyeAngles.Init();
    m_pSenses = NULL;
}

//================================================================================
// Destructor
//================================================================================
CPlayer::~CPlayer()
{
    if ( GetAnimationSystem() ) {
        delete m_pAnimationSystem;
        m_pAnimationSystem = NULL;
    }

    if ( GetSenses() ) {
        delete m_pSenses;
        m_pSenses = NULL;
    }
}

//================================================================================
// Returns if the player is idle
//================================================================================
bool CPlayer::IsIdle() const
{
    if ( GetBotController() ) {
        return GetBotController()->IsIdle();
    }

    return (!IsUnderAttack() && !IsOnCombat());
}

//================================================================================
// Returns if the player is alerted
//================================================================================
bool CPlayer::IsAlerted() const
{
    if ( GetBotController() ) {
        return GetBotController()->IsAlerted();
    }

    return (IsUnderAttack() || IsOnCombat());
}

//================================================================================
// Set the AI that will control the player
//================================================================================
void CPlayer::SetBotController(IBot *pBot)
{
    if ( m_pBotController ) {
        delete m_pBotController;
    }

    m_pBotController = pBot;
}

//================================================================================
// It is called when this player is required to be controlled by the AI
// That is: when it is a bot or the player wants to rest and give the control to the CPU
//================================================================================
void CPlayer::SetUpBot()
{
    SetBotController(new CBot(this));
}

//================================================================================
//================================================================================
int CPlayer::UpdateTransmitState()
{
    return BaseClass::UpdateTransmitState();
}

//================================================================================
// Initial Spawn, is called once when accessing the server.
//================================================================================
void CPlayer::InitialSpawn()
{
    BaseClass::InitialSpawn();

    m_takedamage = DAMAGE_YES;
    pl.deadflag = false;
    m_lifeState = LIFE_ALIVE;

    CreateExpresser();
    CreateAnimationSystem();
    CreateSenses();

    SetPlayerState(PLAYER_STATE_NONE);
}

//================================================================================
// Spawn
//================================================================================
// Note: It is called 2 times in players who have just entered:
// 1. When connecting, state should be PLAYER_STATE_NONE
// 2. After selecting team/class
//================================================================================
void CPlayer::Spawn()
{
    BaseClass::Spawn();

    m_nButtons = 0;
    m_iRespawnFrames = 0;
    m_iCurrentConcept = 0;
    m_bPlayingDeathAnim = false;
    m_nRagdoll = NULL;
    m_InjectedCommand = NULL;
    m_bMovementDisabled = false;
    m_bAimingDisabled = false;
    m_iOldHealth = -1;
    m_bSprinting = false;
    m_bSneaking = false;
    m_bOnCombat = false;
    m_bUnderAttack = false;
    m_iDejectedTimes = 0;
    m_flHelpProgress = 0.0f;
    m_iEyeAngleZ = 0;
    m_Local.m_iHideHUD = 0;

    m_SlowDamageTimer.Invalidate();
    m_UnderAttackTimer.Invalidate();
    m_CombatTimer.Invalidate();
    m_RaiseHelpTimer.Invalidate();
    m_LastDamageTimer.Invalidate();

    AddFlag(FL_AIMTARGET);

    SetSquad((CSquad *)NULL);
    SetPlayerStatus(PLAYER_STATUS_NONE);

    if ( GetPlayerState() == PLAYER_STATE_NONE ) {
        SetPlayerState(PLAYER_STATE_WELCOME);
    }
    else if ( GetTeamNumber() != TEAM_UNASSIGNED ) {
        SetPlayerState(PLAYER_STATE_ACTIVE);
    }

    if ( GetBotController() ) {
        GetBotController()->Spawn();
    }

    SetThink(&CPlayer::PlayerThink);
    SetNextThink(gpGlobals->curtime + 0.1f);
}

//================================================================================
//================================================================================
void CPlayer::Connected()
{
}

//================================================================================
//================================================================================
void CPlayer::PostConstructor(const char *szClassname)
{
    BaseClass::PostConstructor(szClassname);
}

//================================================================================
// Precache important assets
//================================================================================
void CPlayer::Precache()
{
    BaseClass::Precache();

    // Model
    PrecacheModel("models/player.mdl");
    PrecacheModel(GetPlayerModel());

    // Lights
    PrecacheModel("sprites/light_glow01.vmt");
    PrecacheModel("sprites/spotlight01_proxyfade.vmt");
    PrecacheModel("sprites/glow_test02.vmt");
    PrecacheModel("sprites/light_glow03.vmt");
    PrecacheModel("sprites/glow01.vmt");

    // Default sounds
    PrecacheScriptSound("Player.FlashlightOn");
    PrecacheScriptSound("Player.FlashlightOff");
    PrecacheScriptSound("Player.Death");
    PrecacheScriptSound("Player.Pain");
}

//================================================================================
// It is called before processing the movement and normal thinking
//================================================================================
void CPlayer::PreThink()
{
    // In a Vehicle
    if ( IsInAVehicle() ) {
        UpdateClientData();
        CheckTimeBasedDamage();

        CheckSuitUpdate();
        WaterMove();

        return;
    }

    QAngle vOldAngles = GetLocalAngles();
    QAngle vTempAngles = GetLocalAngles();
    vTempAngles = EyeAngles();

    if ( vTempAngles[PITCH] > 180.0f ) {
        vTempAngles[PITCH] -= 360.0f;
    }

    SetLocalAngles(vTempAngles);

    if ( IsAlive() && IsActive() ) {
        PreUpdateAttributes();
        UpdateMovementType();
        UpdateSpeed();
    }

    BaseClass::PreThink();
    SetLocalAngles(vOldAngles);
}

//================================================================================
// It is called after processing the movement and normal thinking
//================================================================================
void CPlayer::PostThink()
{
    BaseClass::PostThink();

    // Are we being controlled by the AI?
    // This will serve the client not to allow player inputs
    m_bIsBot = (GetBotController()) ? true : false;

    if ( IsAlive() && IsActive() ) {
        UpdateComponents();
        UpdateAttributes();

        m_bOnCombat = (m_CombatTimer.HasStarted() && m_CombatTimer.IsLessThen(10.0f));
        m_bUnderAttack = (m_UnderAttackTimer.HasStarted() && m_UnderAttackTimer.IsLessThen(5.0f));

        if ( m_bIsBot && !m_bOnCombat ) {
            m_bOnCombat = GetBotController()->IsAlerted() || GetBotController()->IsCombating();
        }

        ProcessSceneEvents();
        DoBodyLean();
    }

    // We update the animations
    if ( GetAnimationSystem() ) {
        GetAnimationSystem()->Update();
    }

    m_angEyeAngles = EyeAngles();

    QAngle angles = GetLocalAngles();
    angles[PITCH] = 0;
    SetLocalAngles(angles);

    DebugDisplay();
}

//================================================================================
//================================================================================
void CPlayer::PushawayThink()
{
    PerformObstaclePushaway(this);
    SetNextThink(gpGlobals->curtime + PUSHAWAY_THINK_INTERVAL, PUSHAWAY_THINK_CONTEXT);
}

//================================================================================
//================================================================================
CBaseEntity *CPlayer::GetEnemy()
{
    if ( GetBotController() ) {
        return GetBotController()->GetEnemy();
    }

    return BaseClass::GetEnemy();
}

//================================================================================
//================================================================================
CBaseEntity *CPlayer::GetEnemy() const
{
    if ( GetBotController() ) {
        return GetBotController()->GetEnemy();
    }

    return BaseClass::GetEnemy();
}

//================================================================================
// Returns the model the player will use
//================================================================================
const char *CPlayer::GetPlayerModel()
{
    return sv_player_model.GetString();
}

//================================================================================
// TODO: Remove
//================================================================================
const char *CPlayer::GetPlayerType()
{
    return "Default";
}

//================================================================================
// TODO: Remove
//================================================================================
gender_t CPlayer::GetPlayerGender()
{
    return GENDER_NONE;
}

//================================================================================
// TODO
//================================================================================
CInRagdoll *CPlayer::GetRagdoll()
{
    return dynamic_cast< CInRagdoll *>(m_nRagdoll.Get());
}

//================================================================================
// Transforms the player into a corpse (On client side)
//================================================================================
bool CPlayer::BecomeRagdollOnClient(const Vector &force)
{
    if ( !CanBecomeRagdoll() )
        return false;

    // Have to do this dance because m_vecForce is a network vector
    // and can't be sent to ClampRagdollForce as a Vector *
    Vector vecClampedForce;
    ClampRagdollForce(force, &vecClampedForce);
    m_vecForce = vecClampedForce;

    // Remove our flame entity if it's attached to us
    /*CEntityFlame *pFireChild = dynamic_cast<CEntityFlame *>( GetEffectEntity() );
    if ( pFireChild )
    {
    pFireChild->SetThink( &CBaseEntity::SUB_Remove );
    pFireChild->SetNextThink( gpGlobals->curtime + 0.1f );
    }*/

    AddFlag(FL_TRANSRAGDOLL);
    m_bClientSideRagdoll = true;
    return true;
}

//================================================================================
// Transforms the player into a corpse
//================================================================================
void CPlayer::CreateRagdollEntity()
{
    // El último daño ha especificado que sin cadaver
    if ( (GetLastDamage().GetDamageType() & DMG_REMOVENORAGDOLL) != 0 )
        return;

    // TODO
    BecomeRagdollOnClient(GetLastDamage().GetDamageForce());
    return;

    /*CInRagdoll *pRagdoll = GetRagdoll();

    // Nuestro cadaver existe
    if ( pRagdoll ) {
        DestroyRagdoll();
        pRagdoll = NULL;
    }

    // Creamos un nuevo ragdoll
    if ( !pRagdoll ) {
        pRagdoll = dynamic_cast< CInRagdoll * >(CreateEntityByName("in_ragdoll"));
    }

    if ( pRagdoll ) {
        pRagdoll->Init(this);
        m_nRagdoll = pRagdoll;
    }*/
}

//================================================================================
//================================================================================
void CPlayer::DestroyRagdoll()
{
    CInRagdoll *pRagdoll = GetRagdoll();

    if ( !pRagdoll )
        return;

    pRagdoll->SUB_StartFadeOut(1.0f);
}

//================================================================================
// Returns the component by its ID
//================================================================================
CPlayerComponent * CPlayer::GetComponent(int id)
{
    FOR_EACH_VEC(m_nComponents, key)
    {
        CPlayerComponent *pComponent = m_nComponents.Element(key);

        if ( !pComponent )
            continue;

        if ( pComponent->GetID() == id )
            return pComponent;
    }

    return NULL;
}

//================================================================================
// Add a component for your ID
//================================================================================
void CPlayer::AddComponent(int id)
{
    CPlayerComponent *pComponent = NULL;

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

    AssertMsg(pComponent, "AddComponent(id) not handled!");

    if ( !pComponent ) {
        return;
    }

    AddComponent(pComponent);
}

//================================================================================
// Add a component
//================================================================================
void CPlayer::AddComponent(CPlayerComponent *pComponent)
{
    // Ya lo tenemos
    if ( GetComponent(pComponent->GetID()) )
        return;

    pComponent->SetParent(this);
    pComponent->Init();
    m_nComponents.AddToTail(pComponent);
}

//================================================================================
// Player components
//================================================================================
void CPlayer::CreateComponents()
{
    //AddComponent( PLAYER_COMPONENT_HEALTH );
    //AddComponent( PLAYER_COMPONENT_EFFECTS );
    //AddComponent( PLAYER_COMPONENT_DEJECTED );
}

//================================================================================
// Update the components
//================================================================================
void CPlayer::UpdateComponents()
{
    VPROF_BUDGET("CPlayer::UpdateComponents", VPROF_BUDGETGROUP_PLAYER);

    FOR_EACH_VEC(m_nComponents, key)
    {
        CPlayerComponent *pFeature = m_nComponents.Element(key);
        pFeature->Update();
    }
}

//================================================================================
// Agrega un atributo por su nombre
//================================================================================
void CPlayer::AddAttribute(const char *name)
{
    if ( GetAttribute(name) )
        return;

    CAttribute *pAttribute = TheAttributeSystem->GetAttribute(name);
    AssertMsg1(pAttribute, "The attribute %s does not exist!", name);

    if ( !pAttribute ) {
        return;
    }

    AddAttribute(pAttribute);
}

//================================================================================
// Agrega un atributo
//================================================================================
void CPlayer::AddAttribute(CAttribute *pAttribute)
{
    if ( !pAttribute )
        return;

    m_nAttributes.AddToTail(pAttribute);
}

//================================================================================
// Crea los atributos del jugador
//================================================================================
void CPlayer::CreateAttributes()
{
    // Cada juego decide que atributos agregar
    //AddAttribute("health");
    //AddAttribute("stamina");
    //AddAttribute("stress");
    //AddAttribute("shield");
}

//================================================================================
// Pre-Actualiza los atributos
//================================================================================
void CPlayer::PreUpdateAttributes()
{
    FOR_EACH_VEC(m_nAttributes, it)
    {
        m_nAttributes[it]->PreUpdate();
    }
}

//================================================================================
// Actualiza los atributos
//================================================================================
void CPlayer::UpdateAttributes()
{
    FOR_EACH_VEC(m_nAttributes, it)
    {
        m_nAttributes[it]->Update();
    }
}

//================================================================================
// Agrega un modificador a los atributos
//================================================================================
void CPlayer::AddAttributeModifier(const char *name)
{
    AttributeInfo info;

    // El modificador no existe
    if ( !TheAttributeSystem->GetModifierInfo(name, info) )
        return;

    CAttribute *pAttribute = GetAttribute(info.affects);

    if ( !pAttribute )
        return;

    pAttribute->AddModifier(info);
}

//================================================================================
// Devuelve el [CAttribute] por su nombre
//================================================================================
CAttribute *CPlayer::GetAttribute(const char *name)
{
    FOR_EACH_VEC(m_nAttributes, it)
    {
        CAttribute *pAttribute = m_nAttributes.Element(it);

        if ( FStrEq(pAttribute->GetID(), name) )
            return pAttribute;
    }

    return NULL;
}

//================================================================================
// Devuelve el arma actual del jugador
//================================================================================
CBaseWeapon *CPlayer::GetActiveBaseWeapon()
{
    return ToBaseWeapon(GetActiveWeapon());
}

//================================================================================
// Otorga el objeto especificado al jugador
//================================================================================
CBaseEntity * CPlayer::GiveNamedItem(const char *name, int subType, bool removeIfNotCarried)
{
    // If I already own this type don't create one
    if ( Weapon_OwnsThisType(name, subType) )
        return NULL;

    CBaseEntity *pEntity = CreateEntityByName(name);

    if ( !pEntity ) {
        Msg("NULL Ent in GiveNamedItem!\n");
        return NULL;
    }

    pEntity->SetLocalOrigin(GetLocalOrigin());
    pEntity->AddSpawnFlags(SF_NORESPAWN);

    CBaseCombatWeapon *pWeapon = dynamic_cast<CBaseCombatWeapon *>(pEntity);

    DispatchSpawn(pEntity);

    if ( pWeapon ) {
        pWeapon->SetSubType(subType);
        Weapon_Equip(pWeapon);
    }
    else {
        if ( pEntity && !(pEntity->IsMarkedForDeletion()) ) {
            pEntity->Touch(this);
        }
    }

    return pEntity;
}

//================================================================================
// Crea el Expresser, un componente para hablar y reproducir 
// escenas de forma dinámica.
//================================================================================
void CPlayer::CreateExpresser()
{
    m_pExpresser = new CMultiplayer_Expresser(this);
    if ( !m_pExpresser ) return;
    m_pExpresser->Connect(this);
}

//================================================================================
//================================================================================
void CPlayer::ModifyOrAppendCriteria(AI_CriteriaSet& criteriaSet)
{
    BaseClass::ModifyOrAppendCriteria(criteriaSet);
    ModifyOrAppendPlayerCriteria(criteriaSet);
}

//================================================================================
//================================================================================
bool CPlayer::SpeakIfAllowed(AIConcept_t concept, const char *modifiers, char *pszOutResponseChosen, size_t bufsize, IRecipientFilter *filter)
{
    if ( !IsAlive() )
        return false;

    return Speak(concept, modifiers, pszOutResponseChosen, bufsize, filter);
}

//================================================================================
//================================================================================
bool CPlayer::SpeakConcept(AI_Response &response, int iConcept)
{
    // Save the current concept.
    m_iCurrentConcept = iConcept;
    return false;
    //return SpeakFindResponse( response, g_pszMPConcepts[iConcept] );
}

//================================================================================
//================================================================================
bool CPlayer::SpeakConceptIfAllowed(int iConcept, const char *modifiers, char *pszOutResponseChosen, size_t bufsize, IRecipientFilter *filter)
{
    // Save the current concept.
    m_iCurrentConcept = iConcept;
    return SpeakIfAllowed(g_pszMPConcepts[iConcept], modifiers, pszOutResponseChosen, bufsize, filter);
}

//================================================================================
//================================================================================
bool CPlayer::CanHearAndReadChatFrom(CBasePlayer *pPlayer)
{
    return CBasePlayer::CanHearAndReadChatFrom(pPlayer);
}

//================================================================================
// Crea el sistema de sensaciones
//================================================================================
void CPlayer::CreateSenses()
{
    m_pSenses = new CAI_Senses;
    m_pSenses->SetOuter(this);
}

//================================================================================
// Establece la distancia máxima de visión
//================================================================================
void CPlayer::SetDistLook(float flDistLook)
{
    if ( GetSenses() ) {
        GetSenses()->SetDistLook(flDistLook);
    }
}

//================================================================================
// Lista de sonidos que queremos escuchar
//================================================================================
int CPlayer::GetSoundInterests()
{
    return SOUND_DANGER | SOUND_COMBAT | SOUND_PLAYER | SOUND_CARCASS | SOUND_MEAT | SOUND_GARBAGE;
}

//================================================================================
// Devuelve prioridad de sonido
//================================================================================
int CPlayer::GetSoundPriority(CSound *pSound)
{
    if ( pSound->IsSoundType(SOUND_COMBAT) ) {
        return SOUND_PRIORITY_HIGH;
    }

    if ( pSound->IsSoundType(SOUND_DANGER) ) {
        if ( pSound->IsSoundType(SOUND_CONTEXT_FROM_SNIPER | SOUND_CONTEXT_EXPLOSION) ) {
            return SOUND_PRIORITY_HIGHEST;
        }
        else if ( pSound->IsSoundType(SOUND_CONTEXT_GUNFIRE | SOUND_CONTEXT_BULLET_IMPACT) ) {
            return SOUND_PRIORITY_VERY_HIGH;
        }

        return SOUND_PRIORITY_HIGH;
    }

    if ( pSound->IsSoundType(SOUND_CARCASS | SOUND_MEAT | SOUND_GARBAGE) ) {
        return SOUND_PRIORITY_VERY_LOW;
    }

    return SOUND_PRIORITY_NORMAL;
}

//================================================================================
// Devuelve si el jugador puede oir el sonido
//================================================================================
bool CPlayer::QueryHearSound(CSound *pSound)
{
    CBaseEntity *pOwner = pSound->m_hOwner.Get();

    if ( pOwner == this )
        return false;

    if ( pSound->IsSoundType(SOUND_PLAYER) && !pOwner ) {
        return false;
    }

    if ( pSound->IsSoundType(SOUND_CONTEXT_PLAYER_ALLIES_ONLY) ) {
        if ( Classify() != CLASS_PLAYER_ALLY && Classify() != CLASS_PLAYER_ALLY_VITAL ) {
            return false;
        }
    }

    if ( pOwner ) {
        // Solo escuchemos sonidos provocados por nuestros aliados si son de combate.
        if ( TheGameRules->PlayerRelationship(this, pOwner) == GR_ALLY ) {
            if ( pSound->IsSoundType(SOUND_COMBAT) && !pSound->IsSoundType(SOUND_CONTEXT_GUNFIRE) ) {
                return true;
            }

            return false;
        }
    }

    if ( ShouldIgnoreSound(pSound) ) {
        return false;
    }

    return true;
}

//================================================================================
// Devuelve si el jugador puede declarar que ha visto la entidad especificada
//================================================================================
bool CPlayer::QuerySeeEntity(CBaseEntity *pEntity, bool bOnlyHateOrFear)
{
    if ( bOnlyHateOrFear ) {
        Disposition_t disposition = IRelationType(pEntity);
        return (disposition == D_HT || disposition == D_FR);
    }

    return true;
}

//================================================================================
// Al mirar entidades
//================================================================================
void CPlayer::OnLooked(int iDistance)
{
    if ( GetBotController() ) {
        GetBotController()->OnLooked(iDistance);
    }
}

//================================================================================
// Al escuchar sonidos
//================================================================================
void CPlayer::OnListened()
{
    if ( GetBotController() ) {
        GetBotController()->OnListened();
    }
}

//================================================================================
//================================================================================
CSound *CPlayer::GetLoudestSoundOfType(int iType)
{
    return CSoundEnt::GetLoudestSoundOfType(iType, EarPosition());
}

//================================================================================
// Devuelve si podemos ver el origen del sonido
//================================================================================
bool CPlayer::SoundIsVisible(CSound *pSound)
{
    return IsAbleToSee(pSound->GetSoundReactOrigin(), CBaseCombatCharacter::DISREGARD_FOV);
}

//================================================================================
// Devuelve el mejor sonido
//================================================================================
CSound* CPlayer::GetBestSound(int validTypes)
{
    CSound *pResult = GetSenses()->GetClosestSound(false, validTypes);

    if ( pResult == NULL ) {
        Msg("NULL Return from GetBestSound\n");
    }

    return pResult;
}

//================================================================================
// Devuelve el mejor olor
//================================================================================
CSound* CPlayer::GetBestScent()
{
    CSound *pResult = GetSenses()->GetClosestSound(true);

    if ( pResult == NULL ) {
        Msg("NULL Return from GetBestScent\n");
    }

    return pResult;
}

//================================================================================
//================================================================================
bool CPlayer::IsAbleToSee(const Vector & pos, FieldOfViewCheckType checkFOV)
{
    VPROF_BUDGET("CPlayer::IsAbleToSee::Position", VPROF_BUDGETGROUP_PLAYER);

    if ( checkFOV == USE_FOV && !IsInFieldOfView(pos) ) {
        return false;
    }

    if ( IsHiddenByFog(pos) ) {
        return false;
    }

    return FVisible(pos, MASK_BLOCKLOS);
}

//================================================================================
//================================================================================
bool CPlayer::IsAbleToSee(CBaseEntity * entity, FieldOfViewCheckType checkFOV)
{
    VPROF_BUDGET("CPlayer::IsAbleToSee::Entity", VPROF_BUDGETGROUP_PLAYER);

    if ( checkFOV == USE_FOV && !IsInFieldOfView(entity) ) {
        return false;
    }

    if ( IsHiddenByFog(entity) ) {
        return false;
    }

    return FVisible(entity, MASK_BLOCKLOS);
}

//================================================================================
//================================================================================
bool CPlayer::IsAbleToSee(CBaseCombatCharacter * pBCC, FieldOfViewCheckType checkFOV)
{
    return IsAbleToSee((CBaseEntity *)pBCC, checkFOV);
}

//================================================================================
//================================================================================
bool CPlayer::IsInFieldOfView(CBaseEntity * entity)
{
    VPROF_BUDGET("CPlayer::IsInFieldOfView::Entity", VPROF_BUDGETGROUP_PLAYER);
    return FInViewCone(entity);
}

//================================================================================
//================================================================================
bool CPlayer::IsInFieldOfView(const Vector & pos)
{
    VPROF_BUDGET("CPlayer::IsInFieldOfView::Position", VPROF_BUDGETGROUP_PLAYER);
    return FInViewCone(pos);
}

//================================================================================
//================================================================================
void CPlayer::HandleAnimEvent(animevent_t *event)
{
    if ( event->Event() == AE_PLAYER_FOOTSTEP_LEFT || event->Event() == AE_PLAYER_FOOTSTEP_RIGHT ) {
        FootstepSound();
        return;
    }

    CBasePlayer::HandleAnimEvent(event);
    BaseClass::HandleAnimEvent(event);
}

//================================================================================
// [DEPRECATED] Establece la animación actual
// Ahora mismo solo sirve para traducir animaciones del sistema anterior al nuevo
//================================================================================
void CPlayer::SetAnimation(PLAYER_ANIM nAnim)
{
    if ( nAnim == PLAYER_WALK || nAnim == PLAYER_IDLE ) return;

    if ( nAnim == PLAYER_RELOAD ) {
        DoAnimationEvent(PLAYERANIMEVENT_RELOAD, 0, true);
    }
    else if ( nAnim == PLAYER_JUMP ) {
        // TODO: Por alguna razón esto no se ejecuta en cliente
        DoAnimationEvent(PLAYERANIMEVENT_JUMP, 0, false);
    }
    else {
        Assert(!"CPlayer::SetAnimation OBSOLETE!");
    }
}

//================================================================================
// Reproduce una animación
//================================================================================
void CPlayer::DoAnimationEvent(PlayerAnimEvent_t pEvent, int nData, bool bPredicted)
{
    if ( !GetAnimationSystem() )
        return;

    // Procesamos la animación en el servidor
    // y después la enviamos al cliente con una entidad temporal.
    GetAnimationSystem()->DoAnimationEvent(pEvent, nData);
    SendPlayerAnimation(this, pEvent, nData, bPredicted);
}

//================================================================================
// Emite un sonido especial usando la configuración del jugador (Tipo y género)
// TODO: Probablemente ya no es necesario...
//================================================================================
void CPlayer::EmitPlayerSound(const char *pSoundName)
{
    CSoundParameters params;
    const char *pSound = UTIL_VarArgs("%s.%s", GetPlayerType(), pSoundName); // @TODO

                                                                             // El sonido no existe
    if ( !soundemitterbase->GetParametersForSound(pSound, params, GetPlayerGender()) )
        return;

    CRecipientFilter filter;
    filter.AddRecipientsByPAS(GetAbsOrigin());

    EmitSound_t sound;
    sound.m_nChannel = params.channel;
    sound.m_pSoundName = params.soundname;
    sound.m_flVolume = params.volume;
    sound.m_SoundLevel = params.soundlevel;
    sound.m_nPitch = params.pitch;
    sound.m_pOrigin = &GetAbsOrigin();

    EmitSound(filter, entindex(), sound);
    CSoundEnt::InsertSound(SOUND_PLAYER, GetAbsOrigin(), PLAYER_SOUND_RADIUS, 0.5f, this);
}

//================================================================================
// Para de reproducir un sonido especial de Jugador
//================================================================================
void CPlayer::StopPlayerSound(const char *pSoundName)
{
    const char *pSound = UTIL_VarArgs("%s.%s", GetPlayerType(), pSoundName); // @TODO
    StopSound(pSound);
}

//================================================================================
// Nombre engañoso: Solo sonidos para la I.A.
//================================================================================
void CPlayer::FootstepSound()
{
    float flVolume = (IsSprinting()) ? RandomFloat(200.0f, 400.0f) : RandomFloat(100.0f, 200.0f);

    if ( IsSneaking() ) {
        flVolume = RandomFloat(30.0f, 100.0f);
    }

    CSoundEnt::InsertSound(SOUND_PLAYER | SOUND_CONTEXT_FOOTSTEP, GetAbsOrigin(), flVolume, 0.2f, this, SOUNDENT_CHANNEL_FOOTSTEP);
}

//================================================================================
// Emite un sonido al recibir daño
//================================================================================
void CPlayer::PainSound(const CTakeDamageInfo &info)
{
    EmitSound("Player.Pain");
}

//================================================================================
// Emite un sonido al morir
//================================================================================
void CPlayer::DeathSound(const CTakeDamageInfo &info)
{
    if ( !TheGameRules->FCanPlayDeathSound(info) )
        return;

    EmitSound("Player.Death");
    CSoundEnt::InsertSound(SOUND_CARCASS, GetAbsOrigin(), 1024.0f, 5.0f, this);
}

//================================================================================
// Establece si el jugador puede usar la linterna
//================================================================================
void CPlayer::SetFlashlightEnabled(bool bState)
{
    m_bFlashlightEnabled = bState;
}

//================================================================================
// Devuelve si la linterna esta encendida
//================================================================================
int CPlayer::FlashlightIsOn()
{
    return IsEffectActive(EF_DIMLIGHT);
}

//================================================================================
// Enciende la linterna
//================================================================================
void CPlayer::FlashlightTurnOn()
{
    if ( FlashlightIsOn() )
        return;

    if ( !TheGameRules->FAllowFlashlight() || !m_bFlashlightEnabled )
        return;

    AddEffects(EF_DIMLIGHT);
    EmitSound("Player.FlashlightOn");
}

//================================================================================
// Apaga la linterna
//================================================================================
void CPlayer::FlashlightTurnOff()
{
    if ( !FlashlightIsOn() || !m_bFlashlightEnabled )
        return;

    RemoveEffects(EF_DIMLIGHT);
    EmitSound("Player.FlashlightOff");
}

//================================================================================
// Muestra los efectos de haber recibido el daño especificado
//================================================================================
void CPlayer::DamageEffect(const CTakeDamageInfo &info)
{
    float flDamage = info.GetDamage();
    int fDamageType = info.GetDamageType();

    color32 red = {128,0,0,150};
    color32 blue = {0,0,128,100};
    color32 yellow = {215,223,1,100};
    color32 green = {33,97,0,100};
    color32 white = {255,255,255,20};

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
        SpawnBlood(EyePosition(), g_vecAttackDir, BloodColor(), flDamage);
    }

    // Plasma
    else if ( fDamageType & DMG_PLASMA ) {
        backgroundColor = blue;
        flags = FFADE_MODULATE;
        fxColor = LFX_BLUE | LFX_FULL_BRIGHTNESS;

        // Very small screen shake
        if ( IsAlive() ) {
            ViewPunch(QAngle(random->RandomInt(-0.1, 0.1), random->RandomInt(-0.1, 0.1), random->RandomInt(-0.1, 0.1)));
        }

        EmitSound("Player.PlasmaDamage");
    }

    // Choque electrico
    else if ( fDamageType & (DMG_SHOCK | DMG_ENERGYBEAM) ) {
        backgroundColor = yellow;
        flags = FFADE_MODULATE;
        fxColor = LFX_YELLOW | LFX_FULL_BRIGHTNESS;

        g_pEffects->Sparks(info.GetDamagePosition(), 2, 2);
        UTIL_Smoke(info.GetDamagePosition(), random->RandomInt(10, 15), 10);
    }

    // Veneno
    else if ( fDamageType & (DMG_PARALYZE | DMG_NERVEGAS | DMG_POISON | DMG_RADIATION | DMG_ACID) ) {
        backgroundColor = green;
        fxColor = LFX_GREEN | LFX_FULL_BRIGHTNESS;
    }

    // Sonic
    else if ( fDamageType & DMG_SONIC ) {
        EmitSound("Player.SonicDamage");
    }

    // Bala
    else if ( fDamageType & DMG_BULLET ) {
        ViewPunch(QAngle(random->RandomInt(-0.1, 0.1), random->RandomInt(-0.1, 0.1), random->RandomInt(-0.1, 0.1)));
        EmitSound("Flesh.BulletImpact");
    }

    // Explosión
    else if ( fDamageType & DMG_BLAST ) {
        if ( IsAlive() ) {
            OnDamagedByExplosion(info);
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

        UTIL_ScreenFade(this, backgroundColor, 0.4f, 0.1f, flags);
        Utils::AlienFX_SetColor(this, LFX_ALL, fxColor, 0.5f);
    }

    CSoundEnt::InsertSound(SOUND_PLAYER | SOUND_COMBAT | SOUND_CONTEXT_INJURY, GetAbsOrigin(), PLAYER_SOUND_RADIUS, 0.3f, this, SOUNDENT_CHANNEL_INJURY);
}

//================================================================================
// Devuelve si el jugador puede procesar el daño.
// Si la respuesta es false, no se procesara ningún efecto, sonido, etc.
//================================================================================
bool CPlayer::CanTakeDamage(const CTakeDamageInfo &info)
{
    CTakeDamageInfo dinfo = info;
    IServerVehicle *pVehicle = GetVehicle();

    if ( pVehicle ) {
        if ( !pVehicle->PassengerShouldReceiveDamage(dinfo) )
            return false;
    }

    if ( IsInCommentaryMode() ) {
        if ( !ShouldTakeDamageInCommentaryMode(info) )
            return false;
    }

    if ( IsOnGodMode() )
        return false;

    if ( info.GetDamage() <= 0.0f )
        return false;

    if ( m_takedamage != DAMAGE_YES )
        return false;

    if ( !TheGameRules->FPlayerCanTakeDamage(this, info.GetAttacker(), info) )
        return false;

    return true;
}

//================================================================================
// We have received damage
//================================================================================
int CPlayer::OnTakeDamage(const CTakeDamageInfo &inputInfo)
{
    CTakeDamageInfo info = inputInfo;

    // We are invulnerable
    if ( !CanTakeDamage(info) )
        return 0;

    int response = 1;

    // We store the damage information received.
    // Bots always need this information.
    m_LastDamage = info;
    m_LastDamageTimer.Start();
    m_lastDamageAmount = info.GetDamage();
    m_fTimeLastHurt = gpGlobals->curtime;

    // We save the original amount of damage
    info.CopyDamageToBaseDamage();

    switch ( m_lifeState ) {
        case LIFE_ALIVE:
        {
            response = OnTakeDamage_Alive(info);
            break;
        }

        case LIFE_DYING:
        {
            response = OnTakeDamage_Dying(info);
            break;
        }

        case LIFE_DEAD:
        {
            response = OnTakeDamage_Dead(info);
            break;
        }

        default:
        {
            Assert(0);
            false;
        }
    }

    // Apparently we should not get hurt this time
    if ( response == 0 )
        return 0;

    ApplyDamage(info);
    return 1;
}

//================================================================================
// Hemos recibido daño estando vivo
//================================================================================
// Aquí se debe aplicar todo lo necesario ANTES de la reducción de salud, si 
// se devuelve 0 entonces no se hará reducción de salud pero si efectos especiales.
//================================================================================
int CPlayer::OnTakeDamage_Alive(CTakeDamageInfo &info)
{
    // We report damage to the Bots system and squadron.
    if ( GetBotController() ) {
        GetBotController()->OnTakeDamage(info);
    }

    if ( GetSquad() ) {
        GetSquad()->ReportTakeDamage(this, info);
    }

    // The game rules allow us to adjust the damage. (Armor, place of impact, etc.)
    TheGameRules->AdjustPlayerDamageTaken(this, info);

    m_UnderAttackTimer.Start();
    m_CombatTimer.Start();

    // This type of damage slows us down
    if ( TheGameRules->Damage_CausesSlowness(info) ) {
        m_SlowDamageTimer.Start();
    }

    // This type of damage can knock us down
    if ( TheGameRules->FPlayerCanDejected(this, info) ) {
        int newHealth = GetHealth() - info.GetDamage();

        if ( newHealth <= 0 ) {
            if ( GetPlayerStatus() == PLAYER_STATUS_CLIMBING ) {
                SetPlayerStatus(PLAYER_STATUS_FALLING);
            }
            else {
                SetPlayerStatus(PLAYER_STATUS_DEJECTED);
            }

            return 0;
        }
    }

    /*
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
    }*/

    return 1;
}

//================================================================================
// Aplica el daño al jugador, se ejecuta después de haber pasado las validaciones
//================================================================================
void CPlayer::ApplyDamage(const CTakeDamageInfo &info)
{
    if ( !IsAlive() )
        return;

    // Play an animation depending on the location of the impact.
    switch ( LastHitGroup() ) {
        case HITGROUP_GENERIC:
            DoAnimationEvent(PLAYERANIMEVENT_FLINCH);
            break;

        case HITGROUP_HEAD:
            DoAnimationEvent(PLAYERANIMEVENT_FLINCH_HEAD);
            break;

        case HITGROUP_CHEST:
        case HITGROUP_STOMACH:
            DoAnimationEvent(PLAYERANIMEVENT_FLINCH_CHEST);
            break;

        case HITGROUP_LEFTARM:
            DoAnimationEvent(PLAYERANIMEVENT_FLINCH_LEFTARM);
            break;

        case HITGROUP_RIGHTARM:
            DoAnimationEvent(PLAYERANIMEVENT_FLINCH_RIGHTARM);
            break;

        case HITGROUP_LEFTLEG:
            DoAnimationEvent(PLAYERANIMEVENT_FLINCH_LEFTLEG);
            break;

        case HITGROUP_RIGHTLEG:
            DoAnimationEvent(PLAYERANIMEVENT_FLINCH_RIGHTLEG);
            break;
    }

    if ( info.GetDamage() > 0 ) {
        if ( info.GetAttacker() ) {
            if ( info.GetAttacker()->IsPlayer() ) {
                DebugAddMessage("Taking %.2f damage from %s", info.GetDamage(), info.GetAttacker()->GetPlayerName());
            }
            else {
                DebugAddMessage("Taking %.2f damage from %s", info.GetDamage(), info.GetAttacker()->GetClassname());
            }
        }

        if ( IsNetClient() && !ShouldThrottleUserMessage("Damage") ) {
            CUserAndObserversRecipientFilter user(this);
            user.MakeReliable();

            UserMessageBegin(user, "Damage");
            WRITE_BYTE(info.GetBaseDamage());
            WRITE_FLOAT(info.GetDamagePosition().x);
            WRITE_FLOAT(info.GetDamagePosition().y);
            WRITE_FLOAT(info.GetDamagePosition().z);
            MessageEnd();
        }
    }

    DamageEffect(info);

    // grab the vector of the incoming attack. ( pretend that the inflictor is a little lower than it really is, so the body will tend to fly upward a bit).
    Vector vecDir = vec3_origin;

    if ( info.GetInflictor() ) {
        vecDir = info.GetInflictor()->WorldSpaceCenter() - Vector(0, 0, 10) - WorldSpaceCenter();
        VectorNormalize(vecDir);
    }

    g_vecAttackDir = vecDir;

    // add to the damage total for clients, which will be sent as a single
    // message at the end of the frame
    // todo: remove after combining shotgun blasts?
    if ( info.GetInflictor() && info.GetInflictor()->edict() ) {
        m_DmgOrigin = info.GetInflictor()->GetAbsOrigin();
    }

    m_DmgTake += info.GetDamage();

    // Reset damage time countdown for each type of time based damage player just sustained
    for ( int i = 0; i < CDMG_TIMEBASED; i++ ) {
        // Make sure the damage type is really time-based.
        // This is kind of hacky but necessary until we setup DamageType as an enum.
        int iDamage = (DMG_PARALYZE << i);

        if ( (info.GetDamageType() & iDamage) && g_pGameRules->Damage_IsTimeBased(iDamage) )
            m_rgbTimeBasedDamage[i] = 0;
    }

    m_bitsDamageType |= info.GetDamageType(); // Save this so we can report it to the client
    m_bitsHUDDamage = -1;  // make sure the damage bits get resent

    PainSound(info);

    if ( info.GetDamage() <= 0 )
        return;

    // Aplicamos la reducción de vida
    m_iHealth = m_iHealth - info.GetDamage();
    m_iHealth = clamp(m_iHealth, 0, GetMaxHealth());

    // fire global game event
    IGameEvent * event = gameeventmanager->CreateEvent("player_hurt", false);

    if ( event ) {
        event->SetInt("userid", GetUserID());
        event->SetInt("health", m_iHealth);
        event->SetInt("priority", 5);	// HLTV event priority, not transmitted

        if ( info.GetAttacker() && info.GetAttacker()->IsPlayer() ) {
            CBasePlayer *player = ToBasePlayer(info.GetAttacker());
            event->SetInt("attacker", player->GetUserID()); // hurt by other player
        }
        else {
            event->SetInt("attacker", 0); // hurt by "world"
        }

        gameeventmanager->FireEvent(event);
    }

    if ( m_iHealth == 0 ) {
        if ( IsOnBuddhaMode() ) {
            m_iHealth = 1;
        }
        else {
            Event_Killed(info);
        }
    }
}

//================================================================================
// Tira los recursos del jugador al morir
//================================================================================
void CPlayer::DropResources()
{
    if ( GetActiveWeapon() ) {
        int dropRule = TheGameRules->DeadPlayerWeapons(this);

        switch ( dropRule ) {
            case GR_PLR_DROP_GUN_ACTIVE:
                Weapon_Drop(GetActiveWeapon(), NULL, NULL);
                break;

            case GR_PLR_DROP_GUN_ALL:
                Weapon_DropAll();
                break;
        }
    }
}

//================================================================================
// [Evento] Hemos muerto
// Hacemos todo lo necesario para manejar nuestra muerte
//================================================================================
void CPlayer::Event_Killed(const CTakeDamageInfo &info)
{
    CSound *pSound = CSoundEnt::SoundPointerForIndex(CSoundEnt::ClientSoundIndex(edict()));
    IPhysicsObject *pPhysics = VPhysicsGetObject();

    RumbleEffect(RUMBLE_STOP_ALL, 0, RUMBLE_FLAGS_NONE);

    SetPlayerState(PLAYER_STATE_DEAD);
    SnapEyeAnglesZ(0);
    SetFOV(this, 0, 0);

    ClearUseEntity();

    if ( pPhysics ) {
        pPhysics->EnableCollisions(false);
        pPhysics->RecheckContactPoints();
    }

    if ( pSound ) {
        pSound->Reset();
    }

    EmitSound("BaseCombatCharacter.StopWeaponSounds");
    m_iHealth = 0;
    ClearLastKnownArea();

    TheGameRules->PlayerKilled(this, info);
    SendOnKilledGameEvent(info);

    if ( GetBotController() ) {
        GetBotController()->OnDeath(info);
    }

    if ( GetSquad() ) {
        GetSquad()->ReportDeath(this, info);
    }

    if ( ShouldGib(info) ) {
        Event_Gibbed(info);
    }
    else {
        Event_Dying(info);
    }

    // Establecemos la función de pensamiento
    SetThink(&CPlayer::PlayerDeathThink);
    SetNextThink(gpGlobals->curtime + 0.1f);
}

//================================================================================
// Hemos explotado en pedazitos
//================================================================================
bool CPlayer::Event_Gibbed(const CTakeDamageInfo &info)
{
    m_lifeState = LIFE_DEAD;
    return BaseClass::Event_Gibbed(info);
}

//================================================================================
// Estamos en proceso de morir (quizá en la animación)
//================================================================================
void CPlayer::Event_Dying(const CTakeDamageInfo &info)
{
    m_lifeState = LIFE_DYING;
    DeathSound(info);

    // Reproducimos la animación al morir
    if ( TheGameRules->CanPlayDeathAnim(this, info) ) {
        DoAnimationEvent(PLAYERANIMEVENT_DIE);
        m_bPlayingDeathAnim = true;
    }
}

//================================================================================
// Pensamiento al morir
//================================================================================
void CPlayer::PlayerDeathThink()
{
    SetNextThink(gpGlobals->curtime + 0.1f);

    // Reproduciendo animación de muerte...
    if ( m_lifeState == LIFE_DYING ) {
        if ( m_bPlayingDeathAnim && GetModelIndex() && !IsSequenceFinished() ) {
            ++m_iRespawnFrames;

            // Menos de 60 frames, por ahora todo bien y seguimos esperando...
            // Pero si sobrepasamos los 60 frames significa que hubo un problema
            // con la animación (o es muy larga) y debemos forzar la muerte
            if ( m_iRespawnFrames < 60 ) {
                return;
            }
            else {
                Assert(!"m_iRespawnFrames > 60!");
            }
        }

        CreateRagdollEntity();
        SetGroundEntity(NULL);
        AddEffects(EF_NODRAW);

        // Hemos muerto
        m_lifeState = LIFE_DEAD;
        m_flDeathAnimTime = gpGlobals->curtime; // Volvemos a establecerlo para que "CInGameRules::FlPlayerSpawnTime" funcione correctamente

        CBaseEntity *pOwner = GetOwnerEntity();

        // Notificamos a nuestro dueño
        if ( pOwner ) {
            pOwner->DeathNotice(this);
            SetOwnerEntity(NULL);
        }

        return;
    }

    // Paramos todas las animaciones
    StopAnimation();
    AddEffects(EF_NOINTERP);
    SetPlaybackRate(0.0f);

    // Pensamiento al estar muerto
    PlayerDeathPostThink();
}

//================================================================================
// Pensamiento al estar muerto (modo espectador)
//================================================================================
void CPlayer::PlayerDeathPostThink()
{
    if ( TheGameRules->FPlayerCanGoSpectate(this) ) {
        Spectate();
    }

    // Aún no podemos hacer respawn
    if ( !TheGameRules->FPlayerCanRespawn(this) )
        return;

    m_lifeState = LIFE_RESPAWNABLE;

    if ( TheGameRules->FPlayerCanRespawnNow(this) ) {
        Spawn();
    }
}

//================================================================================
// El jugador ha entrado en una nueva condición
//================================================================================
void CPlayer::OnPlayerStatus(int oldStatus, int status)
{
    switch ( status ) {
        // Normal
        case PLAYER_STATUS_NONE:
        {
            // Restauramos acciones y movimiento
            RestoreButtons();
            EnableMovement();
            SetMoveType(MOVETYPE_WALK);
            SetViewOffset(VEC_VIEW);

            // Restauramos la salud anterior
            if ( m_iOldHealth > 0 ) {
                SetHealth(m_iOldHealth);
            }
            else {
                if ( oldStatus > PLAYER_STATUS_NONE && oldStatus != PLAYER_STATUS_FALLING ) {
                    SetHealth((GetMaxHealth() / 3));
                }
            }

            m_iOldHealth = -1;
            break;
        }

        // Incapacitado
        case PLAYER_STATUS_DEJECTED:
        {
            m_flHelpProgress = 0.0f;
            m_iOldHealth = -1;

            // Cámara
            RemoveFlag(FL_DUCKING);
            SetViewOffset(VEC_DEJECTED_VIEWHEIGHT);
            break;
        }

        // Colgando...
        case PLAYER_STATUS_CLIMBING:
        {
            // Progreso de ayuda
            m_flClimbingHold = 100.0f; // TODO: Quitar de aquí?
            m_flHelpProgress = 0.0f;

            // No podemos disparar
            DisableButtons(IN_ATTACK | IN_ATTACK2 | IN_RELOAD);

            // Flotando
            SetMoveType(MOVETYPE_FLY);
            SetAbsVelocity(Vector(0, 0, 0));

            // Guardamos nuestra salud actual
            if ( m_iOldHealth <= 0 ) {
                m_iOldHealth = GetHealth();
            }

            break;
        }

        // Cayendoooo
        case PLAYER_STATUS_FALLING:
        {
            SetMoveType(MOVETYPE_WALK);
            m_iOldHealth = -1;
            break;
        }
    }

    // Incapacitación
    if ( status == PLAYER_STATUS_DEJECTED || status == PLAYER_STATUS_CLIMBING ) {
        // Desactivamos movimiento y acciones
        DisableButtons(IN_JUMP | IN_DUCK);
        DisableMovement();

        // Establecemos salud máxima, esta irá disminuyendo lentamente
        SetHealth(GetMaxHealth());
    }
}

//================================================================================
// Establece la condición del jugador
//================================================================================
void CPlayer::SetPlayerStatus(int status)
{
    if ( status == m_iPlayerStatus )
        return;

    if ( status == PLAYER_STATUS_DEJECTED ) {
        ++m_iDejectedTimes;
    }

    OnPlayerStatus(m_iPlayerStatus, status);
    m_iPlayerStatus = status;
}

//================================================================================
// Alguién me esta ayudando
//================================================================================
void CPlayer::RaiseFromDejected(CPlayer *pRescuer)
{
    if ( !IsDejected() )
        return;

    // Iniciamos la animación
    if ( m_flHelpProgress == 0 ) {
        DoAnimationEvent(PLAYERANIMEVENT_CUSTOM, ACT_TERROR_INCAP_TO_STAND);
    }

    // Progreso
    m_flHelpProgress += 0.35f;
    m_RaiseHelpTimer.Start();

    // Aumentamos nuestra visión poco a poco
    Vector vecView = VEC_DEJECTED_VIEWHEIGHT;
    vecView.z += 0.47 * m_flHelpProgress;
    SetViewOffset(vecView);

    // ¡Arriba!
    if ( m_flHelpProgress > 100 ) {
        SetPlayerStatus(PLAYER_STATUS_NONE);
    }
}

//================================================================================
// El jugador ha entrado en un nuevo estado
//================================================================================
void CPlayer::EnterPlayerState(int status)
{
    switch ( status ) {
        // Hemos entrado al servidor
        case PLAYER_STATE_WELCOME:
        {
            // Equipo y clase sin definir
            ChangeTeam(TEAM_UNASSIGNED);
            SetPlayerClass(PLAYER_CLASS_NONE);

            // Sin linterna
            SetFlashlightEnabled(false);
            RemoveEffects(EF_DIMLIGHT);

            // Somos un fantasma
            SetModel("models/player.mdl");
            SetMoveType(MOVETYPE_NONE);
            AddEffects(EF_NODRAW);
            AddSolidFlags(FSOLID_NOT_SOLID);
            //PhysObjectSleep();
            StartObserverMode(OBS_MODE_ROAMING);

            // Creamos el contexto para el pensamiento de empujar objetos
            SetContextThink(&CPlayer::PushawayThink, TICK_NEVER_THINK, PUSHAWAY_THINK_CONTEXT);
            break;
        }

        // Estamos activos en el juego
        case PLAYER_STATE_ACTIVE:
        {
            //
            SetModel(GetPlayerModel());
            SetUpModel();
            SetUpHands();

            m_nComponents.PurgeAndDeleteElements();
            m_nAttributes.PurgeAndDeleteElements();
            CreateComponents();
            CreateAttributes();

            // Ya podemos interactuar con el mundo
            SetMoveType(MOVETYPE_WALK);
            RemoveEffects(EF_NODRAW);
            RemoveSolidFlags(FSOLID_NOT_SOLID);
            PhysObjectWake();

            DoAnimationEvent(PLAYERANIMEVENT_SPAWN);

            // Modo Historia: Somos el líder del escuadron de jugador
            if ( !TheGameRules->IsMultiplayer() && !IsBot() ) {
                SetSquad("player");
                GetSquad()->SetLeader(this);
            }

            SetFOV(this, 0, 0);
            SetRenderColor(255, 255, 255);
            SetDistLook(3000.0f);

            // Empezamos a empujar objetos
            SetNextThink(gpGlobals->curtime + PUSHAWAY_THINK_INTERVAL, PUSHAWAY_THINK_CONTEXT);
            break;
        }

        // Eligiendo un equipo
        case PLAYER_STATE_PICKING_TEAM:
        {
            break;
        }

        // Eligiendo una clase
        case PLAYER_STATE_PICKING_CLASS:
        {
            break;
        }

        // Entramos a modo espectador
        case PLAYER_STATE_SPECTATOR:
        {
            break;
        }

        // Estoy muerto
        case PLAYER_STATE_DEAD:
        {
            if ( IsInAVehicle() )
                LeaveVehicle();

            pl.deadflag = true;
            m_flDeathTime = gpGlobals->curtime;

            // Somos un fantasma
            SetMoveType(MOVETYPE_NONE);
            AddSolidFlags(FSOLID_NOT_SOLID);

            // Tiramos nuestros recursos
            DropResources();

            if ( HasWeapons() )
                PackDeadPlayerItems();

            // Dejamos de empujar objetos
            SetNextThink(TICK_NEVER_THINK, PUSHAWAY_THINK_CONTEXT);
        }
    }
}

//================================================================================
// Establece el estado del jugador
//================================================================================
void CPlayer::SetPlayerState(int status)
{
    DebugAddMessage("%s -> %s", g_PlayerStateNames[m_iPlayerState], g_PlayerStateNames[status]);

    LeavePlayerState(m_iPlayerState);
    m_iPlayerState = status;
    EnterPlayerState(status);
}

//================================================================================
// Establece la clase del jugador
//================================================================================
void CPlayer::SetPlayerClass(int playerClass)
{
    m_iPlayerClass = playerClass;
    OnPlayerClass(playerClass);
}

//================================================================================
//================================================================================
float CPlayer::GetStamina()
{
    if ( !GetAttribute("stamina") )
        return 0.0f;

    return GetAttribute("stamina")->GetValue();
}

//================================================================================
//================================================================================
float CPlayer::GetStress()
{
    if ( !GetAttribute("stress") )
        return 0.0f;

    return GetAttribute("stress")->GetValue();
}

//================================================================================
// Crea el viewmodel, el modelo en primera persona
//================================================================================
void CPlayer::CreateViewModel(int index)
{
    Assert(index >= 0 && index < MAX_VIEWMODELS);

    if ( GetViewModel(index) )
        return;

    CPredictedViewModel *vm = (CPredictedViewModel *)CreateEntityByName("predicted_viewmodel");

    if ( vm ) {
        vm->SetAbsOrigin(GetAbsOrigin());
        vm->SetOwner(this);
        vm->SetIndex(index);

        DispatchSpawn(vm);
        vm->FollowEntity(this, false);
        vm->AddEffects(EF_NODRAW);

        m_hViewModel.Set(index, vm);
    }
}

//================================================================================
// Devuelve el modelo que se usará para las manos
//================================================================================
const char * CPlayer::GetHandsModel(int handsindex)
{
    return NULL;
}

//================================================================================
// El modelo ha sido establecido, ya podemos crear nuestras manos
//================================================================================
void CPlayer::SetUpHands()
{
}

//================================================================================
// Crea las manos, un viewmodel que es parte del viewmodel principal
//================================================================================
void CPlayer::CreateHands(int handsindex, int viewmodelparent)
{
    Assert(handsindex >= 1 && handsindex < MAX_VIEWMODELS);

    if ( !GetViewModel(viewmodelparent) )
        return;

    CBaseViewModel *hands = GetViewModel(handsindex);
    CBaseViewModel *vm = GetViewModel(viewmodelparent);

    // Ya existe, actualizamos el modelo
    if ( hands ) {
        hands->SetModel(GetHandsModel(handsindex));
        return;
    }

    if ( !GetHandsModel(handsindex) )
        return;

    hands = (CBaseViewModel *)CreateEntityByName("predicted_viewmodel");

    if ( hands ) {
        hands->SetAbsOrigin(GetAbsOrigin());
        hands->SetOwner(this);
        hands->SetIndex(handsindex);

        DispatchSpawn(hands);

        hands->SetModel(GetHandsModel(handsindex));
        hands->SetOwnerViewModel(vm);
        hands->SetOwnerEntity(this);
        hands->AddEffects(EF_NODRAW);

        m_hViewModel.Set(handsindex, hands);
    }
}

//================================================================================
// Al saltar
//================================================================================
void CPlayer::Jump()
{
}

//================================================================================
// Establece el escuadron del jugador
//================================================================================
void CPlayer::SetSquad(CSquad *pSquad)
{
    if ( m_pSquad ) {
        m_pSquad->RemoveMember(this);
    }

    m_pSquad = pSquad;

    if ( m_pSquad ) {
        m_pSquad->AddMember(this);
    }
}

//================================================================================
// Establece o crea el escuadron del jugador
//================================================================================
void CPlayer::SetSquad(const char *name)
{
    CSquad *pSquad = TheSquads->GetOrCreateSquad(name);
    SetSquad(pSquad);
}

//================================================================================
// [pMember] es el nuevo líder de nuestro escuadron
//================================================================================
void CPlayer::OnNewLeader(CPlayer *pMember)
{
}

//================================================================================
// Un miembro de nuestro escuadron ha recibido daño
//================================================================================
void CPlayer::OnMemberTakeDamage(CPlayer *pMember, const CTakeDamageInfo & info)
{
    if ( GetBotController() ) {
        GetBotController()->OnMemberTakeDamage(pMember, info);
    }
}

//================================================================================
// Un miembro de nuestro escuadron ha muerto
//================================================================================
void CPlayer::OnMemberDeath(CPlayer * pMember, const CTakeDamageInfo & info)
{
    if ( GetBotController() ) {
        GetBotController()->OnMemberDeath(pMember, info);
    }

    // Estrés por muerte
    // APOCALYPSE
    //AddAttributeModifier("stress_member_death");
}

//================================================================================
// Un miembro de nuestro escuadron ha visto un enemigo
//================================================================================
void CPlayer::OnMemberReportEnemy(CPlayer *pMember, CBaseEntity *pEnemy)
{
    if ( GetBotController() ) {
        GetBotController()->OnMemberReportEnemy(pMember, pEnemy);
    }
}

//================================================================================
// Devuelve si la entidad es visible por el jugador y esta dentro del campo de visión
//================================================================================
bool CPlayer::FEyesVisible(CBaseEntity *pEntity, int traceMask, CBaseEntity **ppBlocker)
{
    if ( pEntity == this )
        return false;

    if ( !FInViewCone(pEntity) )
        return false;

    return BaseClass::FVisible(pEntity, traceMask, ppBlocker);
}

//================================================================================
// Devuelve si la posición es visible por el jugador y esta dentro del campo de visión
//================================================================================
bool CPlayer::FEyesVisible(const Vector &vecTarget, int traceMask, CBaseEntity **ppBlocker)
{
    if ( !vecTarget.IsValid() )
        return false;

    if ( !FInViewCone(vecTarget) )
        return false;

    return CBasePlayer::FVisible(vecTarget, traceMask, ppBlocker);
}

//================================================================================
// Devuelve el valor de un comando, sea servidor o del cliente
//================================================================================
const char *CPlayer::GetCommandValue(const char *pCommand)
{
    ConVarRef command(pCommand);

    // Es un comando de servidor...
    if ( command.IsValid() ) {
        return command.GetString();
    }

    // Obtenemos el comando desde el cliente
    return engine->GetClientConVarValue(entindex(), pCommand);
}

//================================================================================
// Ejecuta un comando como si fuera escrito en la consola
//================================================================================
void CPlayer::ExecuteCommand(const char *pCommand)
{
    engine->ClientCommand(edict(), pCommand);
}

//================================================================================
//================================================================================
float CPlayer::GetApproximateFallDamage(float height) const
{
    // empirically discovered height values
    const float slope = 0.2178f;
    const float intercept = 26.0f;

    float damage = slope * height - intercept;
    return MAX(0.0f, damage);
}

//================================================================================
// Devuelve si este jugador podrá usar el autoaim
//================================================================================
bool CPlayer::ShouldAutoaim()
{
    if ( IsBot() )
        return false;

    if ( TheGameRules->GetAutoAimMode() == AUTOAIM_NONE )
        return false;

    return true;
}

//================================================================================
// Devuelve el nivel de dificultad de este jugador
//================================================================================
int CPlayer::GetDifficultyLevel()
{
    if ( IsBot() ) {
        return GetBotController()->GetProfile()->GetSkill();
    }

    if ( TheGameRules->HasDirector() && NameMatches("director_*") ) {
        return TheDirector->GetAngry();
    }

    // TODO: Una forma de calcular el nivel de habilidad
    // de cada jugador.
    return SKILL_ULTRA_HARD;
}

//================================================================================
// Expulsa al jugador del servidor
//================================================================================
void CPlayer::Kick()
{
    // Le quitamos todas las cosas
    RemoveAllItems(true);

    // Lo matamos para eliminar linternas y otras cosas
    if ( IsAlive() ) {
        CommitSuicide();
    }

    //GetPlayerInfo()->GetUserID();
    //GetPlayerInfo()->GetNetworkIDString();

    //engine->ServerCommand( UTIL_VarArgs( "kick %s\n", GetPlayerName() ) );
    engine->ServerCommand(UTIL_VarArgs("kickid %i\n", GetPlayerInfo()->GetUserID()));
    Msg("Kicking (%i) %s (%s)\n", GetPlayerInfo()->GetUserID(), GetPlayerName(), GetPlayerInfo()->GetNetworkIDString());
}

//================================================================================
// Ejecuta una posesión demoniaca sobre este jugador
//================================================================================
bool CPlayer::Possess(CPlayer *pOther)
{
    if ( !pOther )
        return false;

    //
    edict_t *otherSoul = pOther->edict();    // Abigail
    edict_t *mySoul = edict();            // Sniper

                                          // Algo salio mal...
    if ( !otherSoul || !mySoul )
        return false;

    bool otherIsBot = pOther->IsBot();
    bool myIsBot = IsBot();

    RemoveFlag(FL_FAKECLIENT);
    pOther->RemoveFlag(FL_FAKECLIENT);

    // Intercambiamos FL_FAKECLIENT
    if ( myIsBot )
        pOther->AddFlag(FL_FAKECLIENT);

    if ( otherIsBot )
        AddFlag(FL_FAKECLIENT);

    // Intercambiamos la I.A.
    IBot *pOtherBot = pOther->GetBotController();
    IBot *pMyBot = GetBotController();

    SetBotController(pOtherBot);
    pOther->SetBotController(pMyBot);

    // Backup
    //edict_t oldPlayerEdict    = *playerSoul;
    //edict_t oldBotEdict        = *botSoul;

    // Cambio!!
    edict_t ootherSoul = *otherSoul;
    *otherSoul = *mySoul;
    *mySoul = ootherSoul;

    //
    CPlayer *pMyPlayer = ToInPlayer(CBaseEntity::Instance(otherSoul));   // Sniper
    CPlayer *pOtherPlayer = ToInPlayer(CBaseEntity::Instance(mySoul));      // Abigail

                                                                            //
    pMyPlayer->NetworkProp()->SetEdict(otherSoul);
    pOtherPlayer->NetworkProp()->SetEdict(mySoul);

    return true;
}

//================================================================================
// Comienza el modo espectador
//================================================================================
void CPlayer::Spectate(CPlayer *pTarget)
{
    if ( IsObserver() )
        return;

    // Necesitamos volver a activar
    // el movimiento y el apuntado.
    EnableMovement();
    EnableAiming();

    if ( IsAlive() ) {
        RemoveAllItems(true);
        CommitSuicide();
    }

    PhysObjectSleep();

    UTIL_ScreenFade(this, {0,0,0,0}, 0.001f, 0.0f, FFADE_OUT);
    SetNextThink(TICK_NEVER_THINK, PUSHAWAY_THINK_CONTEXT);

    if ( pTarget ) {
        SetObserverTarget(pTarget);
    }

    // Empezamos el modo espectador
    SetPlayerState(PLAYER_STATE_SPECTATOR);
    StartObserverMode(m_iObserverLastMode);
}

//================================================================================
// Devuelve si el jugador principal esta viendo este jugador
// NOTA: Solo debe usarse para depurar.
//================================================================================
bool CPlayer::IsLocalPlayerWatchingMe()
{
    if ( engine->IsDedicatedServer() )
        return false;

    CBasePlayer *pPlayer = UTIL_GetListenServerHost();

    if ( !pPlayer )
        return false;

    if ( pPlayer->IsAlive() || !pPlayer->IsObserver() )
        return false;

    if ( pPlayer->GetObserverMode() != OBS_MODE_IN_EYE && pPlayer->GetObserverMode() != OBS_MODE_CHASE )
        return false;

    if ( pPlayer->GetObserverTarget() != this )
        return false;

    return true;
}

//================================================================================
//================================================================================
CPlayer * CPlayer::GetActivePlayer()
{
    // Estamos viendo a un Jugador
    if ( IsObserver() && GetObserverMode() == OBS_MODE_CHASE && GetObserverTarget() )
        return ToInPlayer(GetObserverTarget());

    return this;
}

//================================================================================
//================================================================================
void CPlayer::ImpulseCommands()
{
    switch ( m_nImpulse ) {
        case 100:
        {
            if ( FlashlightIsOn() )
                FlashlightTurnOff();
            else
                FlashlightTurnOn();

            break;
        }
    }

    return BaseClass::ImpulseCommands();
}

extern int gEvilImpulse101;

//================================================================================
//================================================================================
void CPlayer::CheatImpulseCommands(int iImpulse)
{
    if ( !sv_cheats->GetBool() )
        return;

    switch ( iImpulse ) {
        case 101:
        {
            gEvilImpulse101 = true;

            EquipSuit();

            // Give the player everything!
            GiveAmmo(999, "AMMO_TYPE_SNIPERRIFLE");
            GiveAmmo(999, "AMMO_TYPE_ASSAULTRIFLE");
            GiveAmmo(999, "AMMO_TYPE_PISTOL");
            GiveAmmo(999, "AMMO_TYPE_SHOTGUN");

            GiveNamedItem("weapon_cubemap");

            GiveNamedItem("weapon_rifle_ak47");
            GiveNamedItem("weapon_rifle_m16");
            GiveNamedItem("weapon_smg");
            GiveNamedItem("weapon_pistol_p220");
            GiveNamedItem("weapon_shotgun_combat");
            GiveNamedItem("weapon_rifle_sniper");

            SetHealth(100);

            gEvilImpulse101 = false;
            break;
        }

        default:
            BaseClass::CheatImpulseCommands(iImpulse);
    }
}

//================================================================================
// Procesa un comando enviado directamente desde el cliente
//================================================================================
bool CPlayer::ClientCommand(const CCommand &args)
{
    // Convertirse en espectador
    if ( FStrEq(args[0], "spectate") ) {
        Spectate();
        return true;
    }

    // Hacer respawn
    if ( FStrEq(args[0], "respawn") ) {
        //if ( ShouldRunRateLimitedCommand(args) )
        {
            if ( IsAlive() ) {
                CommitSuicide();
            }

            Spawn();
        }

        return true;
    }

    // Unirse a un equipo
    if ( FStrEq(args[0], "jointeam") ) {
        if ( args.ArgC() < 2 ) {
            Warning("Player sent bad jointeam syntax \n");
            return false;
        }

        //if ( ShouldRunRateLimitedCommand(args) )
        {
            int iTeam = atoi(args[1]);
            ChangeTeam(iTeam);
        }

        return true;
    }

    // Unirse a una clase
    if ( FStrEq(args[0], "joinclass") ) {
        if ( args.ArgC() < 2 ) {
            Warning("Player sent bad jointeam syntax \n");
            return false;
        }

        //if ( ShouldRunRateLimitedCommand(args) )
        {
            int iTeam = atoi(args[1]);
            SetPlayerClass(iTeam);
        }

        return true;
    }

    // Hacer que la I.A. controle a mi personaje
    if ( FStrEq(args[0], "control_bot") ) {
        SetUpBot();

        if ( GetBotController() ) {
            GetBotController()->Spawn();
        }
        else {
            Warning("There has been a problem creating the AI to control the player.\n");
        }

        return true;
    }

    // Volver a tomar el control del personaje
    if ( FStrEq(args[0], "control_human") ) {
        SetBotController(NULL);
        return true;
    }

    // Dejected
    if ( FStrEq(args[0], "dejected") ) {
        SetPlayerStatus(PLAYER_STATUS_DEJECTED);
        return true;
    }
    if ( FStrEq(args[0], "climbing") ) {
        SetPlayerStatus(PLAYER_STATUS_CLIMBING);
        return true;
    }

    return BaseClass::ClientCommand(args);
}

//================================================================================
// Devuelve el nombre de la entidad que servirá para crear este jugador
//================================================================================
const char *CPlayer::GetSpawnEntityName()
{
    return "info_player_start";
}

//================================================================================
// Devuelve la entidad que se usará para hacer spawn
//================================================================================
CBaseEntity *CPlayer::EntSelectSpawnPoint()
{
    // Devolvemos el punto de spawn para el Jugador
    if ( m_pSpawner && TheGameRules->IsSpawnMode(SPAWN_MODE_UNIQUE) ) {
        return m_pSpawner;
    }

    CBaseEntity *pSpot = NULL;
    const char *pSpawnPointName = GetSpawnEntityName();

    CUtlVector<CBaseEntity *> nSpawnList;

    do {
        pSpot = gEntList.FindEntityByClassname(pSpot, pSpawnPointName);

        if ( !pSpot )
            continue;

        // Punto de creación inválido
        if ( !TheGameRules->IsSpawnPointValid(pSpot, this) )
            continue;

        // Cada jugador tiene asignado un punto de Spawn
        if ( TheGameRules->IsSpawnMode(SPAWN_MODE_UNIQUE) ) {
            // Ya esta ocupado
            if ( !TheGameRules->CanUseSpawnPoint(pSpot) )
                continue;

            // Lo ocupamos
            m_pSpawner = pSpot;
            TheGameRules->UseSpawnPoint(pSpot);

            return pSpot;
        }

        // Posible punto de spawn
        nSpawnList.AddToTail(pSpot);
    }
    while ( pSpot );

    // Al azar
    if ( TheGameRules->IsSpawnMode(SPAWN_MODE_RANDOM) && nSpawnList.Count() > 0 ) {
        return nSpawnList[RandomInt(0, (nSpawnList.Count() - 1))];
    }

    // Ha ocurrido un problema, usamos el primero que haya
    return gEntList.FindEntityByClassname(NULL, pSpawnPointName);
}

//================================================================================
//================================================================================
void CPlayer::PhysObjectSleep()
{
    IPhysicsObject *pObj = VPhysicsGetObject();

    if ( pObj )
        pObj->Sleep();
}

//================================================================================
//================================================================================
void CPlayer::PhysObjectWake()
{
    IPhysicsObject *pObj = VPhysicsGetObject();

    if ( pObj )
        pObj->Wake();
}

//================================================================================
//================================================================================
void CPlayer::DisableButtons(int nButtons)
{
    m_iButtonsDisabled |= nButtons;
}

//================================================================================
//================================================================================
void CPlayer::EnableButtons(int nButtons)
{
    m_iButtonsDisabled &= ~nButtons;
}

//================================================================================
// Activa todo los botones desactivados
//================================================================================
void CPlayer::RestoreButtons()
{
    m_iButtonsDisabled = 0;
}

//================================================================================
//================================================================================
void CPlayer::ForceButtons(int nButtons)
{
    m_iButtonsForced |= nButtons;
}

//================================================================================
//================================================================================
void CPlayer::UnforceButtons(int nButtons)
{
    m_iButtonsForced &= ~nButtons;
}

//================================================================================
//================================================================================
void CPlayer::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
    BaseClass::Use(pActivator, pCaller, useType, value);

    // Otro jugador nos esta "usando"
    if ( pActivator && pActivator->IsPlayer() ) {
        CPlayer *pRescuer = ToInPlayer(pActivator);

        // Incapacitados, nos estan ayudando...
        if ( IsDejected() && !pRescuer->IsDejected() ) {
            RaiseFromDejected(pRescuer);
        }
    }
}

//================================================================================
//================================================================================
void CPlayer::Touch(CBaseEntity *pOther)
{
    BaseClass::Touch(pOther);
}

//================================================================================
//================================================================================
void CPlayer::InjectMovement(NavRelativeDirType direction)
{
    if ( GetBotController() ) {
        GetBotController()->InjectMovement(direction);
        return;
    }

    if ( !m_InjectedCommand )
        m_InjectedCommand = m_pCurrentCommand;

    if ( !m_InjectedCommand )
        m_InjectedCommand = new CUserCmd();

    switch ( direction ) {
        case FORWARD:
        default:
            m_InjectedCommand->forwardmove = 450.0f;
            InjectButton(IN_FORWARD);
            break;

        case UP:
            m_InjectedCommand->upmove = 450.0f;
            break;

        case DOWN:
            m_InjectedCommand->upmove = -450.0f;
            break;

        case BACKWARD:
            m_InjectedCommand->forwardmove = -450.0f;
            InjectButton(IN_BACK);
            break;

        case LEFT:
            m_InjectedCommand->sidemove = -450.0f;
            InjectButton(IN_LEFT);
            break;

        case RIGHT:
            m_InjectedCommand->sidemove = 450.0f;
            InjectButton(IN_RIGHT);
            break;
    }
}

//================================================================================
//================================================================================
void CPlayer::InjectButton(int btn)
{
    if ( GetBotController() ) {
        GetBotController()->InjectButton(btn);
        return;
    }

    if ( !m_InjectedCommand )
        m_InjectedCommand = m_pCurrentCommand;

    if ( !m_InjectedCommand )
        m_InjectedCommand = new CUserCmd();

    m_InjectedCommand->buttons |= btn;
}

//================================================================================
//================================================================================
void CPlayer::PlayerRunCommand(CUserCmd *ucmd, IMoveHelper *moveHelper)
{
    // Un humano dejando que la I.A. controle su personaje
    if ( !IsBot() && GetBotController() ) {
        CUserCmd *cmd = GetBotController()->GetLastCmd();

        if ( ucmd && cmd ) {
            ucmd->buttons = cmd->buttons;
            ucmd->forwardmove = cmd->forwardmove;
            ucmd->impulse = cmd->impulse;
            ucmd->mousedx = cmd->mousedx;
            ucmd->mousedy = cmd->mousedy;
            ucmd->sidemove = cmd->sidemove;
            ucmd->upmove = cmd->upmove;
            ucmd->viewangles = cmd->viewangles;
            //ucmd->weaponselect = cmd->weaponselect;
            //ucmd->weaponsubtype = cmd->weaponsubtype;
            //SnapEyeAngles(cmd->viewangles);

            SnapEyeAngles(ucmd->viewangles);
            VectorCopy(pl.v_angle, ucmd->viewangles);
        }
    }

    // Visión desactivada
    if ( IsAimingDisabled() ) {
        VectorCopy(pl.v_angle, ucmd->viewangles);
    }

    // Movimiento desactivado
    if ( IsMovementDisabled() ) {
        ucmd->forwardmove = 0;
        ucmd->sidemove = 0;
        ucmd->upmove = 0;
    }

    // Comando inyectado
    else if ( m_InjectedCommand ) {
        ucmd->forwardmove = m_InjectedCommand->forwardmove;
        ucmd->sidemove = m_InjectedCommand->sidemove;
        ucmd->buttons = m_InjectedCommand->buttons;

        delete m_InjectedCommand;
        m_InjectedCommand = NULL;
    }

    BaseClass::PlayerRunCommand(ucmd, moveHelper);
}

//================================================================================
//================================================================================
int CPlayer::ObjectCaps()
{
    // Estamos abatidos,
    // pueden usar E para levantarnos
    if ( IsDejected() ) {
        return (BaseClass::ObjectCaps() | FCAP_CONTINUOUS_USE);
    }

    return BaseClass::ObjectCaps();
}

//================================================================================
//================================================================================
void CPlayer::SnapEyeAnglesZ(int angle)
{
    m_iEyeAngleZ = angle;
}

//================================================================================
//================================================================================
void CPlayer::DebugAddMessage(char *format, ...)
{
    va_list varg;
    char buffer[1024];

    va_start(varg, format);
    vsprintf(buffer, format, varg);
    va_end(varg);

    DebugMessage message;
    message.m_age.Start();
    Q_strncpy(message.m_string, buffer, 1024);

    m_debugMessages.AddToHead(message);
    Msg("[%s] %s \n", GetPlayerName(), message.m_string);

    if ( m_debugMessages.Count() >= 10 )
        m_debugMessages.RemoveMultipleFromTail(1);
}

//================================================================================
//================================================================================
void CPlayer::DebugDisplay()
{
    if ( sv_player_debug.GetInt() == 0 )
        return;

    CPlayer *pPlayer = ToInPlayer(UTIL_PlayerByIndex(sv_player_debug.GetInt()));

    if ( !pPlayer )
        return;

    bool display = false;

    if ( IsLocalPlayerWatchingMe() )
        display = true;

    else if ( pPlayer == this && pPlayer->IsAlive() )
        display = true;

    if ( !display )
        return;

    m_flDebugPosition = 0.13f;

    DebugScreenText(UTIL_VarArgs("%s (%s)", GetPlayerName(), g_PlayerStateNames[GetPlayerState()]));
    DebugScreenText(UTIL_VarArgs("Health: %i / %i", GetHealth(), GetMaxHealth()));
    DebugScreenText("");

    DebugScreenText(UTIL_VarArgs("Is Bot: %i", IsBot()));
    DebugScreenText(UTIL_VarArgs("Has Bot Controller: %s", (GetBotController()) ? "Yes" : "No"));
    DebugScreenText(UTIL_VarArgs("Climbing Hold: %.2f", m_flClimbingHold.Get()));
    DebugScreenText(UTIL_VarArgs("Help Progress: %.2f", m_flHelpProgress.Get()));

    DebugScreenText("");
    DebugScreenText(UTIL_VarArgs("Sprinting: %i", m_bSprinting));
    DebugScreenText(UTIL_VarArgs("Movement Disabled: %i", m_bMovementDisabled));
    DebugScreenText(UTIL_VarArgs("Aiming Disabled: %i", m_bAimingDisabled));
    //DebugScreenText( UTIL_VarArgs( "curTime: %2.f", gpGlobals->curtime ) );

    if ( GetSquad() ) {
        DebugScreenText("");
        DebugScreenText(UTIL_VarArgs("Squad: %s", GetSquad()->GetName()));
        DebugScreenText(UTIL_VarArgs("Members: %i", GetSquad()->GetCount()));
    }

    DebugScreenText("");

    if ( m_LastDamageTimer.HasStarted() )
        DebugScreenText(UTIL_VarArgs("Last Damage: %.2fs", m_LastDamageTimer.GetElapsedTime()));

    if ( m_RaiseHelpTimer.HasStarted() )
        DebugScreenText(UTIL_VarArgs("Raise: %.2fs", m_RaiseHelpTimer.GetElapsedTime()));

    DebugScreenText("");
    DebugScreenText(UTIL_VarArgs("Features: %i", m_nComponents.Count()));

    DebugScreenText("");
    DebugScreenText(UTIL_VarArgs("Attributes: %i", m_nAttributes.Count()));

    FOR_EACH_VEC(m_nAttributes, it)
    {
        CAttribute *pAttribute = m_nAttributes.Element(it);

        //DevMsg("%s\n", m_nAttributes[it]->GetID() );
        DebugScreenText(UTIL_VarArgs("%s:	%2.f	(amount: %.2f)	(rate: %.2f)",
                        pAttribute->GetID(),
                        pAttribute->GetValue(),
                        pAttribute->GetAmount(),
                        pAttribute->GetRate()
        ));

        // Modificadores
        FOR_EACH_VEC(pAttribute->m_nModifiers, key)
        {
            DebugScreenText(UTIL_VarArgs("		%s	(amount: %.2f)	(rate: %.2f) (expire: %.2f)",
                            pAttribute->m_nModifiers[key].name,
                            pAttribute->m_nModifiers[key].amount,
                            pAttribute->m_nModifiers[key].rate,
                            pAttribute->m_nModifiers[key].expire
            ));
        }
    }

    //
    // Mensajes de depuración
    //

    const float fadeAge = 7.0f;
    const float maxAge = 10.0f;

    DebugScreenText("");
    DebugScreenText("");

    FOR_EACH_VEC(m_debugMessages, it)
    {
        DebugMessage *message = &m_debugMessages.Element(it);

        if ( !message )
            continue;

        if ( message->m_age.GetElapsedTime() < maxAge ) {
            int alpha = 255;

            if ( message->m_age.GetElapsedTime() > fadeAge )
                alpha *= (1.0f - (message->m_age.GetElapsedTime() - fadeAge) / (maxAge - fadeAge));

            DebugScreenText(UTIL_VarArgs("%2.f - %s", message->m_age.GetStartTime(), message->m_string), Color(255, 255, 255, alpha));
        }
    }
}

//================================================================================
//================================================================================
void CPlayer::DebugScreenText(const char * pText, Color color, float yPosition, float duration)
{
    if ( yPosition < 0 )
        yPosition = m_flDebugPosition;
    else
        m_flDebugPosition = yPosition;

    NDebugOverlay::ScreenText(0.1f, yPosition, pText, color.r(), color.g(), color.b(), color.a(), duration);
    m_flDebugPosition += 0.02f;
}

//================================================================================
//================================================================================
CON_COMMAND(test_seeds, "")
{
    DevMsg("[test1] SharedRandomInt: %i\n", Utils::RandomInt("test1", 0, 100));
    DevMsg("[test2] SharedRandomInt: %i\n", Utils::RandomInt("test2", 0, 100));
    DevMsg("[test3] SharedRandomInt: %i\n", Utils::RandomInt("test3", 0, 100));
    DevMsg("\n");

    ThreadSleep(2000);

    DevMsg("[test1] SharedRandomInt: %i\n", Utils::RandomInt("test1", 0, 100));
    DevMsg("[test3] SharedRandomInt: %i\n", Utils::RandomInt("test3", 0, 100));
    DevMsg("[test2] SharedRandomInt: %i\n", Utils::RandomInt("test2", 0, 100));
    DevMsg("\n");
}

class CExample
{
public:
    int enemies;
};

CUtlVector<CExample *> g_TestList;

//================================================================================
//================================================================================
CON_COMMAND(test_pointers, "")
{
    CExample *x = new CExample();
    x->enemies = 0;

    CExample *y = new CExample();
    y->enemies = 10;

    g_TestList.AddToTail(x);
    g_TestList.AddToTail(y);

    Msg("%i\n", x->enemies);
    //Msg( "%i\n", y->enemies );

    CExample *e = g_TestList[0];
    e->enemies += 10;

    Msg("%i\n", e->enemies); // 10
    Msg("%i\n", g_TestList[0]->enemies); // 10
    Msg("%i\n", x->enemies); // 10
}

//================================================================================
//================================================================================
CON_COMMAND(test_pointers2, "")
{
    CExample *e = g_TestList[0];
    e->enemies += 10;

    Msg("%i\n", e->enemies); // 20
    Msg("%i\n", g_TestList[0]->enemies); // 20

    g_TestList[0]->enemies += 10;

    Msg("%i\n", e->enemies); // 30
    Msg("%i\n", g_TestList[0]->enemies); // 30
}

//================================================================================
//================================================================================
CON_COMMAND(test_pointers3, "")
{
    CExample *e = g_TestList[0];
    delete e;

    Msg("%i\n", e->enemies); // ?
    Msg("%i\n", g_TestList[0]->enemies); // ?

    g_TestList.Remove(0);

    Msg("%i\n", e->enemies); // ?
    Msg("%i\n", g_TestList[0]->enemies); // 10

    g_TestList.PurgeAndDeleteElements();
}
