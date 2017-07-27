//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef IN_VIEW_VECTORS_H
#define IN_VIEW_VECTORS_H

#pragma once

//====================================================================
// Clase con los distintos vectores de altura para la vista
//====================================================================
class CInViewVectors : public CViewVectors
{
public:
    CInViewVectors(
        Vector vView,
        Vector vHullMin,
        Vector vHullMax,
        Vector vDuckHullMin,
        Vector vDuckHullMax,
        Vector vDuckView,
        Vector vObsHullMin,
        Vector vObsHullMax,
        Vector vDeadViewHeight,
        Vector vDejectedHullMin,
        Vector vDejectedHullMax,
        Vector vDejectedView 
        ) :
        CViewVectors( vView, vHullMin, vHullMax, vDuckHullMin, vDuckHullMax, vDuckView, vObsHullMin, vObsHullMax, vDeadViewHeight )
    {
        m_vDejectedHullMin    = vDejectedHullMin;
        m_vDejectedHullMax    = vDejectedHullMax;
        m_vDejectedView        = vDejectedView;
    }

public:
    Vector m_vDejectedHullMin;
    Vector m_vDejectedHullMax;
    Vector m_vDejectedView;
};

//====================================================================
// Inicialización
//====================================================================

static CInViewVectors g_InViewVectors(
    Vector( 0, 0, 60 ),       // VEC_VIEW (m_vView) 
    Vector( -13, -13, 0 ),    // VEC_HULL_MIN (m_vHullMin)
    Vector( 13, 13, 68 ),     // VEC_HULL_MAX (m_vHullMax)

    Vector( -13, -13, 0 ),    // VEC_DUCK_HULL_MIN (m_vDuckHullMin)
    Vector( 13, 13, 36 ),     // VEC_DUCK_HULL_MAX    (m_vDuckHullMax)
    Vector( 0, 0, 28 ),       // VEC_DUCK_VIEW        (m_vDuckView)

    Vector( -10, -10, -10 ),  // VEC_OBS_HULL_MIN    (m_vObsHullMin)
    Vector( 10, 10, 10 ),     // VEC_OBS_HULL_MAX    (m_vObsHullMax)

    Vector( 0, 0, 15 ),       // VEC_DEAD_VIEWHEIGHT (m_vDeadViewHeight)

    Vector( -30, -30, 0 ),    // VEC_DEJECTED_HULL_MIN
    Vector( 30, 30, 25 ),     // VEC_DEJECTED_HULL_MAX
    Vector( 0, 0, 25 )        // VEC_DEJECTED_VIEWHEIGHT (vDejectedView)
);

#endif // IN_VIEW_VECTORS_H