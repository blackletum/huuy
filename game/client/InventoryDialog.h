//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef INVENTORYDIALOG_H
#define INVENTORYDIALOG_H
#ifdef _WIN32
#pragma once
#endif

#include "vgui_controls/PropertyDialog.h"
#include "vgui_controls/KeyRepeat.h"

//-----------------------------------------------------------------------------
// Purpose: Holds all the game option pages
//-----------------------------------------------------------------------------
class CInventoryDialog : public vgui::PropertyDialog
{
	DECLARE_CLASS_SIMPLE( CInventoryDialog, vgui::PropertyDialog );

public:
	CInventoryDialog(vgui::Panel *parent);
	~CInventoryDialog();

	void Run();
	virtual void Activate();

	void OnKeyCodePressed( vgui::KeyCode code );

	MESSAGE_FUNC( OnGameUIHidden, "GameUIHidden" );	// called when the GameUI is hidden
};

#endif // OPTIONSDIALOG_H