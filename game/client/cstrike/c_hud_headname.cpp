#include "cbase.h"
#include "c_hud_headname.h"
#include "iclientmode.h"
#include "c_baseplayer.h"
#include "c_team.h"
#include "c_cs_player.h"
#include "cs_gamerules.h"
#include "vgui/ISurface.h"
#include "vgui/ILocalize.h"
#include "vgui_controls/Label.h"
#include "vgui_controls/VectorImagePanel.h"
#include "hud_macros.h"
#include "clientmode_shared.h"
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include "view.h"
#include "lunasvg/lunasvg.h"
#include "tier0/memdbgon.h"

static ConVar cl_teamid_overhead("cl_teamid_overhead", "2", FCVAR_ARCHIVE, "Controls overhead team ID display: 0 = off, 1 = name only (money during freezetime), 2 = full display (money during freezetime)", true, 0, true, 2);
static ConVar cl_hud_namepanel_lerp("cl_hud_namepanel_lerp", "1.0", FCVAR_ARCHIVE, "Interpolation factor for HUD name panel movement (0.0 = instant, 1.0 = very smooth)", true, 0.0f, true, 1.0f);
static ConVar cl_hud_weapon_icon_scale("cl_hud_weapon_icon_scale", "0.6", FCVAR_ARCHIVE, "Scale factor for weapon and grenade icon size (1.0 = original SVG size)", true, 0.5f, true, 3.0f);

bool IsFreezetime()
{
    C_CSGameRules* pGameRules = CSGameRules();
    if (!pGameRules)
    {
        Warning("IsFreezetime: CSGameRules is not available!\n");
        return false;
    }
    return pGameRules->IsFreezePeriod();
}

// =======================
// CPlayerNamePanel
// =======================
CPlayerNamePanel::CPlayerNamePanel(vgui::Panel* parent) : vgui::EditablePanel(parent, "PlayerNamePanel")
{
    SetProportional(true);
    SetVisible(false);

    m_pNameLabel = new vgui::Label(this, "NameLabel", "");
    m_pNameLabel->SetContentAlignment(vgui::Label::a_center);

    m_pHPLabel = new vgui::Label(this, "HPLabel", "");
    m_pHPLabel->SetContentAlignment(vgui::Label::a_center);

    m_pWeaponIcon = new vgui::VectorImagePanel(this, "WeaponIcon");
    m_pWeaponIcon->SetVisible(true);

    m_pArrowLabel = new vgui::Label(this, "ArrowLabel", L"▼");
    m_pArrowLabel->SetFgColor(Color(255, 255, 255, 255));
    m_pArrowLabel->SetContentAlignment(vgui::Label::a_center);
    m_pArrowLabel->SetVisible(true);

    vgui::IScheme* scheme = vgui::scheme()->GetIScheme(vgui::scheme()->GetScheme("ClientScheme"));
    m_hFont = scheme ? scheme->GetFont("HeadName", true) : vgui::INVALID_FONT;
    if (m_hFont == vgui::INVALID_FONT)
    {
        Warning("CPlayerNamePanel: Failed to load font 'HeadName'!\n");
    }

    m_pNameLabel->SetFont(m_hFont);
    m_pHPLabel->SetFont(m_hFont);
    m_pArrowLabel->SetFont(m_hFont);

    m_currentPos = Vector2D(0, 0);
    m_targetPos = Vector2D(0, 0);
    m_flLerpFactor = cl_hud_namepanel_lerp.GetFloat();
    m_iLastTeam = -1;
    m_teamColor = Color(255, 255, 255, 255);
    m_iLastHealth = -1;
    m_hLastWeapon = nullptr;
}

static Color GetTeamColor(int team)
{
    switch (team)
    {
    case TEAM_TERRORIST:
        return Color(255, 223, 147, 200);
    case TEAM_CT:
        return Color(162, 198, 255, 200);
    default:
        return Color(255, 255, 255, 255);
    }
}

