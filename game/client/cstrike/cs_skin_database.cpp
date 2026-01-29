#include "cbase.h"
#include "cs_skin_database.h"
#include "materialsystem/imaterialvar.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/itexture.h"
#include "filesystem.h"
#include "KeyValues.h"
#include "cs_weapon_parse.h"

// Singleton instance
CCSkinDatabase g_SkinDatabase;

// =============================================================================
// CONSTRUCTOR / DESTRUCTOR
// =============================================================================

CCSkinDatabase::CCSkinDatabase() : m_bInitialized(false)
{
    SetDefLessFunc(m_SkinDefinitions);
    SetDefLessFunc(m_MaterialCache);
    SetDefLessFunc(m_IconCache);
}

CCSkinDatabase::~CCSkinDatabase()
{
    Shutdown();
}

// =============================================================================
// INITIALIZATION
// =============================================================================

bool CCSkinDatabase::Initialize()
{
    if (m_bInitialized)
    {
        DevMsg("[SkinDB] Already initialized\n");
        return true;
    }
    
    DevMsg("[SkinDB] Initializing skin database...\n");
    
    // Загружаем базу данных скинов
    bool bSuccess = LoadFromFile("scripts/skins.txt");
    
    if (bSuccess && m_SkinDefinitions.Count() > 0)
    {
        m_bInitialized = true;
        DevMsg("[SkinDB] Successfully loaded %d skins\n", m_SkinDefinitions.Count());
    }
    else
    {
        DevMsg("[SkinDB] No skins loaded (file missing or empty)\n");
        m_bInitialized = true; // Все равно помечаем как инициализированную
    }
    
    return true;
}

bool CCSkinDatabase::LoadFromFile(const char* pszFilePath)
{
    if (!pszFilePath || !pszFilePath[0])
    {
        DevMsg("[SkinDB] Invalid file path\n");
        return false;
    }
    
    // Проверяем filesystem
    if (!filesystem)
    {
        Warning("[SkinDB] Filesystem is NULL!\n");
        return false;
    }
    
    // Проверяем существование файла
    if (!filesystem->FileExists(pszFilePath, "MOD"))
    {
        DevMsg("[SkinDB] File not found: %s\n", pszFilePath);
        
        // Показываем полный путь для отладки
        char szFullPath[MAX_PATH];
        filesystem->RelativePathToFullPath(pszFilePath, "MOD", szFullPath, sizeof(szFullPath));
        DevMsg("[SkinDB] Full path: %s\n", szFullPath);
        
        return false;
    }
    
    // Загружаем KeyValues
    KeyValues* pKV = new KeyValues("Skins");
    if (!pKV)
    {
        Warning("[SkinDB] Failed to create KeyValues\n");
        return false;
    }
    
    if (!pKV->LoadFromFile(filesystem, pszFilePath, "MOD"))
    {
        DevMsg("[SkinDB] Failed to load KeyValues from: %s\n", pszFilePath);
        pKV->deleteThis();
        return false;
    }
    
    DevMsg("[SkinDB] Loading skins from: %s\n", pszFilePath);
    
    int iLoadedCount = 0;
    
    // Парсим каждый скин
    for (KeyValues* pSkin = pKV->GetFirstSubKey(); pSkin; pSkin = pSkin->GetNextKey())
    {
        SkinDefinition_t def;
        
        if (ParseSkinDefinition(pSkin, def))
        {
            // Добавляем в основную базу
            m_SkinDefinitions.Insert(def.iPaintKit, def);
            
            // Индексируем по оружию
            bool bFound = false;
            FOR_EACH_VEC(m_WeaponSkins, i)
            {
                if (m_WeaponSkins[i].weaponID == def.weaponID)
                {
                    m_WeaponSkins[i].AddPaintKit(def.iPaintKit);
                    bFound = true;
                    break;
                }
            }
            
            if (!bFound)
            {
                WeaponSkinList_t newEntry;
                newEntry.weaponID = def.weaponID;
                newEntry.AddPaintKit(def.iPaintKit);
                m_WeaponSkins.AddToTail(newEntry);
            }
            
            iLoadedCount++;
        }
    }
    
    pKV->deleteThis();
    
    DevMsg("[SkinDB] Loaded %d skin definitions\n", iLoadedCount);
    return iLoadedCount > 0;
}

