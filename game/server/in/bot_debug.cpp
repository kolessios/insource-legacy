//==== Woots 2017. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "bot.h"

#include "in_utils.h"
#include "in_gamerules.h"

#include "nav.h"
#include "nav_mesh.h"

#include "ai_hint.h"
#include "fmtstr.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern ConVar bot_debug;
extern ConVar bot_debug_navigation;
extern ConVar bot_debug_memory;

extern ConVar bot_debug_desires;
extern ConVar bot_debug_conditions;
extern ConVar bot_debug_cmd;
extern ConVar bot_debug_max_msgs;

//================================================================================
//================================================================================
bool CBot::ShouldShowDebug() 
{
	if ( !bot_debug.GetBool() )
        return false;

    if ( GetHost() != UTIL_GetListenServerHost() && bot_debug.GetInt() != GetHost()->entindex() )
    {
        if ( !IsLocalPlayerWatchingMe() )
            return false;
    }

	return true;
}

//================================================================================
// Muestra información de depuración del Bot cuando el Jugador Host esta
// en modo espectador mirandolo
//================================================================================
void CBot::DebugDisplay()
{
    VPROF_BUDGET( "DebugDisplay", VPROF_BUDGETGROUP_BOTS );

    if ( !ShouldShowDebug() )
        return;

    m_flDebugYPosition = 0.06f;
    CFmtStr msg;

    // Colores
    const Color green = Color( 0, 255, 0, 200 );
    const Color red = Color( 255, 150, 150, 200 );
    const Color yellow = Color( 255, 255, 0, 200 );
    const Color blue = Color( 0, 255, 255, 200 );
    const Color pink = Color( 247, 129, 243, 200 );
    const Color white = Color( 255, 255, 255, 200 );

    // Cuadro rojo: Enemigo actual
    // Cuadro azul: Enemigo ideal
    // Cruz roja: Enemigo
    // Cruz amarilla: Lugar donde estamos mirando
    // Cruz hacia abajo azul: Destino

    // Skill
    if ( GetSkill() ) {
        DebugScreenText( msg.sprintf( "%s (%s)", GetName(), GetSkill()->GetLevelName() ) );
    }

    DebugScreenText( msg.sprintf( "Health: %i", GetHealth() ) );
    DebugScreenText( "" );

    // Estado
    switch ( GetState() ) {
        case STATE_IDLE:
        default:
            DebugScreenText( "IDLE", green );
            break;

        case STATE_ALERT:
            DebugScreenText( "ALERT", yellow );
            DebugScreenText( msg.sprintf( "    Time Left: %.2fs", GetStateDuration() ), yellow );
            break;

        case STATE_COMBAT:
            DebugScreenText( "COMBAT", red );
            break;

        case STATE_PANIC:
            DebugScreenText( "PANIC!!", red );
            DebugScreenText( msg.sprintf( "    Time Left: %.2fs", GetStateDuration() ), red );
            break;
    }

    DebugScreenText( "" );
    DebugScreenText( "A.I." );

    DebugScreenText( msg.sprintf( "    Tactical Mode: %s", g_TacticalModes[GetTacticalMode()] ) );

    if ( GetActiveSchedule() ) {
        CBotSchedule *pSchedule = GetActiveSchedule();
        DebugScreenText( msg.sprintf( "    Schedule: %s (%.2f)", g_BotSchedules[pSchedule->GetID()], pSchedule->GetRealDesire() ) );
        DebugScreenText( msg.sprintf( "    Task: %s", pSchedule->GetActiveTaskName() ) );
    }
    else {
        DebugScreenText( msg.sprintf( "    Schedule: -" ) );
        DebugScreenText( msg.sprintf( "    Task: -" ) );
    }

    {
        CBotSchedule *pIdealSchedule = GetSchedule( SelectIdealSchedule() );

        if ( pIdealSchedule )
            DebugScreenText( msg.sprintf( "    Ideal Schedule: %s (%.2f)", g_BotSchedules[pIdealSchedule->GetID()], pIdealSchedule->GetDesire() ) );
        else
            DebugScreenText( msg.sprintf( "    Ideal Schedule: -" ) );
    }

    if ( bot_debug_desires.GetBool() ) {
        DebugScreenText( "" );

        FOR_EACH_MAP( m_nSchedules, it )
        {
            CBotSchedule *pItem = m_nSchedules[it];

            float realDesire = pItem->GetRealDesire();
            Color desireColor = white;

            if ( realDesire >= BOT_DESIRE_VERYHIGH )
                desireColor = red;
            else if ( realDesire == BOT_DESIRE_HIGH )
                desireColor = pink;
            else if ( realDesire == BOT_DESIRE_MODERATE )
                desireColor = yellow;
            else if ( realDesire > BOT_DESIRE_NONE )
                desireColor = green;

            if ( pItem == GetActiveSchedule() )
                DebugScreenText( msg.sprintf( "    %s = %.2f (%.2f)", g_BotSchedules[pItem->GetID()], pItem->GetRealDesire(), pItem->GetDesire() ), red );
            else
                DebugScreenText( msg.sprintf( "    %s = %.2f (%.2f)", g_BotSchedules[pItem->GetID()], pItem->GetRealDesire(), pItem->GetDesire() ), desireColor );
        }
    }

    // Relaciones y enemigos
    if ( Friends() ) {
        DebugScreenText( "" );
        DebugScreenText( "Friends:", red );

        DebugScreenText( msg.sprintf( "    Enemies: %i (Nearby: %i)", GetEnemiesCount(), GetNearbyEnemiesCount() ), red );

        if ( GetEnemy() ) {
            CBaseEntity *pEnemy = GetEnemy();
            CBaseEntity *pIdeal = Friends()->GetIdealEnemy();
            CEnemyMemory *memory = Friends()->GetEnemyMemory( pEnemy );

            DebugScreenText( msg.sprintf( "    Enemy: %s (%s)", pEnemy->GetClassname(), STRING( pEnemy->GetEntityName() ) ), red );
            DebugScreenText( msg.sprintf( "    Distance: %.2f", Friends()->GetEnemyDistance() ), red );
            DebugScreenText( msg.sprintf( "    Ocludded: %i (%.2f s)", IsEnemyLost(), m_occludedEnemyTimer.GetElapsedTime() ), red );

            if ( pIdeal ) {
                DebugScreenText( msg.sprintf( "    Ideal Enemy: #%i %s (%s)", pIdeal->entindex(), STRING( pIdeal->GetEntityName() ), pIdeal->GetClassname() ), blue );

                if ( pIdeal != pEnemy ) {
                    NDebugOverlay::EntityBounds( pIdeal, blue.r(), blue.g(), blue.b(), 5.0f, 0.1f );
                }
            }

            if ( memory ) {
                if ( memory->GetBodyPositions().head.IsValid() )
                    NDebugOverlay::Text( memory->GetBodyPositions().head, "H", false, 0.1f );
                if ( memory->GetBodyPositions().gut.IsValid() )
                    NDebugOverlay::Text( memory->GetBodyPositions().gut, "G", false, 0.1f );
                if ( memory->GetBodyPositions().leftFoot.IsValid() )
                    NDebugOverlay::Text( memory->GetBodyPositions().leftFoot, "L", false, 0.1f );
                if ( memory->GetBodyPositions().rightFoot.IsValid() )
                    NDebugOverlay::Text( memory->GetBodyPositions().rightFoot, "R", false, 0.1f );

                DebugScreenText( msg.sprintf( "    Memory Duration: %.2fs", memory->ExpireTimer().GetRemainingTime() ), red );
            }

            if ( pEnemy ) {
                NDebugOverlay::EntityBounds( pEnemy, red.r(), red.g(), red.b(), 5.0f, 0.1f );
            }
        }
        else {
            DebugScreenText( msg.sprintf( "    Enemy: None" ), red );
        }
    }

    // Escuadron
    if ( GetSquad() ) {
        DebugScreenText( "" );
        DebugScreenText( "Squad:", pink );
        if ( GetSquad()->GetLeader() == GetHost() )
            DebugScreenText( msg.sprintf( "    Name: %s (LEADER)", GetSquad()->GetName() ), pink );
        else
            DebugScreenText( msg.sprintf( "    Name: %s", GetSquad()->GetName() ), pink );
        DebugScreenText( msg.sprintf( "    Members: %i", GetSquad()->GetCount() ), pink );
    }

    // Aim
    if ( Aim() ) {
        Vector vecLook = Aim()->GetLookingAt();
        int priority = Aim()->GetPriority();

        DebugScreenText( "" );
        DebugScreenText( "Aiming:", yellow );
        DebugScreenText( msg.sprintf( "    Looking At: %s (%.2f,%.2f,%.2f)", Aim()->m_pAimDesc, vecLook.x, vecLook.y, vecLook.z ), yellow );
        DebugScreenText( msg.sprintf( "    Time Left: %.2fs", (Aim()->GetAimTimer().HasStarted()) ? Aim()->GetAimTimer().GetRemainingTime() : -1 ), yellow );
        DebugScreenText( msg.sprintf( "    Priority: %s", g_PriorityNames[priority] ), yellow );

        if ( Aim()->GetLookTarget() ) {
            DebugScreenText( msg.sprintf( "    Entity: %s", Aim()->GetLookTarget()->GetClassname() ), yellow );
        }

        NDebugOverlay::Line( GetHost()->EyePosition(), vecLook, yellow.r(), yellow.g(), yellow.b(), true, 0.1f );
        //NDebugOverlay::Cross3D( vecLook, 3.0f, yellow.r(), yellow.g(), yellow.b(), true, 0.15f );
    }

    // Movimiento
    if ( Navigation() ) {
        Vector vecDestination = Navigation()->GetDestination();
        int priority = Navigation()->GetPriority();

        DebugScreenText( "" );
        DebugScreenText( "Navigation:", blue );
        DebugScreenText( msg.sprintf( "    Destination: %.2f,%.2f,%.2f", vecDestination.x, vecDestination.y, vecDestination.z ), blue );
        DebugScreenText( msg.sprintf( "    Distance Left: %.2f", Navigation()->GetDistanceLeft() ), blue );
        DebugScreenText( msg.sprintf( "    Priority: %s", g_PriorityNames[priority] ), blue );
        DebugScreenText( msg.sprintf( "    C/R/W/J: %i/%i/%i/%i", IsCrouching(), IsRunning(), IsWalking(), IsJumping() ), blue );
        DebugScreenText( msg.sprintf( "    InLadder: %i", Navigation()->IsUsingLadder() ), blue );

        if ( IsFollowingSomeone() )
            DebugScreenText( msg.sprintf( "    Following: #%i (%s)", Follow()->GetFollowing()->entindex(), STRING( Follow()->GetFollowing()->GetEntityName() ) ), blue );

        if ( Navigation()->m_Navigation.IsStuck() )
            DebugScreenText( msg.sprintf( "    STUCK (%.2f)", Navigation()->m_Navigation.GetStuckDuration() ), red );

        NDebugOverlay::Line( GetAbsOrigin(), vecDestination, blue.r(), blue.g(), blue.b(), true, 0.1f );
        //NDebugOverlay::VertArrow( vecDestination + Vector( 0, 0, 15.0f ), vecDestination, 15.0f / 4.0f, blue.r(), blue.g(), blue.b(), 10.0f, true, 0.15f );
    }

    // Condiciones
    if ( bot_debug_conditions.GetBool() ) {
        DebugScreenText( "" );
        DebugScreenText( "Conditions:" );

        for ( int i = 0; i < LAST_BCONDITION; ++i ) {
            if ( HasCondition( (BCOND)i ) ) {
                const char *pName = g_Conditions[i];

                if ( pName )
                    DebugScreenText( msg.sprintf( "    %s", pName ) );
            }
        }
    }

    // Memoria de los enemigos
    if ( bot_debug_memory.GetBool() && Friends() ) {
        FOR_EACH_MAP_FAST( Friends()->m_nEnemyMemory, it )
        {
            CEnemyMemory *memory = Friends()->m_nEnemyMemory.Element( it );

            if ( !memory || !memory->GetEnemy() )
                break;

            CBaseCombatCharacter *pCharacter = memory->GetEnemy()->MyCombatCharacterPointer();

            if ( pCharacter ) {
                Vector lastposition = memory->GetLastPosition();
                lastposition.z -= HalfHumanHeight;

                if ( memory->ReportedBy() ) {
                    NDebugOverlay::Line( memory->GetLastPosition(), memory->ReportedBy()->WorldSpaceCenter(), pink.r(), pink.g(), pink.b(), true, 0.1f );
                    NDebugOverlay::Box( lastposition, pCharacter->WorldAlignMins(), pCharacter->WorldAlignMaxs(), pink.r(), pink.g(), pink.b(), 1.0f, 0.1f );
                }
                else {
                    NDebugOverlay::Box( lastposition, pCharacter->WorldAlignMins(), pCharacter->WorldAlignMaxs(), white.r(), white.g(), white.b(), 5.0f, 0.1f );
                }
            }
        }
    }

    // CBotCmd
    if ( bot_debug_cmd.GetBool() ) {
        DebugScreenText( "" );
        DebugScreenText( msg.sprintf( "command_number: %i", GetCmd()->command_number ) );
        DebugScreenText( msg.sprintf( "tick_count: %i", GetCmd()->tick_count ) );
        DebugScreenText( msg.sprintf( "viewangles: %.2f, %.2f", GetCmd()->viewangles.x, GetCmd()->viewangles.y ) );
        DebugScreenText( msg.sprintf( "forwardmove: %.2f", GetCmd()->forwardmove ) );
        DebugScreenText( msg.sprintf( "sidemove: %.2f", GetCmd()->sidemove ) );
        DebugScreenText( msg.sprintf( "upmove: %.2f", GetCmd()->upmove ) );
        DebugScreenText( msg.sprintf( "buttons:%i", (int)GetCmd()->buttons ) );
        DebugScreenText( msg.sprintf( "impulse: %i", (int)GetCmd()->impulse ) );
        DebugScreenText( msg.sprintf( "weaponselect: %i", GetCmd()->weaponselect ) );
        DebugScreenText( msg.sprintf( "weaponsubtype: %i", GetCmd()->weaponsubtype ) );
        DebugScreenText( msg.sprintf( "random_seed: %i", GetCmd()->random_seed ) );
        DebugScreenText( msg.sprintf( "mousedx: %i", (int)GetCmd()->mousedx ) );
        DebugScreenText( msg.sprintf( "mousedy: %i", (int)GetCmd()->mousedy ) );
        DebugScreenText( msg.sprintf( "hasbeenpredicted: %i", (int)GetCmd()->hasbeenpredicted ) );
    }

    //
    // Mensajes de depuración
    //

    const float fadeAge = 7.0f;
    const float maxAge = 10.0f;

    DebugScreenText( "" );
    DebugScreenText( "" );

    FOR_EACH_VEC( m_debugMessages, it )
    {
        DebugMessage *message = &m_debugMessages.Element( it );

        if ( !message )
            continue;

        if ( message->m_age.GetElapsedTime() < maxAge ) {
            int alpha = 255;

            if ( message->m_age.GetElapsedTime() > fadeAge )
                alpha *= (1.0f - (message->m_age.GetElapsedTime() - fadeAge) / (maxAge - fadeAge));

            DebugScreenText( UTIL_VarArgs( "%2.f - %s", message->m_age.GetStartTime(), message->m_string ), Color( 255, 255, 255, alpha ) );
        }
    }

    //
    // Lugares interesantes
    //

    if ( !GetHost()->IsBot() )
        return;

    Vector vecDummy;

    // CSpotCriteria
    CSpotCriteria criteria;
    criteria.SetMaxRange( 1000.0f );

    // Lugares donde podemos ocultarnos
    SpotVector spotList;
    Utils::FindNavCoverSpot( &vecDummy, GetAbsOrigin(), criteria, GetHost(), &spotList );

    // info_node donde podemos escondernos
    SpotVector hintList;
    CHintCriteria hintCriteria;
    hintCriteria.AddHintType( HINT_TACTICAL_COVER );
    hintCriteria.AddHintType( HINT_WORLD_VISUALLY_INTERESTING );
    Utils::FindHintSpot( GetAbsOrigin(), hintCriteria, criteria, GetHost(), &spotList );

    FOR_EACH_VEC( spotList, it )
    {
        Vector vecSpot = spotList.Element( it );

        if ( GetHost()->FEyesVisible( vecSpot ) )
            NDebugOverlay::VertArrow( vecSpot + Vector( 0, 0, 15.0f ), vecSpot, 15.0f / 4.0f, 255, 255, 255, 100.0f, true, 0.15f );
        else
            NDebugOverlay::VertArrow( vecSpot + Vector( 0, 0, 15.0f ), vecSpot, 15.0f / 4.0f, 0, 0, 0, 100.0f, true, 0.15f );
    }

    FOR_EACH_VEC( hintList, it )
    {
        Vector vecSpot = hintList.Element( it );

        if ( GetHost()->FEyesVisible( vecSpot ) )
            NDebugOverlay::VertArrow( vecSpot + Vector( 0, 0, 15.0f ), vecSpot, 15.0f / 4.0f, 255, 255, 255, 0, true, 0.15f );
        else
            NDebugOverlay::VertArrow( vecSpot + Vector( 0, 0, 15.0f ), vecSpot, 15.0f / 4.0f, 0, 0, 0, 0, true, 0.15f );
    }

    //if ( GetNearestCover(vecDummy) )
        //NDebugOverlay::VertArrow( vecDummy + Vector(0, 0, 25.0f), vecDummy, 25.0f/4.0f, 0, 255, 0, 255, true, 0.15f );
}

