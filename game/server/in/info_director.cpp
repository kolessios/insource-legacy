//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#include "cbase.h"
#include "info_director.h"

#include "director_manager.h"
#include "director.h"



// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
// Información y Red
//================================================================================

LINK_ENTITY_TO_CLASS( info_director, CInfoDirector );

BEGIN_DATADESC( CInfoDirector )
    DEFINE_KEYFIELD( m_bDisabled, FIELD_BOOLEAN, "StartDisabled" ),
    DEFINE_KEYFIELD( m_szPopulation, FIELD_STRING, "Population" ),

	// Inputs
    DEFINE_INPUTFUNC( FIELD_VOID, "Stop", InputStop ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Resume", InputResume ),

	DEFINE_INPUTFUNC( FIELD_VOID, "SetNormal", InputSetNormal ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "StartPanic", InputStartPanic ),
	DEFINE_INPUTFUNC( FIELD_VOID, "StartInfinitePanic", InputStartInfinitePanic ),
	DEFINE_INPUTFUNC( FIELD_VOID, "StartFinale", InputStartFinale ),
	DEFINE_INPUTFUNC( FIELD_VOID, "KillMinions", InputKillMinions ),
    DEFINE_INPUTFUNC( FIELD_STRING, "SetPopulation", InputSetPopulation ),
END_DATADESC()

//================================================================================
//================================================================================
void CInfoDirector::Spawn()
{
    BaseClass::Spawn();

    if ( m_szPopulation != NULL_STRING )
        TheDirectorManager->SetPopulation( STRING(m_szPopulation) );

    if ( m_bDisabled )
        TheDirector->Stop();
}

void CInfoDirector::InputStop( inputdata_t &inputdata )
{
    TheDirector->Stop();
}

void CInfoDirector::InputResume( inputdata_t &inputdata )
{
	TheDirector->Resume();
}

void CInfoDirector::InputSetNormal( inputdata_t &inputdata )
{
	TheDirector->SetStatus( STATUS_NORMAL );
}

void CInfoDirector::InputStartPanic( inputdata_t &inputdata )
{
	int hordes = inputdata.value.Int();

	// Si es menor a 1, usemos la que el Director recomienda
	if ( hordes < 1 ) {
		hordes = TheDirector->GetPanicEventHordes();
	}

	TheDirector->StartPanic( hordes );
}

void CInfoDirector::InputStartInfinitePanic( inputdata_t &inputdata )
{
	TheDirector->StartPanic( INFINITE );
}

void CInfoDirector::InputStartFinale( inputdata_t &inputdata )
{
	TheDirector->SetStatus( STATUS_FINALE );
}

void CInfoDirector::InputKillMinions( inputdata_t &inputdata )
{
	TheDirector->KillAll( false );
}

void CInfoDirector::InputSetPopulation( inputdata_t &inputdata )
{
    TheDirectorManager->SetPopulation( inputdata.value.String() );
}
