//========= Copyright ATOMIC_REAKTOR, All rights reserved. ============//
//
// АВТОМАТИЧЕСКИЕ КАСКАДНЫЕ ТЕНИ БЕЗ РЕДАКТИРОВАНИЯ КАРТЫ
// AUTOMATIC CASCADED SHADOWS WITHOUT MAP EDITING
//
// Для CSSO - работает с любыми существующими картами CS:S!
//
//===========================================================================//

#ifndef CSGO_AUTO_CASCADED_SHADOWS_H
#define CSGO_AUTO_CASCADED_SHADOWS_H
#ifdef _WIN32
#pragma once
#endif

#include "c_baseentity.h"
#include "C_Env_Projected_Texture.h"
#include "igamesystem.h"

//-----------------------------------------------------------------------------
// Автоматический менеджер каскадных теней
// Создаётся при загрузке карты, не требует энтити на карте!
//-----------------------------------------------------------------------------
class CAutoSunShadowManager : public CAutoGameSystemPerFrame
{
public:
    CAutoSunShadowManager();
    virtual ~CAutoSunShadowManager();
    
    // IGameSystem
    virtual char const* Name() { return "CAutoSunShadowManager"; }
    virtual bool Init();
    virtual void Shutdown();
    virtual void LevelInitPreEntity();
    virtual void LevelInitPostEntity();
    virtual void LevelShutdownPreEntity();
    virtual void LevelShutdownPostEntity();
    
    // IGameSystemPerFrame
    virtual void Update(float frametime);
    
    // Управление
    void Enable() { m_bEnabled = true; CreateCascadeLights(); }
    void Disable() { m_bEnabled = false; DestroyCascadeLights(); }
    bool IsEnabled() const { return m_bEnabled; }
    
    // Получение информации
    int GetNumCascades() const { return m_nNumCascades; }
    C_EnvProjectedTexture* GetCascadeLight(int index);
    
    // Настройка
    void SetNumCascades(int num);
    void SetResolution(int resolution);
    void SetDistance(float distance);
    void SetLightDirection(const Vector& dir);
    
private:
    void CreateCascadeLights();
    void DestroyCascadeLights();
    void UpdateCascadeLights();
    void UpdateLightDirectionFromMap();
    
    void ComputeCascadeParameters(int cascadeIndex, 
                                  Vector& outOrigin, 
                                  QAngle& outAngles,
                                  float& outNearZ, 
                                  float& outFarZ,
                                  float& outOrthoSize);
    
    void CalculateSplitDistances(float nearPlane, float farPlane);
    
private:
    bool    m_bEnabled;
    bool    m_bInitialized;
    int     m_nNumCascades;
    int     m_nResolution;
    float   m_flShadowDistance;
    float   m_flLambda;
    Vector  m_vLightDirection;
    
    C_EnvProjectedTexture* m_pCascadeLights[4];
    float   m_flSplitDistances[5];
};

// Глобальный инстанс - создаётся автоматически!
extern CAutoSunShadowManager g_AutoSunShadows;

#endif // CSGO_AUTO_CASCADED_SHADOWS_H
