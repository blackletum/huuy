//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//


#if defined( WIN32 ) && !defined( _X360 )
#include <windows.h> // SRC only!!
#endif

#include "InventorySubKnifes.h"
#include <stdio.h>

#include <vgui_controls/Button.h>
#include "tier1/KeyValues.h"
#include <vgui_controls/Label.h>
#include <vgui/ISystem.h>
#include <vgui/ISurface.h>
#include <vgui_controls/ComboBox.h>

#include "CvarTextEntry.h"
#include "CvarToggleCheckButton.h"
#include "LabeledCommandComboBox.h"
#include "filesystem.h"
#include "EngineInterface.h"
#include "tier1/convar.h"

#include "GameUI_Interface.h"

#include "cs_shareddefs.h"

#if defined( _X360 )
#include "xbox/xbox_win32stubs.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

extern ConVar loadout_music;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

extern ConVar loadout_knife_bayonet_skin;
extern ConVar loadout_knife_butterfly_skin;
extern ConVar loadout_knife_cains_skin;
extern ConVar loadout_knife_cord_skin;
extern ConVar loadout_knife_css_skin;
extern ConVar loadout_knife_falshion_skin;
extern ConVar loadout_knife_flip_skin;
extern ConVar loadout_knife_gut_skin;
extern ConVar loadout_knife_gypsy_jackknife_skin;
extern ConVar loadout_knife_karambit_skin;
extern ConVar loadout_knife_m9_bayonet_skin;
extern ConVar loadout_knife_outdoor_skin;
extern ConVar loadout_knife_push_skin;
extern ConVar loadout_knife_skeleton_skin;
extern ConVar loadout_knife_stiletto_skin;
extern ConVar loadout_knife_survival_bowie_skin;
extern ConVar loadout_knife_tactical_skin;
extern ConVar loadout_knife_ursus_skin;
extern ConVar loadout_knife_widowmaker_skin;

