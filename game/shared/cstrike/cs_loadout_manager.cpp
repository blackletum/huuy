// server/cstrike/cs_loadout_manager.cpp

#include "cbase.h"
#include "cs_loadout_manager.h"
#include "filesystem.h"
#include "KeyValues.h"

#ifdef CLIENT_DLL
#include "c_cs_player.h"
#else
#include "cs_player.h"
#endif

// Global instance
CCSLoadoutManager g_CSLoadoutManager;

// =============================================================================
// CONSTRUCTOR / DESTRUCTOR
// =============================================================================

CCSLoadoutManager::CCSLoadoutManager() 
    : m_bInitialized(false)
    , m_pPlayerLoadouts(nullptr)
    , m_iLoadoutCount(0)
    , m_iLoadoutCapacity(0)
{
}

CCSLoadoutManager::~CCSLoadoutManager()
{
    Shutdown();
}

// =============================================================================
// INITIALIZATION
// =============================================================================

void CCSLoadoutManager::Initialize()
{
    if (m_bInitialized)
        return;
    
    DevMsg("[LoadoutManager] Initializing...\n");
    
    // Выделяем начальную capacity
    m_iLoadoutCapacity = 32;
    m_pPlayerLoadouts = new PlayerLoadout_t[m_iLoadoutCapacity];
    m_iLoadoutCount = 0;
    
    m_bInitialized = true;
    
    DevMsg("[LoadoutManager] Initialized\n");
}

void CCSLoadoutManager::Shutdown()
{
    if (!m_bInitialized)
        return;
    
    DevMsg("[LoadoutManager] Shutting down...\n");
    
    ClearAllLoadouts();
    
    if (m_pPlayerLoadouts)
    {
        delete[] m_pPlayerLoadouts;
        m_pPlayerLoadouts = nullptr;
    }
    
    m_iLoadoutCount = 0;
    m_iLoadoutCapacity = 0;
    m_bInitialized = false;
}

// =============================================================================
// PLAYER MANAGEMENT
// =============================================================================

void CCSLoadoutManager::LoadPlayerLoadout(CCSPlayer* pPlayer)
{
    if (!pPlayer)
        return;
    
    int iUserID = pPlayer->GetUserID();
    DevMsg("[LoadoutManager] Loading loadout for user %d\n", iUserID);
    
    // Создаем loadout если его нет
    PlayerLoadout_t* pLoadout = FindOrCreateLoadout(iUserID);
    if (!pLoadout)
        return;
    
    // Загружаем из ConVar'ов игрока
    LoadFromConVars(pPlayer);
    
    // Можно добавить загрузку из файла или базы данных
    // LoadFromFile(pPlayer, "cfg/loadouts/...");
}

void CCSLoadoutManager::ClearPlayerLoadout(int iUserID)
{
    for (int i = 0; i < m_iLoadoutCount; i++)
    {
        if (m_pPlayerLoadouts[i].iUserID == iUserID)
        {
            // Сдвигаем все элементы после удаляемого
            for (int j = i; j < m_iLoadoutCount - 1; j++)
            {
                m_pPlayerLoadouts[j] = m_pPlayerLoadouts[j + 1];
            }
            m_iLoadoutCount--;
            DevMsg("[LoadoutManager] Cleared loadout for user %d\n", iUserID);
            return;
        }
    }
}

void CCSLoadoutManager::ClearAllLoadouts()
{
    m_iLoadoutCount = 0;
    DevMsg("[LoadoutManager] Cleared all loadouts\n");
}

// =============================================================================
// WEAPON SKINS
// =============================================================================

int CCSLoadoutManager::GetWeaponPaintKit(CCSPlayer* pPlayer, CSWeaponID weaponID)
{
    if (!pPlayer)
        return 0;
    
    PlayerLoadout_t* pLoadout = FindLoadout(pPlayer->GetUserID());
    if (!pLoadout)
        return 0;
    
    return pLoadout->GetWeaponPaintKit(weaponID);
}

