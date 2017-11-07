//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "in_utils.h"

#include "physobj.h"
#include "collisionutils.h"
#include "movevars_shared.h"

#include "ai_basenpc.h"
#include "ai_initutils.h"
#include "ai_hint.h"

#include "BasePropDoor.h"
#include "doors.h"
#include "func_breakablesurf.h"

#include "bots\bot_manager.h"
#include "bots\bot_defs.h"
#include "bots\interfaces\ibot.h"

#include "nav_pathfind.h"
#include "util_shared.h"

#ifdef INSOURCE_DLL
#include "players_system.h"
#include "in_player.h"
#include "in_attribute_system.h"
#endif

#ifdef APOCALYPSE
#include "ap_player.h"
#endif

#include "datacache/imdlcache.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef INSOURCE_DLL
extern ConVar sv_gameseed;
#else
ConVar sv_gameseed("sv_gameseed", "", FCVAR_NOT_CONNECTED | FCVAR_SERVER);
#endif

#define COS_TABLE_SIZE 256
static float cosTable[COS_TABLE_SIZE];

//================================================================================
//================================================================================
static int SeedLineHash(int seedvalue, const char *sharedname)
{
    CRC32_t retval;

    CRC32_Init(&retval);

    CRC32_ProcessBuffer(&retval, (void *)&seedvalue, sizeof(int));
    CRC32_ProcessBuffer(&retval, (void *)sharedname, Q_strlen(sharedname));

    CRC32_Final(&retval);

    return (int)(retval);
}

//================================================================================
//================================================================================
float Utils::RandomFloat(const char *sharedname, float flMinVal, float flMaxVal)
{
    int seed = SeedLineHash(sv_gameseed.GetInt(), sharedname);
    RandomSeed(seed);
    return ::RandomFloat(flMinVal, flMaxVal);
}

//================================================================================
//================================================================================
int Utils::RandomInt(const char *sharedname, int iMinVal, int iMaxVal)
{
    int seed = SeedLineHash(sv_gameseed.GetInt(), sharedname);
    RandomSeed(seed);
    return ::RandomInt(iMinVal, iMaxVal);
}

//================================================================================
//================================================================================
Vector Utils::RandomVector(const char *sharedname, float minVal, float maxVal)
{
    int seed = SeedLineHash(sv_gameseed.GetInt(), sharedname);
    RandomSeed(seed);
    // HACK:  Can't call RandomVector/Angle because it uses rand() not vstlib Random*() functions!
    // Get a random vector.
    Vector random;
    random.x = ::RandomFloat(minVal, maxVal);
    random.y = ::RandomFloat(minVal, maxVal);
    random.z = ::RandomFloat(minVal, maxVal);
    return random;
}

//================================================================================
//================================================================================
QAngle Utils::RandomAngle(const char *sharedname, float minVal, float maxVal)
{
    Assert(CBaseEntity::GetPredictionRandomSeed() != -1);

    int seed = SeedLineHash(sv_gameseed.GetInt(), sharedname);
    RandomSeed(seed);

    // HACK:  Can't call RandomVector/Angle because it uses rand() not vstlib Random*() functions!
    // Get a random vector.
    Vector random;
    random.x = ::RandomFloat(minVal, maxVal);
    random.y = ::RandomFloat(minVal, maxVal);
    random.z = ::RandomFloat(minVal, maxVal);
    return QAngle(random.x, random.y, random.z);
}

//================================================================================
//================================================================================
void Utils::NormalizeAngle(float& fAngle)
{
    if ( fAngle < 0.0f )
        fAngle += 360.0f;
    else if ( fAngle >= 360.0f )
        fAngle -= 360.0f;
}

//================================================================================
//================================================================================
void Utils::DeNormalizeAngle(float& fAngle)
{
    if ( fAngle < -180.0f )
        fAngle += 360.0f;
    else if ( fAngle >= 180.0f )
        fAngle -= 360.0f;
}

//================================================================================
//================================================================================
void Utils::GetAngleDifference(QAngle const& angOrigin, QAngle const& angDestination, QAngle& angDiff)
{
    angDiff = angDestination - angOrigin;

    Utils::DeNormalizeAngle(angDiff.x);
    Utils::DeNormalizeAngle(angDiff.y);
}

