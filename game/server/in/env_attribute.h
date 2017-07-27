//==== Woots 2017. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef ENV_ATTRIBUTE_H
#define ENV_ATTRIBUTE_H

#pragma once

//================================================================================
//================================================================================
class CEnvAttribute : public CPointEntity
{
public:
	DECLARE_CLASS( CEnvAttribute, CPointEntity );
	DECLARE_DATADESC();

	virtual void Spawn();
	virtual void Think();

protected:
	string_t m_nModifierName;
	float m_flRadius;
	int m_iTeam;
	bool m_bDisabled;
};

#endif // ENV_ATTRIBUTE_H