void CCSLoadoutManager::SetWeaponPaintKit(CCSPlayer* pPlayer, CSWeaponID weaponID, int iPaintKit)
{
    if (!pPlayer)
        return;
    
    PlayerLoadout_t* pLoadout = FindOrCreateLoadout(pPlayer->GetUserID());
    if (!pLoadout)
        return;
    
    pLoadout->SetWeaponPaintKit(weaponID, iPaintKit);
}

// =============================================================================
// KNIFE & GLOVES
// =============================================================================

int CCSLoadoutManager::GetKnifePaintKit(CCSPlayer* pPlayer)
{
    if (!pPlayer)
        return 0;
    
    PlayerLoadout_t* pLoadout = FindLoadout(pPlayer->GetUserID());
    if (!pLoadout)
        return 0;
    
    return pLoadout->iKnifePaintKit;
}

const char* CCSLoadoutManager::GetGloveMaterial(CCSPlayer* pPlayer)
{
    if (!pPlayer)
        return "";
    
    PlayerLoadout_t* pLoadout = FindLoadout(pPlayer->GetUserID());
    if (!pLoadout)
        return "";
    
    return pLoadout->szGloveMaterial;
}

// =============================================================================
// LOADING FROM CONVARS
// =============================================================================

bool CCSLoadoutManager::LoadFromConVars(CCSPlayer* pPlayer)
{
    if (!pPlayer)
        return false;
    
    PlayerLoadout_t* pLoadout = FindOrCreateLoadout(pPlayer->GetUserID());
    if (!pLoadout)
        return false;
    
    // Структура для маппинга ConVar -> WeaponID
    struct WeaponConVarMap_t
    {
        const char* convarName;
        CSWeaponID weaponID;
    };
    
    WeaponConVarMap_t weaponConVars[] = {
        { "loadout_ak47_skin", WEAPON_AK47 },
        { "loadout_awp_skin", WEAPON_AWP },
        { "loadout_m4a4_skin", WEAPON_M4A1 },
        { "loadout_deagle_skin", WEAPON_DEAGLE },
        { "loadout_glock_skin", WEAPON_GLOCK },
        { "loadout_famas_skin", WEAPON_FAMAS },
        { "loadout_galilar_skin", WEAPON_GALILAR },
        { "loadout_aug_skin", WEAPON_AUG },
        { "loadout_sg556_skin", WEAPON_SG556 },
        { "loadout_p90_skin", WEAPON_P90 },
        { "loadout_mp9_skin", WEAPON_MP9 },
        { "loadout_mac10_skin", WEAPON_MAC10 },
        { "loadout_ump45_skin", WEAPON_UMP45 },
        { "loadout_bizon_skin", WEAPON_BIZON },
        { "loadout_mp7_skin", WEAPON_MP7 },
        { "loadout_p250_skin", WEAPON_P250 },
        { "loadout_tec9_skin", WEAPON_TEC9 },
        { "loadout_fiveseven_skin", WEAPON_FIVESEVEN },
        { "loadout_elite_skin", WEAPON_ELITE },
        { "loadout_nova_skin", WEAPON_NOVA },
        { "loadout_xm1014_skin", WEAPON_XM1014 },
        { "loadout_mag7_skin", WEAPON_MAG7 },
        { "loadout_sawedoff_skin", WEAPON_SAWEDOFF },
        { "loadout_m249_skin", WEAPON_M249 },
        { "loadout_negev_skin", WEAPON_NEGEV },
        { "loadout_ssg08_skin", WEAPON_SSG08 },
        { "loadout_scar20_skin", WEAPON_SCAR20 },
        { "loadout_g3sg1_skin", WEAPON_G3SG1 },
        { "loadout_knife_skin", WEAPON_KNIFE },
    };
    
    int iNumWeapons = sizeof(weaponConVars) / sizeof(weaponConVars[0]);
    
    // Получаем ConVar'ы игрока и читаем paint kit ID
    for (int i = 0; i < iNumWeapons; i++)
    {
        ConVar* pConVar = cvar->FindVar(weaponConVars[i].convarName);
        if (pConVar)
        {
            int iPaintKit = pConVar->GetInt();
            if (iPaintKit > 0)
            {
                pLoadout->SetWeaponPaintKit(weaponConVars[i].weaponID, iPaintKit);
                DevMsg("[LoadoutManager] Loaded %s = %d for user %d\n", 
                       weaponConVars[i].convarName, iPaintKit, pPlayer->GetUserID());
            }
        }
    }
    
    // Ножи
    ConVar* pKnifeConVar = cvar->FindVar("loadout_knife_skin");
    if (pKnifeConVar)
    {
        pLoadout->iKnifePaintKit = pKnifeConVar->GetInt();
    }
    
    return true;
}