//================================================================================
//================================================================================
bool Utils::IsBreakableSurf(CBaseEntity *pEntity)
{
    if ( !pEntity )
        return false;

    // No puede recibir daño
    if ( pEntity->m_takedamage != DAMAGE_YES )
        return false;

    // Es una superficie que se puede romper
    if ( (dynamic_cast<CBreakableSurface *>(pEntity)) )
        return true;

    // Es una pared que se puede romper
    if ( (dynamic_cast<CBreakable *>(pEntity)) )
        return true;

    return false;
}

//================================================================================
//================================================================================
bool Utils::IsBreakable(CBaseEntity *pEntity)
{
    if ( !pEntity )
        return false;

    // No puede recibir daño
    if ( pEntity->m_takedamage != DAMAGE_YES )
        return false;

    // Es una entidad que se puede romper
    if ( (dynamic_cast<CBreakableProp *>(pEntity)) )
        return true;

    // Es una pared que se puede romper
    if ( (dynamic_cast<CBreakable *>(pEntity)) )
        return true;

    // Es una superficie que se puede romper
    if ( (dynamic_cast<CBreakableSurface *>(pEntity)) ) {
        CBreakableSurface *surf = static_cast< CBreakableSurface * >(pEntity);

        // Ya esta roto
        if ( surf->m_bIsBroken )
            return false;

        return true;
    }

    // Es una puerta
    if ( (dynamic_cast<CBasePropDoor *>(pEntity)) )
        return true;

    return false;
}

//================================================================================
//================================================================================
bool Utils::IsDoor(CBaseEntity *pEntity)
{
    if ( !pEntity )
        return false;

    CBaseDoor *pDoor = dynamic_cast<CBaseDoor *>(pEntity);

    if ( pDoor )
        return true;

    CBasePropDoor *pPropDoor = dynamic_cast<CBasePropDoor *>(pEntity);

    if ( pPropDoor )
        return true;

    return false;
}

//================================================================================
//================================================================================
CBaseEntity *Utils::FindNearestPhysicsObject(const Vector &vOrigin, float fMaxDist, float fMinMass, float fMaxMass, CBaseEntity *pFrom)
{
    CBaseEntity *pFinalEntity = NULL;
    CBaseEntity *pThrowEntity = NULL;
    float flNearestDist = 0;

    // Buscamos los objetos que podemos lanzar
    do {
        // Objetos con físicas
        pThrowEntity = gEntList.FindEntityByClassnameWithin(pThrowEntity, "prop_physics", vOrigin, fMaxDist);

        // Ya no existe
        if ( !pThrowEntity )
            continue;

        // La entidad que lo quiere no puede verlo
        if ( pFrom ) {
            if ( !pFrom->FVisible(pThrowEntity) )
                continue;
        }

        // No se ha podido acceder a la información de su fisica
        if ( !pThrowEntity->VPhysicsGetObject() )
            continue;

        // No se puede mover o en si.. lanzar
        if ( !pThrowEntity->VPhysicsGetObject()->IsMoveable() )
            continue;

        Vector v_center = pThrowEntity->WorldSpaceCenter();
        float flDist = UTIL_DistApprox2D(vOrigin, v_center);

        // Esta más lejos que el objeto anterior
        if ( flDist > flNearestDist && flNearestDist != 0 )
            continue;

        // Calcular la distancia al enemigo
        if ( pFrom && pFrom->IsNPC() ) {
            CAI_BaseNPC *pNPC = dynamic_cast<CAI_BaseNPC *>(pFrom);

            if ( pNPC && pNPC->GetEnemy() ) {
                Vector vecDirToEnemy = pNPC->GetEnemy()->GetAbsOrigin() - pNPC->GetAbsOrigin();
                vecDirToEnemy.z = 0;

                Vector vecDirToObject = pThrowEntity->WorldSpaceCenter() - vOrigin;
                VectorNormalize(vecDirToObject);
                vecDirToObject.z = 0;

                if ( DotProduct(vecDirToEnemy, vecDirToObject) < 0.8 )
                    continue;
            }
        }

        // Obtenemos su peso
        float pEntityMass = pThrowEntity->VPhysicsGetObject()->GetMass();

        // Muy liviano
        if ( pEntityMass < fMinMass && fMinMass > 0 )
            continue;

        // ¡Muy pesado!
        if ( pEntityMass > fMaxMass )
            continue;

        // No lanzar objetos que esten sobre mi cabeza
        if ( v_center.z > vOrigin.z )
            continue;

        if ( pFrom ) {
            Vector vecGruntKnees;
            pFrom->CollisionProp()->NormalizedToWorldSpace(Vector(0.5f, 0.5f, 0.25f), &vecGruntKnees);

            vcollide_t *pCollide = modelinfo->GetVCollide(pThrowEntity->GetModelIndex());

            Vector objMins, objMaxs;
            physcollision->CollideGetAABB(&objMins, &objMaxs, pCollide->solids[0], pThrowEntity->GetAbsOrigin(), pThrowEntity->GetAbsAngles());

            if ( objMaxs.z < vecGruntKnees.z )
                continue;
        }

        // Este objeto es perfecto, guardamos su distancia por si encontramos otro más cerca
        flNearestDist = flDist;
        pFinalEntity = pThrowEntity;

    }
    while ( pThrowEntity );

    // No pudimos encontrar ningún objeto
    if ( !pFinalEntity )
        return NULL;

    return pFinalEntity;
}

