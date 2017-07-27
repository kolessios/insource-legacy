//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"

#include "bot.h"
#include "in_utils.h"

#include "in_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


//================================================================================
// Establece el nivel de habilidad del Bot
//================================================================================
void CBot::SetSkill( int level )
{
    // Dificultad al azar
    if ( level == 0 )
        level = RandomInt( SKILL_EASY, SKILL_HARDEST );

    // Igual que la dificultad del juego
    if ( level == 99 )
        level = TheGameRules->GetSkillLevel();

    if ( !GetSkill() ) {
        m_Skill = new BotSkill( level );
        return;
    }

    GetSkill()->SetLevel( level );
}

//================================================================================
// Establece el estado actual del Bot y su duración
//================================================================================
void CBot::SetState( BotState state, float duration )
{
    // Evitamos quedarnos en pánico para siempre
    if ( IsPanicked() && state == STATE_PANIC )
        return;

    //
    if ( state != m_iState && state != STATE_PANIC && state != STATE_COMBAT )
    {
        // El estado actual no ha expirado
        if ( m_iStateTimer.HasStarted() && !m_iStateTimer.IsElapsed() )
            return;
    }

    m_iState = state;
    m_iStateTimer.Invalidate();

    if ( duration > 0 )
        m_iStateTimer.Start( duration );
}

//================================================================================
// Limpia el estado actual, para ponerse en modo relajado
//================================================================================
void CBot::CleanState() 
{
    if ( IsPanicked() || IsCombating() )
        Alert();
    else
        Idle();
}

//================================================================================
// Mete al Bot en un estado de pánico donde no podrá hacer nada
//================================================================================
void CBot::Panic( float duration )
{
    // Usamos la duración de nuestro nivel
    if ( duration < 0 )
        duration = GetSkill()->GetPanicDuration();

    SetState( STATE_PANIC, duration );
}

//================================================================================
// Mete al Bot en un estado de alerta
//================================================================================
void CBot::Alert( float duration )
{
    // Usamos la duración predeterminada
    if ( duration < 0 )
        duration = GetSkill()->GetAlertDuration();

    SetState( STATE_ALERT, duration );
}

//================================================================================
// Mete al Bot en un estado de relajación
//================================================================================
void CBot::Idle()
{
    SetState( STATE_IDLE, -1 );
	StandUp();
}

//================================================================================
// Mete al Bot en un estado de combate
//================================================================================
void CBot::Combat()
{
    SetState( STATE_COMBAT, 2.0f );
}