CInventorySubKnifes::CInventorySubKnifes(vgui::Panel *parent)
    : vgui::PropertyPage(parent, "InventorySubKnifes")
{
    Button *cancel = new Button(this, "Cancel", "#GameUI_Cancel");
    cancel->SetCommand("Close");

    Button *ok = new Button(this, "OK", "#GameUI_OK");
    ok->SetCommand("Ok");

    Button *apply = new Button(this, "Apply", "#GameUI_Apply");
    apply->SetCommand("Apply");

    m_pBayonetComboBox = new CLabeledCommandComboBox(this, "BayonetComboBox");
    InitKnifeSkinList(m_pBayonetComboBox, "bayonet", &loadout_knife_bayonet_skin);

    m_pButterflyComboBox = new CLabeledCommandComboBox(this, "ButterflyComboBox");
    InitKnifeSkinList(m_pButterflyComboBox, "butterfly", &loadout_knife_butterfly_skin);

    m_pCainsComboBox = new CLabeledCommandComboBox(this, "CainsComboBox");
    InitKnifeSkinList(m_pCainsComboBox, "cains", &loadout_knife_cains_skin);

    m_pCordComboBox = new CLabeledCommandComboBox(this, "CordComboBox");
    InitKnifeSkinList(m_pCordComboBox, "cord", &loadout_knife_cord_skin);

    m_pCSSComboBox = new CLabeledCommandComboBox(this, "CSSComboBox");
    InitKnifeSkinList(m_pCSSComboBox, "css", &loadout_knife_css_skin);

    m_pFalshionComboBox = new CLabeledCommandComboBox(this, "FalshionComboBox");
    InitKnifeSkinList(m_pFalshionComboBox, "falshion", &loadout_knife_falshion_skin);

    m_pFlipComboBox = new CLabeledCommandComboBox(this, "FlipComboBox");
    InitKnifeSkinList(m_pFlipComboBox, "flip", &loadout_knife_flip_skin);

    m_pGutComboBox = new CLabeledCommandComboBox(this, "GutComboBox");
    InitKnifeSkinList(m_pGutComboBox, "gut", &loadout_knife_gut_skin);

    m_pGypsyComboBox = new CLabeledCommandComboBox(this, "GypsyComboBox");
    InitKnifeSkinList(m_pGypsyComboBox, "gypsy_jackknife", &loadout_knife_gypsy_jackknife_skin);

    m_pKarambitComboBox = new CLabeledCommandComboBox(this, "KarambitComboBox");
    InitKnifeSkinList(m_pKarambitComboBox, "karambit", &loadout_knife_karambit_skin);

    m_pM9ComboBox = new CLabeledCommandComboBox(this, "M9ComboBox");
    InitKnifeSkinList(m_pM9ComboBox, "m9_bayonet", &loadout_knife_m9_bayonet_skin);

    m_pOutdoorComboBox = new CLabeledCommandComboBox(this, "OutdoorComboBox");
    InitKnifeSkinList(m_pOutdoorComboBox, "outdoor", &loadout_knife_outdoor_skin);

    m_pPushComboBox = new CLabeledCommandComboBox(this, "PushComboBox");
    InitKnifeSkinList(m_pPushComboBox, "push", &loadout_knife_push_skin);

    m_pSkeletonComboBox = new CLabeledCommandComboBox(this, "SkeletonComboBox");
    InitKnifeSkinList(m_pSkeletonComboBox, "skeleton", &loadout_knife_skeleton_skin);

    m_pStilettoComboBox = new CLabeledCommandComboBox(this, "StilettoComboBox");
    InitKnifeSkinList(m_pStilettoComboBox, "stiletto", &loadout_knife_stiletto_skin);

    m_pSurvivalComboBox = new CLabeledCommandComboBox(this, "SurvivalComboBox");
    InitKnifeSkinList(m_pSurvivalComboBox, "survival_bowie", &loadout_knife_survival_bowie_skin);

    m_pTacticalComboBox = new CLabeledCommandComboBox(this, "TacticalComboBox");
    InitKnifeSkinList(m_pTacticalComboBox, "tactical", &loadout_knife_tactical_skin);

    m_pUrsusComboBox = new CLabeledCommandComboBox(this, "UrsusComboBox");
    InitKnifeSkinList(m_pUrsusComboBox, "ursus", &loadout_knife_ursus_skin);

    m_pWidowmakerComboBox = new CLabeledCommandComboBox(this, "WidowmakerComboBox");
    InitKnifeSkinList(m_pWidowmakerComboBox, "widowmaker", &loadout_knife_widowmaker_skin);

    m_pBayonetComboBox->AddActionSignalTarget(this);
    m_pButterflyComboBox->AddActionSignalTarget(this);
    m_pCainsComboBox->AddActionSignalTarget(this);
    m_pCordComboBox->AddActionSignalTarget(this);
    m_pCSSComboBox->AddActionSignalTarget(this);
    m_pFalshionComboBox->AddActionSignalTarget(this);
    m_pFlipComboBox->AddActionSignalTarget(this);
    m_pGutComboBox->AddActionSignalTarget(this);
    m_pGypsyComboBox->AddActionSignalTarget(this);
    m_pKarambitComboBox->AddActionSignalTarget(this);
    m_pM9ComboBox->AddActionSignalTarget(this);
    m_pOutdoorComboBox->AddActionSignalTarget(this);
    m_pPushComboBox->AddActionSignalTarget(this);
    m_pSkeletonComboBox->AddActionSignalTarget(this);
    m_pStilettoComboBox->AddActionSignalTarget(this);
    m_pSurvivalComboBox->AddActionSignalTarget(this);
    m_pTacticalComboBox->AddActionSignalTarget(this);
    m_pUrsusComboBox->AddActionSignalTarget(this);
    m_pWidowmakerComboBox->AddActionSignalTarget(this);

    LoadControlSettings("Resource/InventorySubKnifes.res");
}

CInventorySubKnifes::~CInventorySubKnifes()
{
}

