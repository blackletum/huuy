#include "cbase.h"
#include "SkinProcessor.h"
#include "materialsystem/imaterialsystem.h"
#include "materialsystem/imaterial.h"
#include "model_types.h"
#include "tier0/dbg.h"

CSkinProcessor g_SkinProcessor;

// -----------------------------------------------------

// Rifles
ConVar loadout_ak47_skin("loadout_ak47_skin", "", FCVAR_ARCHIVE, "AK-47 skin material path");
ConVar loadout_aug_skin("loadout_aug_skin", "", FCVAR_ARCHIVE, "AUG skin material path");
ConVar loadout_awp_skin("loadout_awp_skin", "", FCVAR_ARCHIVE, "AWP skin material path");
ConVar loadout_famas_skin("loadout_famas_skin", "", FCVAR_ARCHIVE, "FAMAS skin material path");
ConVar loadout_galilar_skin("loadout_galilar_skin", "", FCVAR_ARCHIVE, "Galil AR skin material path");
ConVar loadout_m4a1_silenser_skin("loadout_m4a1_silenser_skin", "", FCVAR_ARCHIVE, "M4A1-S skin material path");
ConVar loadout_m4a4_skin("loadout_m4a4_skin", "", FCVAR_ARCHIVE, "M4A4 skin material path");
ConVar loadout_sg556_skin("loadout_sg556_skin", "", FCVAR_ARCHIVE, "SG556 skin material path");

// SMG
ConVar loadout_bizon_skin("loadout_bizon_skin", "", FCVAR_ARCHIVE, "PP-Bizon skin material path");
ConVar loadout_mac10_skin("loadout_mac10_skin", "", FCVAR_ARCHIVE, "MAC-10 skin material path");
ConVar loadout_mp5sd_skin("loadout_mp5sd_skin", "", FCVAR_ARCHIVE, "MP5-SD skin material path");
ConVar loadout_mp9_skin("loadout_mp9_skin", "", FCVAR_ARCHIVE, "MP9 skin material path");
ConVar loadout_p90_skin("loadout_p90_skin", "", FCVAR_ARCHIVE, "P90 skin material path");
ConVar loadout_ump45_skin("loadout_ump45_skin", "", FCVAR_ARCHIVE, "UMP-45 skin material path");

// Heavy
ConVar loadout_m249_skin("loadout_m249_skin", "", FCVAR_ARCHIVE, "M249 skin material path");
ConVar loadout_negev_skin("loadout_negev_skin", "", FCVAR_ARCHIVE, "Negev skin material path");
ConVar loadout_mag7_skin("loadout_mag7_skin", "", FCVAR_ARCHIVE, "MAG-7 skin material path");
ConVar loadout_nova_skin("loadout_nova_skin", "", FCVAR_ARCHIVE, "Nova skin material path");
ConVar loadout_sawedoff_skin("loadout_sawedoff_skin", "", FCVAR_ARCHIVE, "Sawed-Off skin material path");
ConVar loadout_xm1014_skin("loadout_xm1014_skin", "", FCVAR_ARCHIVE, "XM1014 skin material path");

// Pistols
ConVar loadout_cz75a_skin("loadout_cz75a_skin", "", FCVAR_ARCHIVE, "CZ75-Auto skin material path");
ConVar loadout_deagle_skin("loadout_deagle_skin", "", FCVAR_ARCHIVE, "Desert Eagle skin material path");
ConVar loadout_elite_skin("loadout_elite_skin", "", FCVAR_ARCHIVE, "Dual Berettas skin material path");
ConVar loadout_fiveseven_skin("loadout_fiveseven_skin", "", FCVAR_ARCHIVE, "Five-SeveN skin material path");
ConVar loadout_glock_skin("loadout_glock_skin", "", FCVAR_ARCHIVE, "Glock-18 skin material path");
ConVar loadout_hkp2000_skin("loadout_hkp2000_skin", "", FCVAR_ARCHIVE, "P2000 skin material path");
ConVar loadout_p250_skin("loadout_p250_skin", "", FCVAR_ARCHIVE, "P250 skin material path");
ConVar loadout_revolver_skin("loadout_revolver_skin", "", FCVAR_ARCHIVE, "R8 Revolver skin material path");
ConVar loadout_tec9_skin("loadout_tec9_skin", "", FCVAR_ARCHIVE, "Tec-9 skin material path");
ConVar loadout_usp_silencer_skin("loadout_usp_silencer_skin", "", FCVAR_ARCHIVE, "USP-S skin material path");