void CPlayerNamePanel::Update(C_BasePlayer* pPlayer, const Vector& screenPos)
{
    if (!pPlayer || !pPlayer->IsAlive() || !m_pNameLabel || !m_pHPLabel || !m_pWeaponIcon || !m_pArrowLabel)
    {
        SetVisible(false);
        return;
    }

    int teamIdMode = cl_teamid_overhead.GetInt();
    if (teamIdMode == 0)
    {
        SetVisible(false);
        return;
    }
    
    static const int PANEL_WIDTH = 160;
    static const int PANEL_HEIGHT = 32;

    UpdateNameLabel(pPlayer);
    UpdateHealthLabel(pPlayer);
    UpdateWeaponIcon(pPlayer->GetActiveWeapon());
    UpdateArrowLabel();
    UpdatePosition(pPlayer, screenPos);

    ApplyVisibilitySettings(::IsFreezetime());

    SetSize(PANEL_WIDTH, PANEL_HEIGHT);
    SetVisible(teamIdMode != 0);
}

void CPlayerNamePanel::UpdateNameLabel(C_BasePlayer* pPlayer)
{
    static const int PANEL_WIDTH = 160;
    static const int HP_LABEL_WIDTH = 50;
    static const int SPACING = 5;
    static const int PANEL_HEIGHT = 32;

    const char* name = pPlayer->GetPlayerName();
    m_pNameLabel->SetText(name);
    m_pNameLabel->SetFgColor(GetTeamColor(pPlayer->GetTeamNumber()));
    m_pNameLabel->SetSize(PANEL_WIDTH - HP_LABEL_WIDTH - SPACING, PANEL_HEIGHT);
    m_pNameLabel->SetPos(0, 0);
    m_pNameLabel->SetZPos(-5);
}

void CPlayerNamePanel::UpdateHealthLabel(C_BasePlayer* pPlayer)
{
    static const int HP_LABEL_WIDTH = 200;
    static const int SPACING = 25;
    static const int PANEL_HEIGHT = 32;
    static const int PANEL_WIDTH = 160;

    bool isFreezetime = ::IsFreezetime();
    int teamIdMode = cl_teamid_overhead.GetInt();

    if (isFreezetime && (teamIdMode == 1 || teamIdMode == 2))
    {
        C_CSPlayer* pCSPlayer = dynamic_cast<C_CSPlayer*>(pPlayer);
        if (pCSPlayer)
        {
            int money = pCSPlayer->GetAccount();
            money = clamp(money, 0, 16000); 
            static char moneyText[16];
            Q_snprintf(moneyText, sizeof(moneyText), "$%d", money);
            m_pHPLabel->SetText(moneyText);
            m_pHPLabel->SetFgColor(Color(0, 255, 0, 200)); 
        }
        else
        {
            m_pHPLabel->SetText("N/A");
            m_pHPLabel->SetFgColor(Color(255, 255, 255, 200));
        }
    }
    else
    {
        int health = pPlayer->GetHealth();
        health = clamp(health, 0, 100); 
        char hpText[16];
        Q_snprintf(hpText, sizeof(hpText), "%d%%", health);
        m_pHPLabel->SetText(hpText);
        Color hpColor = (health <= 30) ? Color(255, 64, 64, 200) : Color(255, 255, 255, 200);
        m_pHPLabel->SetFgColor(hpColor);
    }

    m_pHPLabel->SetSize(HP_LABEL_WIDTH, PANEL_HEIGHT);
    m_pHPLabel->SetPos(PANEL_WIDTH - HP_LABEL_WIDTH, 0);
    m_pHPLabel->SetZPos(-5);
}