//================================================================================
//================================================================================
bool Utils::IsMoveableObject(CBaseEntity *pEntity)
{
    if ( !pEntity || pEntity->IsWorld() )
        return false;

    if ( pEntity->IsPlayer() || pEntity->IsNPC() )
        return true;

    if ( !pEntity->VPhysicsGetObject() )
        return false;

    if ( !pEntity->VPhysicsGetObject()->IsMoveable() )
        return false;

    return true;
}

//================================================================================
//================================================================================
bool Utils::RunOutEntityLimit(int iTolerance)
{
    if ( gEntList.NumberOfEntities() < (gpGlobals->maxEntities - iTolerance) )
        return false;

    return true;
}

//================================================================================
//================================================================================
IGameEvent *Utils::CreateLesson(const char *pLesson, CBaseEntity *pSubject)
{
    IGameEvent *pEvent = gameeventmanager->CreateEvent(pLesson, true);

    if ( pEvent ) {
        if ( pSubject )
            pEvent->SetInt("subject", pSubject->entindex());
    }

    return pEvent;
}

#ifdef INSOURCE_DLL
//================================================================================
// Aplica el modificador del atributo a todos los jugadores en el radio especificado
//================================================================================
bool Utils::AddAttributeModifier(const char *name, float radius, const Vector &vecPosition, int team)
{
    CTeamRecipientFilter filter(team);
    return AddAttributeModifier(name, radius, vecPosition, filter);
}

//================================================================================
// Aplica el modificador a todos los jugadores
//================================================================================
bool Utils::AddAttributeModifier(const char *name, int team)
{
    return AddAttributeModifier(name, 0.0f, vec3_origin, team);
}

//================================================================================
// Aplica el modificador del atributo a todos los jugadores en el radio especificado
//================================================================================
bool Utils::AddAttributeModifier(const char *name, float radius, const Vector &vecPosition, CRecipientFilter &filter)
{
    AttributeInfo info;

    // El modificador no existe
    if ( !TheAttributeSystem->GetModifierInfo(name, info) )
        return false;

    for ( int i = 1; i <= gpGlobals->maxClients; ++i ) {
        CPlayer *pPlayer = ToInPlayer(UTIL_PlayerByIndex(i));

        if ( !pPlayer )
            continue;

        // Jugador muerto
        if ( !pPlayer->IsAlive() )
            continue;

        // Esta muy lejos
        if ( radius > 0 ) {
            float distance = pPlayer->GetAbsOrigin().DistTo(vecPosition);

            if ( distance > radius )
                continue;
        }

        // Verificamos si esta en el filtro
        for ( int s = 0; s < filter.GetRecipientCount(); ++s ) {
            CPlayer *pItem = ToInPlayer(UTIL_PlayerByIndex(filter.GetRecipientIndex(s)));

            // Aquí esta
            if ( pItem == pPlayer ) {
                // Agregamos el modificador
                pPlayer->AddAttributeModifier(name);
            }
        }
    }

    return true;
}
#endif

//================================================================================
// Set [bones] with the Hitbox IDs of the entity.
// TODO: Rename? (I think technically there are no "bones" here.)
//================================================================================
bool Utils::GetEntityBones(CBaseEntity *pEntity, HitboxBones &bones)
{
    // Invalid
    bones.head = -1;
    bones.chest = -1;

#ifdef APOCALYPSE
    if ( pEntity->ClassMatches("npc_infected") ) {
        bones.head = 16;
        bones.chest = 10;
        return true;
    }

    if ( pEntity->IsPlayer() ) {
        bones.head = 12;
        bones.chest = 9;
        return true;
    }
#elif HL2MP
    // TODO
#endif

    return false;
}

