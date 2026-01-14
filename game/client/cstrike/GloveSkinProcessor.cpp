#include "cbase.h"
#include "GloveSkinProcessor.h"
#include "cs_skin_database.h"
#include "c_baseanimating.h"
#include "materialsystem/imaterialsystem.h"
#include "materialsystem/imaterial.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Global instance
CGloveSkinProcessor g_GloveSkinProcessor;

// =============================================================================
// CONSTRUCTOR / DESTRUCTOR
// =============================================================================

CGloveSkinProcessor::CGloveSkinProcessor()
    : m_bInitialized(false)
{
}

CGloveSkinProcessor::~CGloveSkinProcessor()
{
    Shutdown();
}

// =============================================================================
// INITIALIZATION
// =============================================================================

bool CGloveSkinProcessor::Initialize()
{
    if (m_bInitialized)
        return true;
        
    DevMsg("[GloveSkinProcessor] Initializing...\n");
    
    m_bInitialized = true;
    DevMsg("[GloveSkinProcessor] Initialized successfully\n");
    
    return true;
}

void CGloveSkinProcessor::Shutdown()
{
    if (!m_bInitialized)
        return;
        
    DevMsg("[GloveSkinProcessor] Shutting down...\n");
    
    Clear();
    m_bInitialized = false;
}

// =============================================================================
// MAIN SKIN APPLICATION
// =============================================================================

void CGloveSkinProcessor::ApplyGloveSkin(C_BaseAnimating* pGloveModel, int iGloveID, int iPaintKit)
{
    if (!pGloveModel || !m_bInitialized)
        return;
        
    if (iPaintKit <= 0)
    {
        RemoveGloveSkin(pGloveModel);
        return;
    }
    
    // Получаем или загружаем материал
    IMaterial* pMaterial = GetGloveSkinMaterial(iGloveID, iPaintKit);
    if (!pMaterial)
    {
        Warning("[GloveSkinProcessor] Failed to load material for glove %d, paintkit %d\n", iGloveID, iPaintKit);
        return;
    }
    
    // Применяем материал к слоту 0 (HARDCODED - всегда первый слот)
    pGloveModel->SetModelMaterialOverride( pMaterial);
    
    DevMsg("[GloveSkinProcessor] Applied paintkit %d to glove %d (slot 0)\n", iPaintKit, iGloveID);
}

void CGloveSkinProcessor::RemoveGloveSkin(C_BaseAnimating* pGloveModel)
{
    if (!pGloveModel)
        return;
        
    // Убираем override со слота 0
    pGloveModel->SetModelMaterialOverride( nullptr);
    
    DevMsg("[GloveSkinProcessor] Removed skin (slot 0)\n");
}

// =============================================================================
// MATERIAL RETRIEVAL
// =============================================================================

IMaterial* CGloveSkinProcessor::GetGloveSkinMaterial(int iGloveID, int iPaintKit)
{
    if (!m_bInitialized || iPaintKit <= 0 || iGloveID <= 0)
        return nullptr;
        
    // Проверяем кэш
    GloveSkinCache_t* pCache = FindCacheEntry(iGloveID);
    
    if (!pCache)
    {
        // Создаём новую запись в кэше
        GloveSkinCache_t newCache(iGloveID);
        m_GloveCache.AddToTail(newCache);
        pCache = &m_GloveCache[m_GloveCache.Count() - 1];
    }
    
    // Проверяем нужно ли перезагрузить материал
    if (iPaintKit != pCache->iLastPaintKit || !pCache->pMaterial)
    {
        ReleaseMaterial(pCache);
        pCache->pMaterial = LoadGloveMaterial(iGloveID, iPaintKit);
        pCache->iLastPaintKit = iPaintKit;
    }
    
    return pCache->pMaterial;
}

// =============================================================================
// MATERIAL LOADING
// =============================================================================

IMaterial* CGloveSkinProcessor::LoadGloveMaterial(int iGloveID, int iPaintKit)
{
    if (iPaintKit <= 0 || iGloveID <= 0)
        return nullptr;
    
    // Загружаем из базы данных
    const SkinDefinition_t* pSkinDef = g_SkinDatabase.FindSkinByPaintKit(iPaintKit);
    
    if (pSkinDef && pSkinDef->szMaterialPath[0])
    {
        IMaterial* pMat = materials->FindMaterial(
            pSkinDef->szMaterialPath, 
            TEXTURE_GROUP_MODEL
        );
        
        if (pMat && !pMat->IsErrorMaterial())
        {
            DevMsg("[GloveSkinProcessor] Loaded from database: %s (%s)\n", 
                pSkinDef->szName, pSkinDef->szMaterialPath);
            return pMat;
        }
    }
    
    Warning("[GloveSkinProcessor] Failed to load material for glove %d, paintkit %d\n", iGloveID, iPaintKit);
    return nullptr;
}

void CGloveSkinProcessor::ReleaseMaterial(GloveSkinCache_t* pCache)
{
    if (!pCache)
        return;
        
    // Материалы управляются material system, просто очищаем указатель
    pCache->pMaterial = nullptr;
}

// =============================================================================
// LOOKUP FUNCTIONS
// =============================================================================

GloveSkinCache_t* CGloveSkinProcessor::FindCacheEntry(int iGloveID)
{
    FOR_EACH_VEC(m_GloveCache, i)
    {
        if (m_GloveCache[i].iGloveID == iGloveID)
            return &m_GloveCache[i];
    }
    
    return nullptr;
}

// =============================================================================
// UTILITY
// =============================================================================

void CGloveSkinProcessor::Clear()
{
    FOR_EACH_VEC(m_GloveCache, i)
    {
        ReleaseMaterial(&m_GloveCache[i]);
    }
    
    m_GloveCache.Purge();
}

void CGloveSkinProcessor::ClearCache()
{
    Clear();
}
