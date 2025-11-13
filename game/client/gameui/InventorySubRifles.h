//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef INVENTORYSUBRIFLES_H
#define INVENTORYSUBRIFLES_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/PropertyPage.h>
#include "LabeledCommandComboBox.h"

class CCvarToggleCheckButton;

class CInventorySubRifles : public vgui::PropertyPage
{
    DECLARE_CLASS_SIMPLE(CInventorySubRifles, vgui::PropertyPage);

public:
    CInventorySubRifles(vgui::Panel *parent);
    ~CInventorySubRifles();

    MESSAGE_FUNC(OnControlModified, "ControlModified");
    
    virtual void OnResetData();
    virtual void OnApplyChanges();

private:
    void InitRifleSkinList(CLabeledCommandComboBox* cb, const char* weaponName, ConVar* pConVar);

    CLabeledCommandComboBox* m_pLoadoutM4ComboBox;
    CLabeledCommandComboBox* m_pLoadoutAK47ComboBox;
    CLabeledCommandComboBox* m_pLoadoutAUGComboBox;
    CLabeledCommandComboBox* m_pLoadoutFAMASComboBox;
    CLabeledCommandComboBox* m_pLoadoutGalilComboBox;
    CLabeledCommandComboBox* m_pLoadoutM4A1SSComboBox;
    CLabeledCommandComboBox* m_pLoadoutSG556ComboBox;
};

#endif // INVENTORYSUBRIFLES_H