#ifndef CS_INVENTORY_H
#define CS_INVENTORY_H

#ifdef _WIN32
#pragma once
#endif

#include "vgui_controls/ScrollableEditablePanel.h"
#include "vgui_controls/EditablePanel.h"
#include "vgui_controls/Label.h"
#include "vgui_controls/ImagePanel.h"
#include "cs_skin_database.h"
#include <vgui/IScheme.h>
#include <vgui/IVGui.h>

namespace vgui
{

//-----------------------------------------------------------------------------
// Purpose :
//-----------------------------------------------------------------------------
class CInventoryItemPanel : public vgui::EditablePanel
{
    DECLARE_CLASS_SIMPLE(CInventoryItemPanel, vgui::EditablePanel);

public:
    CInventoryItemPanel(Panel *parent, const char *panelName, const SkinDefinition_t *pSkinDef);
    
    virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
    virtual void PerformLayout();
    virtual void OnMousePressed(vgui::MouseCode code);
    virtual void OnCursorEntered();
    virtual void OnCursorExited();
    
    const SkinDefinition_t* GetSkinDefinition() const { return m_pSkinDef; }
    int GetPaintKit() const { return m_pSkinDef ? m_pSkinDef->iPaintKit : 0; }

private:
    void UpdateDisplay();
    Color GetRarityColor() const;
    
    const SkinDefinition_t *m_pSkinDef;
    vgui::Label *m_pNameLabel;
    vgui::Label *m_pWeaponLabel;
    vgui::ImagePanel *m_pIconImage;
    
    bool m_bMouseOver;
};

//-----------------------------------------------------------------------------
// Purpose
//-----------------------------------------------------------------------------
class CInventoryPanel : public vgui::EditablePanel
{
    DECLARE_CLASS_SIMPLE(CInventoryPanel, vgui::EditablePanel);

public:
    CInventoryPanel(Panel *parent, const char *panelName);
    virtual ~CInventoryPanel();
    
    virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
    virtual void PerformLayout();
    virtual void OnCommand(const char *command);
    virtual void OnThink();
    
    void LoadAllSkins();
    void LoadSkinsForWeapon(CSWeaponID weaponID);
    void ClearInventory();
    
    void SetWeaponFilter(CSWeaponID weaponID);
    void SetRarityFilter(ESkinRarity rarity);
    void ClearFilters();
    
    MESSAGE_FUNC_PARAMS(OnItemSelected, "ItemSelected", data);

protected:
    void RebuildInventory();
    void AddSkinToInventory(const SkinDefinition_t *pSkinDef);
    void SaveSkinToLoadout(int iPaintKit, CSWeaponID weaponID);
    const char* GetConVarNameForWeapon(CSWeaponID weaponID);
    
    vgui::ScrollableEditablePanel *m_pScrollablePanel;
    vgui::EditablePanel *m_pItemContainer;
    CUtlVector<CInventoryItemPanel*> m_Items;
    
    int m_iItemWidth;
    int m_iItemHeight;
    int m_iItemSpacing;
    int m_iItemsPerRow;
    
    CSWeaponID m_FilterWeaponID;
    ESkinRarity m_FilterRarity;
    bool m_bUseWeaponFilter;
    bool m_bUseRarityFilter;
    
    bool m_bFirstLayout;
};

} // namespace vgui

#endif // CS_INVENTORY_H