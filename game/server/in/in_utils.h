//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#ifndef UTILS_H
#define UTILS_H

#ifdef _WIN32
#pragma once
#endif

#include "nav.h"
#include "nav_area.h"
#include "bots\bot_defs.h"

#ifdef INSOURCE_DLL
#include "in_shareddefs.h"
#include "alienfx\LFXDecl.h"
#endif

//================================================================================
//================================================================================

class CAI_Hint;
class CHintCriteria;

typedef CUtlVector<Vector> SpotVector;

//================================================================================
//================================================================================
enum SpotCriteriaFlag
{
    FLAG_USE_SNIPER_POSITIONS = (1 << 0),
    FLAG_USE_NEAREST = (1 << 1),

    FLAG_ONLY_VISIBLE = (1 << 2),
    FLAG_OUT_OF_LINE_OF_FIRE = (1 << 3),
    FLAG_OUT_OF_AVOID_VISIBILITY = (1 << 4),
    FLAG_FIRE_OPPOORTUNITY = (1 << 5),
    FLAG_IGNORE_RESERVED = (1 << 6),

    FLAG_INTERESTING_SPOT = (1 << 7),
    FLAG_COVER_SPOT = (1 << 8),

    LAST_FLAG = (1 << 9)
};

//================================================================================
// Información para la búsqueda de un punto interesante en el mapa
//================================================================================
class CSpotCriteria
{
public:
    DECLARE_CLASS_NOBASE(CSpotCriteria);

    CSpotCriteria()
    {
        m_flMaxRange = 1300.0f;
        m_flMinRangeFromAvoid = 500.0f;
        m_iAvoidTeam = TEAM_UNASSIGNED;
        m_iTacticalMode = TACTICAL_MODE_NONE;
        m_pPlayer = NULL;
        m_vecOrigin.Invalidate();
        m_flags = 0;
    }

    virtual void SetOrigin(Vector position)
    {
        m_vecOrigin = position;
    }

    virtual void SetOrigin(CBaseEntity *pEntity)
    {
        if ( !pEntity )
            return;

        SetOrigin(pEntity->GetAbsOrigin());
    }

    virtual bool HasValidOrigin() const
    {
        return m_vecOrigin.IsValid();
    }

    virtual const Vector &GetOrigin() const
    {
        return m_vecOrigin;
    }

    virtual void SetMaxRange(float maxRange)
    {
        m_flMaxRange = maxRange;
    }

    virtual float GetMaxRange() const
    {
        return m_flMaxRange;
    }

    virtual void SetMinRangeFromAvoid(float minDistance)
    {
        m_flMinRangeFromAvoid = minDistance;
    }

    virtual float GetMinRangeFromAvoid() const
    {
        return m_flMinRangeFromAvoid;
    }

    virtual void AvoidTeam(int team)
    {
        m_iAvoidTeam = team;
    }

    virtual void AvoidTeam(CBaseEntity *pEntity)
    {
        if ( !pEntity )
            return;

        AvoidTeam(pEntity->GetTeamNumber());
    }

    virtual int GetAvoidTeam() const
    {
        return m_iAvoidTeam;
    }

    virtual void SetTacticalMode(int mode)
    {
        m_iTacticalMode = mode;
    }

    virtual int GetTacticalMode() const
    {
        return m_iTacticalMode;
    }

    virtual void SetPlayer(CPlayer *pPlayer);

    virtual CPlayer *GetPlayer() const
    {
        return m_pPlayer;
    }

    virtual void SetFlags(int flags)
    {
        m_flags |= flags;
    }

    virtual void ClearFlags(int flags)
    {
        m_flags &= ~flags;
    }

    virtual bool HasFlags(int flags) const
    {
        return ((m_flags & flags) != 0);
    }

protected:
    int m_flags;

    float m_flMaxRange;
    float m_flMinRangeFromAvoid;

    int m_iTacticalMode;
    int m_iAvoidTeam;

