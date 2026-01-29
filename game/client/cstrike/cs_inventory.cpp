#include "cbase.h"
#include "cs_inventory.h"
#include "vgui_controls/ScrollableEditablePanel.h"
#include "cs_skin_database.h"
#include <vgui/ISurface.h>
#include "convar.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// CInventoryItemPanel - панель одного айтема
//-----------------------------------------------------------------------------
CInventoryItemPanel::CInventoryItemPanel(Panel *parent, const char *panelName, const SkinDefinition_t *pSkinDef)
    : BaseClass(parent, panelName)
{
    m_pSkinDef = pSkinDef;
    m_bMouseOver = false;
    
    m_pNameLabel = new vgui::Label(this, "NameLabel", "");
    m_pWeaponLabel = new vgui::Label(this, "WeaponLabel", "");
    m_pIconImage = new vgui::ImagePanel(this, "IconImage");
    
    SetMouseInputEnabled(true);
    
    UpdateDisplay();
    
    InvalidateLayout();
}

void CInventoryItemPanel::ApplySchemeSettings(vgui::IScheme *pScheme)
{
    BaseClass::ApplySchemeSettings(pScheme);
    
    SetBorder(pScheme->GetBorder("ButtonBorder"));
    SetBgColor(GetRarityColor());
    
    m_pNameLabel->SetFont(pScheme->GetFont("DefaultSmall", true));
    m_pNameLabel->SetFgColor(pScheme->GetColor("Label.TextColor", Color(255, 255, 255, 255)));
    
    m_pWeaponLabel->SetFont(pScheme->GetFont("DefaultVerySmall", true));
    m_pWeaponLabel->SetFgColor(pScheme->GetColor("Label.DisabledTextColor", Color(180, 180, 180, 255)));
}

void CInventoryItemPanel::PerformLayout()
{
    BaseClass::PerformLayout();
    
    int wide, tall;
    GetSize(wide, tall);
    
    int iconSize = wide - 10;
    int iconTop = 5;
    m_pIconImage->SetBounds(5, iconTop, iconSize, iconSize);
    
    int labelTop = iconTop + iconSize + 5;
    m_pWeaponLabel->SetBounds(5, labelTop, wide - 10, 15);
    m_pWeaponLabel->SetContentAlignment(vgui::Label::a_center);
    
    labelTop += 15;
    m_pNameLabel->SetBounds(5, labelTop, wide - 10, 20);
    m_pNameLabel->SetContentAlignment(vgui::Label::a_center);
    
    InvalidateLayout();
}

void CInventoryItemPanel::OnMousePressed(vgui::MouseCode code)
{
    if (code == MOUSE_LEFT && m_pSkinDef)
    {
        KeyValues *pKV = new KeyValues("ItemSelected");
        pKV->SetInt("paintkit", m_pSkinDef->iPaintKit);
        pKV->SetInt("weaponid", m_pSkinDef->weaponID);
        PostActionSignal(pKV);
    }
    
    BaseClass::OnMousePressed(code);
}

void CInventoryItemPanel::OnCursorEntered()
{
    m_bMouseOver = true;
    Color rarityColor = GetRarityColor();

    SetBgColor(Color(
        MIN(rarityColor.r() + 30, 255),
        MIN(rarityColor.g() + 30, 255),
        MIN(rarityColor.b() + 30, 255),
        255
    ));
    BaseClass::OnCursorEntered();
}

void CInventoryItemPanel::OnCursorExited()
{
    m_bMouseOver = false;
    SetBgColor(GetRarityColor());
    BaseClass::OnCursorExited();
}

