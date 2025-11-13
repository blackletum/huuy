//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef INVENTORYSUBHEAVY_H
#define INVENTORYSUBHEAVY_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/PropertyPage.h>

class CCvarToggleCheckButton;
class CLabeledCommandComboBox;

class CInventorySubHeavy;

// PiMoN: change this to zero if you want to
// show a MessageBox to player that they need
// to restart the game or disconnect from a server
#define INSTANT_MUSIC_CHANGE 1

//-----------------------------------------------------------------------------
// Purpose: crosshair options property page
//-----------------------------------------------------------------------------
class CInventorySubHeavy: public vgui::PropertyPage
{
	DECLARE_CLASS_SIMPLE( CInventorySubHeavy, vgui::PropertyPage );

public:
	CInventorySubHeavy( vgui::Panel *parent );
	~CInventorySubHeavy();

	MESSAGE_FUNC( OnControlModified, "ControlModified" );
private:
	// Called when page is loaded.  Data should be reloaded from document into controls.
	virtual void OnResetData();
	// Called when the OK / Apply button is pressed.  Changed data should be written into document.
	virtual void OnApplyChanges();
};

#endif // MODOPTIONSSUBLOADOUT_H