bool CCSkinDatabase::ParseSkinDefinition(KeyValues* pKV, SkinDefinition_t& def)
{
    if (!pKV)
    {
        Warning("[SkinDB] Null KeyValues in ParseSkinDefinition\n");
        return false;
    }
    
    // Paint Kit ID
    const char* pszKeyName = pKV->GetName();
    if (!pszKeyName || !pszKeyName[0])
    {
        Warning("[SkinDB] KeyValues has no name\n");
        return false;
    }
    
    def.iPaintKit = atoi(pszKeyName);
    if (def.iPaintKit <= 0)
    {
        Warning("[SkinDB] Invalid paint kit ID: %s\n", pszKeyName);
        return false;
    }
    
    // Название скина (ОБЯЗАТЕЛЬНОЕ)
    const char* pszName = pKV->GetString("name", nullptr);
    if (!pszName || !pszName[0])
    {
        Warning("[SkinDB] Skin %d has no name\n", def.iPaintKit);
        return false;
    }
    Q_strncpy(def.szName, pszName, sizeof(def.szName));
    
    const char* szDescription = pKV->GetString("description", nullptr);
    Q_strncpy(def.szDescription, szDescription, sizeof(def.szDescription));
    
    // Оружие (ОБЯЗАТЕЛЬНОЕ)
    const char* pszWeapon = pKV->GetString("weapon", nullptr);
    if (!pszWeapon || !pszWeapon[0])
    {
        Warning("[SkinDB] Skin %d (%s) has no weapon\n", def.iPaintKit, def.szName);
        return false;
    }
    def.weaponID = AliasToWeaponID(pszWeapon);
    if (def.weaponID == WEAPON_NONE)
    {
        Warning("[SkinDB] Skin %d (%s) has invalid weapon: %s\n", def.iPaintKit, def.szName, pszWeapon);
        return false;
    }
    
    // Базовый материал (ОБЯЗАТЕЛЬНОЕ для patch system)
    const char* pszBaseMaterial = pKV->GetString("base", nullptr);
    if (!pszBaseMaterial || !pszBaseMaterial[0])
    {
        Warning("[SkinDB] Skin %d (%s) has no base material\n", def.iPaintKit, def.szName);
        return false;
    }
    Q_strncpy(def.szBaseMaterial, pszBaseMaterial, sizeof(def.szBaseMaterial));
    
    // Иконка (ОПЦИОНАЛЬНОЕ)
    const char* pszIcon = pKV->GetString("icon", "");
    Q_strncpy(def.szIconPath, pszIcon, sizeof(def.szIconPath));
    
    // Редкость (ОПЦИОНАЛЬНОЕ)
    def.rarity = (ESkinRarity)pKV->GetInt("rarity", SKIN_RARITY_COMMON);
    if (def.rarity < 0 || def.rarity >= SKIN_RARITY_COUNT)
    {
        Warning("[SkinDB] Skin %d (%s) has invalid rarity: %d, setting to Common\n", 
                def.iPaintKit, def.szName, (int)def.rarity);
        def.rarity = SKIN_RARITY_COMMON;
    }
    
    // Парсим override параметры
    ParseVMTParams(pKV, def.vmtParams);
    
    return true;
}

