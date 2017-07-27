//==== InfoSmart 2015. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef SUN_H
#define SUN_H

#ifdef _WIN32
#pragma once
#endif

//================================================================================
//================================================================================
class CSun : public CBaseEntity
{
public:
    DECLARE_CLASS( CSun, CBaseEntity );
    DECLARE_SERVERCLASS();
    DECLARE_DATADESC();

    CSun();

    virtual void Activate();
    virtual int UpdateTransmitState();
    virtual void Think();

    // Input
    void InputTurnOn( inputdata_t &inputdata );
    void InputTurnOff( inputdata_t &inputdata );
    void InputSetColor( inputdata_t &inputdata );
public:
    CNetworkVector( m_vDirection );
    
    string_t m_strMaterial;
    string_t m_strOverlayMaterial;
    QAngle m_angles;
    
    CNetworkVar( int, m_nSize );        // Size of the main core image
    CNetworkVar( int, m_nOverlaySize ); // Size for the glow overlay
    CNetworkVar( color32, m_clrOverlay );
    CNetworkVar( bool, m_bOn );
    CNetworkVar( int, m_nMaterial );
    CNetworkVar( int, m_nOverlayMaterial );
    CNetworkVar( float, m_flHDRColorScale );
};

#endif // SUN_H