//================================================================================
// Fill in [positions] with the entity's hitbox positions
// If we do not have the Hitbox IDs of the entity then it will return generic positions.
// Use with care: this is often heavy for the engine.
//================================================================================
bool Utils::ComputeHitboxPositions(CBaseEntity *pEntity, HitboxPositions &positions)
{
    VPROF_BUDGET("Utils::ComputeHitboxPositions", VPROF_BUDGETGROUP_BOTS);

    positions.Reset();

    if ( pEntity == NULL )
        return false;

    // Generic Positions
    positions.head = pEntity->EyePosition();
    positions.chest = pEntity->WorldSpaceCenter();

    // It doesn't rely on bones
    positions.feet = pEntity->GetAbsOrigin();
    positions.feet.z += 5.0f;

    CBaseAnimating *pModel = pEntity->GetBaseAnimating();

    if ( pModel == NULL )
        return false;

    MDLCACHE_CRITICAL_SECTION();
    CStudioHdr *studioHdr = pModel->GetModelPtr();

    if ( studioHdr == NULL )
        return false;

    mstudiohitboxset_t *set = studioHdr->pHitboxSet(pModel->GetHitboxSet());

    if ( set == NULL )
        return false;

    QAngle angles;
    mstudiobbox_t *box = NULL;
    HitboxBones bones;

    if ( !GetEntityBones(pEntity, bones) )
        return false;

    if ( bones.head >= 0 ) {
        box = set->pHitbox(bones.head);
        pModel->GetBonePosition(box->bone, positions.head, angles);
    }

    if ( bones.chest >= 0 ) {
        box = set->pHitbox(bones.chest);
        pModel->GetBonePosition(box->bone, positions.chest, angles);
    }

    positions.frame = gpGlobals->framecount;
    return true;
}

HitboxPositions g_CacheHitboxPositions[2048];

//================================================================================
// Fill in [positions] with the entity's hitbox positions
// Use the cache of the current frame if it exists, together we avoid mistreating the engine.
//================================================================================
bool Utils::GetHitboxPositions(CBaseEntity * pEntity, HitboxPositions & positions)
{
    HitboxPositions *info = &g_CacheHitboxPositions[pEntity->entindex()];

    // Use cache!
    if ( info->IsValid() ) {
        positions = *info;
        return true;
    }

    // We compute... The engine screams in pain.
    if ( ComputeHitboxPositions(pEntity, *info) ) {
        positions = *info;
        Assert(g_CacheHitboxPositions[pEntity->entindex()].IsValid());
        return true;
    }

    return false;
}

//================================================================================
// Sets [vecPosition] to the desired hitbox position of the entity'
// 
//================================================================================
bool Utils::GetHitboxPosition(CBaseEntity *pEntity, Vector &vecPosition, HitboxType type)
{
    vecPosition.Invalidate();

    HitboxPositions positions;
    GetHitboxPositions(pEntity, positions);

    if ( !positions.IsValid() )
        return false;

    switch ( type ) {
        case HEAD:
            vecPosition = positions.head;
            break;

        case CHEST:
        default:
            vecPosition = positions.chest;
            break;

        case FEET:
            vecPosition = positions.feet;
            break;
    }

    return true;
}

//================================================================================
//================================================================================
void Utils::InitBotTrig()
{
    for ( int i = 0; i<COS_TABLE_SIZE; ++i ) {
        float angle = (float)(2.0f * M_PI * i / (float)(COS_TABLE_SIZE - 1));
        cosTable[i] = (float)cos(angle);
    }
}

//================================================================================
//================================================================================
float Utils::BotCOS(float angle)
{
    angle = AngleNormalizePositive(angle);
    int i = (int)(angle * (COS_TABLE_SIZE - 1) / 360.0f);
    return cosTable[i];
}

//================================================================================
//================================================================================
float Utils::BotSIN(float angle)
{
    angle = AngleNormalizePositive(angle - 90);
    int i = (int)(angle * (COS_TABLE_SIZE - 1) / 360.0f);
    return cosTable[i];
}

//================================================================================
//================================================================================
bool Utils::IsIntersecting2D(const Vector &startA, const Vector &endA, const Vector &startB, const Vector &endB, Vector *result)
{
    float denom = (endA.x - startA.x) * (endB.y - startB.y) - (endA.y - startA.y) * (endB.x - startB.x);
    if ( denom == 0.0f ) {
        // parallel
        return false;
    }

    float numS = (startA.y - startB.y) * (endB.x - startB.x) - (startA.x - startB.x) * (endB.y - startB.y);
    if ( numS == 0.0f ) {
        // coincident
        return true;
    }

    float numT = (startA.y - startB.y) * (endA.x - startA.x) - (startA.x - startB.x) * (endA.y - startA.y);

    float s = numS / denom;
    if ( s < 0.0f || s > 1.0f ) {
        // intersection is not within line segment of startA to endA
        return false;
    }

    float t = numT / denom;
    if ( t < 0.0f || t > 1.0f ) {
        // intersection is not within line segment of startB to endB
        return false;
    }

    // compute intesection point
    if ( result )
        *result = startA + s * (endA - startA);

    return true;
}

