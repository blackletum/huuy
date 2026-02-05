//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//


#if defined( WIN32 ) && !defined( _X360 )
#include <windows.h> // SRC only!!
#endif

#include "OptionsSubMultiplayer.h"
#include "MultiplayerAdvancedDialog.h"
#include <stdio.h>
#include "BasePanel.h"
#include <vgui_controls/Button.h>
#include <vgui_controls/QueryBox.h>
#include <vgui_controls/CheckButton.h>
#include "tier1/KeyValues.h"
#include <vgui_controls/Label.h>
#include <vgui/ISystem.h>
#include <vgui/ISurface.h>
#include <vgui/Cursor.h>
#include <vgui_controls/RadioButton.h>
#include <vgui_controls/ComboBox.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/FileOpenDialog.h>
#include <vgui_controls/MessageBox.h>
#include <vgui/IVGui.h>
#include <vgui/ILocalize.h>
#include <vgui/IPanel.h>

#include "CvarTextEntry.h"
#include "CvarToggleCheckButton.h"
#include "cvarslider.h"
#include "LabeledCommandComboBox.h"
#include "filesystem.h"
#include "EngineInterface.h"
#include "BitmapImagePanel.h"
#include "tier1/utlbuffer.h"
#include "ModInfo.h"
#include "tier1/convar.h"
#include "tier0/icommandline.h"
#include "tier0/platform.h"   // For va()

#include "materialsystem/imaterial.h"
#include "materialsystem/imesh.h"
#include "materialsystem/imaterialvar.h"

#define JPEGLIB_USE_STDIO
#include "jpeglib/jpeglib.h"
#undef JPEGLIB_USE_STDIO

#include <setjmp.h>
#include "bitmap/tgawriter.h"
#include "ivtex.h"
#ifdef WIN32
#include <io.h>
#endif

#if defined( _X360 )
#include "xbox/xbox_win32stubs.h"
#endif

#include <tier0/memdbgon.h>

using namespace vgui;

#define DEFAULT_SUIT_HUE 30
#define DEFAULT_PLATE_HUE 6
#define MODEL_MATERIAL_BASE_FOLDER "materials/vgui/playermodels/"

void UpdateLogoWAD(void *hdib, int r, int g, int b);

struct ColorItem_t
{
    const char *name;
    int r, g, b;
};

static ColorItem_t itemlist[] =
{
    { "#Valve_Orange", 255, 120, 24 },
    { "#Valve_Yellow", 225, 180, 24 },
    { "#Valve_Blue", 0, 60, 255 },
    { "#Valve_Ltblue", 0, 167, 255 },
    { "#Valve_Green", 0, 167, 0 },
    { "#Valve_Red", 255, 43, 0 },
    { "#Valve_Brown", 123, 73, 0 },
    { "#Valve_Ltgray", 100, 100, 100 },
    { "#Valve_Dkgray", 36, 36, 36 },
};

//-----------------------------------------------------------------------------
// COptionsSubMultiplayer
//-----------------------------------------------------------------------------