void CCSkinDatabase::ParseVMTParams(KeyValues* pKV, SkinDefinition_t::VMTParams_t& params)
{
    // ========================================
    // TEXTURES
    // ========================================
    const char* pszValue;
    
    if ((pszValue = pKV->GetString("$basetexture", nullptr)) != nullptr)
        Q_strncpy(params.szBaseTexture, pszValue, sizeof(params.szBaseTexture));
    
    if ((pszValue = pKV->GetString("$basetexture2", nullptr)) != nullptr)
        Q_strncpy(params.szBaseTexture2, pszValue, sizeof(params.szBaseTexture2));
    
    if ((pszValue = pKV->GetString("$bumpmap", nullptr)) != nullptr)
        Q_strncpy(params.szBumpMap, pszValue, sizeof(params.szBumpMap));
    
    if ((pszValue = pKV->GetString("$bumpmap2", nullptr)) != nullptr)
        Q_strncpy(params.szBumpMap2, pszValue, sizeof(params.szBumpMap2));
    
    if ((pszValue = pKV->GetString("$detail", nullptr)) != nullptr)
        Q_strncpy(params.szDetailTexture, pszValue, sizeof(params.szDetailTexture));
    
    if ((pszValue = pKV->GetString("$envmap", nullptr)) != nullptr)
        Q_strncpy(params.szEnvMap, pszValue, sizeof(params.szEnvMap));
    
    if ((pszValue = pKV->GetString("$phongexponenttexture", nullptr)) != nullptr)
        Q_strncpy(params.szPhongExponentTexture, pszValue, sizeof(params.szPhongExponentTexture));
    
    if ((pszValue = pKV->GetString("$phongwarptexture", nullptr)) != nullptr)
        Q_strncpy(params.szPhongWarpTexture, pszValue, sizeof(params.szPhongWarpTexture));
    
    // ========================================
    // PHONG
    // ========================================
    if (pKV->FindKey("$phong"))
        params.bPhong = pKV->GetInt("$phong");
    
    if (pKV->FindKey("$phongboost"))
        params.flPhongBoost = pKV->GetFloat("$phongboost");
    
    if (pKV->FindKey("$phongexponent"))
        params.flPhongExponent = pKV->GetFloat("$phongexponent");
    
    if (pKV->FindKey("$phongalbedotint"))
        params.flPhongAlbedoTint = pKV->GetFloat("$phongalbedotint");
    
    if (pKV->FindKey("$phongalbedoboost"))
        params.bPhongAlbedoBoost = pKV->GetInt("$phongalbedoboost");
    
    // Phong fresnel ranges "[min mid max]"
    if ((pszValue = pKV->GetString("$phongfresnelranges", nullptr)) != nullptr)
    {
        if (sscanf(pszValue, "[%f %f %f]", 
                   &params.flPhongFresnelRanges[0],
                   &params.flPhongFresnelRanges[1],
                   &params.flPhongFresnelRanges[2]) != 3)
        {
            // Попытка парсинга без скобок
            sscanf(pszValue, "%f %f %f",
                   &params.flPhongFresnelRanges[0],
                   &params.flPhongFresnelRanges[1],
                   &params.flPhongFresnelRanges[2]);
        }
    }
    
    // ========================================
    // ENVIRONMENT MAPPING
    // ========================================
    if ((pszValue = pKV->GetString("$envmaptint", nullptr)) != nullptr)
    {
        if (sscanf(pszValue, "[%f %f %f]",
                   &params.flEnvMapTint[0],
                   &params.flEnvMapTint[1],
                   &params.flEnvMapTint[2]) != 3)
        {
            sscanf(pszValue, "%f %f %f",
                   &params.flEnvMapTint[0],
                   &params.flEnvMapTint[1],
                   &params.flEnvMapTint[2]);
        }
    }
    
    if (pKV->FindKey("$envmapsaturation"))
        params.flEnvMapSaturation = pKV->GetFloat("$envmapsaturation");
    
    if (pKV->FindKey("$envmapcontrast"))
        params.flEnvMapContrast = pKV->GetFloat("$envmapcontrast");
    
    if (pKV->FindKey("$envmapfresnel"))
        params.flEnvMapFresnel = pKV->GetFloat("$envmapfresnel");
    
    if (pKV->FindKey("$fresnelreflection"))
        params.flFresnelReflection = pKV->GetFloat("$fresnelreflection");
    
    // ========================================
    // ALPHA & TRANSPARENCY
    // ========================================
    if (pKV->FindKey("$alphatest"))
        params.bAlphaTest = pKV->GetInt("$alphatest");
    
    if (pKV->FindKey("$alphatestreference"))
        params.flAlphaTestReference = pKV->GetFloat("$alphatestreference");
    
    if (pKV->FindKey("$translucent"))
        params.bTranslucent = pKV->GetInt("$translucent");
    
    if (pKV->FindKey("$additive"))
        params.bAdditive = pKV->GetInt("$additive");
    
    // ========================================
    // TEXTURE MODIFIERS
    // ========================================
    if (pKV->FindKey("$basemapalphaphongmask"))
        params.bBaseTextureNoEnvMap = pKV->GetInt("$basemapalphaphongmask");
    
    if (pKV->FindKey("$normalmapalphaenvmapmask"))
        params.bNormalMapAlphaPhongMask = pKV->GetInt("$normalmapalphaenvmapmask");
    
    if (pKV->FindKey("$basealphaenvmapmask"))
        params.bBaseAlphaEnvMapMask = pKV->GetInt("$basealphaenvmapmask");
    
    // ========================================
    // DETAIL TEXTURE
    // ========================================
    if (pKV->FindKey("$detailscale"))
        params.flDetailScale = pKV->GetFloat("$detailscale");
    
    if (pKV->FindKey("$detailblendmode"))
        params.iDetailBlendMode = pKV->GetInt("$detailblendmode");
    
    if (pKV->FindKey("$detailblendfactor"))
        params.flDetailBlendFactor = pKV->GetFloat("$detailblendfactor");
    
    // ========================================
    // RIM LIGHTING
    // ========================================
    if (pKV->FindKey("$rimlight"))
        params.bRimLight = pKV->GetInt("$rimlight");
    
    if (pKV->FindKey("$rimlightexponent"))
        params.flRimLightExponent = pKV->GetFloat("$rimlightexponent");
    
    if (pKV->FindKey("$rimlightboost"))
        params.flRimLightBoost = pKV->GetFloat("$rimlightboost");
    
    // ========================================
    // SELF ILLUMINATION
    // ========================================
    if (pKV->FindKey("$selfillum"))
        params.bSelfIllum = pKV->GetInt("$selfillum");
    
    if ((pszValue = pKV->GetString("$selfillumtint", nullptr)) != nullptr)
    {
        sscanf(pszValue, "[%f %f %f]",
               &params.flSelfIllumTint[0],
               &params.flSelfIllumTint[1],
               &params.flSelfIllumTint[2]);
    }
    
    // ========================================
    // COLOR
    // ========================================
    if ((pszValue = pKV->GetString("$color", nullptr)) != nullptr)
    {
        sscanf(pszValue, "[%f %f %f]",
               &params.flColor[0],
               &params.flColor[1],
               &params.flColor[2]);
    }
    
    if ((pszValue = pKV->GetString("$color2", nullptr)) != nullptr)
    {
        sscanf(pszValue, "[%f %f %f]",
               &params.flColor2[0],
               &params.flColor2[1],
               &params.flColor2[2]);
    }
    
    // ========================================
    // MISC
    // ========================================
    if (pKV->FindKey("$nocull"))
        params.bNoCull = pKV->GetInt("$nocull");
    
    if (pKV->FindKey("$nodecal"))
        params.bNoDecal = pKV->GetInt("$nodecal");
    
    if (pKV->FindKey("$halflambert"))
        params.bHalfLambert = pKV->GetInt("$halflambert");
    
    // ========================================
    // CUSTOM CS:GO PARAMETERS
    // ========================================
    if (pKV->FindKey("$wear"))
        params.flWear = pKV->GetFloat("$wear");
    
    if (pKV->FindKey("$seed"))
        params.iSeed = pKV->GetInt("$seed");
    
    if (pKV->FindKey("$patternscale"))
        params.flPatternScale = pKV->GetFloat("$patternscale");
    
    if (pKV->FindKey("$patternrotate"))
        params.flPatternRotate = pKV->GetFloat("$patternrotate");
}

void CCSkinDatabase::Shutdown()
{
    if (!m_bInitialized)
        return;
    
    DevMsg("[SkinDB] Shutting down...\n");
    
    // Освобождаем все материалы
    ClearMaterialCache();
    
    // Освобождаем все иконки
    ClearIconCache();
    
    // Очищаем weapon skins (память освободится автоматически через деструкторы)
    m_WeaponSkins.Purge();
    
    // Очищаем данные
    m_SkinDefinitions.Purge();
    
    m_bInitialized = false;
}

// =============================================================================
// LOOKUP FUNCTIONS
// =============================================================================

const SkinDefinition_t* CCSkinDatabase::FindSkinByPaintKit(int iPaintKit) const
{
    if (!m_bInitialized)
        return nullptr;
    
    int idx = m_SkinDefinitions.Find(iPaintKit);
    if (idx == m_SkinDefinitions.InvalidIndex())
        return nullptr;
    
    return &m_SkinDefinitions[idx];
}

const SkinDefinition_t* CCSkinDatabase::FindSkinByName(const char* pszName, CSWeaponID weaponID) const
{
    if (!m_bInitialized || !pszName || !pszName[0])
        return nullptr;
    
    FOR_EACH_MAP_FAST(m_SkinDefinitions, i)
    {
        const SkinDefinition_t& def = m_SkinDefinitions[i];
        
        if (Q_stricmp(def.szName, pszName) != 0)
            continue;
        
        if (weaponID != WEAPON_NONE && def.weaponID != weaponID)
            continue;
        
        return &def;
    }
    
    return nullptr;
}