//================================================================================
//================================================================================
CPlayer *Utils::GetClosestPlayer(const Vector &vecPosition, float *distance, CPlayer *pIgnore, int team)
{
    CPlayer *pClosest = NULL;
    float closeDist = 999999999999.9f;

    for ( int i = 1; i <= gpGlobals->maxClients; ++i ) {
        CPlayer *pPlayer = ToInPlayer(UTIL_PlayerByIndex(i));

        if ( !pPlayer )
            continue;

        if ( !pPlayer->IsAlive() )
            continue;

        if ( pPlayer == pIgnore )
            continue;

        if ( team && pPlayer->GetTeamNumber() != team )
            continue;

        Vector position = pPlayer->GetAbsOrigin();
        float dist = vecPosition.DistTo(position);

        if ( dist < closeDist ) {
            closeDist = dist;
            pClosest = pPlayer;
        }
    }

    if ( distance )
        *distance = closeDist;

    return pClosest;
}

//================================================================================
// Devuelve si algún jugador del equipo especificado esta en el rango indicado
// cerca de la posición indicada
//================================================================================
bool Utils::IsSpotOccupied(const Vector &vecPosition, CPlayer *pIgnore, float closeRange, int avoidTeam)
{
    float distance;
    CPlayer *pPlayer = Utils::GetClosestPlayer(vecPosition, &distance, pIgnore, avoidTeam);

    if ( pPlayer && distance < closeRange )
        return true;

    return false;
}

CPlayer * Utils::GetClosestPlayerByClass(const Vector & vecPosition, Class_T classify, float *distance, CPlayer *pIgnore)
{
    CPlayer *pClosest = NULL;
    float closeDist = 999999999999.9f;

    for ( int i = 1; i <= gpGlobals->maxClients; ++i ) {
        CPlayer *pPlayer = ToInPlayer(UTIL_PlayerByIndex(i));

        if ( !pPlayer )
            continue;

        if ( !pPlayer->IsAlive() )
            continue;

        if ( pPlayer == pIgnore )
            continue;

        if ( pPlayer->Classify() != classify )
            continue;

        Vector position = pPlayer->GetAbsOrigin();
        float dist = vecPosition.DistTo(position);

        if ( dist < closeDist ) {
            closeDist = dist;
            pClosest = pPlayer;
        }
    }

    if ( distance )
        *distance = closeDist;

    return pClosest;
}

bool Utils::IsSpotOccupiedByClass(const Vector &vecPosition, Class_T classify, CPlayer * pIgnore, float closeRange)
{
    float distance;
    CPlayer *pPlayer = Utils::GetClosestPlayerByClass(vecPosition, classify, &distance, pIgnore);

    if ( pPlayer && distance < closeRange )
        return true;

    return false;
}

//================================================================================
// Devuelve si algún jugador esta en la línea de fuego (FOV) de un punto de salida
// a un punto de destino
//================================================================================
bool Utils::IsCrossingLineOfFire(const Vector &vecStart, const Vector &vecFinish, CPlayer *pIgnore, int ignoreTeam)
{
    for ( int i = 1; i <= gpGlobals->maxClients; ++i ) {
        CPlayer *pPlayer = ToInPlayer(UTIL_PlayerByIndex(i));

        if ( !pPlayer )
            continue;

        if ( !pPlayer->IsAlive() )
            continue;

        if ( pPlayer == pIgnore )
            continue;

        if ( ignoreTeam && pPlayer->GetTeamNumber() == ignoreTeam )
            continue;

        // compute player's unit aiming vector 
        Vector viewForward;
        AngleVectors(pPlayer->EyeAngles() + pPlayer->GetPunchAngle(), &viewForward);

        const float longRange = 5000.0f;
        Vector playerOrigin = pPlayer->GetAbsOrigin();
        Vector playerTarget = playerOrigin + longRange * viewForward;

        Vector result(0, 0, 0);

        if ( IsIntersecting2D(vecStart, vecFinish, playerOrigin, playerTarget, &result) ) {
            // simple check to see if intersection lies in the Z range of the path
            float loZ, hiZ;

            if ( vecStart.z < vecFinish.z ) {
                loZ = vecStart.z;
                hiZ = vecFinish.z;
            }
            else {
                loZ = vecFinish.z;
                hiZ = vecStart.z;
            }

            if ( result.z >= loZ && result.z <= hiZ + HumanHeight )
                return true;
        }
    }

    return false;
}