void CInventoryItemPanel::UpdateDisplay()
{
    if (!m_pSkinDef)
        return;
    
    m_pNameLabel->SetText(m_pSkinDef->szName);
    
    const char *pszWeaponName = WeaponIDToAlias(m_pSkinDef->weaponID);
    if (pszWeaponName && Q_strnicmp(pszWeaponName, "weapon_", 7) == 0)
    {
        pszWeaponName += 7;
    }
    
    char szWeaponDisplay[64];
    Q_snprintf(szWeaponDisplay, sizeof(szWeaponDisplay), "%s", pszWeaponName);
    if (szWeaponDisplay[0] >= 'a' && szWeaponDisplay[0] <= 'z')
        szWeaponDisplay[0] -= 32;
    
    m_pWeaponLabel->SetText(szWeaponDisplay);
    
    if (m_pSkinDef->szIconPath[0])
    {
        m_pIconImage->SetImage(m_pSkinDef->szIconPath);
        m_pIconImage->SetShouldScaleImage(true);
        m_pIconImage->SetVisible(true);
    }
    else
    {
        m_pIconImage->SetVisible(false);
    }
}

Color CInventoryItemPanel::GetRarityColor() const
{
    if (!m_pSkinDef)
        return Color(40, 40, 40, 255);
    
    switch (m_pSkinDef->rarity)
    {
        case SKIN_RARITY_COMMON:      return Color(60, 70, 80, 255);
        case SKIN_RARITY_UNCOMMON:    return Color(40, 60, 90, 255);
        case SKIN_RARITY_RARE:        return Color(30, 50, 120, 255);
        case SKIN_RARITY_MYTHICAL:    return Color(60, 30, 120, 255);
        case SKIN_RARITY_LEGENDARY:   return Color(100, 20, 110, 255);
        case SKIN_RARITY_ANCIENT:     return Color(120, 30, 30, 255);
        case SKIN_RARITY_CONTRABAND:  return Color(110, 80, 20, 255);
        default:                      return Color(40, 40, 40, 255);
    }
}

//-----------------------------------------------------------------------------
// CInventoryPanel
//-----------------------------------------------------------------------------
CInventoryPanel::CInventoryPanel(Panel *parent, const char *panelName)
    : BaseClass(parent, panelName)
{
    m_iItemWidth = 200;
    m_iItemHeight = 160;
    m_iItemSpacing = 10;
    m_iItemsPerRow = 4;
    
    m_FilterWeaponID = WEAPON_NONE;
    m_FilterRarity = SKIN_RARITY_COMMON;
    m_bUseWeaponFilter = false;
    m_bUseRarityFilter = false;
    m_bFirstLayout = true;
    
    m_pItemContainer = new vgui::EditablePanel(NULL, "ItemContainer");
    
    m_pScrollablePanel = new vgui::ScrollableEditablePanel(this, m_pItemContainer, "InventoryScroll");
    
    vgui::ivgui()->AddTickSignal(GetVPanel(), 100);
}

CInventoryPanel::~CInventoryPanel()
{
    ClearInventory();
}

void CInventoryPanel::ApplySchemeSettings(vgui::IScheme *pScheme)
{
    BaseClass::ApplySchemeSettings(pScheme);
    SetBgColor(pScheme->GetColor("Frame.BgColor", Color(30, 30, 30, 255)));
}

void CInventoryPanel::OnThink()
{
    BaseClass::OnThink();
    
    if (m_bFirstLayout && GetWide() > 0 && GetTall() > 0)
    {
        m_bFirstLayout = false;
        PostMessage(this, new KeyValues("DelayedLoad"), 0.1f);
    }
}

void CInventoryPanel::PerformLayout()
{
    BaseClass::PerformLayout();
    
    int wide, tall;
    GetSize(wide, tall);
    
    if (wide <= 0 || tall <= 0)
        return;
    
    m_pScrollablePanel->SetBounds(0, 0, wide, tall);
    
    int scrollBarWidth = 16;
    m_iItemsPerRow = (wide - scrollBarWidth - m_iItemSpacing) / (m_iItemWidth + m_iItemSpacing);
    if (m_iItemsPerRow < 1) m_iItemsPerRow = 1;
    
    int numItems = m_Items.Count();
    int numRows = (numItems + m_iItemsPerRow - 1) / m_iItemsPerRow;
    
    for (int i = 0; i < numItems; i++)
    {
        int row = i / m_iItemsPerRow;
        int col = i % m_iItemsPerRow;
        
        int x = col * (m_iItemWidth + m_iItemSpacing) + m_iItemSpacing;
        int y = row * (m_iItemHeight + m_iItemSpacing) + m_iItemSpacing;
        
        m_Items[i]->SetBounds(x, y, m_iItemWidth, m_iItemHeight);
    }
    
    int containerHeight = max(tall, numRows * (m_iItemHeight + m_iItemSpacing) + m_iItemSpacing);
    m_pItemContainer->SetSize(wide - scrollBarWidth, containerHeight);
}

