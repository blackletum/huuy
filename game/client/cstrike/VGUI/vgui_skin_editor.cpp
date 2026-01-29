#include "cbase.h"
#include "vgui_skin_editor.h"
#include "cs_inventory.h"
#include <vgui/IVGui.h>
#include <vgui/IInput.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/ComboBox.h>
#include "filesystem.h"
#include "cs_skin_database.h"

using namespace vgui;

CSkinEditorPanel* g_pSkinEditor = nullptr;

//=============================================================================
// CSkinEditorPanel
//=============================================================================
CSkinEditorPanel::CSkinEditorPanel(Panel* parent) : BaseClass(parent, "SkinEditorPanel")
{
    g_pSkinEditor = this;
    SetTitle("Skin Inventory", true);
    
    int screenWidth, screenHeight;
    vgui::surface()->GetScreenSize(screenWidth, screenHeight);

    int w = static_cast<int>(screenWidth * 0.9f); 
    int h = static_cast<int>(screenHeight * 1.0f); 

    w = MAX(w, 800); 
    h = MAX(h, 600); 

    w = MIN(w, 2674); 
    h = MIN(h, 1220); 

    SetSize(w, h);

    SetSizeable(true);
    SetDeleteSelfOnClose(false);
    SetMinimumSize(800, 600);
    
    // Создаем UI элементы только один раз
    m_pInventoryPanel = nullptr;
    m_pShowAllButton = nullptr;
    m_pWeaponFilter = nullptr;
    m_pRarityFilter = nullptr;
    
    CreateControls();
    
    InvalidateLayout(true, true);
    MoveToCenterOfScreen();
}

CSkinEditorPanel::~CSkinEditorPanel()
{
}

void CSkinEditorPanel::CreateControls()
{
    if (!m_pInventoryPanel)
    {
        m_pInventoryPanel = new vgui::CInventoryPanel(this, "InventoryPanel");
        m_pInventoryPanel->AddActionSignalTarget(this);
    }
    
    if (!m_pShowAllButton)
    {
        m_pShowAllButton = new vgui::Button(this, "ShowAllButton", "Show All Skins", this, "ShowAll");
    }
    
    if (!m_pWeaponFilter)
    {
        m_pWeaponFilter = new vgui::ComboBox(this, "WeaponFilter", 10, false);
        m_pWeaponFilter->AddItem("All Weapons", nullptr);
        m_pWeaponFilter->AddItem("AK-47", new KeyValues("weapon", "id", WEAPON_AK47));
        m_pWeaponFilter->AddItem("M4A1", new KeyValues("weapon", "id", WEAPON_M4A1));
        m_pWeaponFilter->AddItem("AWP", new KeyValues("weapon", "id", WEAPON_AWP));
        m_pWeaponFilter->AddItem("Desert Eagle", new KeyValues("weapon", "id", WEAPON_DEAGLE));
        m_pWeaponFilter->AddItem("Glock-18", new KeyValues("weapon", "id", WEAPON_GLOCK));
        m_pWeaponFilter->AddItem("USP-S", new KeyValues("weapon", "id", WEAPON_USP));
        m_pWeaponFilter->AddItem("P250", new KeyValues("weapon", "id", WEAPON_P250));
        m_pWeaponFilter->AddItem("Five-SeveN", new KeyValues("weapon", "id", WEAPON_FIVESEVEN));
        m_pWeaponFilter->AddActionSignalTarget(this);
    }
    
    if (!m_pRarityFilter)
    {
        m_pRarityFilter = new vgui::ComboBox(this, "RarityFilter", 10, false);
        m_pRarityFilter->AddItem("All Rarities", nullptr);
        m_pRarityFilter->AddItem("Common", new KeyValues("rarity", "id", SKIN_RARITY_COMMON));
        m_pRarityFilter->AddItem("Uncommon", new KeyValues("rarity", "id", SKIN_RARITY_UNCOMMON));
        m_pRarityFilter->AddItem("Rare", new KeyValues("rarity", "id", SKIN_RARITY_RARE));
        m_pRarityFilter->AddItem("Mythical", new KeyValues("rarity", "id", SKIN_RARITY_MYTHICAL));
        m_pRarityFilter->AddItem("Legendary", new KeyValues("rarity", "id", SKIN_RARITY_LEGENDARY));
        m_pRarityFilter->AddItem("Ancient", new KeyValues("rarity", "id", SKIN_RARITY_ANCIENT));
        m_pRarityFilter->AddItem("Contraband", new KeyValues("rarity", "id", SKIN_RARITY_CONTRABAND));
        m_pRarityFilter->AddActionSignalTarget(this);
    }
}