void CCSkinDatabase::GetSkinsForWeapon(CSWeaponID weaponID, CUtlVector<const SkinDefinition_t*>& skins) const
{
    skins.RemoveAll();
    
    if (!m_bInitialized)
        return;
    
    // Ищем weapon в списке
    FOR_EACH_VEC(m_WeaponSkins, i)
    {
        if (m_WeaponSkins[i].weaponID == weaponID)
        {
            // Получаем все paint kit ID для этого оружия
            int count = m_WeaponSkins[i].GetCount();
            
            for (int j = 0; j < count; j++)
            {
                int iPaintKit = m_WeaponSkins[i].GetPaintKit(j);
                
                // Находим полное определение скина
                const SkinDefinition_t* pDef = FindSkinByPaintKit(iPaintKit);
                if (pDef)
                {
                    skins.AddToTail(pDef);
                }
            }
            break;
        }
    }
}

void CCSkinDatabase::GetAllSkins(CUtlVector<const SkinDefinition_t*>& skins) const
{
    skins.RemoveAll();
    
    if (!m_bInitialized)
        return;
    
    FOR_EACH_MAP_FAST(m_SkinDefinitions, i)
    {
        skins.AddToTail(&m_SkinDefinitions[i]);
    }
}

// =============================================================================
// MATERIAL MANAGEMENT
// =============================================================================

IMaterial* CCSkinDatabase::GetSkinMaterial(int iPaintKit)
{
    if (!m_bInitialized || iPaintKit <= 0)
        return nullptr;
    
    // Проверяем кэш
    int cacheIdx = m_MaterialCache.Find(iPaintKit);
    if (cacheIdx != m_MaterialCache.InvalidIndex())
    {
        return m_MaterialCache[cacheIdx];
    }
    
    // Ищем определение скина
    const SkinDefinition_t* pDef = FindSkinByPaintKit(iPaintKit);
    if (!pDef)
    {
        Warning("[SkinDB] Paint kit %d not found\n", iPaintKit);
        return nullptr;
    }
    
    // Загружаем материал
    IMaterial* pMat = LoadMaterial(pDef);
    if (pMat)
    {
        m_MaterialCache.Insert(iPaintKit, pMat);
    }
    
    return pMat;
}

ITexture* CCSkinDatabase::GetSkinIcon(int iPaintKit)
{
    if (!m_bInitialized || iPaintKit <= 0)
        return nullptr;
    
    // Проверяем кэш
    int cacheIdx = m_IconCache.Find(iPaintKit);
    if (cacheIdx != m_IconCache.InvalidIndex())
    {
        return m_IconCache[cacheIdx];
    }
    
    // Ищем определение скина
    const SkinDefinition_t* pDef = FindSkinByPaintKit(iPaintKit);
    if (!pDef)
        return nullptr;
    
    // Загружаем иконку
    ITexture* pIcon = LoadIcon(pDef);
    if (pIcon)
    {
        m_IconCache.Insert(iPaintKit, pIcon);
    }
    
    return pIcon;
}

IMaterial* CCSkinDatabase::LoadMaterial(const SkinDefinition_t* pDef)
{
    if (!pDef)
        return nullptr;
    
    if (!materials)
    {
        Warning("[SkinDB] Materials system is NULL!\n");
        return nullptr;
    }
    
    // Загружаем базовый материал
    IMaterial* pBaseMat = materials->FindMaterial(pDef->szBaseMaterial, TEXTURE_GROUP_MODEL, true);
    
    if (!pBaseMat || IsErrorMaterial(pBaseMat))
    {
        Warning("[SkinDB] Failed to load base material: %s (paint kit %d)\n", 
                pDef->szBaseMaterial, pDef->iPaintKit);
        return nullptr;
    }
    
    // Копируем параметры базового материала
    KeyValues* pVMT = CloneBaseMaterialKeyValues(pBaseMat);
    if (!pVMT)
    {
        Warning("[SkinDB] Failed to clone base material KV for paint kit %d\n", pDef->iPaintKit);
        return nullptr;
    }
    
    // Применяем patch (override параметры)
    ApplyVMTPatch(pVMT, pDef->vmtParams);
    
    // ИСПРАВЛЕНО: Извлекаем только имя файла из пути базового материала
    // Например: "models\weapons\v_models\pist_deagle\pist_deagle" -> "pist_deagle"
    
    char szBaseMaterialPath[256];
    Q_strncpy(szBaseMaterialPath, pDef->szBaseMaterial, sizeof(szBaseMaterialPath));
    
    // Заменяем все обратные слеши на прямые
    for (char* p = szBaseMaterialPath; *p; p++)
    {
        if (*p == '\\')
            *p = '/';
    }
    
    // Находим последний слеш
    const char* pLastSlash = Q_strrchr(szBaseMaterialPath, '/');
    const char* pMaterialName = pLastSlash ? (pLastSlash + 1) : szBaseMaterialPath;
    
    DevMsg("[SkinDB] Extracted material name '%s' from base path '%s'\n", 
           pMaterialName, pDef->szBaseMaterial);
    
    // Создаем новый материал с именем файла базового материала
    IMaterial* pMat = materials->CreateMaterial(pMaterialName, pVMT);
    
    if (!pMat || IsErrorMaterial(pMat))
    {
        Warning("[SkinDB] Failed to create patched material '%s' for paint kit %d\n", 
                pMaterialName, pDef->iPaintKit);
        pVMT->deleteThis();
        return nullptr;
    }
    
    // Увеличиваем reference count
    pMat->IncrementReferenceCount();
    
    // Precache
    if (!pMat->IsPrecached())
    {
        MaterialLock_t hLock = materials->Lock();
        pMat->Refresh();
        materials->Unlock(hLock);
    }
    
    DevMsg("[SkinDB] Successfully created material '%s' for paint kit %d (%s)\n", 
           pMaterialName, pDef->iPaintKit, pDef->szName);
    
    return pMat;
}

