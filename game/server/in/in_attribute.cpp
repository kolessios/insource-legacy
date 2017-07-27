//==== Woots 2017. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "in_attribute.h"

#include "in_buttons.h"
#include "physics_prop_ragdoll.h"
#include "util_shared.h"

#include "in_attribute_system.h"

#include "nav.h"
#include "nav_area.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//================================================================================
// Comandos
//================================================================================



//================================================================================
// Constructor
//================================================================================
CAttribute::CAttribute()
{
    Init();
}

//================================================================================
//================================================================================
CAttribute::CAttribute( AttributeInfo info )
{
    m_pID = AllocPooledString( info.name );

    m_flValue = info.value;
    m_flRate = info.rate;
    m_flAmount = info.amount;
    m_flMax = info.max;
    m_flMin = info.min;

    m_bCanRegenerate = true;
    m_flNextRegeneration = -1.0f;
    SetNextRegeneration( m_flRate );

    m_nModifiers.EnsureCapacity( TheAttributeSystem->m_ModifiersList.Count() );
    m_nModifiers.Purge();
}

//================================================================================
//================================================================================
void CAttribute::Init()
{
    m_flValue = 0.0f;
    m_flRate = 1.0f;
    m_flAmount = 1.0f;
    m_flMax = 100.0f;
    m_flMin = 0.0f;

    m_bCanRegenerate = true;
    m_flNextRegeneration = -1.0f;
    SetNextRegeneration( m_flRate );

    m_nModifiers.EnsureCapacity( TheAttributeSystem->m_ModifiersList.Count() );
    m_nModifiers.Purge();
}

//================================================================================
// Antes de cada actualización
//================================================================================
void CAttribute::PreUpdate()
{
    // Limpiamos todos los modificadores del frame anterior
    FOR_EACH_VEC( m_nModifiers, it )
    {
        AttributeInfo modifier = m_nModifiers.Element( it );

        if ( modifier.expire <= gpGlobals->curtime )
            m_nModifiers.Remove( it );
    }
}

//================================================================================
// Actualización: Se ejecuta en cada frame para ver si podemos empezar la
// regeneración.
//================================================================================
void CAttribute::Update()
{
    if ( m_bCanRegenerate ) {
        if ( gpGlobals->curtime >= m_flNextRegeneration ) {
            AddValue( GetAmount() );
            SetNextRegeneration( GetRate() );
        }
    }
}

//================================================================================
// Agrega un modificador al atributo
//================================================================================
void CAttribute::AddModifier( const char *name )
{
    AttributeInfo info;

    // El modificador no existe
    if ( !TheAttributeSystem->GetModifierInfo( name, info ) )
        return;

    AddModifier( info );
}

//================================================================================
// Agrega un modificador al atributo
//================================================================================
void CAttribute::AddModifier( AttributeInfo info )
{
    float duration = info.duration;

    if ( duration > 0 && duration <= GetRate() )
        duration = GetRate() + 0.1f;

    int modifierKey = HasModifier( info.name );

    // Ya lo tenemos
    // @TODO: Stacks
    if ( modifierKey >= 0 ) {
        // Aumentamos solo la duración
        m_nModifiers.Element( modifierKey ).expire = gpGlobals->curtime + duration;
        return;
    }

    // Lo agregamos
    info.expire = gpGlobals->curtime + duration;
    m_nModifiers.AddToTail( info );
}

//================================================================================
// Devuelve si el atributo tiene el modificador especificado
//================================================================================
int CAttribute::HasModifier( const char *name )
{
    FOR_EACH_VEC( m_nModifiers, it )
    {
        AttributeInfo in = m_nModifiers.Element( it );

        if ( FStrEq( name, in.name ) ) {
            return it;
        }
    }

    return -1;
}

//================================================================================
// Devuelve el valor actual del atributo
//================================================================================
float CAttribute::GetValue()
{
    float value = m_flValue;

    // Modificadores
    FOR_EACH_VEC( m_nModifiers, it )
    {
        value += m_nModifiers[it].value;
    }

    return clamp( value, GetMin(), GetMax() );
}

//================================================================================
// Establece el valor actual del atributo
//================================================================================
void CAttribute::SetValue( float value )
{
    m_flValue = clamp( m_flValue, GetMin(), GetMax() );
}

//================================================================================
// Agrega la cantidad especificada al valor del atributo
//================================================================================
void CAttribute::AddValue( float value )
{
    m_flValue += value;
    m_flValue = clamp( m_flValue, GetMin(), GetMax() );
}

//================================================================================
// Remueve la cantidad especificada al valor del atributo
//================================================================================
void CAttribute::SubtractValue( float value )
{
    m_flValue -= value;
    m_flValue = clamp( m_flValue, GetMin(), GetMax() );
}

//================================================================================
// Devuelve el tiempo de regeneración
//================================================================================
float CAttribute::GetRate()
{
    float value = m_flRate;

    // Modificadores absolutos
    // @TODO: ¿Que hacemos con varios modificadores absolutos?
    FOR_EACH_VEC( m_nModifiers, it )
    {
        if ( m_nModifiers[it].rate_absolute )
            value = m_nModifiers[it].rate;
    }

    // Modificadores
    FOR_EACH_VEC( m_nModifiers, it )
    {
        if ( !m_nModifiers[it].rate_absolute )
            value += m_nModifiers[it].rate;
    }

    return value;
}

//================================================================================
// Establece el tiempo de regeneración
//================================================================================
void CAttribute::SetRate( float rate )
{
    m_flRate = rate;
    SetNextRegeneration( m_flRate );
}

//================================================================================
// Devuelve el valor minimo del atributo
//================================================================================
float CAttribute::GetMin()
{
    float value = m_flMin;

    // Modificadores
    FOR_EACH_VEC( m_nModifiers, it )
    {
        value += m_nModifiers[it].min;
    }

    return value;
}

//================================================================================
// Devuelve el valor máximo del atributo
//================================================================================
float CAttribute::GetMax()
{
    float value = m_flMax;

    // Modificadores
    FOR_EACH_VEC( m_nModifiers, it )
    {
        value += m_nModifiers[it].max;
    }

    return value;
}

//================================================================================
// Devuelve la cantidad que se debe agregar/remover en cada regeneración
//================================================================================
float CAttribute::GetAmount()
{
    float value = m_flAmount;

    // Modificadores absolutos
    // @TODO: ¿Que hacemos con varios modificadores absolutos?
    FOR_EACH_VEC( m_nModifiers, key )
    {
        if ( m_nModifiers[key].amount_absolute )
            value = m_nModifiers[key].amount;
    }

    // Modificadores
    FOR_EACH_VEC( m_nModifiers, key )
    {
        if ( !m_nModifiers[key].amount_absolute )
            value += m_nModifiers[key].amount;
    }

    return value;
}

//================================================================================
// Devuelve el valor en porcentaje
//================================================================================
float CAttribute::GetPercent()
{
    return (GetValue() / GetMax()) * 100;
}

//================================================================================
// Establece cuando será el próximo tiempo para poder regenerar
//================================================================================
void CAttribute::SetNextRegeneration( float time )
{
    if ( gpGlobals->curtime < m_flNextRegeneration ) {
        m_flNextRegeneration = m_flBaseTime + time;
        return;
    }

    PauseRegeneration( time );
}

//================================================================================
// Pausa la regeneración por el tiempo indicado
//================================================================================
void CAttribute::PauseRegeneration( float time )
{
    m_flNextRegeneration = gpGlobals->curtime + time;
    m_flBaseTime = gpGlobals->curtime;
}