void CInventoryPanel::OnCommand(const char *command)
{
    if (Q_stricmp(command, "DelayedLoad") == 0)
    {
        LoadAllSkins();
    }
    else
    {
        BaseClass::OnCommand(command);
    }
}

void CInventoryPanel::LoadAllSkins()
{
    ClearInventory();
    
    if (!g_SkinDatabase.Initialize())
    {
        Warning("[Inventory] Failed to initialize skin database\n");
        return;
    }
    
    CUtlVector<const SkinDefinition_t*> allSkins;
    g_SkinDatabase.GetAllSkins(allSkins);
    
    DevMsg("[Inventory] Loading %d skins into inventory\n", allSkins.Count());
    
    FOR_EACH_VEC(allSkins, i)
    {
        const SkinDefinition_t *pSkinDef = allSkins[i];
        
        if (m_bUseWeaponFilter && pSkinDef->weaponID != m_FilterWeaponID)
            continue;
            
        if (m_bUseRarityFilter && pSkinDef->rarity != m_FilterRarity)
            continue;
        
        AddSkinToInventory(pSkinDef);
    }
    
    InvalidateLayout();
}

void CInventoryPanel::LoadSkinsForWeapon(CSWeaponID weaponID)
{
    ClearInventory();
    
    if (!g_SkinDatabase.Initialize())
    {
        Warning("[Inventory] Failed to initialize skin database\n");
        return;
    }
    
    CUtlVector<const SkinDefinition_t*> weaponSkins;
    g_SkinDatabase.GetSkinsForWeapon(weaponID, weaponSkins);
    
    DevMsg("[Inventory] Loading %d skins for weapon %d\n", weaponSkins.Count(), weaponID);
    
    FOR_EACH_VEC(weaponSkins, i)
    {
        AddSkinToInventory(weaponSkins[i]);
    }
    
    InvalidateLayout();
}

void CInventoryPanel::AddSkinToInventory(const SkinDefinition_t *pSkinDef)
{
    if (!pSkinDef)
        return;
    
    char panelName[64];
    Q_snprintf(panelName, sizeof(panelName), "Item_%d", m_Items.Count());
    
    CInventoryItemPanel *pItem = new CInventoryItemPanel(m_pItemContainer, panelName, pSkinDef);
    pItem->AddActionSignalTarget(this);
    m_Items.AddToTail(pItem);
}

void CInventoryPanel::ClearInventory()
{
    FOR_EACH_VEC(m_Items, i)
    {
        m_Items[i]->MarkForDeletion();
    }
    m_Items.RemoveAll();
}

void CInventoryPanel::SetWeaponFilter(CSWeaponID weaponID)
{
    m_FilterWeaponID = weaponID;
    m_bUseWeaponFilter = true;
    RebuildInventory();
}

void CInventoryPanel::SetRarityFilter(ESkinRarity rarity)
{
    m_FilterRarity = rarity;
    m_bUseRarityFilter = true;
    RebuildInventory();
}

void CInventoryPanel::ClearFilters()
{
    m_bUseWeaponFilter = false;
    m_bUseRarityFilter = false;
    RebuildInventory();
}

void CInventoryPanel::RebuildInventory()
{
    LoadAllSkins();
}

