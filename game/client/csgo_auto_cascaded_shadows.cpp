//========= Copyright ATOMIC_REAKTOR, All rights reserved. ============//
//
// Реализация автоматических каскадных теней
//
//===========================================================================//

#include "cbase.h"
#include "csgo_auto_cascaded_shadows.h"
#include "view.h"
#include "viewrender.h"
#include "c_sun.h"

// ConVars
ConVar csgo_shadows_auto("csgo_shadows_auto", "1", FCVAR_ARCHIVE, 
    "Automatically create cascaded sun shadows on map load");
ConVar csgo_shadows_cascades("csgo_shadows_cascades", "3", FCVAR_ARCHIVE, 
    "Number of shadow cascades (2-4)");
ConVar csgo_shadows_distance("csgo_shadows_distance", "2500", FCVAR_ARCHIVE, 
    "Maximum shadow distance");
ConVar csgo_shadows_resolution("csgo_shadows_resolution", "2048", FCVAR_ARCHIVE, 
    "Shadow map resolution (512, 1024, 2048, 4096)");
ConVar csgo_shadows_lambda("csgo_shadows_lambda", "0.75", FCVAR_ARCHIVE, 
    "PSSM split lambda (0.5-0.95)");

// Глобальный инстанс - создаётся автоматически через CAutoGameSystem!
CAutoSunShadowManager g_AutoSunShadows;

//-----------------------------------------------------------------------------
// Конструктор
//-----------------------------------------------------------------------------
CAutoSunShadowManager::CAutoSunShadowManager() : CAutoGameSystemPerFrame("CAutoSunShadowManager")
{
    m_bEnabled = false;
    m_bInitialized = false;
    m_nNumCascades = 3;
    m_nResolution = 2048;
    m_flShadowDistance = 2500.0f;
    m_flLambda = 0.75f;
    m_vLightDirection = Vector(0.577f, 0.577f, -0.577f);
    
    for (int i = 0; i < 4; i++)
    {
        m_pCascadeLights[i] = NULL;
        m_flSplitDistances[i] = 0.0f;
    }
    m_flSplitDistances[4] = 0.0f;
}

//-----------------------------------------------------------------------------
// Деструктор
//-----------------------------------------------------------------------------
CAutoSunShadowManager::~CAutoSunShadowManager()
{
    DestroyCascadeLights();
}

//-----------------------------------------------------------------------------
// Init
//-----------------------------------------------------------------------------
bool CAutoSunShadowManager::Init()
{
    Msg("[CSGO Shadows] Auto Sun Shadow Manager initialized\n");
    return true;
}

//-----------------------------------------------------------------------------
// Shutdown
//-----------------------------------------------------------------------------
void CAutoSunShadowManager::Shutdown()
{
    DestroyCascadeLights();
}

//-----------------------------------------------------------------------------
// LevelInitPreEntity - вызывается ДО загрузки энтити
//-----------------------------------------------------------------------------
void CAutoSunShadowManager::LevelInitPreEntity()
{
    m_bInitialized = false;
    DestroyCascadeLights();
}

//-----------------------------------------------------------------------------
// LevelInitPostEntity - вызывается ПОСЛЕ загрузки всех энтити
// ЗДЕСЬ МЫ СОЗДАЁМ ТЕНИ АВТОМАТИЧЕСКИ!
//-----------------------------------------------------------------------------
void CAutoSunShadowManager::LevelInitPostEntity()
{
    if (!csgo_shadows_auto.GetBool())
    {
        Msg("[CSGO Shadows] Auto shadows disabled\n");
        return;
    }
    
    // Загружаем настройки из ConVars
    m_nNumCascades = clamp(csgo_shadows_cascades.GetInt(), 2, 4);
    m_nResolution = csgo_shadows_resolution.GetInt();
    m_flShadowDistance = csgo_shadows_distance.GetFloat();
    m_flLambda = clamp(csgo_shadows_lambda.GetFloat(), 0.5f, 0.95f);
    
    // Пытаемся найти направление света из карты
    UpdateLightDirectionFromMap();
    
    // Создаём cascade lights
    m_bEnabled = true;
    CreateCascadeLights();
    
    m_bInitialized = true;
    
    Msg("[CSGO Shadows] Auto sun shadows created: %d cascades, %dx%d resolution\n",
        m_nNumCascades, m_nResolution, m_nResolution);
}

//-----------------------------------------------------------------------------
// LevelShutdownPreEntity
//-----------------------------------------------------------------------------
void CAutoSunShadowManager::LevelShutdownPreEntity()
{
    DestroyCascadeLights();
    m_bInitialized = false;
}

