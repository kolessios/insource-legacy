//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#ifndef C_FUNC_REFLECTIVE_GLASS
#define C_FUNC_REFLECTIVE_GLASS

#ifdef _WIN32
#pragma once
#endif

#include "c_func_brush.h"

struct cplane_t;
class CViewSetup;

class C_FuncReflectiveGlass : public C_FuncBrush
{
public:
    DECLARE_CLASS( C_FuncReflectiveGlass, C_FuncBrush );
    DECLARE_CLIENTCLASS();

    // C_BaseEntity.
public:
    C_FuncReflectiveGlass();
    virtual ~C_FuncReflectiveGlass();

    int GetReflectionId() { return m_iReflectionID; }

    virtual bool ShouldDraw();
    virtual int	DrawModel( int flags, const RenderableInstance_t &instance );

    //C_FuncReflectiveGlass	*m_pNext;
    int m_iReflectionID;
};

typedef CUtlVector<C_FuncReflectiveGlass *> ReflectiveGlassList;
extern ReflectiveGlassList g_ReflectiveGlassList;

//-----------------------------------------------------------------------------
// Do we have reflective glass in view? If so, what's the reflection plane?
//-----------------------------------------------------------------------------
bool IsReflectiveGlassInView( const CViewSetup& view, cplane_t &plane, C_FuncReflectiveGlass *pReflectiveGlass );

#endif // C_FUNC_REFLECTIVE_GLASS