void CSkinEditorPanel::PerformLayout()
{
    BaseClass::PerformLayout();
    
    int wide, tall;
    GetSize(wide, tall);
    
    int filterPanelHeight = 40;
    int padding = 10;
    int buttonWidth = 120;
    int comboWidth = 150;
    
    int xPos = padding;
    
    if (m_pShowAllButton)
    {
        m_pShowAllButton->SetBounds(xPos, padding, buttonWidth, 25);
        xPos += buttonWidth + padding;
    }
    
    if (m_pWeaponFilter)
    {
        m_pWeaponFilter->SetBounds(xPos, padding, comboWidth, 25);
        xPos += comboWidth + padding;
    }
    
    if (m_pRarityFilter)
    {
        m_pRarityFilter->SetBounds(xPos, padding, comboWidth, 25);
    }
    
    if (m_pInventoryPanel)
    {
        m_pInventoryPanel->SetBounds(padding, filterPanelHeight + padding, 
                                    wide - padding * 2, 
                                    tall - filterPanelHeight - padding * 3);
    }
}

void CSkinEditorPanel::ApplySchemeSettings(vgui::IScheme *pScheme)
{
    BaseClass::ApplySchemeSettings(pScheme);
    SetBgColor(pScheme->GetColor("Frame.BgColor", Color(50, 50, 50, 255)));
}

void CSkinEditorPanel::OnCommand(const char *command)
{
    if (Q_stricmp(command, "ShowAll") == 0)
    {
        if (m_pInventoryPanel)
        {
            m_pInventoryPanel->ClearFilters();
        }
        
        if (m_pWeaponFilter)
            m_pWeaponFilter->ActivateItemByRow(0);
            
        if (m_pRarityFilter)
            m_pRarityFilter->ActivateItemByRow(0);
    }
    else
    {
        BaseClass::OnCommand(command);
    }
}

void CSkinEditorPanel::OnTextChanged(vgui::Panel *panel)
{
    if (!m_pInventoryPanel)
        return;
    
    if (panel == m_pWeaponFilter)
    {
        KeyValues *pUserData = m_pWeaponFilter->GetActiveItemUserData();
        
        if (pUserData)
        {
            CSWeaponID weaponID = (CSWeaponID)pUserData->GetInt("id", WEAPON_NONE);
            if (weaponID != WEAPON_NONE)
            {
                m_pInventoryPanel->SetWeaponFilter(weaponID);
            }
            else
            {
                m_pInventoryPanel->ClearFilters();
            }
        }
        else
        {
            m_pInventoryPanel->ClearFilters();
        }
    }
    else if (panel == m_pRarityFilter)
    {
        KeyValues *pUserData = m_pRarityFilter->GetActiveItemUserData();
        
        if (pUserData)
        {
            ESkinRarity rarity = (ESkinRarity)pUserData->GetInt("id", SKIN_RARITY_COMMON);
            m_pInventoryPanel->SetRarityFilter(rarity);
        }
        else
        {
            m_pInventoryPanel->ClearFilters();
        }
    }
}

void CSkinEditorPanel::OnSkinSelected(KeyValues *data)
{
    int iPaintKit = data->GetInt("paintkit", 0);
    
    if (iPaintKit > 0)
    {
        const SkinDefinition_t *pSkinDef = g_SkinDatabase.FindSkinByPaintKit(iPaintKit);
        
        if (pSkinDef)
        {
            Msg("[Inventory] Equipped skin: %s (PaintKit: %d)\n", 
                pSkinDef->szName, 
                iPaintKit);
        }
    }
}

void CSkinEditorPanel::OnThink()
{
    BaseClass::OnThink();
}

void CSkinEditorPanel::OnClose()
{
    BaseClass::OnClose();
    SetVisible(false);
}

//=============================================================================
// Console Commands
//=============================================================================
CON_COMMAND(skin_inventory, "Open skin inventory")
{
    if (!g_pSkinEditor)
    {
        g_pSkinEditor = new CSkinEditorPanel(nullptr);
    }
    
    g_pSkinEditor->Activate();
    g_pSkinEditor->SetVisible(true);
}

CON_COMMAND(skin_inventory_close, "Close skin inventory")
{
    if (g_pSkinEditor)
    {
        g_pSkinEditor->OnClose();
    }
}