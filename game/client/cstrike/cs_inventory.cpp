#include "cbase.h"
#include "cs_inventory.h"
#include "vgui_controls/ScrollableEditablePanel.h"
#include "cs_skin_database.h"
#include <vgui_controls/Image.h>
#include <vgui/ISurface.h>
#include "convar.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Кастомный контейнер который не сбрасывает позиции детей
//-----------------------------------------------------------------------------
class CInventoryItemContainer : public vgui::EditablePanel
{
    DECLARE_CLASS_SIMPLE(CInventoryItemContainer, vgui::EditablePanel);
public:
    CInventoryItemContainer(Panel *parent, const char *name) : BaseClass(parent, name) {}
    
    virtual void PerformLayout() override
    {
    }
    virtual void InvalidateLayout()
    {
    }
};

//-----------------------------------------------------------------------------
// CInventoryItemPanel 
//-----------------------------------------------------------------------------
CInventoryItemPanel::CInventoryItemPanel(Panel *parent, const char *panelName, const SkinDefinition_t *pSkinDef)
    : BaseClass(parent, panelName)
{
    m_pSkinDef = pSkinDef;
    m_bMouseOver = false;
    
    // Создаем лейблы и иконку с отключенным mouse input
    m_pNameLabel = new vgui::Label(this, "NameLabel", "");
    m_pNameLabel->SetMouseInputEnabled(false);
    
    m_pWeaponLabel = new vgui::Label(this, "WeaponLabel", "");
    m_pWeaponLabel->SetMouseInputEnabled(false);
    
    m_pIconImage = new vgui::ImagePanel(this, "IconImage");
    m_pIconImage->SetMouseInputEnabled(false);
    
    m_pRarityBar = new vgui::Panel(this, "RarityBar");
    m_pRarityBar->SetMouseInputEnabled(false);
    
    SetMouseInputEnabled(true);
    
    UpdateDisplay();
}

void CInventoryItemPanel::ApplySchemeSettings(vgui::IScheme *pScheme)
{
    BaseClass::ApplySchemeSettings(pScheme);
    
    SetBorder(pScheme->GetBorder("ButtonBorder"));
    SetBgColor(Color(0, 0, 0, 0));
    
    m_pRarityBar->SetBgColor(GetRarityColor());
    
    // Используем более крупные шрифты
    m_pNameLabel->SetFont(pScheme->GetFont("DefaultLarge", true));
    m_pNameLabel->SetFgColor(Color(255, 255, 255, 255));
    
    m_pWeaponLabel->SetFont(pScheme->GetFont("Default", true));
    m_pWeaponLabel->SetFgColor(Color(200, 200, 200, 255));
}

void CInventoryItemPanel::PerformLayout()
{
    BaseClass::PerformLayout();
    
    int wide, tall;
    GetSize(wide, tall);

    // Полоса редкости слева
    m_pRarityBar->SetBounds(0, 0, 8, tall);
    
    // Иконка в верхней части панели
    IImage *pImg = m_pIconImage->GetImage();
    if (pImg)
    {
        int texW, texH;
        pImg->GetSize(texW, texH);

        if (texW > 0 && texH > 0)
        {
            // Область для иконки (оставляем место для текста внизу)
            int iconAreaHeight = tall - 80;  // 80 пикселей для текста
            
            float scaleX = (float)(wide - 20) / texW;
            float scaleY = (float)iconAreaHeight / texH;
            float scale = MIN(scaleX, scaleY);

            int drawW = texW * scale;
            int drawH = texH * scale;

            int x = (wide - drawW) / 2;
            int y = 10 + (iconAreaHeight - drawH) / 2;

            m_pIconImage->SetBounds(x, y, drawW, drawH);
        }
    }
    
    // Текст внизу ПОД айтемом
    int textStartY = tall - 53;  // Начинаем ниже панели
    
    // Название оружия (первая строка под айтемом)
    m_pWeaponLabel->SetBounds(12, textStartY + 5, wide, 22);
    m_pWeaponLabel->SetContentAlignment(vgui::Label::a_west);
    
    // Название скина (вторая строка)
    m_pNameLabel->SetBounds(12, textStartY + 27, wide, 26);
    m_pNameLabel->SetContentAlignment(vgui::Label::a_west);
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
    SetBgColor(Color(255, 255, 255, 60));
    BaseClass::OnCursorEntered();
}