// Snipers
ConVar loadout_g3sg1_skin("loadout_g3sg1_skin", "", FCVAR_ARCHIVE, "G3SG1 skin material path");
ConVar loadout_scar20_skin("loadout_scar20_skin", "", FCVAR_ARCHIVE, "SCAR-20 skin material path");
ConVar loadout_ssg08_skin("loadout_ssg08_skin", "", FCVAR_ARCHIVE, "SSG 08 skin material path");

// Knives
ConVar loadout_knife_bayonet_skin("loadout_knife_bayonet_skin", "", FCVAR_ARCHIVE, "Bayonet skin material path");
ConVar loadout_knife_butterfly_skin("loadout_knife_butterfly_skin", "", FCVAR_ARCHIVE, "Butterfly Knife skin material path");
ConVar loadout_knife_cains_skin("loadout_knife_cains_skin", "", FCVAR_ARCHIVE, "Cains Knife skin material path");
ConVar loadout_knife_cord_skin("loadout_knife_cord_skin", "", FCVAR_ARCHIVE, "Cord Knife skin material path");
ConVar loadout_knife_css_skin("loadout_knife_css_skin", "", FCVAR_ARCHIVE, "Classic Knife skin material path");
ConVar loadout_knife_falshion_skin("loadout_knife_falshion_skin", "", FCVAR_ARCHIVE, "Falchion Knife skin material path");
ConVar loadout_knife_flip_skin("loadout_knife_flip_skin", "", FCVAR_ARCHIVE, "Flip Knife skin material path");
ConVar loadout_knife_gut_skin("loadout_knife_gut_skin", "", FCVAR_ARCHIVE, "Gut Knife skin material path");
ConVar loadout_knife_gypsy_jackknife_skin("loadout_knife_gypsy_jackknife_skin", "", FCVAR_ARCHIVE, "Navaja Knife skin material path");
ConVar loadout_knife_karambit_skin("loadout_knife_karambit_skin", "", FCVAR_ARCHIVE, "Karambit skin material path");
ConVar loadout_knife_m9_bayonet_skin("loadout_knife_m9_bayonet_skin", "", FCVAR_ARCHIVE, "M9 Bayonet skin material path");
ConVar loadout_knife_outdoor_skin("loadout_knife_outdoor_skin", "", FCVAR_ARCHIVE, "Outdoor Knife skin material path");
ConVar loadout_knife_push_skin("loadout_knife_push_skin", "", FCVAR_ARCHIVE, "Shadow Daggers skin material path");
ConVar loadout_knife_skeleton_skin("loadout_knife_skeleton_skin", "", FCVAR_ARCHIVE, "Skeleton Knife skin material path");
ConVar loadout_knife_stiletto_skin("loadout_knife_stiletto_skin", "", FCVAR_ARCHIVE, "Stiletto Knife skin material path");
ConVar loadout_knife_survival_bowie_skin("loadout_knife_survival_bowie_skin", "", FCVAR_ARCHIVE, "Bowie Knife skin material path");
ConVar loadout_knife_tactical_skin("loadout_knife_tactical_skin", "", FCVAR_ARCHIVE, "Huntsman Knife skin material path");
ConVar loadout_knife_ursus_skin("loadout_knife_ursus_skin", "", FCVAR_ARCHIVE, "Ursus Knife skin material path");
ConVar loadout_knife_widowmaker_skin("loadout_knife_widowmaker_skin", "", FCVAR_ARCHIVE, "Widowmaker Knife skin material path");