void CInventorySubKnifes::InitKnifeSkinList(CLabeledCommandComboBox* cb, const char* knifeName, ConVar* pConVar)
{
    cb->DeleteAllItems();

    FileFindHandle_t fh;
    char pattern[256];
    Q_snprintf(pattern, sizeof(pattern), "materials/models/weapons/v_models/knife_%s_skins/*.vmt", knifeName);

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
                   "loadout_knife_%s_skin models/weapons/v_models/knife_%s_skins/%s",
                   knifeName, knifeName, displayName);

        cb->AddItem(displayName, command);

        char fullPath[512];
        Q_snprintf(fullPath, sizeof(fullPath),
                   "models/weapons/v_models/knife_%s_skins/%s", knifeName, displayName);

        if (!Q_stricmp(fullPath, currentSkin))
            initialItem = i;

        ++i;
        fn = g_pFullFileSystem->FindNext(fh);
    }

    g_pFullFileSystem->FindClose(fh);
    cb->SetInitialItem(initialItem);
}

void CInventorySubKnifes::OnControlModified()
{
    PostMessage(GetParent(), new KeyValues("ApplyButtonEnable"));
    InvalidateLayout();
}

void CInventorySubKnifes::OnApplyChanges()
{
    m_pBayonetComboBox->ApplyChanges();
    m_pButterflyComboBox->ApplyChanges();
    m_pCainsComboBox->ApplyChanges();
    m_pCordComboBox->ApplyChanges();
    m_pCSSComboBox->ApplyChanges();
    m_pFalshionComboBox->ApplyChanges();
    m_pFlipComboBox->ApplyChanges();
    m_pGutComboBox->ApplyChanges();
    m_pGypsyComboBox->ApplyChanges();
    m_pKarambitComboBox->ApplyChanges();
    m_pM9ComboBox->ApplyChanges();
    m_pOutdoorComboBox->ApplyChanges();
    m_pPushComboBox->ApplyChanges();
    m_pSkeletonComboBox->ApplyChanges();
    m_pStilettoComboBox->ApplyChanges();
    m_pSurvivalComboBox->ApplyChanges();
    m_pTacticalComboBox->ApplyChanges();
    m_pUrsusComboBox->ApplyChanges();
    m_pWidowmakerComboBox->ApplyChanges();
}

void CInventorySubKnifes::OnResetData()
{
    struct ComboSkinData
    {
        CLabeledCommandComboBox* cb;
        const char* convarName;
        const char* folder;
    };

    ComboSkinData comboData[] = {
        { m_pBayonetComboBox, "loadout_knife_bayonet_skin", "knife_bayonet_skins" },
        { m_pButterflyComboBox, "loadout_knife_butterfly_skin", "knife_butterfly_skins" },
        { m_pCainsComboBox, "loadout_knife_cains_skin", "knife_cains_skins" },
        { m_pCordComboBox, "loadout_knife_cord_skin", "knife_cord_skins" },
        { m_pCSSComboBox, "loadout_knife_css_skin", "knife_css_skins" },
        { m_pFalshionComboBox, "loadout_knife_falshion_skin", "knife_falshion_skins" },
        { m_pFlipComboBox, "loadout_knife_flip_skin", "knife_flip_skins" },
        { m_pGutComboBox, "loadout_knife_gut_skin", "knife_gut_skins" },
        { m_pGypsyComboBox, "loadout_knife_gypsy_jackknife_skin", "knife_gypsy_jackknife_skins" },
        { m_pKarambitComboBox, "loadout_knife_karambit_skin", "knife_karambit_skins" },
        { m_pM9ComboBox, "loadout_knife_m9_bayonet_skin", "knife_m9_bayonet_skins" },
        { m_pOutdoorComboBox, "loadout_knife_outdoor_skin", "knife_outdoor_skins" },
        { m_pPushComboBox, "loadout_knife_push_skin", "knife_push_skins" },
        { m_pSkeletonComboBox, "loadout_knife_skeleton_skin", "knife_skeleton_skins" },
        { m_pStilettoComboBox, "loadout_knife_stiletto_skin", "knife_stiletto_skins" },
        { m_pSurvivalComboBox, "loadout_knife_survival_bowie_skin", "knife_survival_bowie_skins" },
        { m_pTacticalComboBox, "loadout_knife_tactical_skin", "knife_tactical_skins" },
        { m_pUrsusComboBox, "loadout_knife_ursus_skin", "knife_ursus_skins" },
        { m_pWidowmakerComboBox, "loadout_knife_widowmaker_skin", "knife_widowmaker_skins" }
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
                       "models/weapons/v_models/knife_%s_skins/%s",
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