    CPlayer *m_pPlayer;
    Vector m_vecOrigin;
};

//================================================================================
// Clase con funciones de utilidad
//================================================================================
class Utils
{
public:
    DECLARE_CLASS_NOBASE(Utils);

    static float RandomFloat(const char *sharedname, float flMinVal, float flMaxVal);
    static int RandomInt(const char *sharedname, int iMinVal, int iMaxVal);
    static Vector RandomVector(const char *sharedname, float minVal, float maxVal);
    static QAngle RandomAngle(const char *sharedname, float minVal, float maxVal);

    static void NormalizeAngle(float& fAngle);
    static void DeNormalizeAngle(float& fAngle);
    static void GetAngleDifference(QAngle const& angOrigin, QAngle const& angDestination, QAngle& angDiff);

    static bool IsBreakable(CBaseEntity *pEntity);
    static bool IsBreakableSurf(CBaseEntity *pEntity);
    static bool IsDoor(CBaseEntity *pEntity);
    static CBaseEntity *FindNearestPhysicsObject(const Vector &vOrigin, float fMaxDist, float fMinMass = 0, float fMaxMass = 500, CBaseEntity *pFrom = NULL);
    static bool IsMoveableObject(CBaseEntity *pEntity);

    static bool RunOutEntityLimit(int iTolerance = 20);
    static IGameEvent *CreateLesson(const char *pLesson, CBaseEntity *pSubject = NULL);

#ifdef INSOURCE_DLL
    static bool AddAttributeModifier(const char *name, float radius, const Vector &vecPosition, int team = TEAM_ANY);
    static bool AddAttributeModifier(const char *name, float radius, const Vector &vecPosition, CRecipientFilter &filter);
    static bool AddAttributeModifier(const char *name, int team = TEAM_ANY);
#endif

    static bool GetEntityBones(CBaseEntity *pEntity, HitboxBones &bones);
    static bool ComputeHitboxPositions(CBaseEntity *pEntity, HitboxPositions &positions);
    static bool GetHitboxPositions(CBaseEntity *pEntity, HitboxPositions &positions);

    static bool GetHitboxPosition(CBaseEntity *pEntity, Vector &vecPosition, HitboxType type);

    static void InitBotTrig();
    static float BotCOS(float angle);
    static float BotSIN(float angle);

    static bool IsIntersecting2D(const Vector &startA, const Vector &endA, const Vector &startB, const Vector &endB, Vector *result = NULL);

    static CPlayer *GetClosestPlayer(const Vector &vecPosition, float *distance = NULL, CPlayer *pIgnore = NULL, int team = NULL);
    static bool IsSpotOccupied(const Vector &vecPosition, CPlayer *pIgnore = NULL, float closeRange = 75.0f, int avoidTeam = NULL);

    static CPlayer *GetClosestPlayerByClass(const Vector &vecPosition, Class_T classify, float *distance = NULL, CPlayer *pIgnore = NULL);
    static bool IsSpotOccupiedByClass(const Vector &vecPosition, Class_T classify, CPlayer *pIgnore = NULL, float closeRange = 75.0f);

    static bool IsCrossingLineOfFire(const Vector &vecStart, const Vector &vecFinish, CPlayer *pIgnore = NULL, int ignoreTeam = NULL);
    static bool IsValidSpot(const Vector &vecSpot, const CSpotCriteria &criteria);

    // 

    static bool FindNavCoverSpot(const CSpotCriteria &criteria, SpotVector *list);
    //static bool FindNavCoverSpotInArea(Vector *vecResult, const Vector &vecOrigin, CNavArea *pArea, const CSpotCriteria &criteria, CPlayer *pPlayer = NULL, SpotVector *list = NULL);

    static CAI_Hint *FindHintSpot(const CHintCriteria &hintCriteria, const CSpotCriteria &criteria, SpotVector *list);