void CInventoryPanel::OnItemSelected(KeyValues *data)
{
    int iPaintKit = data->GetInt("paintkit", 0);
    int iWeaponID = data->GetInt("weaponid", WEAPON_NONE);
    
    if (iPaintKit > 0 && iWeaponID != WEAPON_NONE)
    {
        const SkinDefinition_t *pSkinDef = g_SkinDatabase.FindSkinByPaintKit(iPaintKit);
        
        if (pSkinDef)
        {
            Msg("[Inventory] Selected skin: %s (PaintKit: %d, Weapon: %s)\n", 
                pSkinDef->szName, 
                iPaintKit,
                WeaponIDToAlias(pSkinDef->weaponID));
            
            SaveSkinToLoadout(iPaintKit, (CSWeaponID)iWeaponID);
            
            PostActionSignal(new KeyValues("SkinSelected", "paintkit", iPaintKit));
        }
    }
}

void CInventoryPanel::SaveSkinToLoadout(int iPaintKit, CSWeaponID weaponID)
{
    const char *pszConVarName = GetConVarNameForWeapon(weaponID);
    
    if (!pszConVarName)
    {
        Warning("[Inventory] No ConVar found for weapon ID: %d\n", weaponID);
        return;
    }
    
    ConVar *pConVar = cvar->FindVar(pszConVarName);
    
    if (pConVar)
    {
        pConVar->SetValue(iPaintKit);
        Msg("[Inventory] Saved skin %d to %s\n", iPaintKit, pszConVarName);
    }
    else
    {
        Warning("[Inventory] ConVar not found: %s\n", pszConVarName);
    }
}

const char* CInventoryPanel::GetConVarNameForWeapon(CSWeaponID weaponID)
{
    switch (weaponID)
    {
        case WEAPON_AK47:       return "loadout_skin_ak47";
        case WEAPON_M4A4:       return "loadout_skin_m4a4";
        case WEAPON_M4A1:       return "loadout_skin_m4a1s";
        case WEAPON_AWP:        return "loadout_skin_awp";
        case WEAPON_DEAGLE:     return "loadout_skin_deagle";
        case WEAPON_REVOLVER:   return "loadout_skin_revolver";
        case WEAPON_GLOCK:      return "loadout_skin_glock";
        case WEAPON_USP:        return "loadout_skin_usp";
        case WEAPON_HKP2000:    return "loadout_skin_hkp2000";
        case WEAPON_P250:       return "loadout_skin_p250";
        case WEAPON_FIVESEVEN:  return "loadout_skin_fiveseven";
        case WEAPON_TEC9:       return "loadout_skin_tec9";
        case WEAPON_CZ75A:      return "loadout_skin_cz75a";
        case WEAPON_ELITE:      return "loadout_skin_dualberettas";
        case WEAPON_MAG7:       return "loadout_skin_mag7";
        case WEAPON_NOVA:       return "loadout_skin_nova";
        case WEAPON_SAWEDOFF:   return "loadout_skin_sawedoff";
        case WEAPON_XM1014:     return "loadout_skin_xm1014";
        case WEAPON_M249:       return "loadout_skin_m249";
        case WEAPON_NEGEV:      return "loadout_skin_negev";
        case WEAPON_MAC10:      return "loadout_skin_mac10";
        case WEAPON_MP9:        return "loadout_skin_mp9";
        case WEAPON_MP7:        return "loadout_skin_mp7";
        case WEAPON_MP5SD:      return "loadout_skin_mp5sd";
        case WEAPON_UMP45:      return "loadout_skin_ump45";
        case WEAPON_P90:        return "loadout_skin_p90";
        case WEAPON_BIZON:      return "loadout_skin_bizon";
        case WEAPON_GALILAR:    return "loadout_skin_galilar";
        case WEAPON_FAMAS:      return "loadout_skin_famas";
        case WEAPON_AUG:        return "loadout_skin_aug";
        case WEAPON_SG556:      return "loadout_skin_sg556";
        case WEAPON_SSG08:      return "loadout_skin_ssg08";
        case WEAPON_SCAR20:     return "loadout_skin_scar20";
        case WEAPON_G3SG1:      return "loadout_skin_g3sg1";
        default:                return nullptr;
    }
}