//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//


#include "InventorySubRifles.h"
#include "vgui_controls/Button.h"
#include "filesystem.h"
#include "tier1/convar.h"
#include "cs_shareddefs.h"
#include "vgui/ILocalize.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

extern ConVar loadout_ak47_skin;
extern ConVar loadout_aug_skin;
extern ConVar loadout_famas_skin;
extern ConVar loadout_galilar_skin;
extern ConVar loadout_m4a1_silenser_skin;
extern ConVar loadout_m4a4_skin;
extern ConVar loadout_sg556_skin;

CInventorySubRifles::CInventorySubRifles(vgui::Panel *parent) 
    : vgui::PropertyPage(parent, "InventorySubRifles")
{
    Button *cancel = new Button(this, "Cancel", "#GameUI_Cancel");
    cancel->SetCommand("Close");

    Button *ok = new Button(this, "OK", "#GameUI_OK");
    ok->SetCommand("Ok");

    Button *apply = new Button(this, "Apply", "#GameUI_Apply");
    apply->SetCommand("Apply");

    m_pLoadoutM4ComboBox = new CLabeledCommandComboBox(this, "M4ComboBox");
    InitRifleSkinList(m_pLoadoutM4ComboBox, "m4a4", &loadout_m4a4_skin);
    m_pLoadoutM4ComboBox->AddActionSignalTarget(this);

    m_pLoadoutAK47ComboBox = new CLabeledCommandComboBox(this, "AK47ComboBox");
    InitRifleSkinList(m_pLoadoutAK47ComboBox, "ak47", &loadout_ak47_skin);
    m_pLoadoutAK47ComboBox->AddActionSignalTarget(this);

    m_pLoadoutAUGComboBox = new CLabeledCommandComboBox(this, "AUGComboBox");
    InitRifleSkinList(m_pLoadoutAUGComboBox, "aug", &loadout_aug_skin);
    m_pLoadoutAUGComboBox->AddActionSignalTarget(this);

    m_pLoadoutFAMASComboBox = new CLabeledCommandComboBox(this, "FAMASComboBox");
    InitRifleSkinList(m_pLoadoutFAMASComboBox, "famas", &loadout_famas_skin);
    m_pLoadoutFAMASComboBox->AddActionSignalTarget(this);

    m_pLoadoutGalilComboBox = new CLabeledCommandComboBox(this, "GalilComboBox");
    InitRifleSkinList(m_pLoadoutGalilComboBox, "galilar", &loadout_galilar_skin);
    m_pLoadoutGalilComboBox->AddActionSignalTarget(this);

    m_pLoadoutM4A1SSComboBox = new CLabeledCommandComboBox(this, "M4A1SSComboBox");
    InitRifleSkinList(m_pLoadoutM4A1SSComboBox, "m4a1_silenser", &loadout_m4a1_silenser_skin);
    m_pLoadoutM4A1SSComboBox->AddActionSignalTarget(this);

    m_pLoadoutSG556ComboBox = new CLabeledCommandComboBox(this, "SG556ComboBox");
    InitRifleSkinList(m_pLoadoutSG556ComboBox, "sg556", &loadout_sg556_skin);
    m_pLoadoutSG556ComboBox->AddActionSignalTarget(this);

    LoadControlSettings("Resource/InventorySubRifles.res");
}

CInventorySubRifles::~CInventorySubRifles() 
{
}

void CInventorySubRifles::OnControlModified()
{
	PostMessage(GetParent(), new KeyValues("ApplyButtonEnable"));
	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CInventorySubRifles::InitRifleSkinList(CLabeledCommandComboBox* cb, const char* weaponName, ConVar* pConVar)
{
    cb->DeleteAllItems();

    FileFindHandle_t fh;
    char pattern[256];
    Q_snprintf(pattern, sizeof(pattern), "materials/models/weapons/v_models/rif_%s_skins/*.vmt", weaponName);

    const char* fn = g_pFullFileSystem->FindFirst(pattern, &fh);
    int initialItem = 0;
    int i = 0;

    const char* currentSkin = pConVar ? pConVar->GetString() : "";

    while (fn)
    {
        char displayName[128];
        Q_strncpy(displayName, fn, sizeof(displayName));
        displayName[strlen(displayName) - 4] = 0;

        char command[512];
        Q_snprintf(command, sizeof(command),
                   "loadout_%s_skin models/weapons/v_models/rif_%s_skins/%s",
                   weaponName, weaponName, displayName);

        cb->AddItem(displayName, command);

        char fullPath[512];
        Q_snprintf(fullPath, sizeof(fullPath),
                   "models/weapons/v_models/rif_%s_skins/%s", weaponName, displayName);

        if (!Q_stricmp(fullPath, currentSkin))
            initialItem = i;

        ++i;
        fn = g_pFullFileSystem->FindNext(fh);
    }

    g_pFullFileSystem->FindClose(fh);
    cb->SetInitialItem(initialItem);
}

//-----------------------------------------------------------------------------
// OnResetData
//-----------------------------------------------------------------------------
void CInventorySubRifles::OnResetData()
{
    struct ComboSkinData
    {
        CLabeledCommandComboBox* cb;
        const char* convarName;
        const char* folder;
    };

    ComboSkinData comboData[] = {
        { m_pLoadoutM4ComboBox, "loadout_m4a4_skin", "rif_m4a4_skins" },
        { m_pLoadoutAK47ComboBox, "loadout_ak47_skin", "rif_ak47_skins" },
        { m_pLoadoutAUGComboBox, "loadout_aug_skin", "rif_aug_skins" },
        { m_pLoadoutFAMASComboBox, "loadout_famas_skin", "rif_famas_skins" },
        { m_pLoadoutGalilComboBox, "loadout_galilar_skin", "rif_galilar_skins" },
        { m_pLoadoutM4A1SSComboBox, "loadout_m4a1_silenser_skin", "rif_m4a1_silenser_skins" },
        { m_pLoadoutSG556ComboBox, "loadout_sg556_skin", "rif_sg556_skins" },
    };

    for (int c = 0; c < ARRAYSIZE(comboData); c++)
    {
        ConVarRef cv(comboData[c].convarName);
        if (!cv.IsValid())
            continue;

        const char* skinPath = cv.GetString();
        int initialItem = 0;
        int count = comboData[c].cb->GetItemCount();
        for (int i = 0; i < count; i++)
        {
            wchar_t itemText[512];
            comboData[c].cb->GetItemText(i, itemText, sizeof(itemText));
            char ansiText[512];
            g_pVGuiLocalize->ConvertUnicodeToANSI(itemText, ansiText, sizeof(ansiText));

            char fullPath[512];
            Q_snprintf(fullPath, sizeof(fullPath),
                       "models/weapons/v_models/fif_%s_skins/%s",
                       comboData[c].folder, ansiText);

            if (!Q_stricmp(fullPath, skinPath))
            {
                initialItem = i;
                break;
            }
        }
        comboData[c].cb->SetInitialItem(initialItem);
    }
}

//-----------------------------------------------------------------------------
// OnApplyChanges
//-----------------------------------------------------------------------------
void CInventorySubRifles::OnApplyChanges()
{
    m_pLoadoutM4ComboBox->ApplyChanges();
    m_pLoadoutAK47ComboBox->ApplyChanges();
    m_pLoadoutAUGComboBox->ApplyChanges();
    m_pLoadoutFAMASComboBox->ApplyChanges();
    m_pLoadoutGalilComboBox->ApplyChanges();
    m_pLoadoutM4A1SSComboBox->ApplyChanges();
    m_pLoadoutSG556ComboBox->ApplyChanges();
}