//-----------------------------------------------------------------------------
// LevelShutdownPostEntity
//-----------------------------------------------------------------------------
void CAutoSunShadowManager::LevelShutdownPostEntity()
{
    // Ничего не делаем
}

//-----------------------------------------------------------------------------
// Update - вызывается каждый кадр
//-----------------------------------------------------------------------------
void CAutoSunShadowManager::Update(float frametime)
{
    if (!m_bEnabled || !m_bInitialized)
        return;
    
    // Обновляем направление света (может меняться в рантайме)
    UpdateLightDirectionFromMap();
    
    // Обновляем позиции и параметры cascade lights
    UpdateCascadeLights();
}

//-----------------------------------------------------------------------------
// Получение направления света из карты
// Ищет env_sun, light_environment и другие источники света
//-----------------------------------------------------------------------------
void CAutoSunShadowManager::UpdateLightDirectionFromMap()
{
    // 1. Пытаемся найти env_sun (используется в некоторых картах)
    C_BaseEntity* pSun = gEntList.FindEntityByClassname(NULL, "env_sun");
    if (pSun)
    {
        Vector forward;
        AngleVectors(pSun->GetAbsAngles(), &forward);
        m_vLightDirection = forward;
        return;
    }
    
    // 2. Ищем C_Sun (клиентская энтити солнца)
    C_Sun* pClientSun = GetGlobalSun();
    if (pClientSun)
    {
        // Получаем направление из angles
        Vector forward;
        AngleVectors(pClientSun->GetAbsAngles(), &forward);
        m_vLightDirection = forward;
        return;
    }
    
    // 3. Ищем light_environment (основной источник света карты)
    C_BaseEntity* pLightEnv = gEntList.FindEntityByClassname(NULL, "light_environment");
    if (pLightEnv)
    {
        Vector forward;
        AngleVectors(pLightEnv->GetAbsAngles(), &forward);
        m_vLightDirection = forward;
        return;
    }
    
    // 4. Если ничего не найдено, используем дефолтное направление
    // 45 градусов сверху справа (типичное для CS карт)
    m_vLightDirection = Vector(0.577f, 0.577f, -0.577f);
}

//-----------------------------------------------------------------------------
// Создание cascade lights
//-----------------------------------------------------------------------------
void CAutoSunShadowManager::CreateCascadeLights()
{
    if (!m_bEnabled)
        return;
    
    for (int i = 0; i < m_nNumCascades; i++)
    {
        if (m_pCascadeLights[i])
            continue; // Уже создан
        
        // Создаём projected texture
        C_EnvProjectedTexture* pLight = new C_EnvProjectedTexture();
        if (!pLight)
        {
            Warning("[CSGO Shadows] Failed to create cascade light %d\n", i);
            continue;
        }
        
        // Инициализация
        pLight->InitializeAsClientEntity(NULL, RENDER_GROUP_OTHER);
        
        // Базовые параметры
        pLight->m_bState = true;
        pLight->m_bAlwaysUpdate = true;
        pLight->m_LightColor = Vector(255, 255, 255);
        pLight->m_flBrightness = 1.0f;
        
        // Shadow parameters
        pLight->m_bEnableShadows = true;
        pLight->m_bLightOnlyTarget = false;
        pLight->m_bLightWorld = true;
        pLight->m_flShadowQuality = 1.0f;
        
        // Texture
        pLight->m_SpotlightTextureName = "effects/flashlight001";
        pLight->m_nSpotlightTextureFrame = 0;
        
        // Resolution
        pLight->m_nShadowTextureWidth = m_nResolution;
        pLight->m_nShadowTextureHeight = m_nResolution;
        
        // КЛЮЧЕВОЕ: Ортогональная проекция для солнца!
        pLight->m_bOrtho = true;
        pLight->m_flOrthoWidth = 1000.0f;
        pLight->m_flOrthoHeight = 1000.0f;
        
        // Near/Far
        pLight->m_flNearZ = 1.0f;
        pLight->m_flFarZ = 5000.0f;
        
        // Сохраняем
        m_pCascadeLights[i] = pLight;
        
        DevMsg("[CSGO Shadows] Created cascade light %d\n", i);
    }
}

//-----------------------------------------------------------------------------
// Уничтожение cascade lights
//-----------------------------------------------------------------------------
void CAutoSunShadowManager::DestroyCascadeLights()
{
    for (int i = 0; i < 4; i++)
    {
        if (m_pCascadeLights[i])
        {
            m_pCascadeLights[i]->Release();
            m_pCascadeLights[i] = NULL;
        }
    }
}

