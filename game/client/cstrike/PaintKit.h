#include "cbase.h"
#include "ivisualsdataprocessor.h"
#include "KeyValues.h"
#include "filesystem.h"
#include "materialsystem/IMaterial.h"
#include "utlbuffer.h"

// -------------------------------------------------------
//  Твой фейковый PaintKit (можно оставить без изменений)
// -------------------------------------------------------

struct FakedPaintKit
{
    char sPattern[128] = "";
    Vector rgbaColor[4] = { Vector(255,255,255), Vector(255,255,255), Vector(255,255,255), Vector(255,255,255) };
    int nStyle = 0;
    float flPatternScale = 1.0f;
    float flPatternOffsetXStart = 0.0f;
    float flPatternOffsetYStart = 0.0f;
    float flPatternRotateStart = 0.0f;
    unsigned char uchPhongExponent = 20;
    unsigned char uchPhongAlbedoBoost = 2;
    unsigned char uchPhongIntensity = 50;
    bool bIgnoreWeaponSizeScale = false;
    float flWeaponLength = 36.0f;
    float flUVScale = 1.0f;
    bool bBaseTextureOverride = false;
};

// -------------------------------------------------------
//  Минимальная реализация IVisualsDataCompare
// -------------------------------------------------------

class CFakeVisualsDataCompare : public IVisualsDataCompare
{
    CUtlBuffer m_Blob;
public:
    void FillCompareBlob() override { m_Blob.Clear(); }
    const CUtlBuffer& GetCompareBlob() const override { return m_Blob; }
    bool Compare(const CUtlBuffer& pOther) override { return false; }
};

// -------------------------------------------------------
//  Реализация IVisualsDataProcessor
// -------------------------------------------------------

class CFakeVisualsDataProcessor : public IVisualsDataProcessor
{
public:
    CFakeVisualsDataProcessor(const FakedPaintKit& kit)
        : m_PaintKit(kit)
    {
        m_pCompare = new CFakeVisualsDataCompare();
    }

    ~CFakeVisualsDataProcessor() override
    {
        if (m_pCompare)
            m_pCompare->Release();
    }

    KeyValues* GenerateCustomMaterialKeyValues() override
    {
        KeyValues* pKV = new KeyValues("VertexLitGeneric");
        pKV->SetString("$basetexture", m_PaintKit.sPattern);
        pKV->SetInt("$model", 1);
        pKV->SetInt("$phong", 1);
        pKV->SetFloat("$phongexponent", (float)m_PaintKit.uchPhongExponent);
        pKV->SetFloat("$phongboost", (float)m_PaintKit.uchPhongAlbedoBoost / 10.0f);
        pKV->SetFloat("$phongfresnelranges", 0.3f);
        return pKV;
    }

    KeyValues* GenerateCompositeMaterialKeyValues(int nMaterialParamId) override
    {
        return nullptr;
    }

    IVisualsDataCompare* GetCompareObject() override { return m_pCompare; }

    bool HasCustomMaterial() const override { return true; }

    const char* GetOriginalMaterialName() const override { return m_PaintKit.sPattern; }
    const char* GetOriginalMaterialBaseName() const override { return m_PaintKit.sPattern; }
    const char* GetPatternVTFName() const override { return m_PaintKit.sPattern; }

    void Refresh() override {}

private:
    FakedPaintKit m_PaintKit;
    CFakeVisualsDataCompare* m_pCompare;
};