void CInventoryItemPanel::OnCursorExited()
{
    m_bMouseOver = false;
    SetBgColor(Color(0, 0, 0, 0));
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
    
    // Форматируем название оружия
    char szWeaponDisplay[64];
    Q_snprintf(szWeaponDisplay, sizeof(szWeaponDisplay), "%s", pszWeaponName);
    
    // Заменяем underscore на пробел и делаем первую букву заглавной
    for (int i = 0; szWeaponDisplay[i]; i++)
    {
        if (szWeaponDisplay[i] == '_')
            szWeaponDisplay[i] = ' ';
            
        if (i == 0 && szWeaponDisplay[i] >= 'a' && szWeaponDisplay[i] <= 'z')
            szWeaponDisplay[i] -= 32;
    }
    
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
        return Color(100, 100, 100, 255);
    
    switch (m_pSkinDef->rarity)
    {
        case SKIN_RARITY_COMMON:      return Color(176, 195, 217, 255);  // Светло-серый
        case SKIN_RARITY_UNCOMMON:    return Color(94, 152, 217, 255);   // Голубой
        case SKIN_RARITY_RARE:        return Color(75, 105, 255, 255);   // Синий
        case SKIN_RARITY_MYTHICAL:    return Color(136, 71, 255, 255);   // Фиолетовый
        case SKIN_RARITY_LEGENDARY:   return Color(211, 44, 230, 255);   // Розовый
        case SKIN_RARITY_ANCIENT:     return Color(235, 75, 75, 255);    // Красный
        case SKIN_RARITY_CONTRABAND:  return Color(228, 174, 57, 255);   // Золотой
        default:                      return Color(100, 100, 100, 255);
    }
}

//-----------------------------------------------------------------------------
// CInventoryPanel
//-----------------------------------------------------------------------------
CInventoryPanel::CInventoryPanel(Panel *parent, const char *panelName)
    : BaseClass(parent, panelName)
{
    // Получаем разрешение экрана для масштабирования
    int screenWidth, screenHeight;
    vgui::surface()->GetScreenSize(screenWidth, screenHeight);
    
    // Масштабируем размеры в зависимости от разрешения
    // Базовое разрешение 1920x1080
    float scaleX = screenWidth / 1920.0f;
    float scaleY = screenHeight / 1080.0f;
    float scale = (scaleX + scaleY) / 2.0f;
    
    // Базовые размеры для 1920x1080
    int baseItemWidth = 240;
    int baseItemHeight = 120;
    
    m_iItemWidth = baseItemWidth * scale;
    m_iItemHeight = baseItemHeight * scale;
    m_iItemSpacing = 15 * scale;
    m_iItemsPerRow = 4;
    
    // Отступы слева и справа (1-1.5 см = примерно 38-57 пикселей при 96 DPI)
    m_iLeftMargin = 45 * scale;
    m_iRightMargin = 45 * scale;
    
    m_FilterWeaponID = WEAPON_NONE;
    m_FilterRarity = SKIN_RARITY_COMMON;
    m_bUseWeaponFilter = false;
    m_bUseRarityFilter = false;
    m_bFirstLayout = true;
    
    m_pItemContainer = new CInventoryItemContainer(NULL, "ItemContainer");
    m_pScrollablePanel = new vgui::ScrollableEditablePanel(this, m_pItemContainer, "InventoryScroll");
    
    vgui::ivgui()->AddTickSignal(GetVPanel(), 100);
    
    InvalidateLayout();
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
        // Даем время для инициализации всех элементов
        PostMessage(this, new KeyValues("DelayedLoad"), 0.2f);
    }
}

