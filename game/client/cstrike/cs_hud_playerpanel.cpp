#include "cbase.h"
#include "cs_hud_playerpanel.h"
#include <vgui/IVGui.h>
#include <vgui/IScheme.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/ProgressBar.h>
#include "vgui_avatarimage_nonsteam.h"
#include "c_cs_player.h"
#include "c_cs_playerresource.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

CCSPlayerPanel::CCSPlayerPanel(Panel *parent, const char *panelName)
    : BaseClass(parent, panelName)
{
    m_pPlayer = NULL;
    
    m_pAvatarImage = new CAvatarImagePanel(this, "AvatarImage");
    m_pPlayerName = new Label(this, "PlayerName", "");
    m_pHealthBar = new ContinuousProgressBar(this, "HealthBar");
    m_pHealthBarBG = new Panel(this, "HealthBarBG");

    m_HealthColor = Color(0, 255, 0, 255);
    m_HealthColorLow = Color(255, 0, 0, 255);
    m_BackgroundColor = Color(0, 0, 0, 180);

    SetMouseInputEnabled(false);
    SetKeyBoardInputEnabled(false);
    
    LoadControlSettings("Resource/UI/HudPlayerPanel.res");

    ivgui()->AddTickSignal(GetVPanel(), 100);
}

CCSPlayerPanel::~CCSPlayerPanel()
{
}

void CCSPlayerPanel::ApplySchemeSettings(IScheme *pScheme)
{
    BaseClass::ApplySchemeSettings(pScheme);

    SetBgColor(m_BackgroundColor);
    SetPaintBackgroundEnabled(true);
    SetPaintBackgroundType(2); // rounded

    if (m_pPlayerName)
    {
        m_pPlayerName->SetFont(pScheme->GetFont("Default", true));
        m_pPlayerName->SetFgColor(pScheme->GetColor("TanLight", Color(255, 255, 255, 255)));
    }

    if (m_pHealthBar)
    {
        m_pHealthBar->SetFgColor(m_HealthColor);
        m_pHealthBar->SetBgColor(Color(60, 60, 60, 255));
    }

    if (m_pHealthBarBG)
    {
        m_pHealthBarBG->SetBgColor(Color(40, 40, 40, 255));
        m_pHealthBarBG->SetPaintBackgroundEnabled(true);
    }
}

void CCSPlayerPanel::PerformLayout()
{
    BaseClass::PerformLayout();

   /* int wide, tall;
    GetSize(wide, tall);

    if (m_pAvatarImage)
    {
        m_pAvatarImage->SetBounds(5, 5, tall - 10, tall - 10);
    }

    if (m_pPlayerName)
    {
        m_pPlayerName->SetBounds(tall, 5, wide - tall - 10, tall / 2 - 5);
    }

    if (m_pHealthBarBG)
    {
        m_pHealthBarBG->SetBounds(tall, tall / 2 + 2, wide - tall - 10, 8);
    }

    if (m_pHealthBar)
    {
        m_pHealthBar->SetBounds(tall + 1, tall / 2 + 3, wide - tall - 12, 6);
    }
    */
}

void CCSPlayerPanel::SetPlayer(C_CSPlayer *pPlayer)
{
    m_pPlayer = pPlayer;

    if (!pPlayer)
    {
        Clear();
        return;
    }

    if (m_pAvatarImage)
    {
        player_info_t pi;
        if (engine->GetPlayerInfo(pPlayer->entindex(), &pi))
        {
            m_pAvatarImage->SetPlayer(pPlayer->entindex(), k_EAvatarSize64x64);
        }
    }

    if (m_pPlayerName)
    {
        m_pPlayerName->SetText(pPlayer->GetPlayerName());
    }

    UpdatePlayerData();
}

void CCSPlayerPanel::UpdatePlayerData()
{
    if (!m_pPlayer || !m_pHealthBar)
        return;

    if (!m_pPlayer->IsAlive())
    {
        if (m_pHealthBar)
        {
            m_pHealthBar->SetProgress(0.0f);
            m_pHealthBar->SetFgColor(Color(100, 100, 100, 255));
        }
        return;
    }

    int health = m_pPlayer->GetHealth();
    int maxHealth = m_pPlayer->GetMaxHealth();
    
    if (maxHealth <= 0)
        maxHealth = 100;

    float healthPercent = (float)health / (float)maxHealth;
    
    if (m_pHealthBar)
    {
        m_pHealthBar->SetProgress(healthPercent);
        
        if (healthPercent > 0.5f)
        {
            m_pHealthBar->SetFgColor(m_HealthColor);
        }
        else if (healthPercent > 0.25f)
        {
            m_pHealthBar->SetFgColor(Color(255, 165, 0, 255)); // Orange
        }
        else
        {
            m_pHealthBar->SetFgColor(m_HealthColorLow);
        }
    }
}

void CCSPlayerPanel::OnThink()
{
    BaseClass::OnThink();
    
    if (m_pPlayer)
    {
        UpdatePlayerData();
    }
}

void CCSPlayerPanel::Clear()
{
    m_pPlayer = NULL;
    
    if (m_pPlayerName)
        m_pPlayerName->SetText("");
    
    if (m_pHealthBar)
        m_pHealthBar->SetProgress(0.0f);
    
    if (m_pAvatarImage)
        m_pAvatarImage->SetPlayer(0, k_EAvatarSize64x64);
}