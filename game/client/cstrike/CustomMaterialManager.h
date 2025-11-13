#pragma once
#include "cbase.h"
#include "utlvector.h"

// ======================
// 
// ======================
struct FPaintableMaterial
{
    char szVMTPath[256];   
    int  nBaseTextureSize;
    float uvScale;
};

// ======================
// 
// ======================
struct FWeaponData
{
    char szWeaponName[64];          
    char szViewModel[128];          
    char szPlayerModel[128];        
    CUtlVector<FPaintableMaterial> Materials; 
};