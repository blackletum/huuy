//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//


#if defined( WIN32 ) && !defined( _X360 )
#include <windows.h> // SRC only!!
#endif

#include "ModOptionsSubAgents.h"
#include <stdio.h>

#include <vgui_controls/Button.h>
#include "tier1/KeyValues.h"

#include "LabeledCommandComboBox.h"
#include "tier1/convar.h"
#include "BitmapImagePanel.h"
#include "vgui_controls/ImagePanel.h"
#include <vgui_controls/ScrollBar.h>
//#include "vgui_controls/SectionedListPanel.h"

#include "cs_shareddefs.h"
#include "GameUI_Interface.h"

#if defined( _X360 )
#include "xbox/xbox_win32stubs.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

struct Agents
{
	const char*		m_szUIName;
	const char*		m_szImage;
};

static Agents agentsCT[] =
{
	{ "#GameUI_Loadout_Agent_None",							"ct_none"					},
	{ "#GameUI_Loadout_Agent_ctm_fbi_variantf",				"ctm_fbi_variantf"			},
	{ "#GameUI_Loadout_Agent_ctm_fbi_variantf_legacy",		"ctm_fbi_variantf_legacy"	},
	{ "#GameUI_Loadout_Agent_ctm_fbi_variantg",				"ctm_fbi_variantg"			},
	{ "#GameUI_Loadout_Agent_ctm_fbi_varianth",				"ctm_fbi_varianth"			},
	{ "#GameUI_Loadout_Agent_ctm_fbi_variantb",				"ctm_fbi_variantb"			},
	{ "#GameUI_Loadout_Agent_ctm_sas_variantf",				"ctm_sas_variantf"			},
	{ "#GameUI_Loadout_Agent_ctm_st6_variantk",				"ctm_st6_variantk"			},
	{ "#GameUI_Loadout_Agent_ctm_st6_variantk_legacy",		"ctm_st6_variantk_legacy"	},
	{ "#GameUI_Loadout_Agent_ctm_st6_variante",				"ctm_st6_variante"			},
	{ "#GameUI_Loadout_Agent_ctm_st6_variante_legacy",		"ctm_st6_variante_legacy"	},
	{ "#GameUI_Loadout_Agent_ctm_st6_variantg",				"ctm_st6_variantg"			},
	{ "#GameUI_Loadout_Agent_ctm_st6_variantm",				"ctm_st6_variantm"			},
	{ "#GameUI_Loadout_Agent_ctm_st6_variantm_legacy",		"ctm_st6_variantm_legacy"	},
	{ "#GameUI_Loadout_Agent_ctm_st6_varianti",				"ctm_st6_varianti"			},
	{ "#GameUI_Loadout_Agent_ctm_st6_varianti_legacy",		"ctm_st6_varianti_legacy"	},
	{ "#GameUI_Loadout_Agent_ctm_swat_variantj",			"ctm_swat_variantj"			},
	{ "#GameUI_Loadout_Agent_ctm_swat_varianth",			"ctm_swat_varianth"			},
	{ "#GameUI_Loadout_Agent_ctm_st6_variantj",				"ctm_st6_variantj"			},
	{ "#GameUI_Loadout_Agent_ctm_swat_variantg",			"ctm_swat_variantg"			},
	{ "#GameUI_Loadout_Agent_ctm_swat_varianti",			"ctm_swat_varianti"			},
	{ "#GameUI_Loadout_Agent_ctm_swat_variantf",			"ctm_swat_variantf"			},
	{ "#GameUI_Loadout_Agent_ctm_st6_variantl",				"ctm_st6_variantl"			},
	{ "#GameUI_Loadout_Agent_ctm_swat_variante",			"ctm_swat_variante"			},
	{ "#GameUI_Loadout_Agent_ctm_diver_varianta",			"ctm_diver_varianta"		},
	{ "#GameUI_Loadout_Agent_ctm_diver_variantb",			"ctm_diver_variantb"		},
	{ "#GameUI_Loadout_Agent_ctm_diver_variantc",			"ctm_diver_variantc"		},
	{ "#GameUI_Loadout_Agent_ctm_gendarmerie_varianta",		"ctm_gendarmerie_varianta"	},
	{ "#GameUI_Loadout_Agent_ctm_gendarmerie_variantb",		"ctm_gendarmerie_variantb"	},
	{ "#GameUI_Loadout_Agent_ctm_gendarmerie_variantc",		"ctm_gendarmerie_variantc"	},
	{ "#GameUI_Loadout_Agent_ctm_gendarmerie_variantd",		"ctm_gendarmerie_variantd"	},
	{ "#GameUI_Loadout_Agent_ctm_gendarmerie_variante",		"ctm_gendarmerie_variante"	},
	{ "#GameUI_Loadout_Agent_ctm_sas_variantg",				"ctm_sas_variantg"			},
	{ "#GameUI_Loadout_Agent_ctm_st6_variantn",				"ctm_st6_variantn"			},
	{ "#GameUI_Loadout_Agent_ctm_swat_variantk",			"ctm_swat_variantk"			},
	{ "#GameUI_Loadout_Agent_ctm_sas_old",					"ctm_sas_old"				},
	{ "#GameUI_Loadout_Agent_ctm_fbi_old",					"ctm_fbi_old"				},
	{ "#GameUI_Loadout_Agent_ctm_jumpsuit_varianta",		"ctm_jumpsuit_varianta"		},
	{ "#GameUI_Loadout_Agent_ctm_jumpsuit_variantb",		"ctm_jumpsuit_variantb"		},
	{ "#GameUI_Loadout_Agent_ctm_jumpsuit_variantc",		"ctm_jumpsuit_variantc"		},
};
static Agents agentsT[] =
{
	{ "#GameUI_Loadout_Agent_None",							"t_none"					},
	{ "#GameUI_Loadout_Agent_tm_leet_variantg",				"tm_leet_variantg"			},
	{ "#GameUI_Loadout_Agent_tm_leet_variantg_legacy",		"tm_leet_variantg_legacy"	},
	{ "#GameUI_Loadout_Agent_tm_leet_varianth",				"tm_leet_varianth"			},
	{ "#GameUI_Loadout_Agent_tm_leet_varianti",				"tm_leet_varianti"			},
	{ "#GameUI_Loadout_Agent_tm_leet_varianti_legacy",		"tm_leet_varianti_legacy"	},
	{ "#GameUI_Loadout_Agent_tm_leet_variantf",				"tm_leet_variantf"			},
	{ "#GameUI_Loadout_Agent_tm_phoenix_varianth",			"tm_phoenix_varianth"		},
	{ "#GameUI_Loadout_Agent_tm_phoenix_variantf",			"tm_phoenix_variantf"		},
	{ "#GameUI_Loadout_Agent_tm_phoenix_variantf_legacy",	"tm_phoenix_variantf_legacy"},
	{ "#GameUI_Loadout_Agent_tm_phoenix_variantg",			"tm_phoenix_variantg"		},
	{ "#GameUI_Loadout_Agent_tm_balkan_variantf",			"tm_balkan_variantf"		},
	{ "#GameUI_Loadout_Agent_tm_balkan_varianti",			"tm_balkan_varianti"		},
	{ "#GameUI_Loadout_Agent_tm_balkan_variantg",			"tm_balkan_variantg"		},
	{ "#GameUI_Loadout_Agent_tm_balkan_variantj",			"tm_balkan_variantj"		},
	{ "#GameUI_Loadout_Agent_tm_balkan_varianth",			"tm_balkan_varianth"		},
	{ "#GameUI_Loadout_Agent_tm_balkan_variantl",			"tm_balkan_variantl"		},
	{ "#GameUI_Loadout_Agent_tm_phoenix_varianti",			"tm_phoenix_varianti"		},
	{ "#GameUI_Loadout_Agent_tm_professional_varj",			"tm_professional_varj"		},
	{ "#GameUI_Loadout_Agent_tm_professional_varh",			"tm_professional_varh"		},
	{ "#GameUI_Loadout_Agent_tm_balkan_variantk",			"tm_balkan_variantk"		},
	{ "#GameUI_Loadout_Agent_tm_professional_varg",			"tm_professional_varg"		},
	{ "#GameUI_Loadout_Agent_tm_professional_vari",			"tm_professional_vari"		},
	{ "#GameUI_Loadout_Agent_tm_professional_varf",			"tm_professional_varf"		},
	{ "#GameUI_Loadout_Agent_tm_professional_varf1",		"tm_professional_varf1"		},
	{ "#GameUI_Loadout_Agent_tm_professional_varf2",		"tm_professional_varf2"		},
	{ "#GameUI_Loadout_Agent_tm_professional_varf3",		"tm_professional_varf3"		},
	{ "#GameUI_Loadout_Agent_tm_professional_varf4",		"tm_professional_varf4"		},
	{ "#GameUI_Loadout_Agent_tm_jungle_raider_varianta",	"tm_jungle_raider_varianta"	},
	{ "#GameUI_Loadout_Agent_tm_jungle_raider_variantb",	"tm_jungle_raider_variantb"	},
	{ "#GameUI_Loadout_Agent_tm_jungle_raider_variantb2",	"tm_jungle_raider_variantb2"},
	{ "#GameUI_Loadout_Agent_tm_jungle_raider_variantc",	"tm_jungle_raider_variantc"	},
	{ "#GameUI_Loadout_Agent_tm_jungle_raider_variantd",	"tm_jungle_raider_variantd"	},
	{ "#GameUI_Loadout_Agent_tm_jungle_raider_variante",	"tm_jungle_raider_variante"	},
	{ "#GameUI_Loadout_Agent_tm_jungle_raider_variantf",	"tm_jungle_raider_variantf"	},
	{ "#GameUI_Loadout_Agent_tm_jungle_raider_variantf2",	"tm_jungle_raider_variantf2"},
	{ "#GameUI_Loadout_Agent_tm_leet_variantj",				"tm_leet_variantj"			},
	{ "#GameUI_Loadout_Agent_tm_professional_varf5",		"tm_professional_varf5"		},
	{ "#GameUI_Loadout_Agent_tm_phoenix_old",				"tm_phoenix_old"			},
	{ "#GameUI_Loadout_Agent_tm_leet_old",					"tm_leet_old"				},
	{ "#GameUI_Loadout_Agent_tm_jumpsuit_varianta",			"tm_jumpsuit_varianta"		},
	{ "#GameUI_Loadout_Agent_tm_jumpsuit_variantb",			"tm_jumpsuit_variantb"		},
	{ "#GameUI_Loadout_Agent_tm_jumpsuit_variantc",			"tm_jumpsuit_variantc"		},
};