KeyValues* CCSkinDatabase::CloneBaseMaterialKeyValues(IMaterial* pBaseMat)
{
    if (!pBaseMat)
        return nullptr;
    
    // Получаем shader name
    const char* pszShaderName = pBaseMat->GetShaderName();
    if (!pszShaderName || !pszShaderName[0])
        pszShaderName = "VertexLitGeneric";
    
    KeyValues* pKV = new KeyValues(pszShaderName);
    if (!pKV)
        return nullptr;
    
    // Копируем все material vars
    IMaterialVar** ppParams = pBaseMat->GetShaderParams();
    int numParams = pBaseMat->ShaderParamCount();
    
    for (int i = 0; i < numParams; i++)
    {
        IMaterialVar* pVar = ppParams[i];
        if (!pVar)
            continue;
        
        const char* pszName = pVar->GetName();
        if (!pszName || !pszName[0])
            continue;
        
        switch (pVar->GetType())
        {
            case MATERIAL_VAR_TYPE_FLOAT:
                pKV->SetFloat(pszName, pVar->GetFloatValue());
                break;
                
            case MATERIAL_VAR_TYPE_INT:
                pKV->SetInt(pszName, pVar->GetIntValue());
                break;
                
            case MATERIAL_VAR_TYPE_STRING:
                pKV->SetString(pszName, pVar->GetStringValue());
                break;
                
            case MATERIAL_VAR_TYPE_VECTOR:
            {
                float const* pVec = pVar->GetVecValue();
                int dim = pVar->VectorSize();
                
                if (dim == 3)
                {
                    char szValue[128];
                    Q_snprintf(szValue, sizeof(szValue), "[%f %f %f]", pVec[0], pVec[1], pVec[2]);
                    pKV->SetString(pszName, szValue);
                }
                else if (dim == 4)
                {
                    char szValue[128];
                    Q_snprintf(szValue, sizeof(szValue), "[%f %f %f %f]", pVec[0], pVec[1], pVec[2], pVec[3]);
                    pKV->SetString(pszName, szValue);
                }
                break;
            }
                
            case MATERIAL_VAR_TYPE_TEXTURE:
            {
                ITexture* pTex = pVar->GetTextureValue();
                if (pTex && !pTex->IsError())
                {
                    pKV->SetString(pszName, pTex->GetName());
                }
                break;
            }
                
            case MATERIAL_VAR_TYPE_MATERIAL:
            {
                IMaterial* pMat = pVar->GetMaterialValue();
                if (pMat && !IsErrorMaterial(pMat))
                {
                    pKV->SetString(pszName, pMat->GetName());
                }
                break;
            }
        }
    }
    
    return pKV;
}