//================================================================================
// Devuelve si la posición es válida usando los filtros de [criteria]
//================================================================================
bool Utils::IsValidSpot(const Vector & vecSpot, const CSpotCriteria & criteria)
{
    // Only if it is within the limit range
    if ( criteria.GetMaxRange() > 0 ) {
        if ( criteria.GetOrigin().DistTo(vecSpot) > criteria.GetMaxRange() ) {
            return false;
        }
    }

#ifdef INSOURCE_DLL
    // Only if we can see it
    if ( criteria.HasFlags(FLAG_ONLY_VISIBLE) && criteria.GetPlayer() ) {
        if ( !criteria.GetPlayer()->IsAbleToSee(vecSpot, CBaseCombatCharacter::DISREGARD_FOV) ) {
            return false;
        }
    }
#endif

    // Only if it is out of the line of fire of other players
    if ( criteria.HasFlags(FLAG_OUT_OF_LINE_OF_FIRE) ) {
        if ( IsCrossingLineOfFire(criteria.GetOrigin(), vecSpot, criteria.GetPlayer()) ) {
            return false;
        }
    }

    // We have to avoid players of a team, they can be enemies.
    if ( criteria.GetAvoidTeam() != TEAM_UNASSIGNED ) {
        // Only if there are no enemies nearby
        if ( IsSpotOccupied(vecSpot, criteria.GetPlayer(), criteria.GetMinRangeFromAvoid(), criteria.GetAvoidTeam()) ) {
            return false;
        }

#ifdef INSOURCE_DLL
        CUsersRecipientFilter filter(criteria.GetAvoidTeam(), false);

        // Only if it is outside the enemy's vision
        if ( criteria.HasFlags(FLAG_OUT_OF_AVOID_VISIBILITY) ) {
            if ( ThePlayersSystem->IsAbleToSee(vecSpot, filter, CBaseCombatCharacter::DISREGARD_FOV) ) {
                return false;
            }
        }

        // Only places where we have the opportunity to shoot while standing.
        if ( criteria.HasFlags(FLAG_FIRE_OPPOORTUNITY) ) {
            // Position where the eyes would be when standing.
            Vector vecEyes = vecSpot;
            vecEyes.z += VEC_VIEW.z;

            // There is no visibility towards any enemy
            if ( !ThePlayersSystem->IsAbleToSee(vecEyes, filter, CBaseCombatCharacter::DISREGARD_FOV) ) {
                return false;
            }
        }
#endif
    }

    // Only places that are not being used by another Bot.
    if ( !criteria.HasFlags(FLAG_IGNORE_RESERVED) ) {
        if ( TheBots->IsSpotReserved(vecSpot) ) {
            return false;
        }
    }

    return true;
}

//================================================================================
//================================================================================
Vector g_OriginSort;

int SortNearestSpot(const Vector *spot1, const Vector *spot2)
{
    Assert(g_OriginSort.IsValid());

    if ( !g_OriginSort.IsValid() )
        return 0;

    float distance1 = g_OriginSort.DistTo(*spot1);
    float distance2 = g_OriginSort.DistTo(*spot2);

    if ( distance1 > distance2 ) {
        return 1;
    }
    else if ( distance1 == distance2 ) {
        return 0;
    }
    else {
        return -1;
    }
}

int SortNearestHint(CAI_Hint * const *spot1, CAI_Hint * const *spot2)
{
    Assert(g_OriginSort.IsValid());

    if ( !g_OriginSort.IsValid() )
        return 0;

    float distance1 = g_OriginSort.DistTo((*spot1)->GetAbsOrigin());
    float distance2 = g_OriginSort.DistTo((*spot2)->GetAbsOrigin());

    if ( distance1 > distance2 ) {
        return 1;
    }
    else if ( distance1 == distance2 ) {
        return 0;
    }
    else {
        return -1;
    }
}

