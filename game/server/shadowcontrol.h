//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Shadow control entity.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//------------------------------------------------------------------------------
// FIXME: This really should inherit from something	more lightweight
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Purpose : Shadow control entity
//------------------------------------------------------------------------------
class CShadowControl : public CBaseEntity
{
public:
	DECLARE_CLASS( CShadowControl, CBaseEntity );

	CShadowControl();

	void Spawn( void );
	void SetupShadowControlFromSun();
	bool KeyValue( const char *szKeyName, const char *szValue );
	int  UpdateTransmitState();
	void InputSetAngles( inputdata_t &inputdata );

	virtual int	ObjectCaps( void ) { return BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

private:
	CNetworkVector( m_shadowDirection );
	CNetworkColor32( m_shadowColor );
	CNetworkVar( float, m_flShadowMaxDist );
	CNetworkVar( bool, m_bDisableShadows );
};