//==== Woots 2017. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef INFO_DIRECTOR_H
#define INFO_DIRECTOR_H

#ifdef _WIN32
#pragma once
#endif

class CInfoDirector : public CLogicalEntity
{
public:
	DECLARE_CLASS( CInfoDirector, CLogicalEntity );
	DECLARE_DATADESC();

    virtual void Spawn();

	// Inputs
	void InputStop( inputdata_t &inputdata );
	void InputResume( inputdata_t &inputdata );

	void InputSetNormal( inputdata_t &inputdata );
	void InputStartPanic( inputdata_t &inputdata );
	void InputStartInfinitePanic( inputdata_t &inputdata );
	void InputStartFinale( inputdata_t &inputdata );
	void InputKillMinions( inputdata_t &inputdata );
    void InputSetPopulation( inputdata_t &inputdata );

protected:
    bool m_bDisabled;
    string_t m_szPopulation;

};

#endif // INFO_DIRECTOR_H