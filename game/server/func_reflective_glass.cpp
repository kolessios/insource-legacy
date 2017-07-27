//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
//===========================================================================//

#include "cbase.h"
#include "modelentities.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CFuncReflectiveGlass : public CFuncBrush
{
	DECLARE_DATADESC();
	DECLARE_CLASS( CFuncReflectiveGlass, CFuncBrush );
	DECLARE_SERVERCLASS();

public:
    CNetworkVar( int, m_iReflectionID );
};

// automatically hooks in the system's callbacks
BEGIN_DATADESC( CFuncReflectiveGlass )
    DEFINE_KEYFIELD( m_iReflectionID, FIELD_INTEGER, "ReflectionID" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( func_reflective_glass, CFuncReflectiveGlass );

IMPLEMENT_SERVERCLASS_ST( CFuncReflectiveGlass, DT_FuncReflectiveGlass )
    SendPropInt( SENDINFO( m_iReflectionID ) ),
END_SEND_TABLE()
