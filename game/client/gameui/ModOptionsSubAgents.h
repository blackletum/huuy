//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef MODOPTIONSSUBAGENTS_H
#define MODOPTIONSSUBAGENTS_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/PropertyPage.h>
#include "PlayerModelPanel.h"
#include <vgui_controls/Button.h>
#include "vgui_controls/EditablePanel.h"
#include "vgui_controls/ScrollableEditablePanel.h"
#include "VGuiMatSurface/IMatSystemSurface.h"
#include "vgui/IInput.h"
#include "vgui_controls/Label.h"

class CLabeledCommandComboBox;
class CBitmapImagePanel;

class CModOptionsSubAgents;

class ImageButton : public vgui::Button
{
    DECLARE_CLASS_SIMPLE( ImageButton, vgui::Button );

public:
    ImageButton( vgui::Panel *parent, const char *imageName );

    virtual void Paint() override;
    virtual void OnMousePressed(vgui::MouseCode code) override;
    virtual void OnMouseReleased(vgui::MouseCode code) override;

    void SetImage( const char *imageName );
    void SetSelected( bool selected ) { m_bSelected = selected; }
    bool IsSelected() const { return m_bSelected; }

private:
    int  m_textureID;
    bool m_bSelected;
};

//-----------------------------------------------------------------------------
// Purpose: crosshair options property page
//-----------------------------------------------------------------------------
struct AgentButton
{
    ImageButton *button;
    vgui::Label *label;
};

class CImageButtonContainer : public vgui::EditablePanel
{
    DECLARE_CLASS_SIMPLE( CImageButtonContainer, vgui::EditablePanel );

public:
    CImageButtonContainer( vgui::Panel *pParent, const char *pName )
        : BaseClass(pParent, pName), m_nColumns(4), m_nSpacing(5) {}
        
    CUtlVector<AgentButton> m_Buttons;

    void AddImageButton(const char *imageName, const char *command, const char *labelText, vgui::Panel *actionTarget)
    {
        AgentButton ab;

        ab.button = new ImageButton(this, imageName);
        ab.button->SetCommand(command);
        
        if (actionTarget)
        {
            ab.button->AddActionSignalTarget(actionTarget);
        }

        vgui::IScheme* pScheme = vgui::scheme()->GetIScheme(GetScheme());
        vgui::HFont hFont = pScheme->GetFont("InventorySmall", true);

        ab.label = new vgui::Label(this, "", labelText);
        ab.label->SetFgColor(Color(255, 255, 255, 255));
        ab.label->SetFont(hFont);
        ab.label->SetContentAlignment(vgui::Label::a_northwest);  
        ab.label->SetWrap(true);

        m_Buttons.AddToTail(ab);

        InvalidateLayout();
    }

    void RemoveAll()
    {
        for (int i = 0; i < m_Buttons.Count(); i++)
        {
            if (m_Buttons[i].button)
                m_Buttons[i].button->MarkForDeletion();
            if (m_Buttons[i].label)
                m_Buttons[i].label->MarkForDeletion();
        }
        m_Buttons.RemoveAll();
    }

    virtual void PerformLayout() override
    {
        int buttonWide = 288;
        int buttonTall = 192;
        int spacingX = 5;
        int spacingY = buttonTall / 2; 
        int maxColumns = 4;

        for (int i = 0; i < m_Buttons.Count(); ++i)
        {
            int col = i % maxColumns;
            int row = i / maxColumns;

            int x = spacingX + col * (buttonWide + spacingX);
            int y = row * int(buttonTall * 1.9);
            
            m_Buttons[i].button->SetBounds(x, y, buttonWide, buttonTall);

            int labelTall = buttonTall;
            m_Buttons[i].label->SetBounds(x, y + buttonTall, buttonWide, labelTall);
        }

        int numRows = (m_Buttons.Count() + maxColumns - 1) / maxColumns;
        SetTall(numRows * int(buttonTall * 1.5));
    }
    
    int m_nColumns;
    int m_nSpacing;
};

class CModOptionsSubAgents: public vgui::PropertyPage
{
	DECLARE_CLASS_SIMPLE( CModOptionsSubAgents, vgui::PropertyPage );

public:
	CModOptionsSubAgents( vgui::Panel *parent );
	~CModOptionsSubAgents();

	MESSAGE_FUNC( OnControlModified, "ControlModified" );
	MESSAGE_FUNC_PTR( OnTextChanged, "TextChanged", panel );

protected:
	// Called when page is loaded.  Data should be reloaded from document into controls.
	virtual void OnResetData();
	// Called when the OK / Apply button is pressed.  Changed data should be written into document.
	virtual void OnApplyChanges();
	virtual void OnCommand( const char *command ) override;

private:
    void                    UpdateAgentModel();
    void                    PopulateAgentButtons();
    void                    UpdateAgentImages();
	void					RemapAgentsImage();
	CBitmapImagePanel		*m_pAgentImageCT;
	CBitmapImagePanel		*m_pAgentImageT;
	
	CBasePlayerModelPanel* m_pPlayerModel;
	int m_iCTAgent;
	int m_iTAgent;
	int m_iCTGloves;
	int m_iTGloves;
	int m_iCTWeapon;
	int m_iTWeapon;
	int m_iAgentToUse;

	CLabeledCommandComboBox *m_pLoadoutAgentCTComboBox;
	CLabeledCommandComboBox *m_pLoadoutAgentTComboBox;
	CLabeledCommandComboBox *m_pLoadoutMainMenuWeaponCTComboBox;
	CLabeledCommandComboBox * m_pLoadoutMainMenuWeaponTComboBox;
	
	vgui::ScrollableEditablePanel* m_pScrollablePanel;
    vgui::EditablePanel* m_pScrollableChild;
    
    CImageButtonContainer *m_pAgentButtonContainer;
};
#endif // MODOPTIONSSUBAGENTS_H