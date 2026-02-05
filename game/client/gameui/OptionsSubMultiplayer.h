//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef OPTIONSSUBMULTIPLAYER_H
#define OPTIONSSUBMULTIPLAYER_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/PropertyPage.h>
#include <vgui_controls/ImagePanel.h>
#include "BitmapImagePanel.h"
#include "imageutils.h"

class CLabeledCommandComboBox;
//class CBitmapImagePanel;

class CCvarToggleCheckButton;
class CCvarTextEntry;
class CCvarSlider;

class CMultiplayerAdvancedDialog;

class COptionsSubMultiplayer;

//-----------------------------------------------------------------------------
// Purpose: multiplayer options property page
//-----------------------------------------------------------------------------
class COptionsSubMultiplayer : public vgui::PropertyPage
{
	DECLARE_CLASS_SIMPLE( COptionsSubMultiplayer, vgui::PropertyPage );

public:
	COptionsSubMultiplayer(vgui::Panel *parent);
	~COptionsSubMultiplayer();

	virtual vgui::Panel *CreateControlByName(const char *controlName);

	MESSAGE_FUNC( OnControlModified, "ControlModified" );

protected:
	// Called when page is loaded.  Data should be reloaded from document into controls.
	virtual void OnResetData();
	// Called when the OK / Apply button is pressed.  Changed data should be written into document.
	virtual void OnApplyChanges();

	virtual void OnCommand( const char *command );
public:
	void InitModelList(CLabeledCommandComboBox *cb);
	void RemapModel();

	void InitLogoList(CLabeledCommandComboBox *cb);
	void RemapLogo();
	void SelectLogo(const char *logoName);
	void OpenSprayImportDialog();
	void OnSprayFileSelected(const char *fullpath);
	void ShowSprayError(ConversionErrorType err);

	void InitAvatarList(CLabeledCommandComboBox *cb);
	void RemapAvatar();
	void SelectAvatar(const char *avatarName);
	void OpenAvatarImportDialog();
	void OnAvatarFileSelected(const char *fullpath);
	void ShowAvatarError(ConversionErrorType err);

	void ColorForName(char const *pszColorName, int &r, int &g, int &b);

	MESSAGE_FUNC_PTR( OnTextChanged, "TextChanged", panel );
	MESSAGE_FUNC_CHARPTR( OnFileSelected, "FileSelected", fullpath );
	
	CBitmapImagePanel *m_pModelImage = nullptr;
	CLabeledCommandComboBox *m_pModelList = nullptr;
	char m_ModelName[128] = {0};

	vgui::ImagePanel *m_pLogoImage = nullptr;
	CLabeledCommandComboBox *m_pLogoList = nullptr;
	char m_LogoName[128] = {0};
	vgui::FileOpenDialog *m_hImportSprayDialog = nullptr;

	vgui::ImagePanel *m_pAvatarImage = nullptr;
	CLabeledCommandComboBox *m_pAvatarList = nullptr;
	char m_AvatarName[128] = {0};
	vgui::FileOpenDialog *m_hImportAvatarDialog = nullptr;

	CCvarTextEntry *m_pNameEntry = nullptr;

	CCvarSlider *m_pPrimaryColorSlider = nullptr;
	CCvarSlider *m_pSecondaryColorSlider = nullptr;
	CCvarToggleCheckButton *m_pHighQualityModelCheckBox = nullptr;

	vgui::Dar< CCvarToggleCheckButton * > m_cvarToggleCheckButtons;

	CCvarToggleCheckButton *m_pLockRadarRotationCheckbox = nullptr;
	vgui::ComboBox *m_pDownloadFilterCombo = nullptr;

	int m_nLogoR = 255;
	int m_nLogoG = 255;
	int m_nLogoB = 255;
	
#ifndef _XBOX
	vgui::DHANDLE<CMultiplayerAdvancedDialog> m_hMultiplayerAdvancedDialog;
#endif
};

#endif // OPTIONSSUBMULTIPLAYER_H