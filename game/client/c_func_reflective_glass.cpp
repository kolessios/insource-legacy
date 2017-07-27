//===== Copyright 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//
#include "cbase.h"
#include "view_shared.h"
#include "viewrender.h"
#include "c_func_reflective_glass.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_CLIENTCLASS_DT( C_FuncReflectiveGlass, DT_FuncReflectiveGlass, CFuncReflectiveGlass )
    RecvPropInt( RECVINFO( m_iReflectionID ) ),
END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Globals
//-----------------------------------------------------------------------------

ReflectiveGlassList g_ReflectiveGlassList;

//-----------------------------------------------------------------------------
// Constructor, destructor
//-----------------------------------------------------------------------------
C_FuncReflectiveGlass::C_FuncReflectiveGlass()
{
    g_ReflectiveGlassList.AddToTail( this );
}

C_FuncReflectiveGlass::~C_FuncReflectiveGlass()
{
    g_ReflectiveGlassList.FindAndRemove( this );
} 

bool C_FuncReflectiveGlass::ShouldDraw()
{
    //if ( CurrentViewID() == VIEW_REFLECTION )
        //return false;

	return true;
}

int C_FuncReflectiveGlass::DrawModel( int flags, const RenderableInstance_t & instance )
{
    if ( CurrentViewID() == VIEW_REFLECTION )
        return 0;

    return BaseClass::DrawModel( flags, instance );
}


//-----------------------------------------------------------------------------
// Do we have reflective glass in view?
//-----------------------------------------------------------------------------
bool IsReflectiveGlassInView( const CViewSetup& view, cplane_t &plane, C_FuncReflectiveGlass *pReflectiveGlass )
{
	if ( !pReflectiveGlass )
		return false;

	Frustum_t frustum;
	GeneratePerspectiveFrustum( view.origin, view.angles, view.zNear, view.zFar, view.fov, view.m_flAspectRatio, frustum );

	cplane_t localPlane;
	Vector vecOrigin, vecWorld, vecDelta, vecForward;
	AngleVectors( view.angles, &vecForward, NULL, NULL );

	if ( pReflectiveGlass->IsDormant() )
		return false;

	Vector vecMins, vecMaxs;
	pReflectiveGlass->GetRenderBoundsWorldspace( vecMins, vecMaxs );

    if ( frustum.CullBox( vecMins, vecMaxs ) )
        return false;;

	const model_t *pModel = pReflectiveGlass->GetModel();
	const matrix3x4_t& mat = pReflectiveGlass->EntityToWorldTransform();

	int nCount = modelinfo->GetBrushModelPlaneCount( pModel );
	for ( int i = 0; i < nCount; ++i )
	{
		modelinfo->GetBrushModelPlane( pModel, i, localPlane, &vecOrigin );

		MatrixTransformPlane( mat, localPlane, plane );			// Transform to world space
		VectorTransform( vecOrigin, mat, vecWorld );
					 
		if ( view.origin.Dot( plane.normal ) <= plane.dist )	// Check for view behind plane
			return false;
			
		VectorSubtract( vecWorld, view.origin, vecDelta );		// Backface cull
		if ( vecDelta.Dot( plane.normal ) >= 0 )
			return false;

		return true;
	}

	return false;
}



