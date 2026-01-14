#include "custom_material.h"
#include "keyvalues.h"
#include "materialsystem/imaterialsystem.h"
#include "tier0/dbg.h"

ICustomMaterial* CCustomWeaponVisualsDataProcessor::GetCustomMaterial(const char* szBaseMaterial, int nMaterialIndex)
{
    KeyValues* pVMTKeyValues = new KeyValues("VertexLitGeneric");
    pVMTKeyValues->SetString("$basetexture", szBaseMaterial);

    CCustomMaterial* pMaterial = new CCustomMaterial(pVMTKeyValues);

    // В SDK 2013 композитные текстуры нет, поэтому можно пропустить
    pMaterial->Finalize();

    return pMaterial;
}