    static bool GetSpotCriteria(Vector *vecResult, CSpotCriteria &criteria, SpotVector *list = NULL);

#ifdef INSOURCE_DLL
    static void AlienFX_SetColor(CPlayer *pPlayer, unsigned int lights, unsigned int color, float duration = 2.0f);
#endif
};

extern Vector g_OriginSort;
extern int SortNearestSpot(const Vector *spot1, const Vector *spot2);
extern int SortNearestHint(CAI_Hint * const *spot1, CAI_Hint * const *spot2);

//================================================================================
// CollectHidingSpotsFunctor
//================================================================================
class CollectHidingSpotsFunctor
{
public:
    CollectHidingSpotsFunctor(CPlayer *me, const Vector &origin, float range, int flags, Place place = UNDEFINED_PLACE) : m_origin(origin)
    {
        m_me = me;
        m_count = 0;
        m_range = range;
        m_flags = (unsigned char)flags;
        m_place = place;
        m_totalWeight = 0;
    }

    enum
    {
        MAX_SPOTS = 256
    };

    bool operator() (CNavArea *area)
    {
        // if a place is specified, only consider hiding spots from areas in that place
        if ( m_place != UNDEFINED_PLACE && area->GetPlace() != m_place )
            return true;

        // collect all the hiding spots in this area
        const HidingSpotVector *list = area->GetHidingSpots();

        FOR_EACH_VEC((*list), it)
        {
            const HidingSpot *spot = (*list)[it];

            // if we've filled up, stop searching
            if ( m_count == MAX_SPOTS ) {
                return false;
            }

            // make sure hiding spot is in range
            if ( m_range > 0.0f ) {
                //if ( (spot->GetPosition() - m_origin).IsLengthGreaterThan(m_range) )
                if ( m_origin.DistTo(spot->GetPosition()) > m_range ) {
                    continue;
                }
            }

            // if a Player is using this hiding spot, don't consider it
            if ( Utils::IsSpotOccupied(spot->GetPosition(), m_me) ) {
                // player is in hiding spot
                /// @todo Check if player is moving or sitting still
                continue;
            }

            if ( spot->GetArea() && (spot->GetArea()->GetAttributes() & NAV_MESH_DONT_HIDE) ) {
                // the area has been marked as DONT_HIDE since the last analysis, so let's ignore it
                continue;
            }

            if ( spot->GetArea() && spot->GetArea()->IsUnderwater() )
                continue;

            // only collect hiding spots with matching flags
            if ( m_flags & spot->GetFlags() ) {
                m_hidingSpot[m_count] = &spot->GetPosition();
                m_hidingSpotWeight[m_count] = m_totalWeight;

                // if it's an 'avoid' area, give it a low weight
                if ( spot->GetArea() && (spot->GetArea()->GetAttributes() & NAV_MESH_AVOID) ) {
                    m_totalWeight += 1;
                }
                else {
                    m_totalWeight += 2;
                }

                ++m_count;
            }
        }

        return (m_count < MAX_SPOTS);
    }

    /**
    * Remove the spot at index "i"
    */
    void RemoveSpot(int i)
    {
        if ( m_count == 0 )
            return;

        for ( int j = i + 1; j<m_count; ++j )
            m_hidingSpot[j - 1] = m_hidingSpot[j];

        --m_count;
    }


    int GetRandomHidingSpot(void)
    {
        int weight = RandomInt(0, m_totalWeight - 1);
        for ( int i = 0; i<m_count - 1; ++i ) {
            // if the next spot's starting weight is over the target weight, this spot is the one
            if ( m_hidingSpotWeight[i + 1] >= weight ) {
                return i;
            }
        }

        // if we didn't find any, it's the last one
        return m_count - 1;
    }

    CPlayer *m_me;
    const Vector &m_origin;
    float m_range;

    const Vector *m_hidingSpot[MAX_SPOTS];
    int m_hidingSpotWeight[MAX_SPOTS];
    int m_totalWeight;
    int m_count;

    unsigned char m_flags;

    Place m_place;
};

#endif