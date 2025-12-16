#ifndef CS_SKIN_DATABASE_H
#define CS_SKIN_DATABASE_H
#ifdef _WIN32
#pragma once
#endif

#include "utlvector.h"
#include "utlmap.h"
#include "utlstring.h"
#include "cs_skin_shareddefs.h"
#include "cs_weapon_parse.h"  // Для CSWeaponID

class IMaterial;
class ITexture;
class KeyValues;

// =============================================================================
// SKIN DEFINITION (Client-side data)
// =============================================================================

struct SkinDefinition_t
{
    int             iPaintKit;              // Уникальный ID paint kit
    CSWeaponID      weaponID;               // К какому оружию относится
    char            szName[MAX_SKIN_NAME];  // Название скина (например, "Asiimov")
    char            szMaterialPath[256];    // Путь к материалу (VMT)
    char            szIconPath[256];        // Путь к иконке (VTF)
    ESkinRarity     rarity;                 // Редкость
    
    SkinDefinition_t()
    {
        iPaintKit = 0;
        weaponID = WEAPON_NONE;
        szName[0] = '\0';
        szMaterialPath[0] = '\0';
        szIconPath[0] = '\0';
        rarity = SKIN_RARITY_COMMON;
    }
    
    // Получить полное название
    const char* GetDisplayName() const
    {
        return szName;
    }
};

// =============================================================================
// SKIN DATABASE CLASS
// =============================================================================

class CCSkinDatabase
{
public:
    CCSkinDatabase();
    ~CCSkinDatabase();
    
    // Инициализация
    bool Initialize();
    bool LoadFromFile(const char* pszFilePath);
    void Shutdown();
    
    // Поиск скинов
    const SkinDefinition_t* FindSkinByPaintKit(int iPaintKit) const;
    const SkinDefinition_t* FindSkinByName(const char* pszName, CSWeaponID weaponID = WEAPON_NONE) const;
    
    // Получение списков
    void GetSkinsForWeapon(CSWeaponID weaponID, CUtlVector<const SkinDefinition_t*>& skins) const;
    void GetAllSkins(CUtlVector<const SkinDefinition_t*>& skins) const;
    int GetSkinCount() const { return m_SkinDefinitions.Count(); }
    
    // Работа с материалами
    IMaterial* GetSkinMaterial(int iPaintKit);
    ITexture* GetSkinIcon(int iPaintKit);
    
    // Очистка кэша
    void ClearMaterialCache();
    void ClearIconCache();
    
    // Debug
    void PrintAllSkins() const;
    void PrintSkinsForWeapon(CSWeaponID weaponID) const;
    
private:
    // Загрузка из KeyValues
    bool ParseSkinDefinition(KeyValues* pKV, SkinDefinition_t& def);
    
    // Управление материалами
    IMaterial* LoadMaterial(const SkinDefinition_t* pDef);
    ITexture* LoadIcon(const SkinDefinition_t* pDef);
    void ReleaseMaterial(int iPaintKit);
    void ReleaseIcon(int iPaintKit);
    
    // Данные
    CUtlMap<int, SkinDefinition_t>      m_SkinDefinitions;  // PaintKit -> Definition
    CUtlMap<int, IMaterial*>            m_MaterialCache;    // PaintKit -> Material
    CUtlMap<int, ITexture*>             m_IconCache;        // PaintKit -> Icon
    
    // Индекс скинов по оружию (для быстрого поиска)
    struct WeaponSkinList_t
    {
        int weaponID;
        CUtlVector<int> paintKits;  // Динамический список paint kit ID
        
        WeaponSkinList_t()
        {
            weaponID = 0;
        }
        
        WeaponSkinList_t(const WeaponSkinList_t& other)
        {
            weaponID = other.weaponID;
            paintKits.CopyArray(other.paintKits.Base(), other.paintKits.Count());
        }
        
        WeaponSkinList_t& operator=(const WeaponSkinList_t& other)
        {
            weaponID = other.weaponID;
            paintKits.CopyArray(other.paintKits.Base(), other.paintKits.Count());
            return *this;
        }
        
        void AddPaintKit(int iPaintKit)
        {
            // Проверка на дубликат
            if (paintKits.Find(iPaintKit) == paintKits.InvalidIndex())
            {
                paintKits.AddToTail(iPaintKit);
            }
        }
        
        int GetCount() const { return paintKits.Count(); }
        int GetPaintKit(int index) const { return paintKits[index]; }
    };
    CUtlVector<WeaponSkinList_t> m_WeaponSkins;
    
    bool m_bInitialized;
};

// Global instance
extern CCSkinDatabase g_SkinDatabase;

#endif // CS_SKIN_DATABASE_H