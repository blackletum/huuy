//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "BasePanel.h"
#include "InventoryDialog.h"

#include "vgui_controls/Button.h"
#include "vgui_controls/CheckButton.h"
#include "vgui_controls/PropertySheet.h"
#include "vgui_controls/Label.h"
#include "vgui_controls/QueryBox.h"

#include "vgui/ILocalize.h"
#include "vgui/ISurface.h"
#include "vgui/ISystem.h"
#include "vgui/IVGui.h"

#include "KeyValues.h"
#include "InventorySubLoadout.h"
#include "ModInfo.h"

using namespace vgui;

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

//-----------------------------------------------------------------------------
// Purpose: Basic help dialog
//-----------------------------------------------------------------------------
CInventoryDialog::CInventoryDialog(vgui::Panel *parent) : PropertyDialog(parent, "InventoryDialog")
{
	SetDeleteSelfOnClose(true);

	int w = 512;
	int h = 406;
	if (IsProportional())
	{
		w = scheme()->GetProportionalScaledValueEx(GetScheme(), w);
		h = scheme()->GetProportionalScaledValueEx(GetScheme(), h);
	}

	SetBounds(0, 0, w, h);
	
	SetSizeable( false );

	SetTitle("#GameUI_Inventory_Dialog", true);

/*	AddPage(new CInventorySubKnifes(this), "#GameUI_Knifes");
	AddPage(new CInventorySubPistols(this), "#GameUI_Pistols");
	AddPage(new CInventorySubSMG(this), "#GameUI_SMG");
	AddPage(new CInventorySubHeavy(this), "#GameUI_Heavy");
	AddPage(new CInventorySubRifles(this), "#GameUI_Rifles");
	AddPage(new CInventorySubSnipers(this), "#GameUI_Snipers");*/
	AddPage(new CInventorySubLoadout(this), "#GameUI_Loadout");

	SetApplyButtonVisible(true);
	GetPropertySheet()->SetTabWidth(84);
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CInventoryDialog::~CInventoryDialog()
{
}

//-----------------------------------------------------------------------------
// Purpose: Brings the dialog to the fore
//-----------------------------------------------------------------------------
void CInventoryDialog::Activate()
{
	BaseClass::Activate();
	EnableApplyButton(false);
}

void CInventoryDialog::OnKeyCodePressed( KeyCode code )
{
	switch ( GetBaseButtonCode( code ) )
	{
	case KEY_XBUTTON_B:
		OnCommand( "Cancel" );
		return;
	}

	BaseClass::OnKeyCodePressed( code );
}

//-----------------------------------------------------------------------------
// Purpose: Opens the dialog
//-----------------------------------------------------------------------------
void CInventoryDialog::Run()
{
	SetTitle("#GameUI_Inventory_Dialog", true);
	Activate();
}

//-----------------------------------------------------------------------------
// Purpose: Called when the GameUI is hidden
//-----------------------------------------------------------------------------
void CInventoryDialog::OnGameUIHidden()
{
	// tell our children about it
	for ( int i = 0 ; i < GetChildCount() ; i++ )
	{
		Panel *pChild = GetChild( i );
		if ( pChild )
		{
			PostMessage( pChild, new KeyValues( "GameUIHidden" ) );
		}
	}
}