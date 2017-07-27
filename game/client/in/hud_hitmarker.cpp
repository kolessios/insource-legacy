//==== Woots 2017. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "iclientmode.h"
#include "c_in_player.h"
#include "engine/ienginesound.h"

// VGUI panel includes
#include <vgui_controls/AnimationController.h>
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>

using namespace vgui;

// memdbgon must be the last include file in a .cpp file!
#include "tier0/memdbgon.h"

class CHudHitmarker : public vgui::Panel, public CHudElement
{
    DECLARE_CLASS_SIMPLE( CHudHitmarker, vgui::Panel );

public:
    CHudHitmarker( const char *pElementName );

    void Init();
    void Reset();
    bool ShouldDraw();
    void ShowHitmarker();

    void MsgFunc_ShowHitmarker( bf_read &msg );

protected:
    virtual void ApplySchemeSettings( vgui::IScheme *scheme );
    virtual void Paint( void );

private:
    bool m_bHitmarkerShow;
    CPanelAnimationVar( Color, m_HitmarkerColor, "HitMarkerColor", "255 255 255 255" );
};

DECLARE_HUDELEMENT( CHudHitmarker );
DECLARE_HUD_MESSAGE( CHudHitmarker, ShowHitmarker );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------

CHudHitmarker::CHudHitmarker( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "HudHitmarker" )
{
    vgui::Panel *pParent = GetClientMode()->GetViewport();
    SetParent( pParent );

    // Hitmarker will not show when the player is dead
    SetHiddenBits( HIDEHUD_PLAYERDEAD );

    int screenWide, screenTall;
    GetHudSize( screenWide, screenTall );
    SetBounds( 0, 0, screenWide, screenTall );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudHitmarker::Init()
{
    HOOK_HUD_MESSAGE( CHudHitmarker, ShowHitmarker );

    SetAlpha( 0 );
    m_bHitmarkerShow = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudHitmarker::Reset()
{
    SetAlpha( 0 );
    m_bHitmarkerShow = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudHitmarker::ApplySchemeSettings( vgui::IScheme *scheme )
{
    BaseClass::ApplySchemeSettings( scheme );

    SetPaintBackgroundEnabled( false );
    SetPaintBorderEnabled( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudHitmarker::ShouldDraw( void )
{
    return (m_bHitmarkerShow && CHudElement::ShouldDraw());
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CHudHitmarker::ShowHitmarker()
{
    m_bHitmarkerShow = true;
    GetClientMode()->GetViewportAnimationController()->StartAnimationSequence( "HitMarkerShow" );

    C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
    pPlayer->EmitSound("items/hitmark.wav");
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudHitmarker::Paint( void )
{
    C_BasePlayer* pPlayer = C_BasePlayer::GetLocalPlayer();

    if ( !pPlayer ) {
        return;
    }

    if ( m_bHitmarkerShow ) {
        int	x, y;

        // Find our screen position to start from
        x = XRES( 320 );
        y = YRES( 240 );

        vgui::surface()->DrawSetColor( m_HitmarkerColor );
        vgui::surface()->DrawLine( x - 6, y - 5, x - 11, y - 10 );
        vgui::surface()->DrawLine( x + 5, y - 5, x + 10, y - 10 );
        vgui::surface()->DrawLine( x - 6, y + 5, x - 11, y + 10 );
        vgui::surface()->DrawLine( x + 5, y + 5, x + 10, y + 10 );
    }
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudHitmarker::MsgFunc_ShowHitmarker( bf_read &msg )
{
    m_bHitmarkerShow = ( msg.ReadByte() == 1 );
    GetClientMode()->GetViewportAnimationController()->StartAnimationSequence( "HitMarkerShow" );

    C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
    pPlayer->EmitSound( "items/hitmark.wav" );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CON_COMMAND( cl_show_hitmarker, "" )
{
    CHudHitmarker *pElement = (CHudHitmarker *)GetHud().FindElement( "CHudHitmarker" );

    if ( !pElement )
        return;

    pElement->ShowHitmarker();
}