COptionsSubMultiplayer::COptionsSubMultiplayer(vgui::Panel *parent)
    : vgui::PropertyPage(parent, "OptionsSubMultiplayer")
{
    new Button(this, "Cancel", "#GameUI_Cancel", this, "Close");
    new Button(this, "OK", "#GameUI_OK", this, "Ok");
    new Button(this, "Apply", "#GameUI_Apply", this, "Apply");
    new Button(this, "Advanced", "#GameUI_AdvancedEllipsis", this, "Advanced");

    new Button(this, "ImportSprayImage", "#GameUI_ImportSprayEllipsis", this, "ImportSprayImage");
    new Button(this, "ImportAvatarImage", "#GameUI_ImportAvatarEllipsis", this, "ImportAvatarImage");

    m_pPrimaryColorSlider = new CCvarSlider(this, "Primary Color Slider", "#GameUI_PrimaryColor", 0.0f, 255.0f, "topcolor");
    m_pSecondaryColorSlider = new CCvarSlider(this, "Secondary Color Slider", "#GameUI_SecondaryColor", 0.0f, 255.0f, "bottomcolor");

    m_pHighQualityModelCheckBox = new CCvarToggleCheckButton(this, "High Quality Models", "#GameUI_HighModels", "cl_himodels");
    m_pLockRadarRotationCheckbox = new CCvarToggleCheckButton(this, "LockRadarRotationCheckbox", "#Cstrike_RadarLocked", "cl_radar_locked");

    m_pModelList = new CLabeledCommandComboBox(this, "Player model");
    m_pLogoList = new CLabeledCommandComboBox(this, "SpraypaintList");
    m_pAvatarList = new CLabeledCommandComboBox(this, "AvatarList");

    m_pDownloadFilterCombo = new ComboBox(this, "DownloadFilterCheck", 4, false);
    m_pDownloadFilterCombo->AddItem("#GameUI_DownloadFilter_ALL", NULL);
    m_pDownloadFilterCombo->AddItem("#GameUI_DownloadFilter_NoSounds", NULL);
    m_pDownloadFilterCombo->AddItem("#GameUI_DownloadFilter_MapsOnly", NULL);
    m_pDownloadFilterCombo->AddItem("#GameUI_DownloadFilter_None", NULL);

    m_pModelImage = new CBitmapImagePanel(this, "ModelImage", NULL);
    m_pModelImage->AddActionSignalTarget(this);

    m_pLogoImage = new ImagePanel(this, "LogoImage");
    m_pLogoImage->AddActionSignalTarget(this);

    m_pAvatarImage = new ImagePanel(this, "AvatarImage");
    m_pAvatarImage->AddActionSignalTarget(this);

    // === Имя игрока ===
    m_pNameEntry = new CCvarTextEntry(this, "NameEntry", "name");
    m_pNameEntry->AddActionSignalTarget(this);

    InitModelList(m_pModelList);
    InitLogoList(m_pLogoList);
    InitAvatarList(m_pAvatarList);

    g_pVGuiLocalize->AddFile("resource/playersettings_%language%.txt", "GAME", true);

    LoadControlSettings("Resource/OptionsSubMultiplayer.res");

    if (ModInfo().NoModels())
    {
        Panel *p = nullptr;
        if (m_pModelImage) m_pModelImage->SetVisible(false);
        if (m_pModelList) m_pModelList->SetVisible(false);
        if (m_pPrimaryColorSlider) m_pPrimaryColorSlider->SetVisible(false);
        if (m_pSecondaryColorSlider) m_pSecondaryColorSlider->SetVisible(false);
        if ((p = FindChildByName("Label1"))) p->SetVisible(false);
        if ((p = FindChildByName("Colors"))) p->SetVisible(false);
    }

    if (ModInfo().NoHiModel() && m_pHighQualityModelCheckBox)
        m_pHighQualityModelCheckBox->SetVisible(false);
}

COptionsSubMultiplayer::~COptionsSubMultiplayer() {}

