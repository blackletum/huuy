//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef INVENTORYSUBKNIFES_H
#define INVENTORYSUBKNIFES_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/PropertyPage.h>

class CCvarToggleCheckButton;
class CLabeledCommandComboBox;

class CInventorySubKnifes;

// PiMoN: change this to zero if you want to
// show a MessageBox to player that they need
// to restart the game or disconnect from a server
#define INSTANT_MUSIC_CHANGE 1

//-----------------------------------------------------------------------------
// Purpose: crosshair options property page
//-----------------------------------------------------------------------------
class CInventorySubKnifes: public vgui::PropertyPage
{
	DECLARE_CLASS_SIMPLE( CInventorySubKnifes, vgui::PropertyPage );

public:
	CInventorySubKnifes( vgui::Panel *parent );
	~CInventorySubKnifes();

	MESSAGE_FUNC( OnControlModified, "ControlModified" );
private:
	// Called when page is loaded.  Data should be reloaded from document into controls.
	virtual void OnResetData();
	// Called when the OK / Apply button is pressed.  Changed data should be written into document.
	virtual void OnApplyChanges();
	void InitKnifeSkinList(CLabeledCommandComboBox* cb, const char* knifeName, ConVar* pConVar);
	
     CLabeledCommandComboBox* m_pBayonetComboBox;
     CLabeledCommandComboBox* m_pButterflyComboBox;
     CLabeledCommandComboBox* m_pCainsComboBox;
     CLabeledCommandComboBox* m_pCordComboBox;
     CLabeledCommandComboBox* m_pCSSComboBox;
     CLabeledCommandComboBox* m_pFalshionComboBox;
     CLabeledCommandComboBox* m_pFlipComboBox;
     CLabeledCommandComboBox* m_pGutComboBox;
     CLabeledCommandComboBox* m_pGypsyComboBox;
     CLabeledCommandComboBox* m_pKarambitComboBox;
     CLabeledCommandComboBox* m_pM9ComboBox;
     CLabeledCommandComboBox* m_pOutdoorComboBox;
     CLabeledCommandComboBox* m_pPushComboBox;
     CLabeledCommandComboBox* m_pSkeletonComboBox;
     CLabeledCommandComboBox* m_pStilettoComboBox;
     CLabeledCommandComboBox* m_pSurvivalComboBox;
     CLabeledCommandComboBox* m_pTacticalComboBox;
     CLabeledCommandComboBox* m_pUrsusComboBox;
     CLabeledCommandComboBox* m_pWidowmakerComboBox;
};

#endif // MODOPTIONSSUBLOADOUT_H