//-----------------------------------------------------------------------------
// Обновление cascade lights каждый кадр
//-----------------------------------------------------------------------------
void CAutoSunShadowManager::UpdateCascadeLights()
{
    // Получаем текущий view
    const CViewSetup* pView = view->GetPlayerViewSetup();
    if (!pView)
        return;
    
    // Вычисляем split distances
    CalculateSplitDistances(pView->zNear, 
                            min(m_flShadowDistance, pView->zFar));
    
    // Обновляем каждый каскад
    for (int i = 0; i < m_nNumCascades; i++)
    {
        C_EnvProjectedTexture* pLight = m_pCascadeLights[i];
        if (!pLight)
            continue;
        
        Vector origin;
        QAngle angles;
        float nearZ, farZ, orthoSize;
        
        ComputeCascadeParameters(i, origin, angles, nearZ, farZ, orthoSize);
        
        // Обновляем параметры
        pLight->SetAbsOrigin(origin);
        pLight->SetAbsAngles(angles);
        pLight->m_flNearZ = nearZ;
        pLight->m_flFarZ = farZ;
        pLight->m_flOrthoWidth = orthoSize;
        pLight->m_flOrthoHeight = orthoSize;
        
        // Принудительно обновляем свет
        pLight->UpdateLight(false);
    }
}

//-----------------------------------------------------------------------------
// Вычисление split distances (PSSM)
//-----------------------------------------------------------------------------
void CAutoSunShadowManager::CalculateSplitDistances(float nearPlane, float farPlane)
{
    m_flSplitDistances[0] = nearPlane;
    
    for (int i = 1; i < m_nNumCascades; i++)
    {
        float fraction = (float)i / (float)m_nNumCascades;
        float uniform = nearPlane + (farPlane - nearPlane) * fraction;
        float logarithmic = nearPlane * powf(farPlane / nearPlane, fraction);
        m_flSplitDistances[i] = m_flLambda * logarithmic + (1.0f - m_flLambda) * uniform;
    }
    
    m_flSplitDistances[m_nNumCascades] = farPlane;
}

//-----------------------------------------------------------------------------
// Вычисление параметров каскада
//-----------------------------------------------------------------------------
void CAutoSunShadowManager::ComputeCascadeParameters(int cascadeIndex,
                                                      Vector& outOrigin,
                                                      QAngle& outAngles,
                                                      float& outNearZ,
                                                      float& outFarZ,
                                                      float& outOrthoSize)
{
    const CViewSetup* pView = view->GetPlayerViewSetup();
    if (!pView)
        return;
    
    float nearDist = m_flSplitDistances[cascadeIndex];
    float farDist = m_flSplitDistances[cascadeIndex + 1];
    
    Vector forward, right, up;
    AngleVectors(pView->angles, &forward, &right, &up);
    
    float tanHalfFOV = tan(DEG2RAD(pView->fov * 0.5f));
    float aspectRatio = (float)pView->width / (float)pView->height;
    
    // 8 углов frustum
    Vector frustumCorners[8];
    
    // Near plane
    float nearHeight = 2.0f * tanHalfFOV * nearDist;
    float nearWidth = nearHeight * aspectRatio;
    Vector nearCenter = pView->origin + forward * nearDist;
    
    frustumCorners[0] = nearCenter + up * (nearHeight * 0.5f) - right * (nearWidth * 0.5f);
    frustumCorners[1] = nearCenter + up * (nearHeight * 0.5f) + right * (nearWidth * 0.5f);
    frustumCorners[2] = nearCenter - up * (nearHeight * 0.5f) - right * (nearWidth * 0.5f);
    frustumCorners[3] = nearCenter - up * (nearHeight * 0.5f) + right * (nearWidth * 0.5f);
    
    // Far plane
    float farHeight = 2.0f * tanHalfFOV * farDist;
    float farWidth = farHeight * aspectRatio;
    Vector farCenter = pView->origin + forward * farDist;
    
    frustumCorners[4] = farCenter + up * (farHeight * 0.5f) - right * (farWidth * 0.5f);
    frustumCorners[5] = farCenter + up * (farHeight * 0.5f) + right * (farWidth * 0.5f);
    frustumCorners[6] = farCenter - up * (farHeight * 0.5f) - right * (farWidth * 0.5f);
    frustumCorners[7] = farCenter - up * (farHeight * 0.5f) + right * (farWidth * 0.5f);
    
    // Центр frustum
    Vector frustumCenter = vec3_origin;
    for (int i = 0; i < 8; i++)
        frustumCenter += frustumCorners[i];
    frustumCenter /= 8.0f;
    
    // Максимальный размер
    float maxDistance = 0.0f;
    for (int i = 0; i < 8; i++)
    {
        for (int j = i + 1; j < 8; j++)
        {
            float dist = (frustumCorners[i] - frustumCorners[j]).Length();
            maxDistance = max(maxDistance, dist);
        }
    }
    
    // Позиция света
    outOrigin = frustumCenter - m_vLightDirection * (maxDistance * 0.5f + 500.0f);
    
    // Углы света
    VectorAngles(m_vLightDirection, outAngles);
    
    // Near/Far
    outNearZ = 1.0f;
    outFarZ = maxDistance + 1000.0f;
    
    // Ortho size
    outOrthoSize = maxDistance * 0.6f;
    
    // Стабилизация (округление до пикселя)
    float worldUnitsPerTexel = outOrthoSize / (float)m_nResolution;
    outOrigin.x = floor(outOrigin.x / worldUnitsPerTexel) * worldUnitsPerTexel;
    outOrigin.y = floor(outOrigin.y / worldUnitsPerTexel) * worldUnitsPerTexel;
    outOrigin.z = floor(outOrigin.z / worldUnitsPerTexel) * worldUnitsPerTexel;
}

