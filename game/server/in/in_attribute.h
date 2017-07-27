//==== Woots 2017. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef IN_ATTRIBUTES_H
#define IN_ATTRIBUTES_H

#ifdef _WIN32
#pragma once
#endif

#include "in_shareddefs.h"

//====================================================================
// Información de atributo
//====================================================================
struct AttributeInfo
{
	char name[38];

	float value;
	float rate;
	float amount;
	float max;
	float min;

	bool amount_absolute;
	bool rate_absolute;
	float duration;
	char affects[38];

	float expire;
};

typedef CUtlVector<AttributeInfo> AttributesList;

//====================================================================
// Base para la creación de atributos
//====================================================================
class CAttribute
{
public:
	//DECLARE_CLASS_NOBASE( CAttribute );

	CAttribute();
	CAttribute( AttributeInfo info );

	const char *GetID() { return STRING(m_pID); }
	//virtual void SetID( const char *id ) { m_pID = id; }

	virtual void Init();

	virtual void PreUpdate();
	virtual void Update();

	virtual void AddModifier( const char *name );
	virtual void AddModifier( AttributeInfo info );
    virtual int HasModifier( const char *name );

	virtual float GetValue();
	virtual void SetValue( float value );
	virtual void AddValue( float value );
	virtual void SubtractValue( float value );

	virtual float GetRate();
	virtual void SetRate( float rate );

	virtual float GetAmount();
	virtual void SetRegen( float amount ) { m_flAmount = amount; }

	virtual float GetMin();
	virtual void SetMin( float value ) { m_flMin = value; }

	virtual float GetMax();
	virtual void SetMax( float value ) { m_flMax = value; }

    virtual float GetPercent();

    virtual bool CanRegenerate() { return m_bCanRegenerate; }
    virtual void SetRegeneration( bool enabled ) { m_bCanRegenerate = enabled; }

	virtual void SetNextRegeneration( float time );
	virtual void PauseRegeneration( float time );

public:
	CUtlVector<AttributeInfo> m_nModifiers;

protected:
	string_t m_pID;

	float m_flValue;
	float m_flRate;
	float m_flAmount;

	float m_flMax;
	float m_flMin;

    bool m_bCanRegenerate;
	float m_flNextRegeneration;
	float m_flBaseTime;
};

#endif // IN_ATTRIBUTES_H