void CCSkinDatabase::ApplyVMTPatch(KeyValues* pKV, const SkinDefinition_t::VMTParams_t& params)
{
    if (!pKV)
        return;
    
    // ========================================
    // TEXTURES
    // ========================================
    if (params.szBaseTexture[0])
        pKV->SetString("$basetexture", params.szBaseTexture);
    
    if (params.szBaseTexture2[0])
        pKV->SetString("$basetexture2", params.szBaseTexture2);
    
    if (params.szBumpMap[0])
        pKV->SetString("$bumpmap", params.szBumpMap);
    
    if (params.szBumpMap2[0])
        pKV->SetString("$bumpmap2", params.szBumpMap2);
    
    if (params.szDetailTexture[0])
        pKV->SetString("$detail", params.szDetailTexture);
    
    if (params.szEnvMap[0])
        pKV->SetString("$envmap", params.szEnvMap);
    
    if (params.szPhongExponentTexture[0])
        pKV->SetString("$phongexponenttexture", params.szPhongExponentTexture);
    
    if (params.szPhongWarpTexture[0])
        pKV->SetString("$phongwarptexture", params.szPhongWarpTexture);
    
    // ========================================
    // PHONG
    // ========================================
    if (params.bPhong >= 0)
        pKV->SetInt("$phong", params.bPhong);
    
    if (params.flPhongBoost >= 0.0f)
        pKV->SetFloat("$phongboost", params.flPhongBoost);
    
    if (params.flPhongExponent >= 0.0f)
        pKV->SetFloat("$phongexponent", params.flPhongExponent);
    
    if (params.flPhongAlbedoTint >= 0.0f)
        pKV->SetFloat("$phongalbedotint", params.flPhongAlbedoTint);
    
    if (params.bPhongAlbedoBoost >= 0)
        pKV->SetInt("$phongalbedoboost", params.bPhongAlbedoBoost);
    
    // Phong fresnel ranges
    if (params.flPhongFresnelRanges[0] >= 0.0f)
    {
        char szValue[128];
        Q_snprintf(szValue, sizeof(szValue), "[%f %f %f]",
                   params.flPhongFresnelRanges[0],
                   params.flPhongFresnelRanges[1],
                   params.flPhongFresnelRanges[2]);
        pKV->SetString("$phongfresnelranges", szValue);
    }
    
    // ========================================
    // ENVIRONMENT MAPPING
    // ========================================
    if (params.flEnvMapTint[0] >= 0.0f)
    {
        char szValue[128];
        Q_snprintf(szValue, sizeof(szValue), "[%f %f %f]",
                   params.flEnvMapTint[0],
                   params.flEnvMapTint[1],
                   params.flEnvMapTint[2]);
        pKV->SetString("$envmaptint", szValue);
    }
    
    if (params.flEnvMapSaturation >= 0.0f)
        pKV->SetFloat("$envmapsaturation", params.flEnvMapSaturation);
    
    if (params.flEnvMapContrast >= 0.0f)
        pKV->SetFloat("$envmapcontrast", params.flEnvMapContrast);
    
    if (params.flEnvMapFresnel >= 0.0f)
        pKV->SetFloat("$envmapfresnel", params.flEnvMapFresnel);
    
    if (params.flFresnelReflection >= 0.0f)
        pKV->SetFloat("$fresnelreflection", params.flFresnelReflection);
    
    // ========================================
    // ALPHA & TRANSPARENCY
    // ========================================
    if (params.bAlphaTest >= 0)
        pKV->SetInt("$alphatest", params.bAlphaTest);
    
    if (params.flAlphaTestReference >= 0.0f)
        pKV->SetFloat("$alphatestreference", params.flAlphaTestReference);
    
    if (params.bTranslucent >= 0)
        pKV->SetInt("$translucent", params.bTranslucent);
    
    if (params.bAdditive >= 0)
        pKV->SetInt("$additive", params.bAdditive);
    
    // ========================================
    // TEXTURE MODIFIERS
    // ========================================
    if (params.bBaseTextureNoEnvMap >= 0)
        pKV->SetInt("$basemapalphaphongmask", params.bBaseTextureNoEnvMap);
    
    if (params.bNormalMapAlphaPhongMask >= 0)
        pKV->SetInt("$normalmapalphaenvmapmask", params.bNormalMapAlphaPhongMask);
    
    if (params.bBaseAlphaEnvMapMask >= 0)
        pKV->SetInt("$basealphaenvmapmask", params.bBaseAlphaEnvMapMask);
    
    // ========================================
    // DETAIL TEXTURE
    // ========================================
    if (params.flDetailScale >= 0.0f)
        pKV->SetFloat("$detailscale", params.flDetailScale);
    
    if (params.iDetailBlendMode >= 0)
        pKV->SetInt("$detailblendmode", params.iDetailBlendMode);
    
    if (params.flDetailBlendFactor >= 0.0f)
        pKV->SetFloat("$detailblendfactor", params.flDetailBlendFactor);
    
    // ========================================
    // RIM LIGHTING
    // ========================================
    if (params.bRimLight >= 0)
        pKV->SetInt("$rimlight", params.bRimLight);
    
    if (params.flRimLightExponent >= 0.0f)
        pKV->SetFloat("$rimlightexponent", params.flRimLightExponent);
    
    if (params.flRimLightBoost >= 0.0f)
        pKV->SetFloat("$rimlightboost", params.flRimLightBoost);
    
    // ========================================
    // SELF ILLUMINATION
    // ========================================
    if (params.bSelfIllum >= 0)
        pKV->SetInt("$selfillum", params.bSelfIllum);
    
    if (params.flSelfIllumTint[0] >= 0.0f)
    {
        char szValue[128];
        Q_snprintf(szValue, sizeof(szValue), "[%f %f %f]",
                   params.flSelfIllumTint[0],
                   params.flSelfIllumTint[1],
                   params.flSelfIllumTint[2]);
        pKV->SetString("$selfillumtint", szValue);
    }
    
    // ========================================
    // COLOR MODULATION
    // ========================================
    if (params.flColor[0] >= 0.0f)
    {
        char szValue[128];
        Q_snprintf(szValue, sizeof(szValue), "[%f %f %f]",
                   params.flColor[0],
                   params.flColor[1],
                   params.flColor[2]);
        pKV->SetString("$color", szValue);
    }
    
    if (params.flColor2[0] >= 0.0f)
    {
        char szValue[128];
        Q_snprintf(szValue, sizeof(szValue), "[%f %f %f]",
                   params.flColor2[0],
                   params.flColor2[1],
                   params.flColor2[2]);
        pKV->SetString("$color2", szValue);
    }
    
    // ========================================
    // MISC RENDERING
    // ========================================
    if (params.bNoCull >= 0)
        pKV->SetInt("$nocull", params.bNoCull);
    
    if (params.bNoDecal >= 0)
        pKV->SetInt("$nodecal", params.bNoDecal);
    
    if (params.bHalfLambert >= 0)
        pKV->SetInt("$halflambert", params.bHalfLambert);
    
    // ========================================
    // CUSTOM CS:GO SKIN PARAMETERS
    // ========================================
    if (params.flWear >= 0.0f)
        pKV->SetFloat("$wear", params.flWear);
    
    if (params.iSeed >= 0)
        pKV->SetInt("$seed", params.iSeed);
    
    if (params.flPatternScale >= 0.0f)
        pKV->SetFloat("$patternscale", params.flPatternScale);
    
    if (params.flPatternRotate >= 0.0f)
        pKV->SetFloat("$patternrotate", params.flPatternRotate);
}

ITexture* CCSkinDatabase::LoadIcon(const SkinDefinition_t* pDef)
{
    if (!pDef || !pDef->szIconPath[0])
        return nullptr;
    
    if (!materials)
        return nullptr;
    
    // Загружаем текстуру иконки
    ITexture* pIcon = materials->FindTexture(pDef->szIconPath, TEXTURE_GROUP_VGUI);
    
    if (!pIcon || pIcon->IsError())
    {
        // Не критично если иконка не загрузилась
        DevMsg("[SkinDB] Icon not found: %s (paint kit %d)\n", 
               pDef->szIconPath, pDef->iPaintKit);
        return nullptr;
    }
    
    DevMsg("[SkinDB] Loaded icon for paint kit %d: %s\n", 
           pDef->iPaintKit, pDef->szIconPath);
    
    return pIcon;
}

void CCSkinDatabase::ReleaseMaterial(int iPaintKit)
{
    int idx = m_MaterialCache.Find(iPaintKit);
    if (idx == m_MaterialCache.InvalidIndex())
        return;
    
    IMaterial* pMat = m_MaterialCache[idx];
    if (pMat)
    {
        pMat->DecrementReferenceCount();
    }
    
    m_MaterialCache.RemoveAt(idx);
}