CSkinProcessor::CSkinProcessor()
{
// Rifles
m_WeaponSkins.AddToTail(WeaponSkin_t("weapon_ak47", &loadout_ak47_skin));
m_WeaponSkins.AddToTail(WeaponSkin_t("weapon_awp", &loadout_awp_skin));
m_WeaponSkins.AddToTail(WeaponSkin_t("weapon_m4a1_silencer", &loadout_m4a1_silenser_skin));
m_WeaponSkins.AddToTail(WeaponSkin_t("weapon_m4a4", &loadout_m4a4_skin));
m_WeaponSkins.AddToTail(WeaponSkin_t("weapon_famas", &loadout_famas_skin));
m_WeaponSkins.AddToTail(WeaponSkin_t("weapon_galilar", &loadout_galilar_skin));
m_WeaponSkins.AddToTail(WeaponSkin_t("weapon_sg556", &loadout_sg556_skin));

// SMG  
m_WeaponSkins.AddToTail(WeaponSkin_t("weapon_mac10", &loadout_mac10_skin));  
m_WeaponSkins.AddToTail(WeaponSkin_t("weapon_mp9", &loadout_mp9_skin));  
m_WeaponSkins.AddToTail(WeaponSkin_t("weapon_ump45", &loadout_ump45_skin));  
m_WeaponSkins.AddToTail(WeaponSkin_t("weapon_p90", &loadout_p90_skin));  
m_WeaponSkins.AddToTail(WeaponSkin_t("weapon_bizon", &loadout_bizon_skin));  

// Pistols  
m_WeaponSkins.AddToTail(WeaponSkin_t("weapon_glock", &loadout_glock_skin));  
m_WeaponSkins.AddToTail(WeaponSkin_t("weapon_p250", &loadout_p250_skin));  
m_WeaponSkins.AddToTail(WeaponSkin_t("weapon_deagle", &loadout_deagle_skin));  
m_WeaponSkins.AddToTail(WeaponSkin_t("weapon_tec9", &loadout_tec9_skin));  
m_WeaponSkins.AddToTail(WeaponSkin_t("weapon_usp_silencer", &loadout_usp_silencer_skin));  

// Knives  
m_WeaponSkins.AddToTail(WeaponSkin_t("weapon_knife_karambit", &loadout_knife_karambit_skin));  
m_WeaponSkins.AddToTail(WeaponSkin_t("weapon_knife_bayonet", &loadout_knife_bayonet_skin));  
m_WeaponSkins.AddToTail(WeaponSkin_t("weapon_knife_m9_bayonet", &loadout_knife_m9_bayonet_skin));  
m_WeaponSkins.AddToTail(WeaponSkin_t("weapon_knife_butterfly", &loadout_knife_butterfly_skin));

}

CSkinProcessor::~CSkinProcessor()
{
Clear();
}

// -----------------------------------------------------

IMaterial* CSkinProcessor::GetSkinMaterial(C_BaseCombatWeapon* pWeapon)
{
if (!pWeapon)
return nullptr;

const char* pszClass = pWeapon->GetClassname();  
if (!pszClass)  
	return nullptr;  

WeaponSkin_t* ws = FindSkinEntry(pszClass);  
if (!ws)  
	return nullptr;  

const char* skinPath = ws->pSkinConVar->GetString();  
if (!skinPath || !skinPath[0])  
	return nullptr;  

if (!ws->pMaterial || ws->sLastSkin != skinPath)  
{  
	if (ws->pMaterial)  
	{  
		ws->pMaterial->DecrementReferenceCount();  
		ws->pMaterial = nullptr;  
	}  
	ws->pMaterial = LoadMaterial(*ws, skinPath);  
}  

return ws->pMaterial;

}

IMaterial* CSkinProcessor::LoadMaterial(WeaponSkin_t& ws, const char* pszPath)
{
ws.sLastSkin = pszPath;

IMaterial* pMat = materials->FindMaterial(pszPath, TEXTURE_GROUP_MODEL, true);  
if (!pMat || IsErrorMaterial(pMat))  
{  
	Warning("Failed to load skin: %s for weapon: %s\n", pszPath, ws.pszWeaponClass);  
	return nullptr;  
}  

pMat->IncrementReferenceCount();  

if (!pMat->IsPrecached())  
{  
	MaterialLock_t hLock = materials->Lock();  
	pMat->Refresh();  
	materials->Unlock(hLock);  
}  

return pMat;

}

WeaponSkin_t* CSkinProcessor::FindSkinEntry(const char* pszClass)
{
FOR_EACH_VEC(m_WeaponSkins, i)
{
if (FStrEq(m_WeaponSkins[i].pszWeaponClass, pszClass))
return &m_WeaponSkins[i];
}
return nullptr;
}

IMaterial* CSkinProcessor::GetWeaponMaterial(const char* pszWeaponClass)
{
for (int i = 0; i < m_WeaponSkins.Count(); i++)
{
auto& skin = m_WeaponSkins[i];
if (!Q_stricmp(skin.pszWeaponClass, pszWeaponClass))
{
const char* newPath = skin.pSkinConVar->GetString();
if (newPath && newPath[0])
{
if (!skin.pMaterial || Q_stricmp(skin.sLastSkin.Get(), newPath))
{
skin.pMaterial = materials->FindMaterial(newPath, TEXTURE_GROUP_MODEL);
skin.sLastSkin = newPath;
}
return skin.pMaterial;
}
}
}
return nullptr;
}

void CSkinProcessor::Clear()
{
FOR_EACH_VEC(m_WeaponSkins, i)
{
if (m_WeaponSkins[i].pMaterial)
{
m_WeaponSkins[i].pMaterial->DecrementReferenceCount();
m_WeaponSkins[i].pMaterial = nullptr;
}
}
}