//================================================================================
//================================================================================
void CBot::DebugScreenText( const char *pText, Color color, float yPosition, float duration )
{
    if ( yPosition < 0 )
        yPosition = m_flDebugYPosition;
    else
        m_flDebugYPosition = yPosition;

    NDebugOverlay::ScreenText( 0.6f, yPosition, pText, color.r(), color.g(), color.b(), color.a(), duration );
    m_flDebugYPosition += 0.02f;
}

//================================================================================
//================================================================================
void CBot::DebugAddMessage( char *format, ... )
{
    va_list varg;
    char buffer[ 1024 ];

    va_start( varg, format );
    vsprintf( buffer, format, varg );
    va_end( varg );

    DebugMessage message;
    message.m_age.Start();
    Q_strncpy( message.m_string, buffer, 1024 );

    m_debugMessages.AddToHead( message );

    if ( m_debugMessages.Count() >= bot_debug_max_msgs.GetInt() )
        m_debugMessages.RemoveMultipleFromTail(1);
}

Vector g_DebugSpot1;
Vector g_DebugSpot2;

CNavPath g_DebugPath;
CNavPathFollower g_DebugNavigation;

CON_COMMAND_F( bot_debug_mark_spot1, "", FCVAR_SERVER )
{
	CPlayer *pOwner = ToInPlayer(CBasePlayer::Instance(UTIL_GetCommandClientIndex()));

    if ( !pOwner )
        return;

	g_DebugSpot1 = pOwner->GetAbsOrigin();
}

CON_COMMAND_F( bot_debug_mark_spot2, "", FCVAR_SERVER )
{
	CPlayer *pOwner = ToInPlayer(CBasePlayer::Instance(UTIL_GetCommandClientIndex()));

    if ( !pOwner )
        return;

	g_DebugSpot2 = pOwner->GetAbsOrigin();
}

CON_COMMAND_F( bot_debug_process_navigation, "", FCVAR_SERVER )
{
	CPlayer *pOwner = ToInPlayer(CBasePlayer::Instance(UTIL_GetCommandClientIndex()));

    if ( !pOwner )
        return;

	g_DebugPath.Invalidate();

	BotPathCost pathCost( pOwner );
	g_DebugPath.Compute( g_DebugSpot1, g_DebugSpot2, pathCost );
	g_DebugPath.Draw();
}