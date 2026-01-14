#ifndef GLOVE_SKIN_PROCESSOR_H
#define GLOVE_SKIN_PROCESSOR_H
#ifdef _WIN32
#pragma once
#endif

#include "utlvector.h"
#include "cs_skin_shareddefs.h"

class IMaterial;
class C_BaseAnimating;

// =============================================================================
// GLOVE SKIN CACHE STRUCTURE
// =============================================================================

struct GloveSkinCache_t
{
    int             iGloveID;           // ID перчаток (1-19)
    IMaterial*      pMaterial;          // Cached material
    int             iLastPaintKit;      // Last loaded paint kit
    
    GloveSkinCache_t()
        : iGloveID(0)
        , pMaterial(nullptr)
        , iLastPaintKit(0)
    {
    }
    
    GloveSkinCache_t(int gloveID)
        : iGloveID(gloveID)
        , pMaterial(nullptr)
        , iLastPaintKit(0)
    {
    }
};

// =============================================================================
// GLOVE SKIN PROCESSOR CLASS
// =============================================================================

class CGloveSkinProcessor
{
public:
    CGloveSkinProcessor();
    ~CGloveSkinProcessor();
    
    // Инициализация
    bool Initialize();
    void Shutdown();
    
    // Применение скинов к viewmodel перчаток (всегда slot 0)
    void ApplyGloveSkin(C_BaseAnimating* pGloveModel, int iGloveID, int iPaintKit);
    void RemoveGloveSkin(C_BaseAnimating* pGloveModel);
    
    // Получение материала по glove ID и paint kit
    IMaterial* GetGloveSkinMaterial(int iGloveID, int iPaintKit);
    
    // Utility
    void Clear();
    void ClearCache();
    
private:
    // Загрузка и управление материалами
    IMaterial* LoadGloveMaterial(int iGloveID, int iPaintKit);
    void ReleaseMaterial(GloveSkinCache_t* pCache);
    
    // Поиск
    GloveSkinCache_t* FindCacheEntry(int iGloveID);
    
    // Data members
    CUtlVector<GloveSkinCache_t>        m_GloveCache;
    
    bool                                m_bInitialized;
};

// Global instance
extern CGloveSkinProcessor g_GloveSkinProcessor;

#endif // GLOVE_SKIN_PROCESSOR_H