void CInventoryPanel::PerformLayout()
{
    InvalidateLayout();
    BaseClass::PerformLayout();
    
    if (!m_pItemContainer)
    return;
    
    int wide, tall;
    GetSize(wide, tall);
    
    if (wide <= 0 || tall <= 0)
        return;
    
    m_pScrollablePanel->SetBounds(0, 0, wide, tall);
    
    int scrollBarWidth = 16;
    int availableWidth = wide - scrollBarWidth - m_iLeftMargin - m_iRightMargin;
    
    // Вычисляем количество айтемов в ряду
    m_iItemsPerRow = availableWidth / (m_iItemWidth + m_iItemSpacing);
    if (m_iItemsPerRow < 1) m_iItemsPerRow = 1;
    
    int numItems = m_Items.Count();
    
    // Расстояние между айтемами по Y = 1.6 * высота айтема
    int itemYSpacing = (int)(m_iItemHeight * 1.6f);
    
    // Высота для текста под каждым айтемом (название оружия + название скина)
    int textHeight = 53;
    
    int numRows = (numItems + m_iItemsPerRow - 1) / m_iItemsPerRow;
    
    for (int i = 0; i < numItems; i++)
    {
        int row = i / m_iItemsPerRow;
        int col = i % m_iItemsPerRow;
        
        // Позиция с учетом левого отступа
        int x = m_iLeftMargin + col * (m_iItemWidth + m_iItemSpacing);
        int y = m_iItemSpacing + row * itemYSpacing;
        
        m_Items[i]->SetBounds(x, y, m_iItemWidth, m_iItemHeight + textHeight);
    }
    
    // Высота контейнера с учетом отступов
    int containerHeight = max(tall, m_iItemSpacing + numRows * itemYSpacing + m_iItemSpacing);
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
    InvalidateLayout();
}

void CInventoryPanel::SetRarityFilter(ESkinRarity rarity)
{
    m_FilterRarity = rarity;
    m_bUseRarityFilter = true;
    RebuildInventory();
    InvalidateLayout();
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
        case WEAPON_KNIFE_CSS:              return "loadout_skin_knife_css";
        case WEAPON_KNIFE_KARAMBIT:         return "loadout_skin_knife_karambit";
        case WEAPON_KNIFE_FLIP:             return "loadout_skin_knife_flip";
        case WEAPON_KNIFE_BAYONET:          return "loadout_skin_knife_bayonet";
        case WEAPON_KNIFE_M9_BAYONET:       return "loadout_skin_knife_m9_bayonet";
        case WEAPON_KNIFE_BUTTERFLY:        return "loadout_skin_knife_butterfly";
        case WEAPON_KNIFE_GUT:              return "loadout_skin_knife_gut";
        case WEAPON_KNIFE_TACTICAL:         return "loadout_skin_knife_tactical";
        case WEAPON_KNIFE_FALCHION:         return "loadout_skin_knife_falchion";
        case WEAPON_KNIFE_SURVIVAL_BOWIE:   return "loadout_skin_knife_survival_bowie";
        case WEAPON_KNIFE_CANIS:            return "loadout_skin_knife_canis";
        case WEAPON_KNIFE_CORD:             return "loadout_skin_knife_cord";
        case WEAPON_KNIFE_GYPSY:            return "loadout_skin_knife_gypsy_jackknife";
        case WEAPON_KNIFE_OUTDOOR:          return "loadout_skin_knife_outdoor";
        case WEAPON_KNIFE_SKELETON:         return "loadout_skin_knife_skeleton";
        case WEAPON_KNIFE_STILETTO:         return "loadout_skin_knife_stiletto";
        case WEAPON_KNIFE_URSUS:            return "loadout_skin_knife_ursus";
        case WEAPON_KNIFE_WIDOWMAKER:       return "loadout_skin_knife_widowmaker";
        case WEAPON_KNIFE_PUSH:             return "loadout_skin_knife_push";
        default:                return nullptr;
    }
}