void CPlayerNamePanel::UpdateWeaponIcon(C_BaseCombatWeapon* pWeapon)
{
    static const int PANEL_WIDTH = 160;

    if (pWeapon != m_hLastWeapon)
    {
        m_hLastWeapon = pWeapon;
        if (pWeapon)
        {
            const char* weaponName = pWeapon->GetClassname();
            const char* shortName = Q_strstr(weaponName, "weapon_");
            shortName = shortName ? shortName + 7 : "";
            if (Q_strlen(shortName) > 0)
            {
                char path[128];
                Q_snprintf(path, sizeof(path), "materials/vgui/weapons/svg/%s.svg", shortName);

                
                FileHandle_t f = g_pFullFileSystem->Open(path, "rt");
                if (!f)
                {
                    Warning("CPlayerNamePanel: Failed to open SVG file %s\n", path);
                    m_pWeaponIcon->SetVisible(false);
                    return;
                }

                int size = g_pFullFileSystem->Size(f);
                int nBufSize = size + 1;
                char* pMem = (char*)malloc(nBufSize);
                int bytesRead = g_pFullFileSystem->ReadEx(pMem, nBufSize, size, f);
                pMem[bytesRead] = 0;
                g_pFullFileSystem->Close(f);

                std::unique_ptr<lunasvg::Document> document = lunasvg::Document::loadFromData(pMem);
                free(pMem);

                int originalWide, originalTall;
                if (document)
                {
                    originalWide = static_cast<int>(std::ceil(document->width()));
                    originalTall = static_cast<int>(std::ceil(document->height()));
                }
                else
                {
                    Warning("CPlayerNamePanel: Failed to load SVG %s\n", path);
                    originalWide = 16;
                    originalTall = 16;
                }

                
                float scale = cl_hud_weapon_icon_scale.GetFloat();
                int scaledWide = static_cast<int>(originalWide * scale);
                int scaledTall = static_cast<int>(originalTall * scale);

                
                m_pWeaponIcon->SetRenderSize(scaledWide, scaledTall);
                m_pWeaponIcon->SetTexture(path);
                m_pWeaponIcon->SetSize(scaledWide, scaledTall);
                m_pWeaponIcon->SetFgColor(Color(255, 255, 255, 255));
                m_pWeaponIcon->SetVisible(true);
                m_pWeaponIcon->SetMirrorX(true);

                
                m_pWeaponIcon->SetPos((PANEL_WIDTH - scaledWide) / 2, -scaledTall);

                DevMsg("Weapon icon size: original (%d, %d), scaled (%d, %d), pos (%d, %d)\n",
                       originalWide, originalTall, scaledWide, scaledTall, (PANEL_WIDTH - scaledWide) / 2, -scaledTall);
            }
            else
            {
                m_pWeaponIcon->SetVisible(false);
            }
        }
        else
        {
            m_pWeaponIcon->SetVisible(false);
        }
    }
}

void CPlayerNamePanel::UpdateArrowLabel()
{
    static const int PANEL_WIDTH = 160;
    static const int PANEL_HEIGHT = 32;
    static const int ARROW_SIZE = 16; 

    m_pArrowLabel->SetBounds((PANEL_WIDTH - ARROW_SIZE) / 2, PANEL_HEIGHT, ARROW_SIZE, ARROW_SIZE);
    m_pArrowLabel->SetZPos(-4); 
    m_pArrowLabel->SetVisible(true);
}

void CPlayerNamePanel::UpdatePosition(C_BasePlayer* pPlayer, const Vector& screenPos)
{
    static const int PANEL_WIDTH = 160;
    static const int PANEL_HEIGHT = 32;
    static const float MIN_DISTANCE = 500.0f; 
    static const float MAX_DISTANCE = 2000.0f; 
    static const int MIN_ALPHA = 64; 
    static const int MAX_ALPHA = 200; 
    static const int ARROW_SIZE = 16;
    
    m_targetPos.x = screenPos.x - PANEL_WIDTH / 2;
    m_targetPos.y = screenPos.y - PANEL_HEIGHT - ARROW_SIZE - 5; 

    if (!IsVisible())
    {
        m_currentPos = m_targetPos; 
    }
    else
    {
        m_flLerpFactor = cl_hud_namepanel_lerp.GetFloat();
        m_currentPos.x = Lerp(m_flLerpFactor, m_currentPos.x, m_targetPos.x);
        m_currentPos.y = Lerp(m_flLerpFactor, m_currentPos.y, m_targetPos.y);
    }

    int screenWide, screenTall;
    vgui::surface()->GetScreenSize(screenWide, screenTall);
    m_currentPos.x = clamp(m_currentPos.x, 0.0f, static_cast<float>(screenWide - PANEL_WIDTH));
    m_currentPos.y = clamp(m_currentPos.y, 0.0f, static_cast<float>(screenTall - PANEL_HEIGHT));

    SetPos(m_currentPos.x, m_currentPos.y);

    C_BasePlayer* pLocalPlayer = C_BasePlayer::GetLocalPlayer();
    if (pLocalPlayer && pPlayer)
    {
        Vector localPos = pLocalPlayer->GetAbsOrigin();
        Vector targetPos = pPlayer->GetAbsOrigin();
        float distance = (localPos - targetPos).Length();

        float alphaLerp = clamp((MAX_DISTANCE - distance) / (MAX_DISTANCE - MIN_DISTANCE), 0.0f, 1.0f);
        int alpha = static_cast<int>(Lerp(alphaLerp, MIN_ALPHA, MAX_ALPHA));
        SetAlpha(alpha);

        DevMsg("Distance to player: %.2f, Alpha: %d\n", distance, alpha);
    }
    else
    {
        SetAlpha(MAX_ALPHA); 
    }
}