// =============================================================================
// FILE I/O (Optional)
// =============================================================================

bool CCSLoadoutManager::LoadFromFile(CCSPlayer* pPlayer, const char* pszFilePath)
{
    if (!pPlayer || !pszFilePath)
        return false;
    
    // TODO: Реализовать загрузку из файла
    // Формат: cfg/loadouts/STEAMID64.txt
    
    return false;
}

bool CCSLoadoutManager::SaveToFile(CCSPlayer* pPlayer, const char* pszFilePath)
{
    if (!pPlayer || !pszFilePath)
        return false;
    
    // TODO: Реализовать сохранение в файл
    
    return false;
}

// =============================================================================
// HELPERS
// =============================================================================

PlayerLoadout_t* CCSLoadoutManager::FindOrCreateLoadout(int iUserID)
{
    // Ищем существующий
    PlayerLoadout_t* pLoadout = FindLoadout(iUserID);
    if (pLoadout)
        return pLoadout;
    
    // Проверяем capacity
    if (m_iLoadoutCount >= m_iLoadoutCapacity)
    {
        // Увеличиваем capacity
        int newCapacity = m_iLoadoutCapacity * 2;
        PlayerLoadout_t* pNewArray = new PlayerLoadout_t[newCapacity];
        
        // Копируем существующие (нельзя использовать memcpy из-за деструкторов)
        for (int i = 0; i < m_iLoadoutCount; i++)
        {
            pNewArray[i].iUserID = m_pPlayerLoadouts[i].iUserID;
            pNewArray[i].iKnifePaintKit = m_pPlayerLoadouts[i].iKnifePaintKit;
            Q_strncpy(pNewArray[i].szGloveMaterial, m_pPlayerLoadouts[i].szGloveMaterial, sizeof(pNewArray[i].szGloveMaterial));
            
            // Копируем weapons
            for (int j = 0; j < m_pPlayerLoadouts[i].iWeaponCount; j++)
            {
                pNewArray[i].SetWeaponPaintKit(
                    m_pPlayerLoadouts[i].pWeapons[j].weaponID,
                    m_pPlayerLoadouts[i].pWeapons[j].iPaintKit
                );
            }
        }
        
        // Освобождаем старый
        delete[] m_pPlayerLoadouts;
        
        m_pPlayerLoadouts = pNewArray;
        m_iLoadoutCapacity = newCapacity;
    }
    
    // Создаем новый
    m_pPlayerLoadouts[m_iLoadoutCount].iUserID = iUserID;
    return &m_pPlayerLoadouts[m_iLoadoutCount++];
}

PlayerLoadout_t* CCSLoadoutManager::FindLoadout(int iUserID)
{
    for (int i = 0; i < m_iLoadoutCount; i++)
    {
        if (m_pPlayerLoadouts[i].iUserID == iUserID)
            return &m_pPlayerLoadouts[i];
    }
    
    return nullptr;
}