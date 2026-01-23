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
    int             iPaintKit;
    CSWeaponID      weaponID;
    char            szName[MAX_SKIN_NAME];
    char            szIconPath[256];
    ESkinRarity     rarity;
    
    // Patch system
    char            szBaseMaterial[256];    // Путь к базовому VMT (include)
    
    // VMT параметры для override/insert
    struct VMTParams_t
    {
        // ========================================
        // CORE PARAMETERS
        // ========================================
        char szShader[64];              // Shader name (обычно не override, берется из base)
        
        // ========================================
        // TEXTURE PARAMETERS
        // ========================================
        char szBaseTexture[256];        // $basetexture
        char szBaseTexture2[256];       // $basetexture2 (для blend)
        char szBumpMap[256];            // $bumpmap (normal map)
        char szBumpMap2[256];           // $bumpmap2
        char szDetailTexture[256];      // $detail
        char szEnvMap[256];             // $envmap
        char szPhongExponentTexture[256]; // $phongexponenttexture
        char szPhongWarpTexture[256];   // $phongwarptexture
        
        // ========================================
        // PHONG LIGHTING
        // ========================================
        int   bPhong;                   // $phong (0/1/-1 = not set)
        float flPhongBoost;             // $phongboost
        float flPhongFresnelRanges[3];  // $phongfresnelranges "[min mid max]"
        float flPhongAlbedoTint;        // $phongalbedotint
        int   bPhongAlbedoBoost;        // $phongalbedoboost
        float flPhongExponent;          // $phongexponent
        
        // ========================================
        // ENVIRONMENT MAPPING
        // ========================================
        float flEnvMapTint[3];          // $envmaptint "[r g b]"
        float flEnvMapSaturation;       // $envmapsaturation
        float flEnvMapContrast;         // $envmapcontrast
        float flEnvMapFresnel;          // $envmapfresnel
        float flFresnelReflection;      // $fresnelreflection
        
        // ========================================
        // ALPHA & TRANSPARENCY
        // ========================================
        int   bAlphaTest;               // $alphatest
        float flAlphaTestReference;     // $alphatestreference
        int   bTranslucent;             // $translucent
        int   bAdditive;                // $additive
        
        // ========================================
        // TEXTURE MODIFIERS
        // ========================================
        float flBaseTextureTransform[16]; // $basetexturetransform (matrix)
        float flBumpTransform[16];        // $bumptransform
        int   bBaseTextureNoEnvMap;     // $basemapalphaphongmask
        int   bNormalMapAlphaPhongMask; // $normalmapalphaenvmapmask
        int   bBaseAlphaEnvMapMask;     // $basealphaenvmapmask
        
        // ========================================
        // DETAIL TEXTURE
        // ========================================
        float flDetailScale;            // $detailscale
        int   iDetailBlendMode;         // $detailblendmode
        float flDetailBlendFactor;      // $detailblendfactor
        
        // ========================================
        // RIM LIGHTING
        // ========================================
        int   bRimLight;                // $rimlight
        float flRimLightExponent;       // $rimlightexponent
        float flRimLightBoost;          // $rimlightboost
        
        // ========================================
        // SELF ILLUMINATION
        // ========================================
        int   bSelfIllum;               // $selfillum
        float flSelfIllumTint[3];       // $selfillumtint "[r g b]"
        float flSelfIllumFresnelMinMaxExp[3]; // $selfillum_envmapmask_alpha
        
        // ========================================
        // COLOR MODULATION
        // ========================================
        float flColor[3];               // $color "[r g b]"
        float flColor2[3];              // $color2
        
        // ========================================
        // MISC RENDERING
        // ========================================
        int   bNoCull;                  // $nocull
        int   bNoDecal;                 // $nodecal
        int   bHalfLambert;             // $halflambert
        
        // ========================================
        // CUSTOM CS:GO SKIN PARAMETERS
        // ========================================
        float flWear;                   // $wear (custom)
        int   iSeed;                    // $seed (custom)
        float flPatternScale;           // $patternscale (custom)
        float flPatternRotate;          // $patternrotate (custom)
        
        VMTParams_t()
        {
            // Core
            Q_strncpy(szShader, "VertexLitGeneric", sizeof(szShader));
            
            // Textures - все пустые по умолчанию
            szBaseTexture[0] = '\0';
            szBaseTexture2[0] = '\0';
            szBumpMap[0] = '\0';
            szBumpMap2[0] = '\0';
            szDetailTexture[0] = '\0';
            szEnvMap[0] = '\0';
            szPhongExponentTexture[0] = '\0';
            szPhongWarpTexture[0] = '\0';
            
            // Phong - используем -1 для "not set"
            bPhong = -1;
            flPhongBoost = -1.0f;
            flPhongFresnelRanges[0] = -1.0f;
            flPhongFresnelRanges[1] = -1.0f;
            flPhongFresnelRanges[2] = -1.0f;
            flPhongAlbedoTint = -1.0f;
            bPhongAlbedoBoost = -1;
            flPhongExponent = -1.0f;
            
            // EnvMap
            flEnvMapTint[0] = -1.0f;
            flEnvMapTint[1] = -1.0f;
            flEnvMapTint[2] = -1.0f;
            flEnvMapSaturation = -1.0f;
            flEnvMapContrast = -1.0f;
            flEnvMapFresnel = -1.0f;
            flFresnelReflection = -1.0f;
            
            // Alpha
            bAlphaTest = -1;
            flAlphaTestReference = -1.0f;
            bTranslucent = -1;
            bAdditive = -1;
            
            // Texture modifiers
            for (int i = 0; i < 16; i++)
            {
                flBaseTextureTransform[i] = 0.0f;
                flBumpTransform[i] = 0.0f;
            }
            bBaseTextureNoEnvMap = -1;
            bNormalMapAlphaPhongMask = -1;
            bBaseAlphaEnvMapMask = -1;
            
            // Detail
            flDetailScale = -1.0f;
            iDetailBlendMode = -1;
            flDetailBlendFactor = -1.0f;
            
            // Rim
            bRimLight = -1;
            flRimLightExponent = -1.0f;
            flRimLightBoost = -1.0f;
            
            // Self illum
            bSelfIllum = -1;
            flSelfIllumTint[0] = -1.0f;
            flSelfIllumTint[1] = -1.0f;
            flSelfIllumTint[2] = -1.0f;
            flSelfIllumFresnelMinMaxExp[0] = -1.0f;
            flSelfIllumFresnelMinMaxExp[1] = -1.0f;
            flSelfIllumFresnelMinMaxExp[2] = -1.0f;
            
            // Color
            flColor[0] = -1.0f;
            flColor[1] = -1.0f;
            flColor[2] = -1.0f;
            flColor2[0] = -1.0f;
            flColor2[1] = -1.0f;
            flColor2[2] = -1.0f;
            
            // Misc
            bNoCull = -1;
            bNoDecal = -1;
            bHalfLambert = -1;
            
            // Custom
            flWear = -1.0f;
            iSeed = -1;
            flPatternScale = -1.0f;
            flPatternRotate = -1.0f;
        }
    };
    
    VMTParams_t vmtParams;
    
    SkinDefinition_t()
    {
        iPaintKit = 0;
        weaponID = WEAPON_NONE;
        szName[0] = '\0';
        szBaseMaterial[0] = '\0';
        szIconPath[0] = '\0';
        rarity = SKIN_RARITY_COMMON;
    }
    
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
    
    bool SaveSkinToFile(const SkinDefinition_t& skin, const char* pszFilePath = "scripts/skins.txt");
    
    // Генерация нового paint kit ID
    int GenerateNewPaintKitID() const;
    
    KeyValues* CloneBaseMaterialKeyValues(IMaterial* pBaseMat);
    void ApplyVMTPatch(KeyValues* pKV, const SkinDefinition_t::VMTParams_t& params);
    
private:
    // Загрузка из KeyValues
    bool ParseSkinDefinition(KeyValues* pKV, SkinDefinition_t& def);
    
    // Управление материалами
    IMaterial* LoadMaterial(const SkinDefinition_t* pDef);
    ITexture* LoadIcon(const SkinDefinition_t* pDef);
    void ReleaseMaterial(int iPaintKit);
    void ReleaseIcon(int iPaintKit);
    void ParseVMTParams(KeyValues* pKV, SkinDefinition_t::VMTParams_t& params);
    
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