//-----------------------------------------------------------------------------
// Purpose: Basic help dialog
//-----------------------------------------------------------------------------
CModOptionsSubAgents::CModOptionsSubAgents(vgui::Panel *parent) : vgui::PropertyPage(parent, "ModOptionsSubAgents") 
{
	Button *cancel = new Button( this, "Cancel", "#GameUI_Cancel" );
	cancel->SetCommand( "Close" );

	Button *ok = new Button( this, "OK", "#GameUI_OK" );
	ok->SetCommand( "Ok" );

	Button *apply = new Button( this, "Apply", "#GameUI_Apply" );
	apply->SetCommand( "Apply" );

	//=========
	
	m_pScrollableChild = new EditablePanel(this, "ScrollableChild");
    m_pScrollablePanel = new ScrollableEditablePanel(this, m_pScrollableChild, "ScrollablePanel");

m_pAgentButtonContainer = new CImageButtonContainer(m_pScrollableChild, "AgentButtonContainer");

m_pAgentButtonContainer->SetBounds( 0, 0, 800, 2000 );

	m_pLoadoutAgentCTComboBox = new CLabeledCommandComboBox( this, "AgentCTComboBox" );
	m_pLoadoutAgentTComboBox = new CLabeledCommandComboBox( this, "AgentTComboBox" );
	m_pLoadoutMainMenuWeaponCTComboBox = new CLabeledCommandComboBox( this, "MainMenuWeaponCTComboBox" );
	m_pLoadoutMainMenuWeaponTComboBox = new CLabeledCommandComboBox( this, "MainMenuWeaponTComboBox" );

	m_pAgentImageCT = new CBitmapImagePanel( this, "AgentImageCT", NULL );
	m_pAgentImageCT->AddActionSignalTarget( this );
	m_pAgentImageT = new CBitmapImagePanel( this, "AgentImageT", NULL );
	m_pAgentImageT->AddActionSignalTarget( this );
	
	m_pPlayerModel = new CBasePlayerModelPanel(this, "PlayerModel");
	
	char command[64];
	int i;
	for ( i = 0; i < ARRAYSIZE( agentsCT ); i++ )
	{
		Q_snprintf( command, sizeof( command ), "loadout_slot_agent_ct %d", i );
		m_pLoadoutAgentCTComboBox->AddItem( agentsCT[i].m_szUIName, command );
	}
	for ( i = 0; i < ARRAYSIZE( agentsT ); i++ )
	{
		Q_snprintf( command, sizeof( command ), "loadout_slot_agent_t %d", i );
		m_pLoadoutAgentTComboBox->AddItem( agentsT[i].m_szUIName, command );
	}
	for ( i = 0; i < MAX_MAINMENU_WEAPONS_CT; i++ )
	{
		Q_snprintf( command, sizeof( command ), "loadout_mainmenu_weapon_ct %d", i );
		m_pLoadoutMainMenuWeaponCTComboBox->AddItem( GetCSMainMenuWeaponCT( i )->m_pszName, command );
	}
	for ( i = 0; i < MAX_MAINMENU_WEAPONS_T; i++ )
	{
		Q_snprintf( command, sizeof( command ), "loadout_mainmenu_weapon_t %d", i );
		m_pLoadoutMainMenuWeaponTComboBox->AddItem( GetCSMainMenuWeaponT( i )->m_pszName, command );
	}

	m_pLoadoutAgentCTComboBox->AddActionSignalTarget( this );
	m_pLoadoutAgentTComboBox->AddActionSignalTarget( this );
	m_pLoadoutMainMenuWeaponCTComboBox->AddActionSignalTarget( this );
	m_pLoadoutMainMenuWeaponTComboBox->AddActionSignalTarget( this );
	
	UpdateAgentModel();
	
	UpdateAgentImages();

	LoadControlSettings("Resource/ModOptionsSubAgents.res");
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CModOptionsSubAgents::~CModOptionsSubAgents()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CModOptionsSubAgents::RemapAgentsImage()
{
	const char *pImageNameCT = agentsCT[m_pLoadoutAgentCTComboBox->GetActiveItem()].m_szImage;
	const char *pImageNameT = agentsT[m_pLoadoutAgentTComboBox->GetActiveItem()].m_szImage;

	char texture[256];
	if ( pImageNameCT != NULL )
	{
		Q_snprintf( texture, sizeof( texture ), "vgui/agents/%s", pImageNameCT );
		m_pAgentImageCT->setTexture( texture );
	}
	else
	{
		m_pAgentImageCT->setTexture( "vgui/agents/ct_none" );
	}

	if ( pImageNameT != NULL )
	{
		Q_snprintf( texture, sizeof( texture ), "vgui/agents/%s", pImageNameT );
		m_pAgentImageT->setTexture( texture );
	}
	else
	{
		m_pAgentImageT->setTexture( "vgui/agents/t_none" );
	}
}

void CModOptionsSubAgents::UpdateAgentImages()
{
	
	m_pScrollableChild->InvalidateLayout(true);
    m_pScrollablePanel->InvalidateLayout(true);
	
	for (int i = 0; i < ARRAYSIZE(agentsCT); i++)
{
    char cmd[64];
    Q_snprintf(cmd, sizeof(cmd), "loadout_slot_agent_ct %d", i);

    char texture[128];
    Q_snprintf(texture, sizeof(texture), "vgui/agents/%s", agentsCT[i].m_szImage);

    m_pAgentButtonContainer->AddImageButton(texture, cmd, agentsCT[i].m_szUIName);
}

for (int i = 0; i < ARRAYSIZE(agentsT); i++)
{
    char cmd[64];
    Q_snprintf(cmd, sizeof(cmd), "loadout_slot_agent_t %d", i);

    char texture[128];
    Q_snprintf(texture, sizeof(texture), "vgui/agents/%s", agentsT[i].m_szImage);

    m_pAgentButtonContainer->AddImageButton(texture, cmd, agentsT[i].m_szUIName);
}
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CModOptionsSubAgents::OnControlModified()
{
	PostMessage(GetParent(), new KeyValues("ApplyButtonEnable"));
	InvalidateLayout();
	UpdateAgentModel();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CModOptionsSubAgents::OnTextChanged( vgui::Panel *panel )
{
	RemapAgentsImage();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CModOptionsSubAgents::OnResetData()
{
	ConVarRef loadout_slot_agent_ct( "loadout_slot_agent_ct" );
	m_pLoadoutAgentCTComboBox->SetInitialItem( loadout_slot_agent_ct.GetInt() );

	ConVarRef loadout_slot_agent_t( "loadout_slot_agent_t" );
	m_pLoadoutAgentTComboBox->SetInitialItem( loadout_slot_agent_t.GetInt() );

	ConVarRef loadout_mainmenu_weapon_ct( "loadout_mainmenu_weapon_ct" );
	m_pLoadoutMainMenuWeaponCTComboBox->SetInitialItem( loadout_mainmenu_weapon_ct.GetInt() );

	ConVarRef loadout_mainmenu_weapon_t( "loadout_mainmenu_weapon_t" );
	m_pLoadoutMainMenuWeaponTComboBox->SetInitialItem( loadout_mainmenu_weapon_t.GetInt() );

	RemapAgentsImage();
}

extern ConVar loadout_mainmenu_agent;
extern ConVar loadout_mainmenu_weapon_t;
extern ConVar loadout_mainmenu_weapon_ct;
extern ConVar loadout_slot_agent_t;
extern ConVar loadout_slot_agent_ct;
extern ConVar loadout_slot_gloves_t;
extern ConVar loadout_slot_gloves_ct;

void CModOptionsSubAgents::UpdateAgentModel()
{
	// PiMoN: I hate this spaghetti code.

	bool bUpdateAgent = false;

	if ( m_iAgentToUse != loadout_mainmenu_agent.GetInt() )
	{
		m_iAgentToUse = loadout_mainmenu_agent.GetInt();
		bUpdateAgent = true;
	}

	if ( m_iAgentToUse == 0 )
	{
		m_pPlayerModel->SetVisible( false );
		return;
	}
	else
	{
		int wide = 400;
		int tall = 570;
		m_pPlayerModel->SetVisible( true );
	}

	if ( m_iAgentToUse == 1 )
	{
		if ( m_iTAgent != loadout_slot_agent_t.GetInt() )
		{
			m_iTAgent = loadout_slot_agent_t.GetInt();
			bUpdateAgent = true;
		}
		if ( m_iTGloves != loadout_slot_gloves_t.GetInt() )
		{
			m_iTGloves = loadout_slot_gloves_t.GetInt();
			bUpdateAgent = true;
		}
		if ( m_iTWeapon != loadout_mainmenu_weapon_t.GetInt() )
		{
			m_iTWeapon = loadout_mainmenu_weapon_t.GetInt();
			bUpdateAgent = true;
		}
	}
	else
	{
		if ( m_iCTAgent != loadout_slot_agent_ct.GetInt() )
		{
			m_iCTAgent = loadout_slot_agent_ct.GetInt();
			bUpdateAgent = true;
		}
		if ( m_iCTGloves != loadout_slot_gloves_ct.GetInt() )
		{
			m_iCTGloves = loadout_slot_gloves_ct.GetInt();
			bUpdateAgent = true;
		}
		if ( m_iCTWeapon != loadout_mainmenu_weapon_ct.GetInt() )
		{
			m_iCTWeapon = loadout_mainmenu_weapon_ct.GetInt();
			bUpdateAgent = true;
		}
	}

	if ( bUpdateAgent )
	{
		m_pPlayerModel->ClearMergeMDLs();

		if ( m_iAgentToUse == 1 )
		{
			const char* pszModel = GetCSAgentInfoT( m_iTAgent )->m_szModel;
			m_pPlayerModel->SetMDL( pszModel );
			m_pPlayerModel->SetMergeMDL( GetCSMainMenuWeaponT( m_iTWeapon )->m_pszModel );
			m_pPlayerModel->PlaySequence( GetCSMainMenuWeaponT( m_iTWeapon )->m_pszSequence );

			if ( m_iTGloves > 0 )
			{
				if ( m_pPlayerModel->SetBodygroup( "gloves", 1 ) )
				{
					CMDL* pGloves = m_pPlayerModel->SetMergeMDL( GetGlovesInfo( m_iTGloves )->szWorldModel );
					if ( pGloves )
						pGloves->m_nSkin = GetPlayerViewmodelArmConfigForPlayerModel( pszModel )->iSkintoneIndex;
				}
			}
			else
			{
				m_pPlayerModel->SetBodygroup( "gloves", 0 );
			}
		}
		else
		{
			const char* pszModel = GetCSAgentInfoCT( m_iCTAgent )->m_szModel;
			m_pPlayerModel->SetMDL( pszModel );
			m_pPlayerModel->SetMergeMDL( GetCSMainMenuWeaponCT( m_iCTWeapon )->m_pszModel );
			m_pPlayerModel->PlaySequence( GetCSMainMenuWeaponCT( m_iCTWeapon )->m_pszSequence );

			if ( m_iCTGloves > 0 )
			{
				if ( m_pPlayerModel->SetBodygroup( "gloves", 1 ) )
				{
					CMDL* pGloves = m_pPlayerModel->SetMergeMDL( GetGlovesInfo( m_iCTGloves )->szWorldModel );
					if ( pGloves )
						pGloves->m_nSkin = GetPlayerViewmodelArmConfigForPlayerModel( pszModel )->iSkintoneIndex;
				}
			}
			else
			{
				m_pPlayerModel->SetBodygroup( "gloves", 0 );
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CModOptionsSubAgents::OnApplyChanges()
{
	m_pLoadoutAgentCTComboBox->ApplyChanges();
	m_pLoadoutAgentTComboBox->ApplyChanges();
	m_pLoadoutMainMenuWeaponCTComboBox->ApplyChanges();
	m_pLoadoutMainMenuWeaponTComboBox->ApplyChanges();

	// update agent on main menu
	GameUI().UpdateAgentModel();
	UpdateAgentModel();
}

ImageButton::ImageButton( Panel *parent, const char *imageName )
    : Button( parent, "", "" )
{
    m_textureID = surface()->CreateNewTextureID();
    surface()->DrawSetTextureFile( m_textureID, imageName, true, false );

    SetPaintBackgroundEnabled( false );
    SetMouseInputEnabled( true );
    SetKeyBoardInputEnabled( false );

    m_bSelected = false;
}

void ImageButton::SetImage( const char *imageName )
{
    surface()->DrawSetTextureFile( m_textureID, imageName, true, false );
}

void ImageButton::Paint()
{
    int color = m_bSelected ? 120 : 160;

    surface()->DrawSetColor( color, color, color, 100 );
    surface()->DrawFilledRect( 0, 0, GetWide(), GetTall() );

    surface()->DrawSetTexture( m_textureID );
    surface()->DrawSetColor( 255, 255, 255, 255 );
    surface()->DrawTexturedRect( 0, 0, GetWide(), GetTall() );
}

void ImageButton::OnCommand( const char *cmd )
{
    engine->ClientCmd( cmd );
    BaseClass::OnCommand( cmd );
}

#if 0
void CModOptionsSubAgents::UpdateScrollPanel()
{
    if (!m_pScrollableChild || !m_pScrollablePanel)
        return;

    static const int BUTTON_SIZE = 96;        
    static const int BUTTON_PADDING = 10;
    static const int BUTTONS_PER_ROW = 2;     
    static const int START_X = 5;
    static const int START_Y = 5;

    static const char *g_ctAgentImages[] =
    {
        "ct_none", "ctm_fbi_variantf", "ctm_fbi_variantf_legacy", "ctm_fbi_variantg",
        "ctm_fbi_varianth", "ctm_fbi_variantb", "ctm_sas_variantf", "ctm_st6_variantk",
        "ctm_st6_variantk_legacy", "ctm_st6_variante", "ctm_st6_variante_legacy",
        "ctm_st6_variantg", "ctm_st6_variantm", "ctm_st6_variantm_legacy",
        "ctm_st6_varianti", "ctm_st6_varianti_legacy", "ctm_swat_variantj",
        "ctm_swat_varianth", "ctm_st6_variantj", "ctm_swat_variantg",
        "ctm_swat_varianti", "ctm_swat_variantf", "ctm_st6_variantl",
        "ctm_swat_variante", "ctm_diver_varianta", "ctm_diver_variantb",
        "ctm_diver_variantc", "ctm_gendarmerie_varianta", "ctm_gendarmerie_variantb",
        "ctm_gendarmerie_variantc", "ctm_gendarmerie_variantd", "ctm_gendarmerie_variante",
        "ctm_sas_variantg", "ctm_st6_variantn", "ctm_swat_variantk", "ctm_sas_old",
        "ctm_fbi_old", "ctm_jumpsuit_varianta", "ctm_jumpsuit_variantb", "ctm_jumpsuit_variantc"
    };

    const int numAgents = ARRAYSIZE(g_ctAgentImages);

    int numRows = (numAgents + BUTTONS_PER_ROW - 1) / BUTTONS_PER_ROW;
    int childTall = START_Y + numRows * (BUTTON_SIZE + BUTTON_PADDING);
    m_pScrollableChild->SetBounds(0, 0, m_pScrollablePanel->GetWide(), childTall);

    CUtlVector<ImageButton*> agentButtons;
    agentButtons.EnsureCapacity(numAgents);

    for (int i = 0; i < numAgents; ++i)
{
    char texturePath[128];
    Q_snprintf(texturePath, sizeof(texturePath), "vgui/agents/%s", g_ctAgentImages[i]);

    ImageButton *pBtn = new ImageButton(nullptr, texturePath);
    agentButtons.AddToTail(pBtn);

    int row = i / BUTTONS_PER_ROW;
    int col = i % BUTTONS_PER_ROW;

    int x = START_X + col * (BUTTON_SIZE + BUTTON_PADDING);
    int y = START_Y + row * (BUTTON_SIZE + BUTTON_PADDING);

    pBtn->SetPos(x, y);    
    pBtn->SetSize(BUTTON_SIZE, BUTTON_SIZE);

    pBtn->SetParent(m_pScrollableChild); 
    pBtn->SetVisible(true);

    char cmd[64];
    Q_snprintf(cmd, sizeof(cmd), "loadout_slot_agent_ct %d", i);
    pBtn->SetCommand(cmd);
}
    
    m_pScrollableChild->InvalidateLayout(true, true);
    m_pScrollablePanel->InvalidateLayout(true, true);
    m_pScrollablePanel->GetScrollbar()->InvalidateLayout(true);
    m_pScrollablePanel->GetScrollbar()->SetAutohideButtons(true);
}
#endif