//================================================================================
// Devuelve una posición donde ocultarse en un rango máximo
//================================================================================
// Los Bots lo usan como primera opción para ocultarse de los enemigos.
//================================================================================
bool Utils::FindNavCoverSpot(const CSpotCriteria &criteria, SpotVector *outputList)
{
    CNavArea *pStartArea = TheNavMesh->GetNearestNavArea(criteria.GetOrigin());

    if ( !pStartArea )
        return false;

    // Type of cover we are looking for
    int hidingType = (criteria.HasFlags(FLAG_USE_SNIPER_POSITIONS)) ? HidingSpot::IDEAL_SNIPER_SPOT : HidingSpot::IN_COVER;

    while ( true ) {
        // We seek cover positions within the range
        CollectHidingSpotsFunctor collector(criteria.GetPlayer(), criteria.GetOrigin(), criteria.GetMaxRange(), hidingType);
        SearchSurroundingAreas(pStartArea, criteria.GetOrigin(), collector, criteria.GetMaxRange());

        for ( int i = 0; i < collector.m_count; ++i ) {
            Vector vecSpot = *collector.m_hidingSpot[i];

            // Invalid spot
            if ( !IsValidSpot(vecSpot, criteria) ) {
                continue;
            }

            // We add it to the list of results
            outputList->AddToTail(vecSpot);
        }

        // So we have not found any place
        if ( outputList->Count() == 0 ) {
            // It's over man
            if ( hidingType == HidingSpot::IN_COVER ) {
                break;
            }

            // Not the ideal, but at least it's something...
            if ( hidingType == HidingSpot::IDEAL_SNIPER_SPOT ) {
                hidingType = HidingSpot::GOOD_SNIPER_SPOT;
                continue;
            }

            // Well, we have to hide, I suppose.
            if ( hidingType == HidingSpot::GOOD_SNIPER_SPOT ) {
                hidingType = HidingSpot::IN_COVER;
                continue;
            }
        }
        else {
            return true;
        }
    }

    return false;
}

//================================================================================
// Devuelve una posición donde ocultarse dentro del area indicado
//================================================================================
// Los Bots lo usan como última opción para ocultarse de los enemigos.
//================================================================================
/*
bool Utils::FindNavCoverSpotInArea(Vector *vecResult, const Vector &vecOrigin, CNavArea *pArea, const CSpotCriteria &criteria, CPlayer *pPlayer, SpotVector *list)
{
    if ( vecResult )
        vecResult->Invalidate();

    if ( !pArea )
        return false;

    CUtlVector<Vector> collector;

    for ( int e = 0; e <= 15; ++e ) {
        Vector position = pArea->GetRandomPoint();
        position.z += HumanCrouchHeight;

        if ( !position.IsValid() )
            continue;

        if ( collector.Find(position) > -1 )
            continue;

        if ( !IsValidSpot(position, criteria) ) {
            continue;
        }

        collector.AddToTail(position);

        // Lo agregamos a la lista
        if ( list )
            list->AddToTail(position);
    }

    if ( !vecResult )
        return false;

    if ( collector.Count() > 0 ) {
        if ( criteria.HasFlags(FLAG_USE_NEAREST) ) {
            float closest = MAX_TRACE_LENGTH;

            for ( int it = 0; it < collector.Count(); ++it ) {
                Vector vecDummy = collector.Element(it);
                float distance = vecOrigin.DistTo(vecDummy);

                if ( distance < closest ) {
                    closest = distance;
                    *vecResult = vecDummy;
                }
            }

            return true;
        }
        else {
            int random = ::RandomInt(0, collector.Count() - 1);
            *vecResult = collector.Element(random);

            return true;
        }
    }

    return false;
}
*/

//================================================================================
// Devuelve una posición donde ocultarse usando los ai_hint (CAI_Hint)
//================================================================================
// Los Bots lo usan como segunda opción para ocultarse de los enemigos.
//================================================================================
CAI_Hint *Utils::FindHintSpot(const CHintCriteria &hintCriteria, const CSpotCriteria &criteria, SpotVector *outputList)
{
    // We fill in a list of all the info_hints within the search criteria.
    CUtlVector<CAI_Hint *> collector;
    CAI_HintManager::FindAllHints(criteria.GetOrigin(), hintCriteria, &collector);

    // None...
    if ( collector.Count() == 0 )
        return NULL;

    CUtlVector<CAI_Hint *> list;

    FOR_EACH_VEC(collector, it)
    {
        CAI_Hint *pHint = collector[it];
        Assert(pHint);

        if ( !pHint )
            continue;

        Vector position = pHint->GetAbsOrigin();

        // Invalid spot
        if ( !IsValidSpot(position, criteria) ) {
            continue;
        }

        // We add it to the list of results
        outputList->AddToTail(position);
        list.AddToTail(pHint);
    }

    // None...
    if ( list.Count() == 0 )
        return NULL;

    // We sort the list, the first ones are the closest
    if ( list.Count() > 1 && criteria.HasFlags(FLAG_USE_NEAREST) ) {
        g_OriginSort = criteria.GetOrigin();
        list.Sort(SortNearestHint);
    }

    if ( list.Count() == 1 || criteria.HasFlags(FLAG_USE_NEAREST) ) {
        return list[0];
    }
    else {
        int random = ::RandomInt(0, list.Count() - 1);
        return list[random];
    }

    return NULL;
}