//-----------------------------------------------------------------------------
// Получение cascade light
//-----------------------------------------------------------------------------
C_EnvProjectedTexture* CAutoSunShadowManager::GetCascadeLight(int index)
{
    if (index < 0 || index >= 4)
        return NULL;
    return m_pCascadeLights[index];
}

//-----------------------------------------------------------------------------
// Настройки
//-----------------------------------------------------------------------------
void CAutoSunShadowManager::SetNumCascades(int num)
{
    num = clamp(num, 2, 4);
    if (num == m_nNumCascades)
        return;
    
    m_nNumCascades = num;
    
    if (m_bEnabled)
    {
        DestroyCascadeLights();
        CreateCascadeLights();
    }
}

void CAutoSunShadowManager::SetResolution(int resolution)
{
    m_nResolution = resolution;
    
    // Обновляем существующие lights
    for (int i = 0; i < m_nNumCascades; i++)
    {
        if (m_pCascadeLights[i])
        {
            m_pCascadeLights[i]->m_nShadowTextureWidth = resolution;
            m_pCascadeLights[i]->m_nShadowTextureHeight = resolution;
        }
    }
}

void CAutoSunShadowManager::SetDistance(float distance)
{
    m_flShadowDistance = distance;
}

void CAutoSunShadowManager::SetLightDirection(const Vector& dir)
{
    m_vLightDirection = dir;
    VectorNormalize(m_vLightDirection);
}

//-----------------------------------------------------------------------------
// Console commands
//-----------------------------------------------------------------------------
CON_COMMAND(csgo_shadows_reload, "Reload cascaded sun shadows")
{
    g_AutoSunShadows.DestroyCascadeLights();
    g_AutoSunShadows.CreateCascadeLights();
    Msg("Sun shadows reloaded\n");
}

CON_COMMAND(csgo_shadows_info, "Show cascade shadows info")
{
    Msg("=== CSGO Cascaded Sun Shadows ===\n");
    Msg("Enabled: %s\n", g_AutoSunShadows.IsEnabled() ? "Yes" : "No");
    Msg("Cascades: %d\n", g_AutoSunShadows.GetNumCascades());
    Msg("Auto mode: %s\n", csgo_shadows_auto.GetBool() ? "Yes" : "No");
    
    for (int i = 0; i < g_AutoSunShadows.GetNumCascades(); i++)
    {
        C_EnvProjectedTexture* pLight = g_AutoSunShadows.GetCascadeLight(i);
        if (pLight)
        {
            Msg("Cascade %d: Active, Pos=(%.0f, %.0f, %.0f)\n", 
                i, 
                pLight->GetAbsOrigin().x,
                pLight->GetAbsOrigin().y,
                pLight->GetAbsOrigin().z);
        }
        else
        {
            Msg("Cascade %d: Not created\n", i);
        }
    }
}

CON_COMMAND(csgo_shadows_toggle, "Toggle cascaded shadows on/off")
{
    if (g_AutoSunShadows.IsEnabled())
    {
        g_AutoSunShadows.Disable();
        Msg("Cascaded shadows disabled\n");
    }
    else
    {
        g_AutoSunShadows.Enable();
        Msg("Cascaded shadows enabled\n");
    }
}

