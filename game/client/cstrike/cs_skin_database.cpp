#include "cbase.h"
#include "cs_skin_database.h"
#include "materialsystem/imaterialsystem.h"
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
    
    // Paint Kit ID - это имя ключа
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
    
    // Путь к материалу (ОБЯЗАТЕЛЬНОЕ)
    const char* pszMaterial = pKV->GetString("material", nullptr);
    if (!pszMaterial || !pszMaterial[0])
    {
        Warning("[SkinDB] Skin %d (%s) has no material\n", def.iPaintKit, def.szName);
        return false;
    }
    Q_strncpy(def.szMaterialPath, pszMaterial, sizeof(def.szMaterialPath));
    
    // Путь к иконке (ОПЦИОНАЛЬНОЕ)
    const char* pszIcon = pKV->GetString("icon", "");
    Q_strncpy(def.szIconPath, pszIcon, sizeof(def.szIconPath));
    
    // Редкость (ОПЦИОНАЛЬНОЕ, по умолчанию Common)
    def.rarity = (ESkinRarity)pKV->GetInt("rarity", SKIN_RARITY_COMMON);
    
    // Проверяем валидность редкости
    if (def.rarity < 0 || def.rarity >= SKIN_RARITY_COUNT)
    {
        Warning("[SkinDB] Skin %d (%s) has invalid rarity: %d, setting to Common\n", 
                def.iPaintKit, def.szName, (int)def.rarity);
        def.rarity = SKIN_RARITY_COMMON;
    }
    
    return true;
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
    if (!pDef || !pDef->szMaterialPath[0])
        return nullptr;
    
    if (!materials)
    {
        Warning("[SkinDB] Materials system is NULL!\n");
        return nullptr;
    }
    
    // Загружаем материал
    IMaterial* pMat = materials->FindMaterial(pDef->szMaterialPath, TEXTURE_GROUP_MODEL, true);
    
    if (!pMat || IsErrorMaterial(pMat))
    {
        Warning("[SkinDB] Failed to load material: %s (paint kit %d)\n", 
                pDef->szMaterialPath, pDef->iPaintKit);
        return nullptr;
    }
    
    // Увеличиваем reference count
    pMat->IncrementReferenceCount();
    
    // Precache если нужно
    if (!pMat->IsPrecached())
    {
        MaterialLock_t hLock = materials->Lock();
        pMat->Refresh();
        materials->Unlock(hLock);
    }
    
    DevMsg("[SkinDB] Loaded material for paint kit %d (%s): %s\n", 
           pDef->iPaintKit, pDef->szName, pDef->szMaterialPath);
    
    return pMat;
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