//-----------------------------------------------------------------------------
// OnCommand
//-----------------------------------------------------------------------------
void COptionsSubMultiplayer::OnCommand(const char *command)
{
    if (!Q_stricmp(command, "ImportSprayImage"))
    {
        OpenSprayImportDialog();
    }
    else if (!Q_stricmp(command, "ImportAvatarImage"))
    {
        OpenAvatarImportDialog();
    }
    else if (!stricmp(command, "Advanced"))
    {
#ifndef _XBOX
        if (!m_hMultiplayerAdvancedDialog.Get())
            m_hMultiplayerAdvancedDialog = new CMultiplayerAdvancedDialog(this);
        m_hMultiplayerAdvancedDialog->Activate();
#endif
    }
    else if (!stricmp(command, "ResetStats"))
    {
        QueryBox *box = new QueryBox("#GameUI_ConfirmResetStatsTitle", "#GameUI_ConfirmResetStatsText", this);
        box->SetOKButtonText("#GameUI_Reset");
        box->SetOKCommand(new KeyValues("Command", "command", "ResetStats_NoConfirm"));
        box->SetCancelCommand(new KeyValues("Command", "command", "ReleaseModalWindow"));
        box->AddActionSignalTarget(this);
        box->DoModal();
    }
    else if (!stricmp(command, "ResetStats_NoConfirm"))
    {
        engine->ClientCmd_Unrestricted("stats_reset");
    }

    BaseClass::OnCommand(command);
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void COptionsSubMultiplayer::OpenSprayImportDialog()
{
    if (!m_hImportSprayDialog)
    {
        m_hImportSprayDialog = new FileOpenDialog(NULL, "#GameUI_ImportSprayImage", true);
#ifdef WIN32
        m_hImportSprayDialog->AddFilter("*.tga,*.jpg,*.bmp,*.vtf", "#GameUI_All_Images", true);
#else
        m_hImportSprayDialog->AddFilter("*.tga,*.jpg,*.vtf", "#GameUI_All_ImagesNoBmp", true);
#endif
        m_hImportSprayDialog->AddFilter("*.tga", "#GameUI_TGA_Images", false);
        m_hImportSprayDialog->AddFilter("*.jpg", "#GameUI_JPEG_Images", false);
#ifdef WIN32
        m_hImportSprayDialog->AddFilter("*.bmp", "#GameUI_BMP_Images", false);
#endif
        m_hImportSprayDialog->AddFilter("*.vtf", "#GameUI_VTF_Images", false);
        m_hImportSprayDialog->AddActionSignalTarget(this);
    }
    m_hImportSprayDialog->DoModal(false);
    m_hImportSprayDialog->Activate();
}

void COptionsSubMultiplayer::OpenAvatarImportDialog()
{
    if (!m_hImportAvatarDialog)
    {
        m_hImportAvatarDialog = new FileOpenDialog(NULL, "#GameUI_ImportAvatarImage", true);
        m_hImportAvatarDialog->AddFilter(IsPosix() ? "*.tga,*.jpg,*.vtf" : "*.tga,*.jpg,*.bmp,*.vtf", "#GameUI_All_Images", true);
        m_hImportAvatarDialog->AddFilter("*.tga", "#GameUI_TGA_Images", false);
        m_hImportAvatarDialog->AddFilter("*.jpg", "#GameUI_JPEG_Images", false);
#ifdef WIN32
        m_hImportAvatarDialog->AddFilter("*.bmp", "#GameUI_BMP_Images", false);
#endif
        m_hImportAvatarDialog->AddFilter("*.vtf", "#GameUI_VTF_Images", false);
        m_hImportAvatarDialog->AddActionSignalTarget(this);
    }
    m_hImportAvatarDialog->DoModal(false);
    m_hImportAvatarDialog->Activate();
}

//-----------------------------------------------------------------------------
// OnFileSelected 
//-----------------------------------------------------------------------------
void COptionsSubMultiplayer::OnFileSelected(const char *fullpath)
{
    if (m_hImportAvatarDialog && m_hImportAvatarDialog->IsVisible())
        OnAvatarFileSelected(fullpath);
    else if (m_hImportSprayDialog && m_hImportSprayDialog->IsVisible())
        OnSprayFileSelected(fullpath);
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void COptionsSubMultiplayer::OnSprayFileSelected(const char *fullpath)
{
#ifndef _XBOX
    surface()->SetCursor(dc_hourglass);
    ConversionErrorType err = ImgUtl_ConvertToVTFAndDumpVMT(fullpath, IsPosix() ? "/vgui/logos" : "\\vgui\\logos", 256, 256);
    if (err == CE_SUCCESS)
    {
        InitLogoList(m_pLogoList);
        char base[MAX_PATH];
        V_FileBase(fullpath, base, sizeof(base));
        SelectLogo(base);
    }
    else
    {
        ShowSprayError(err);
    }
    surface()->SetCursor(dc_user);
#endif
}

void COptionsSubMultiplayer::ShowSprayError(ConversionErrorType err)
{
    const char *text = nullptr;
    switch (err)
    {
    case CE_MEMORY_ERROR: text = "#GameUI_Spray_Import_Error_Memory"; break;
    case CE_CANT_OPEN_SOURCE_FILE: text = "#GameUI_Spray_Import_Error_Reading_Image"; break;
    case CE_ERROR_PARSING_SOURCE: text = "#GameUI_Spray_Import_Error_Image_File_Corrupt"; break;
    case CE_SOURCE_FILE_SIZE_NOT_SUPPORTED:
    case CE_SOURCE_FILE_FORMAT_NOT_SUPPORTED: text = "#GameUI_Spray_Import_Image_Wrong_Size"; break;
    case CE_ERROR_WRITING_OUTPUT_FILE: text = "#GameUI_Spray_Import_Error_Writing_Temp_Output"; break;
    case CE_ERROR_LOADING_DLL: text = "#GameUI_Spray_Import_Error_Cant_Load_VTEX_DLL"; break;
    }
    if (text)
    {
        MessageBox *dlg = new MessageBox("#GameUI_Spray_Import_Error_Title", text);
        dlg->DoModal();
    }
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
void COptionsSubMultiplayer::OnAvatarFileSelected(const char *fullpath)
{
    surface()->SetCursor(dc_hourglass);
    ConversionErrorType err = ImgUtl_ConvertToVTFAndDumpVMT(fullpath, IsPosix() ? "/vgui/avatars" : "\\vgui\\avatars", 256, 256);
    if (err == CE_SUCCESS)
    {
        InitAvatarList(m_pAvatarList);
        char base[MAX_PATH];
        V_FileBase(fullpath, base, sizeof(base));
        SelectAvatar(base);
    }
    else
    {
        ShowAvatarError(err);
    }
    surface()->SetCursor(dc_user);
}

void COptionsSubMultiplayer::ShowAvatarError(ConversionErrorType err)
{
    const char *text = nullptr;
    switch (err)
    {
    case CE_MEMORY_ERROR: text = "#GameUI_Avatar_Import_Error_Memory"; break;
    case CE_CANT_OPEN_SOURCE_FILE: text = "#GameUI_Avatar_Import_Error_Reading_Image"; break;
    case CE_ERROR_PARSING_SOURCE: text = "#GameUI_Avatar_Import_Error_Image_File_Corrupt"; break;
    case CE_SOURCE_FILE_SIZE_NOT_SUPPORTED:
    case CE_SOURCE_FILE_FORMAT_NOT_SUPPORTED: text = "#GameUI_Avatar_Import_Image_Wrong_Size"; break;
    case CE_ERROR_WRITING_OUTPUT_FILE: text = "#GameUI_Avatar_Import_Error_Writing_Temp_Output"; break;
    case CE_ERROR_LOADING_DLL: text = "#GameUI_Avatar_Import_Error_Cant_Load_VTEX_DLL"; break;
    }
    if (text)
    {
        MessageBox *dlg = new MessageBox("#GameUI_Avatar_Import_Error_Title", text);
        dlg->DoModal();
    }
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void COptionsSubMultiplayer::InitLogoList(CLabeledCommandComboBox *cb)
{
    FileFindHandle_t fh;
    char dir[512];
    ConVarRef cl_logofile("cl_logofile", true);
    if (!cl_logofile.IsValid()) return;

    cb->DeleteAllItems();
    Q_snprintf(dir, sizeof(dir), "materials/vgui/logos/*.vtf");
    const char *fn = g_pFullFileSystem->FindFirst(dir, &fh);
    int i = 0, init = 0;
    while (fn)
    {
        char path[512];
        Q_snprintf(path, sizeof(path), "materials/vgui/logos/%s", fn);
        if (strlen(path) >= 4)
        {
            Q_strncpy(path + strlen(path) - 4, ".vmt", 5);
            if (g_pFullFileSystem->FileExists(path))
            {
                Q_strncpy(path, fn, sizeof(path));
                path[strlen(path) - 4] = 0;
                cb->AddItem(path, "");
                char fullpath[512];
                Q_snprintf(fullpath, sizeof(fullpath), "materials/vgui/logos/%s", fn);
                if (!Q_stricmp(fullpath, cl_logofile.GetString()))
                init = i;
                ++i;
            }
        }
        fn = g_pFullFileSystem->FindNext(fh);
    }
    g_pFullFileSystem->FindClose(fh);
    cb->SetInitialItem(init);
}

void COptionsSubMultiplayer::InitAvatarList(CLabeledCommandComboBox *cb)
{
    FileFindHandle_t fh;
    char dir[512];
    ConVarRef cl_avatarfile("cl_avatarfile", true);
    if (!cl_avatarfile.IsValid()) return;

    cb->DeleteAllItems();
    Q_snprintf(dir, sizeof(dir), "materials/vgui/avatars/*.vtf");
    const char *fn = g_pFullFileSystem->FindFirst(dir, &fh);
    int i = 0, init = 0;
    while (fn)
    {
        char path[512];
        Q_snprintf(path, sizeof(path), "materials/vgui/avatars/%s", fn);
        if (strlen(path) >= 4)
        {
            Q_strncpy(path + strlen(path) - 4, ".vmt", 5);
            if (g_pFullFileSystem->FileExists(path))
            {
                Q_strncpy(path, fn, sizeof(path));
                path[strlen(path) - 4] = 0;
                cb->AddItem(path, "");
                char fullpath[512];
                Q_snprintf(fullpath, sizeof(fullpath), "materials/vgui/avatars/%s", fn);
                if (!Q_stricmp(fullpath, cl_avatarfile.GetString()))
                init = i;
                ++i;
            }
        }
        fn = g_pFullFileSystem->FindNext(fh);
    }
    g_pFullFileSystem->FindClose(fh);
    cb->SetInitialItem(init);
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void COptionsSubMultiplayer::SelectLogo(const char *name)
{
    int n = m_pLogoList->GetItemCount();
    wchar_t text[256], want[256];
    g_pVGuiLocalize->ConvertANSIToUnicode(name, want, sizeof(want));
    for (int i = 0; i < n; ++i)
    {
        m_pLogoList->GetItemText(i, text, sizeof(text));
        if (!wcscmp(text, want)) { m_pLogoList->ActivateItem(i); break; }
    }
}

void COptionsSubMultiplayer::SelectAvatar(const char *name)
{
    int n = m_pAvatarList->GetItemCount();
    wchar_t text[256], want[256];
    g_pVGuiLocalize->ConvertANSIToUnicode(name, want, sizeof(want));
    for (int i = 0; i < n; ++i)
    {
        m_pAvatarList->GetItemText(i, text, sizeof(text));
        if (!wcscmp(text, want)) { m_pAvatarList->ActivateItem(i); break; }
    }
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void COptionsSubMultiplayer::RemapLogo()
{
    char name[256];
    m_pLogoList->GetText(name, sizeof(name));
    if (!name[0]) return;

    g_pFullFileSystem->CreateDirHierarchy("materials/VGUI/logos/UI", "GAME");
    char vmt[512];
    Q_snprintf(vmt, sizeof(vmt), "materials/VGUI/logos/UI/%s.vmt", name);
    if (!g_pFullFileSystem->FileExists(vmt))
    {
        FileHandle_t fp = g_pFullFileSystem->Open(vmt, "wb");
        if (fp)
        {
            char data[1024];
            Q_snprintf(data, sizeof(data),
                "\"UnlitGeneric\"\n{\n\t\"$translucent\" 1\n\t\"$basetexture\" \"VGUI/logos/%s\"\n\t\"$vertexcolor\" 1\n\t\"$vertexalpha\" 1\n\t\"$no_fullbright\" 1\n\t\"$ignorez\" 1\n}\n",
                name);
            g_pFullFileSystem->Write(data, strlen(data), fp);
            g_pFullFileSystem->Close(fp);
        }
    }
    Q_snprintf(vmt, sizeof(vmt), "logos/UI/%s", name);
    m_pLogoImage->SetImage(vmt);
}

void COptionsSubMultiplayer::RemapAvatar()
{
    char name[256];
    m_pAvatarList->GetText(name, sizeof(name));
    if (!name[0]) return;

    g_pFullFileSystem->CreateDirHierarchy("materials/vgui/avatars", "GAME");
    char vmt[512];
    Q_snprintf(vmt, sizeof(vmt), "materials/vgui/avatars/%s.vmt", name);
    if (!g_pFullFileSystem->FileExists(vmt))
    {
        FileHandle_t fp = g_pFullFileSystem->Open(vmt, "wb");
        if (fp)
        {
            char data[1024];
            Q_snprintf(data, sizeof(data),
                "\"UnlitGeneric\"\n{\n\t\"$translucent\" 1\n\t\"$basetexture\" \"vgui/avatars/%s\"\n\t\"$vertexcolor\" 1\n\t\"$vertexalpha\" 1\n\t\"$no_fullbright\" 1\n\t\"$ignorez\" 1\n}\n",
                name);
            g_pFullFileSystem->Write(data, strlen(data), fp);
            g_pFullFileSystem->Close(fp);
        }
    }
    Q_snprintf(vmt, sizeof(vmt), "avatars/%s", name);
    m_pAvatarImage->SetImage(vmt);
}

void COptionsSubMultiplayer::RemapModel()
{
    const char *cmd = m_pModelList->GetActiveItemCommand();
    if (!cmd) return;
    char tex[256];
    Q_snprintf(tex, sizeof(tex), "vgui/playermodels/%s", cmd);
    tex[strlen(tex) - 4] = 0;
    m_pModelImage->setTexture(tex);
}

//-----------------------------------------------------------------------------
// OnTextChanged
//-----------------------------------------------------------------------------
void COptionsSubMultiplayer::OnTextChanged(vgui::Panel *panel)
{
    if (panel == m_pModelList) RemapModel();
    else if (panel == m_pLogoList) RemapLogo();
    else if (panel == m_pAvatarList) RemapAvatar();
}

//-----------------------------------------------------------------------------
// OnControlModified
//-----------------------------------------------------------------------------
void COptionsSubMultiplayer::OnControlModified()
{
    PostMessage(GetParent(), new KeyValues("ApplyButtonEnable"));
    InvalidateLayout();
}

//-----------------------------------------------------------------------------
// OnApplyChanges
//-----------------------------------------------------------------------------
void COptionsSubMultiplayer::OnApplyChanges()
{
    if (m_pNameEntry)
        m_pNameEntry->ApplyChanges();

    if (m_pAvatarList)
    {
        m_pAvatarList->ApplyChanges();
        m_pAvatarList->GetText(m_AvatarName, sizeof(m_AvatarName));

        char avatarPath[512];
        if (m_AvatarName[0])
            Q_snprintf(avatarPath, sizeof(avatarPath), "materials/vgui/avatars/%s.vtf", m_AvatarName);
        else
            Q_strncpy(avatarPath, "", sizeof(avatarPath));

        char cmd[512];
        Q_snprintf(cmd, sizeof(cmd), "cl_avatarfile %s\n", avatarPath);
        engine->ClientCmd_Unrestricted(cmd);
    }
    
    CBaseModPanel* pBasePanel = BasePanel();
	if (pBasePanel)
	{
		pBasePanel->UpdateAvatarImage();
	}

    if (m_pLogoList)
    {
        m_pLogoList->ApplyChanges();
        m_pLogoList->GetText(m_LogoName, sizeof(m_LogoName));

        char logoPath[512];
        if (m_LogoName[0])
            Q_snprintf(logoPath, sizeof(logoPath), "materials/vgui/logos/%s.vtf", m_LogoName);
        else
            Q_strncpy(logoPath, "", sizeof(logoPath));

        char cmd[512];
        Q_snprintf(cmd, sizeof(cmd), "cl_logofile %s\n", logoPath);
        engine->ClientCmd_Unrestricted(cmd);
    }

    if (m_pPrimaryColorSlider) m_pPrimaryColorSlider->ApplyChanges();
    if (m_pSecondaryColorSlider) m_pSecondaryColorSlider->ApplyChanges();

    if (m_pHighQualityModelCheckBox) m_pHighQualityModelCheckBox->ApplyChanges();
    if (m_pLockRadarRotationCheckbox) m_pLockRadarRotationCheckbox->ApplyChanges();

    for (int i = 0; i < m_cvarToggleCheckButtons.GetCount(); ++i)
    {
        CCvarToggleCheckButton *b = m_cvarToggleCheckButtons[i];
        if (b && b->IsVisible() && b->IsEnabled())
            b->ApplyChanges();
    }

    if (m_pModelList && m_pModelList->IsVisible() && m_pModelList->GetActiveItemCommand())
    {
        Q_strncpy(m_ModelName, m_pModelList->GetActiveItemCommand(), sizeof(m_ModelName));
        Q_StripExtension(m_ModelName, m_ModelName, sizeof(m_ModelName));

        char cmd[512];
        Q_snprintf(cmd, sizeof(cmd), "cl_playermodel models/%s.mdl\n", m_ModelName);
        engine->ClientCmd_Unrestricted(cmd);
    }

    if (m_pDownloadFilterCombo)
    {
        ConVarRef cl_downloadfilter("cl_downloadfilter");
        switch (m_pDownloadFilterCombo->GetActiveItem())
        {
        case 0: cl_downloadfilter.SetValue("all"); break;
        case 1: cl_downloadfilter.SetValue("nosounds"); break;
        case 2: cl_downloadfilter.SetValue("mapsonly"); break;
        case 3: cl_downloadfilter.SetValue("none"); break;
        }
    }

#if defined(GAME_TF2CLASSIC)
    engine->ClientCmd_Unrestricted("tf2c_mainmenu_reload\n");
#endif
}

//-----------------------------------------------------------------------------
// CreateControlByName
//-----------------------------------------------------------------------------
Panel *COptionsSubMultiplayer::CreateControlByName(const char *controlName)
{
    if (!Q_stricmp("CCvarToggleCheckButton", controlName))
    {
        CCvarToggleCheckButton *btn = new CCvarToggleCheckButton(this, controlName, "", "");
        m_cvarToggleCheckButtons.AddElement(btn);
        return btn;
    }
    return BaseClass::CreateControlByName(controlName);
}

//-----------------------------------------------------------------------------
// InitModelList 
//-----------------------------------------------------------------------------
void StripStringOutOfString(const char *pPattern, const char *pIn, char *pOut)
{
    int iLengthBase = strlen(pPattern);
    int iLengthString = strlen(pIn);
    int k = 0;

    for (int j = iLengthBase; j < iLengthString; j++)
    {
        pOut[k++] = pIn[j];
    }
    pOut[k] = 0;
}

void FindVMTFilesInFolder(const char *pFolder, const char *pFolderName, CLabeledCommandComboBox *cb, int &iCount, int &iInitialItem)
{
    ConVarRef cl_modelfile("cl_playermodel", true);
    if (!cl_modelfile.IsValid()) return;

    char directory[512];
    Q_snprintf(directory, sizeof(directory), "%s/*.*", pFolder);

    FileFindHandle_t fh;
    const char *fn = g_pFullFileSystem->FindFirst(directory, &fh);
    const char *modelfile = cl_modelfile.GetString();

    while (fn)
    {
        if (!stricmp(fn, ".") || !stricmp(fn, ".."))
        {
            fn = g_pFullFileSystem->FindNext(fh);
            continue;
        }

        if (g_pFullFileSystem->FindIsDirectory(fh))
        {
            char folderpath[512];
            Q_snprintf(folderpath, sizeof(folderpath), "%s/%s", pFolder, fn);
            FindVMTFilesInFolder(folderpath, fn, cb, iCount, iInitialItem);
            fn = g_pFullFileSystem->FindNext(fh);
            continue;
        }

        if (!strstr(fn, ".vmt"))
        {
            fn = g_pFullFileSystem->FindNext(fh);
            continue;
        }

        char filename[512];
        Q_snprintf(filename, sizeof(filename), "%s/%s", pFolder, fn);
        if (strlen(filename) >= 4)
        {
            filename[strlen(filename) - 4] = 0;
            Q_strncat(filename, ".vmt", sizeof(filename), COPY_ALL_CHARACTERS);
            if (g_pFullFileSystem->FileExists(filename))
            {
                char displayname[512];
                char texturepath[512];
                Q_strncpy(displayname, fn, sizeof(displayname));
                StripStringOutOfString(MODEL_MATERIAL_BASE_FOLDER, filename, texturepath);
                displayname[strlen(displayname) - 4] = 0;

                if (CORRECT_PATH_SEPARATOR == texturepath[0])
                    cb->AddItem(displayname, texturepath + 1);
                else
                    cb->AddItem(displayname, texturepath);

                char realname[512];
                Q_FileBase(modelfile, realname, sizeof(realname));
                Q_FileBase(filename, filename, sizeof(filename));

                if (!stricmp(filename, realname))
                    iInitialItem = iCount;

                ++iCount;
            }
        }

        fn = g_pFullFileSystem->FindNext(fh);
    }
}

void COptionsSubMultiplayer::InitModelList(CLabeledCommandComboBox *cb)
{
    int i = 0, initialItem = 0;
    cb->DeleteAllItems();
    FindVMTFilesInFolder(MODEL_MATERIAL_BASE_FOLDER, "", cb, i, initialItem);
    cb->SetInitialItem(initialItem);
}

//-----------------------------------------------------------------------------
// PaletteHueReplace 
//-----------------------------------------------------------------------------
#ifdef POSIX
typedef struct tagRGBQUAD {
    uint8 rgbBlue;
    uint8 rgbGreen;
    uint8 rgbRed;
    uint8 rgbReserved;
} RGBQUAD;
#endif

static void PaletteHueReplace(RGBQUAD *palSrc, int newHue, int Start, int end)
{
    for (int i = Start; i <= end; i++)
    {
        float r = palSrc[i].rgbRed / 255.0f;
        float g = palSrc[i].rgbGreen / 255.0f;
        float b = palSrc[i].rgbBlue / 255.0f;

        float maxcol = max(max(r, g), b);
        float mincol = min(min(r, g), b);
        float val = maxcol;
        float sat = maxcol > 0.0f ? (maxcol - mincol) / maxcol : 0.0f;
        float hue = (float)(newHue * (360.0 / 255));

        mincol = val * (1.0f - sat);

        float nr, ng, nb;
        if (hue <= 120)
        {
            nb = mincol;
            if (hue < 60)
            {
                nr = val;
                ng = mincol + hue * (val - mincol) / (120 - hue);
            }
            else
            {
                ng = val;
                nr = mincol + (120 - hue) * (val - mincol) / hue;
            }
        }
        else if (hue <= 240)
        {
            nr = mincol;
            if (hue < 180)
            {
                ng = val;
                nb = mincol + (hue - 120) * (val - mincol) / (240 - hue);
            }
            else
            {
                nb = val;
                ng = mincol + (240 - hue) * (val - mincol) / (hue - 120);
            }
        }
        else
        {
            ng = mincol;
            if (hue < 300)
            {
                nb = val;
                nr = mincol + (hue - 240) * (val - mincol) / (360 - hue);
            }
            else
            {
                nr = val;
                nb = mincol + (360 - hue) * (val - mincol) / (hue - 240);
            }
        }

        palSrc[i].rgbRed = (unsigned char)(nr * 255);
        palSrc[i].rgbGreen = (unsigned char)(ng * 255);
        palSrc[i].rgbBlue = (unsigned char)(nb * 255);
    }
}

//-----------------------------------------------------------------------------
// ColorForName 
//-----------------------------------------------------------------------------
void COptionsSubMultiplayer::ColorForName(char const *pszColorName, int &r, int &g, int &b)
{
    r = g = b = 0;
    int count = sizeof(itemlist) / sizeof(itemlist[0]);

    for (int i = 0; i < count; i++)
    {
        if (!Q_strnicmp(pszColorName, itemlist[i].name, strlen(itemlist[i].name)))
        {
            r = itemlist[i].r;
            g = itemlist[i].g;
            b = itemlist[i].b;
            return;
        }
    }
}

//-----------------------------------------------------------------------------
// OnResetData 
//-----------------------------------------------------------------------------
void COptionsSubMultiplayer::OnResetData()
{
    if (m_pDownloadFilterCombo)
    {
        ConVarRef cl_downloadfilter("cl_downloadfilter");
        const char *val = cl_downloadfilter.GetString();

        if (!Q_stricmp(val, "none")) m_pDownloadFilterCombo->ActivateItem(3);
        else if (!Q_stricmp(val, "nosounds")) m_pDownloadFilterCombo->ActivateItem(1);
        else if (!Q_stricmp(val, "mapsonly")) m_pDownloadFilterCombo->ActivateItem(2);
        else m_pDownloadFilterCombo->ActivateItem(0);
    }
}

//-----------------------------------------------------------------------------
// JPEG Error Handler 
//-----------------------------------------------------------------------------
struct ValveJpegErrorHandler_t
{
    struct jpeg_error_mgr m_Base;
    jmp_buf m_ErrorContext;
};

static void ValveJpegErrorHandler(j_common_ptr cinfo)
{
    ValveJpegErrorHandler_t *pError = (ValveJpegErrorHandler_t*)cinfo->err;
    char buffer[JMSG_LENGTH_MAX];
    (*cinfo->err->format_message)(cinfo, buffer);
    Warning("%s\n", buffer);
    longjmp(pError->m_ErrorContext, 1);
}