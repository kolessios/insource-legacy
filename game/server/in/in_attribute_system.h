//==== Woots 2017. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef IN_ATTRIBUTE_MANAGER_H
#define IN_ATTRIBUTE_MANAGER_H

#ifdef _WIN32
#pragma once
#endif

#include "in_attribute.h"

//================================================================================
// Administra y actualiza los atributos
//================================================================================
class CAttributeSystem : public CAutoGameSystem
{
public:
	CAttributeSystem();
	DECLARE_CLASS_GAMEROOT( CAttributeSystem, CAutoGameSystem );

    virtual void PostInit();
	virtual void Load();

	virtual void LoadAttributes();
	virtual void LoadModifiers();

	virtual bool GetAttributeInfo( const char *name, AttributeInfo &info );
	virtual bool GetModifierInfo( const char *name, AttributeInfo &info );

	virtual CAttribute *GetAttribute( const char *name );

protected:
	AttributesList m_AttributesList;
	AttributesList m_ModifiersList;

	friend class CAttribute;
};

extern CAttributeSystem *TheAttributeSystem;

#endif // IN_ATTRIBUTE_MODIFIER_H