void CCSkinDatabase::ReleaseIcon(int iPaintKit)
{
    int idx = m_IconCache.Find(iPaintKit);
    if (idx == m_IconCache.InvalidIndex())
        return;
    
    // Текстуры не требуют DecrementReferenceCount
    m_IconCache.RemoveAt(idx);
}

void CCSkinDatabase::ClearMaterialCache()
{
    DevMsg("[SkinDB] Clearing material cache (%d materials)...\n", m_MaterialCache.Count());
    
    FOR_EACH_MAP_FAST(m_MaterialCache, i)
    {
        IMaterial* pMat = m_MaterialCache[i];
        if (pMat)
        {
            pMat->DecrementReferenceCount();
        }
    }
    
    m_MaterialCache.Purge();
}

void CCSkinDatabase::ClearIconCache()
{
    DevMsg("[SkinDB] Clearing icon cache (%d icons)...\n", m_IconCache.Count());
    m_IconCache.Purge();
}

// =============================================================================
// DEBUG FUNCTIONS
// =============================================================================

void CCSkinDatabase::PrintAllSkins() const
{
    if (!m_bInitialized)
    {
        Msg("[SkinDB] Database not initialized\n");
        return;
    }
    
    Msg("[SkinDB] Total skins: %d\n", m_SkinDefinitions.Count());
    Msg("%-6s %-30s %-20s %s\n", "ID", "Name", "Weapon", "Rarity");
    Msg("--------------------------------------------------------------------------------\n");
    
    FOR_EACH_MAP_FAST(m_SkinDefinitions, i)
    {
        const SkinDefinition_t& def = m_SkinDefinitions[i];
        Msg("%-6d %-30s %-20s %s\n", 
            def.iPaintKit, 
            def.szName, 
            WeaponIDToAlias(def.weaponID),
            GetRarityName(def.rarity));
    }
}

void CCSkinDatabase::PrintSkinsForWeapon(CSWeaponID weaponID) const
{
    if (!m_bInitialized)
    {
        Msg("[SkinDB] Database not initialized\n");
        return;
    }
    
    CUtlVector<const SkinDefinition_t*> skins;
    GetSkinsForWeapon(weaponID, skins);
    
    Msg("[SkinDB] Skins for %s: %d\n", WeaponIDToAlias(weaponID), skins.Count());
    Msg("%-6s %-30s %s\n", "ID", "Name", "Rarity");
    Msg("--------------------------------------------------------------------------------\n");
    
    FOR_EACH_VEC(skins, i)
    {
        const SkinDefinition_t* pDef = skins[i];
        Msg("%-6d %-30s %s\n", 
            pDef->iPaintKit, 
            pDef->szName,
            GetRarityName(pDef->rarity));
    }
}

