//========= Copyright (C) 2021, CSProMod Team, All rights reserved. =========//
//
// Purpose: Provide world light-related functions to the client
//
// Written: November 2011
// Author: Saul Rennison
//
//===========================================================================//

#ifndef WORLDLIGHT_H
#define WORLDLIGHT_H
#ifdef _WIN32
#pragma once
#endif

#include "igamesystem.h" // CAutoGameSystem

class Vector;
struct dworldlight_t;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
struct LightSourceInfo_t
{
    Vector position;
    Vector direction;
    Vector intensity;
    float brightnessSqr;
    int lightType;        // emit_point, emit_spotlight, etc.
    bool bVisible;
};
class CWorldLights : public CAutoGameSystem
{
public:
	CWorldLights();
	~CWorldLights() { Clear(); }

	bool GetBrightestLightSource( const Vector &vecPosition, Vector &vecLightPos, Vector &vecLightBrightness );

	// CAutoGameSystem overrides
	bool Init() OVERRIDE;
	void LevelInitPreEntity() OVERRIDE;
	void LevelShutdownPostEntity() OVERRIDE { Clear(); }
	void FixLightParameters(dworldlight_t &light);
	int GetAllInfluencingLights( const Vector &vecPosition, CUtlVector<LightSourceInfo_t> &lightSources, float flMinBrightness = 0.0f );
	void GetShadowDirectionFromLight( const Vector &vecPosition, const LightSourceInfo_t &light, Vector &vecShadowDir, float &flIntensity );

private:
	void Clear();

private:
	int m_nWorldLights;
	dworldlight_t *m_pWorldLights;
};

// Singleton accessor
extern CWorldLights *g_pWorldLights;

#endif // WORLDLIGHT_H