//================================================================================
//================================================================================
bool Utils::GetSpotCriteria(Vector * vecResult, CSpotCriteria & criteria, SpotVector *outputList)
{
    if ( !criteria.HasValidOrigin() ) {
        Assert(!"This function requires a valid origin position");
        return false;
    }

    if ( !outputList ) {
        SpotVector theList;
        outputList = &theList;
    }

    Assert(outputList);

    // info_hint
    {
        CHintCriteria hintCriteria;

        // We are looking for interesting places
        if ( criteria.HasFlags(FLAG_INTERESTING_SPOT) ) {
            hintCriteria.AddHintType(HINT_WORLD_VISUALLY_INTERESTING);
            hintCriteria.AddHintType(HINT_WORLD_WINDOW);

#ifdef INSOURCE_DLL
            if ( criteria.GetTacticalMode() == TACTICAL_MODE_STEALTH ) {
                hintCriteria.AddHintType(HINT_WORLD_VISUALLY_INTERESTING_STEALTH);
            }

            if ( criteria.GetTacticalMode() == TACTICAL_MODE_ASSAULT ) {
                hintCriteria.AddHintType(HINT_TACTICAL_VISUALLY_INTERESTING);
                hintCriteria.AddHintType(HINT_TACTICAL_ASSAULT_APPROACH);
                hintCriteria.AddHintType(HINT_TACTICAL_PINCH);
            }
#endif
        }

        // We are looking for cover places
        if ( criteria.HasFlags(FLAG_COVER_SPOT) ) {
#ifdef INSOURCE_DLL
            hintCriteria.AddHintType(HINT_TACTICAL_COVER);
#else
            hintCriteria.AddHintType(HINT_TACTICAL_COVER_MED);
            hintCriteria.AddHintType(HINT_TACTICAL_COVER_LOW);
#endif
        }

        // Within a range
        if ( criteria.GetMaxRange() > 0.0f ) {
            hintCriteria.AddIncludePosition(criteria.GetOrigin(), criteria.GetMaxRange());
        }

        FindHintSpot(hintCriteria, criteria, outputList);
    }

    // Navigation Mesh
    {
        FindNavCoverSpot(criteria, outputList);
    }

    if ( outputList->Count() == 0 )
        return false;

    // We sort the list, the first ones are the closest
    if ( outputList->Count() > 1 && criteria.HasFlags(FLAG_USE_NEAREST) ) {
        g_OriginSort = criteria.GetOrigin();
        outputList->Sort(SortNearestSpot);
    }

    // We do not want a result, we were probably just looking for a list.
    if ( !vecResult ) {
        return (outputList->Count() > 0);
    }

    if ( outputList->Count() == 1 || criteria.HasFlags(FLAG_USE_NEAREST) ) {
        *vecResult = outputList->Element(0);
        return true;
    }
    else {
        int random = ::RandomInt(0, outputList->Count() - 1);
        *vecResult = outputList->Element(random);
        return true;
    }

    return false;
}

#ifdef INSOURCE_DLL
//================================================================================
//================================================================================
void Utils::AlienFX_SetColor(CPlayer *pPlayer, unsigned int lights, unsigned int color, float duration)
{
    if ( !pPlayer || !pPlayer->IsNetClient() )
        return;

    if ( pPlayer->ShouldThrottleUserMessage("AlienFX") )
        return;

    CSingleUserRecipientFilter user(pPlayer);
    user.MakeReliable();

    UserMessageBegin(user, "AlienFX");
    WRITE_SHORT(ALIENFX_SETCOLOR);
    WRITE_LONG(lights);
    WRITE_LONG(color);
    WRITE_FLOAT(duration);
    MessageEnd();
}
#endif

void CSpotCriteria::SetPlayer(CPlayer * pPlayer)
{
    m_pPlayer = pPlayer;
    SetOrigin(pPlayer->GetAbsOrigin());
}
