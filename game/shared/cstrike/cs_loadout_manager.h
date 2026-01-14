// server/cstrike/cs_loadout_manager.h
// ВАЖНО: Этот файл должен быть ТОЛЬКО на сервере!

#ifndef CS_LOADOUT_MANAGER_H
#define CS_LOADOUT_MANAGER_H
#ifdef _WIN32
#pragma once
#endif

#include "utlvector.h"
#include "cs_weapon_parse.h"

class CCSPlayer;  // Forward declaration - серверный класс
class KeyValues;

// =============================================================================
// PLAYER LOADOUT STRUCTURE
// =============================================================================

struct PlayerLoadout_t
{
    int iUserID;
    
    // Weapon paint kits
    struct WeaponSkin_t
    {
        CSWeaponID weaponID;
        int iPaintKit;
        
        WeaponSkin_t() : weaponID(WEAPON_NONE), iPaintKit(0) {}
        WeaponSkin_t(CSWeaponID wep, int paint) : weaponID(wep), iPaintKit(paint) {}
    };
    
    // Используем массив указателей чтобы избежать проблем с copy constructor
    WeaponSkin_t* pWeapons;
    int iWeaponCount;
    int iWeaponCapacity;
    
    // Knife & Gloves
    int iKnifePaintKit;
    char szGloveMaterial[256];
    
    PlayerLoadout_t()
    {
        iUserID = 0;
        pWeapons = nullptr;
        iWeaponCount = 0;
        iWeaponCapacity = 0;
        iKnifePaintKit = 0;
        szGloveMaterial[0] = '\0';
    }
    
    ~PlayerLoadout_t()
    {
        if (pWeapons)
        {
            delete[] pWeapons;
            pWeapons = nullptr;
        }
    }
    
    // Получить paint kit для оружия
    int GetWeaponPaintKit(CSWeaponID weaponID) const
    {
        for (int i = 0; i < iWeaponCount; i++)
        {
            if (pWeapons[i].weaponID == weaponID)
                return pWeapons[i].iPaintKit;
        }
        return 0;
    }
    
    // Установить paint kit для оружия
    void SetWeaponPaintKit(CSWeaponID weaponID, int iPaintKit)
    {
        // Ищем существующий
        for (int i = 0; i < iWeaponCount; i++)
        {
            if (pWeapons[i].weaponID == weaponID)
            {
                pWeapons[i].iPaintKit = iPaintKit;
                return;
            }
        }
        
        // Добавляем новый
        if (iWeaponCount >= iWeaponCapacity)
        {
            // Увеличиваем capacity
            int newCapacity = (iWeaponCapacity == 0) ? 16 : (iWeaponCapacity * 2);
            WeaponSkin_t* pNewArray = new WeaponSkin_t[newCapacity];
            
            // Копируем существующие
            for (int i = 0; i < iWeaponCount; i++)
            {
                pNewArray[i] = pWeapons[i];
            }
            
            // Освобождаем старый
            if (pWeapons)
                delete[] pWeapons;
            
            pWeapons = pNewArray;
            iWeaponCapacity = newCapacity;
        }
        
        pWeapons[iWeaponCount].weaponID = weaponID;
        pWeapons[iWeaponCount].iPaintKit = iPaintKit;
        iWeaponCount++;
    }
    
private:
    // Запретить копирование
    PlayerLoadout_t(const PlayerLoadout_t&);
    PlayerLoadout_t& operator=(const PlayerLoadout_t&);
};

// =============================================================================
// LOADOUT MANAGER CLASS
// =============================================================================

class CCSLoadoutManager
{
public:
    CCSLoadoutManager();
    ~CCSLoadoutManager();
    
    // Initialization
    void Initialize();
    void Shutdown();
    
    // Player management
    void LoadPlayerLoadout(CCSPlayer* pPlayer);
    void ClearPlayerLoadout(int iUserID);
    void ClearAllLoadouts();
    
    // Weapon skins
    int GetWeaponPaintKit(CCSPlayer* pPlayer, CSWeaponID weaponID);
    void SetWeaponPaintKit(CCSPlayer* pPlayer, CSWeaponID weaponID, int iPaintKit);
    
    // Knife & Gloves
    int GetKnifePaintKit(CCSPlayer* pPlayer);
    const char* GetGloveMaterial(CCSPlayer* pPlayer);
    
    // Loading/Saving
    bool LoadFromConVars(CCSPlayer* pPlayer);
    bool LoadFromFile(CCSPlayer* pPlayer, const char* pszFilePath);
    bool SaveToFile(CCSPlayer* pPlayer, const char* pszFilePath);
    
private:
    // Helpers
    PlayerLoadout_t* FindOrCreateLoadout(int iUserID);
    PlayerLoadout_t* FindLoadout(int iUserID);
    
    // Data
    PlayerLoadout_t* m_pPlayerLoadouts;
    int m_iLoadoutCount;
    int m_iLoadoutCapacity;
    bool m_bInitialized;
};

// Global instance
extern CCSLoadoutManager g_CSLoadoutManager;

#endif // CS_LOADOUT_MANAGER_H