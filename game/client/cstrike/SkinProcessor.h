
#pragma once
#include "cbase.h"
#include "tier1/utlvector.h"
#include "tier1/utlstring.h"

class C_BaseCombatWeapon;
class IMaterial;
class ConVar;

struct WeaponSkin_t
{
    const char* pszWeaponClass;      
    ConVar* pSkinConVar;             
    CUtlString sLastSkin;            
    IMaterial* pMaterial;            

    WeaponSkin_t()
        : pszWeaponClass(nullptr), pSkinConVar(nullptr), pMaterial(nullptr)
    {}

    WeaponSkin_t(const char* weaponClass, ConVar* cvar)
        : pszWeaponClass(weaponClass), pSkinConVar(cvar), pMaterial(nullptr)
    {}
};

class CSkinProcessor
{
public:
    CSkinProcessor();
    ~CSkinProcessor();

    IMaterial* GetSkinMaterial(C_BaseCombatWeapon* pWeapon);

    IMaterial* GetWeaponMaterial(const char* pszWeaponClass);

    void Clear();

    IMaterial* LoadMaterial(WeaponSkin_t& ws, const char* pszPath);
    WeaponSkin_t* FindSkinEntry(const char* pszClass);

    CUtlVector<WeaponSkin_t> m_WeaponSkins;
};

extern CSkinProcessor g_SkinProcessor;