bool CCSkinDatabase::SaveSkinToFile(const SkinDefinition_t& skin, const char* pszFilePath)
{
    if (!pszFilePath || !pszFilePath[0])
        return false;
    
    DevMsg("[SkinDB] SaveSkinToFile called for paint kit %d\n", skin.iPaintKit);
    
    // Загружаем существующий файл
    KeyValues* pKV = new KeyValues("Skins");
    
    bool bFileExists = filesystem->FileExists(pszFilePath, "MOD");
    if (bFileExists)
    {
        if (!pKV->LoadFromFile(filesystem, pszFilePath, "MOD"))
        {
            Warning("[SkinDB] Failed to load existing %s\n", pszFilePath);
            pKV->deleteThis();
            return false;
        }
    }
    
    // Создаем или обновляем запись
    char szPaintKit[16];
    Q_snprintf(szPaintKit, sizeof(szPaintKit), "%d", skin.iPaintKit);
    
    KeyValues* pSkinKV = pKV->FindKey(szPaintKit, true);
    if (!pSkinKV)
    {
        Warning("[SkinDB] Failed to create skin entry for paint kit %d\n", skin.iPaintKit);
        pKV->deleteThis();
        return false;
    }
    
    // Очищаем существующие данные
    pSkinKV->Clear();
    
    DevMsg("[SkinDB] Writing skin data...\n");
    
    // ========================================
    // ОБЩИЕ ПАРАМЕТРЫ
    // ========================================
    pSkinKV->SetString("name", skin.szName);
    DevMsg("[SkinDB] Name: %s\n", skin.szName);
    
    // Конвертируем weapon ID в строку
    const char* pszWeaponAlias = WeaponIDToAlias(skin.weaponID);
    if (pszWeaponAlias && Q_strnicmp(pszWeaponAlias, "weapon_", 7) == 0)
    {
        pszWeaponAlias += 7; // Убираем префикс "weapon_"
    }
    pSkinKV->SetString("weapon", pszWeaponAlias);
    DevMsg("[SkinDB] Weapon: %s\n", pszWeaponAlias);
    
    if (skin.szBaseMaterial[0])
    {
        pSkinKV->SetString("base", skin.szBaseMaterial);
        DevMsg("[SkinDB] Base: %s\n", skin.szBaseMaterial);
    }
    
    if (skin.szIconPath[0])
        pSkinKV->SetString("icon", skin.szIconPath);
    
    pSkinKV->SetInt("rarity", (int)skin.rarity);
    
    // ========================================
    // VMT ПАРАМЕТРЫ (только установленные)
    // ========================================
    const SkinDefinition_t::VMTParams_t& params = skin.vmtParams;
    
    // Текстуры
    if (params.szBaseTexture[0])
    {
        pSkinKV->SetString("$basetexture", params.szBaseTexture);
        DevMsg("[SkinDB] $basetexture: %s\n", params.szBaseTexture);
    }
    
    if (params.szBumpMap[0])
    {
        pSkinKV->SetString("$bumpmap", params.szBumpMap);
        DevMsg("[SkinDB] $bumpmap: %s\n", params.szBumpMap);
    }
    
    if (params.szPhongExponentTexture[0])
    {
        pSkinKV->SetString("$phongexponenttexture", params.szPhongExponentTexture);
        DevMsg("[SkinDB] $phongexponenttexture: %s\n", params.szPhongExponentTexture);
    }
    
    if (params.szDetailTexture[0])
        pSkinKV->SetString("$detail", params.szDetailTexture);
    
    if (params.szEnvMap[0])
        pSkinKV->SetString("$envmap", params.szEnvMap);
    
    // Phong (только если явно установлено)
    if (params.bPhong >= 0)
        pSkinKV->SetInt("$phong", params.bPhong);
    
    if (params.flPhongBoost >= 0.0f)
        pSkinKV->SetFloat("$phongboost", params.flPhongBoost);
    
    if (params.flPhongExponent >= 0.0f)
        pSkinKV->SetFloat("$phongexponent", params.flPhongExponent);
    
    if (params.flPhongFresnelRanges[0] >= 0.0f)
    {
        char szValue[128];
        Q_snprintf(szValue, sizeof(szValue), "[%.2f %.2f %.2f]",
                   params.flPhongFresnelRanges[0],
                   params.flPhongFresnelRanges[1],
                   params.flPhongFresnelRanges[2]);
        pSkinKV->SetString("$phongfresnelranges", szValue);
    }
    
    // EnvMap
    if (params.flEnvMapTint[0] >= 0.0f)
    {
        char szValue[128];
        Q_snprintf(szValue, sizeof(szValue), "[%.2f %.2f %.2f]",
                   params.flEnvMapTint[0],
                   params.flEnvMapTint[1],
                   params.flEnvMapTint[2]);
        pSkinKV->SetString("$envmaptint", szValue);
    }
    
    if (params.bNormalMapAlphaPhongMask >= 0)
        pSkinKV->SetInt("$normalmapalphaenvmapmask", params.bNormalMapAlphaPhongMask);
        
    if (params.bBaseAlphaEnvMapMask >= 0)
        pSkinKV->SetInt("$basealphaenvmapmask", params.bBaseAlphaEnvMapMask);
    
    // Detail
    if (params.flDetailScale >= 0.0f)
        pSkinKV->SetFloat("$detailscale", params.flDetailScale);
    
    if (params.iDetailBlendMode >= 0)
        pSkinKV->SetInt("$detailblendmode", params.iDetailBlendMode);
    
    if (params.flDetailBlendFactor >= 0.0f)
        pSkinKV->SetFloat("$detailblendfactor", params.flDetailBlendFactor);
    
    // Rim lighting
    if (params.bRimLight >= 0)
        pSkinKV->SetInt("$rimlight", params.bRimLight);
    
    if (params.flRimLightExponent >= 0.0f)
        pSkinKV->SetFloat("$rimlightexponent", params.flRimLightExponent);
    
    if (params.flRimLightBoost >= 0.0f)
        pSkinKV->SetFloat("$rimlightboost", params.flRimLightBoost);
    
    // Self illum
    if (params.bSelfIllum >= 0)
        pSkinKV->SetInt("$selfillum", params.bSelfIllum);
    
    if (params.flSelfIllumTint[0] >= 0.0f)
    {
        char szValue[128];
        Q_snprintf(szValue, sizeof(szValue), "[%.2f %.2f %.2f]",
                   params.flSelfIllumTint[0],
                   params.flSelfIllumTint[1],
                   params.flSelfIllumTint[2]);
        pSkinKV->SetString("$selfillumtint", szValue);
    }
    
    // Color
    if (params.flColor[0] >= 0.0f)
    {
        char szValue[128];
        Q_snprintf(szValue, sizeof(szValue), "[%.2f %.2f %.2f]",
                   params.flColor[0],
                   params.flColor[1],
                   params.flColor[2]);
        pSkinKV->SetString("$color", szValue);
    }
    
    if (params.flColor2[0] >= 0.0f)
    {
        char szValue[128];
        Q_snprintf(szValue, sizeof(szValue), "[%.2f %.2f %.2f]",
                   params.flColor2[0],
                   params.flColor2[1],
                   params.flColor2[2]);
        pSkinKV->SetString("$color2", szValue);
    }
    
    // Misc
    if (params.bNoCull >= 0)
        pSkinKV->SetInt("$nocull", params.bNoCull);
    
    if (params.bNoDecal >= 0)
        pSkinKV->SetInt("$nodecal", params.bNoDecal);
    
    if (params.bHalfLambert >= 0)
        pSkinKV->SetInt("$halflambert", params.bHalfLambert);
    
    // Сохраняем в файл
    DevMsg("[SkinDB] Saving to file: %s\n", pszFilePath);
    
    bool bSuccess = pKV->SaveToFile(filesystem, pszFilePath, "MOD");
    
    if (bSuccess)
    {
        DevMsg("[SkinDB] Successfully saved skin %d (%s) to %s\n", 
               skin.iPaintKit, skin.szName, pszFilePath);
    }
    else
    {
        Warning("[SkinDB] Failed to save to file: %s\n", pszFilePath);
    }
    
    pKV->deleteThis();
    
    return bSuccess;
}

//=============================================================================
// Генерация нового paint kit ID
//=============================================================================
int CCSkinDatabase::GenerateNewPaintKitID() const
{
    int maxID = 0;
    
    FOR_EACH_MAP_FAST(m_SkinDefinitions, i)
    {
        if (m_SkinDefinitions[i].iPaintKit > maxID)
            maxID = m_SkinDefinitions[i].iPaintKit;
    }
    
    return maxID + 1;
}