void CPlayerNamePanel::ApplyVisibilitySettings(bool isFreezetime)
{
    int teamIdMode = cl_teamid_overhead.GetInt();

    m_pNameLabel->SetVisible(true);
    m_pHPLabel->SetVisible(true);
    m_pWeaponIcon->SetVisible(m_hLastWeapon != nullptr);
    m_pArrowLabel->SetVisible(true);

    if (teamIdMode == 1)
    {
        m_pHPLabel->SetVisible(isFreezetime);
        m_pArrowLabel->SetVisible(false);
        m_pWeaponIcon->SetVisible(isFreezetime && m_hLastWeapon != nullptr);
    }
    else if (teamIdMode == 2)
    {
        m_pHPLabel->SetVisible(true);
        m_pArrowLabel->SetVisible(true);
        m_pWeaponIcon->SetVisible(m_hLastWeapon != nullptr);
    }
    
}

void CPlayerNamePanel::Reset()
{
    SetVisible(false);
    m_currentPos = Vector2D(0, 0);
    m_targetPos = Vector2D(0, 0);
    m_iLastTeam = -1;
    m_iLastHealth = -1;
    m_hLastWeapon = nullptr;
}
// =======================
// CHudPlayerName
// =======================

DECLARE_HUDELEMENT(CHudPlayerName);

CHudPlayerName::CHudPlayerName(const char* pElementName)
    : CHudElement(pElementName), vgui::EditablePanel(nullptr, "HudPlayerName")
{
    SetParent(GetClientModeNormal()->GetViewport());
    SetHiddenBits(HIDEHUD_PLAYERDEAD);
}

void CHudPlayerName::Init()
{
    m_PlayerPanels.RemoveAll();

    for (int i = 1; i <= MAX_PLAYERS; ++i)
    {
        CPlayerNamePanel* panel = new CPlayerNamePanel(this);
        panel->SetVisible(false);
        m_PlayerPanels.AddToTail(panel);
    }
}

void CHudPlayerName::Reset()
{
    for (int i = 0; i < m_PlayerPanels.Count(); ++i)
    {
        if (m_PlayerPanels[i])
            m_PlayerPanels[i]->Reset();
    }
}

void CHudPlayerName::OnThink()
{
    if (cl_teamid_overhead.GetInt() == 0)
    {
        for (int i = 0; i < m_PlayerPanels.Count(); ++i)
        {
            if (m_PlayerPanels[i]->IsVisible())
                m_PlayerPanels[i]->SetVisible(false);
        }
        return;
    }

    C_BasePlayer* pLocal = C_BasePlayer::GetLocalPlayer();
    if (!pLocal)
        return;

    for (int i = 1; i <= gpGlobals->maxClients && i - 1 < m_PlayerPanels.Count(); ++i)
    {
        if (i == pLocal->entindex())
        {
            if (m_PlayerPanels[i - 1]->IsVisible())
                m_PlayerPanels[i - 1]->SetVisible(false);
            continue;
        }

        C_CSPlayer* pPlayer = dynamic_cast<C_CSPlayer*>(UTIL_PlayerByIndex(i));
        if (!pPlayer || !pPlayer->IsAlive() || pPlayer->GetTeamNumber() != pLocal->GetTeamNumber())
        {
            if (m_PlayerPanels[i - 1]->IsVisible())
                m_PlayerPanels[i - 1]->SetVisible(false);
            continue;
        }

        Vector screenPos;
        if (!GetHeadScreenPosition(pPlayer, screenPos))
        {
            if (m_PlayerPanels[i - 1]->IsVisible())
                m_PlayerPanels[i - 1]->SetVisible(false);
            continue;
        }

        m_PlayerPanels[i - 1]->Update(pPlayer, screenPos);
    }
}



bool CHudPlayerName::GetHeadScreenPosition(C_BasePlayer* pPlayer, Vector& screenPos)
{
    Vector vecOrigin = pPlayer->GetAbsOrigin();
    vecOrigin.z += 72.0f; 
    
    Vector screen;
    if (ScreenTransform(vecOrigin, screen) != 0)
        return false;

    int screenWide, screenTall;
    vgui::surface()->GetScreenSize(screenWide, screenTall);

    screenPos.x = 0.5f * (1.0f + screen.x) * screenWide;
    screenPos.y = 0.5f * (1.0f - screen.y) * screenTall;
    return true;
}