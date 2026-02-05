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
static ConVar cl_hud_namepanel_lerp("cl_hud_namepanel_lerp", "0.3", FCVAR_ARCHIVE, "Interpolation factor for HUD name panel movement (0.0 = instant, 1.0 = very smooth)", true, 0.0f, true, 1.0f);
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

    for (int i = 0; i < 4; i++)
    {
        char grenadeName[32];
        Q_snprintf(grenadeName, sizeof(grenadeName), "GrenadeIcon%d", i);
        m_pGrenadeIcons[i] = new vgui::VectorImagePanel(this, grenadeName);
        m_pGrenadeIcons[i]->SetVisible(false);
    }

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
    C_CSPlayer* pCSPlayer = dynamic_cast<C_CSPlayer*>(pPlayer);
    UpdateWeaponIcon(pPlayer->GetActiveWeapon(), pCSPlayer);
    UpdateArrowLabel();
    UpdatePosition(screenPos);

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
    static const int HP_LABEL_WIDTH = 120;
    static const int SPACING = 5;
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
        DevMsg("Player health: %d\n", health); 
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

void CPlayerNamePanel::UpdateWeaponIcon(C_BaseCombatWeapon* pWeapon, C_CSPlayer* pCSPlayer)
{
    static const int PANEL_WIDTH = 160;
    static const int SPACING = 4; 

    for (int i = 0; i < 4; i++)
        m_pGrenadeIcons[i]->SetVisible(false);

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

                int grenadeCount = 0;
                int currentX = PANEL_WIDTH; 
                if (pCSPlayer)
                {
                    const char* grenadeTypes[] = {"hegrenade", "flashbang", "smokegrenade", "molotov", "decoy"};
                    int grenadeSlots[4] = {-1, -1, -1, -1}; 

                    for (int i = 0; i < MAX_WEAPONS && grenadeCount < 4; i++)
                    {
                        CBaseHandle hWeapon = pCSPlayer->m_hMyWeapons[i];
                        if (!hWeapon.IsValid())
                            continue;

                        C_BaseCombatWeapon* pInventoryWeapon = dynamic_cast<C_BaseCombatWeapon*>(CBaseEntity::Instance(hWeapon));
                        if (!pInventoryWeapon)
                            continue;

                        const char* invWeaponName = pInventoryWeapon->GetClassname();
                        const char* invShortName = Q_strstr(invWeaponName, "weapon_") ? invWeaponName + 7 : "";
                        for (int j = 0; j < ARRAYSIZE(grenadeTypes); j++)
                        {
                
                            if (pWeapon == pInventoryWeapon)
                                continue;

                            if (Q_strcmp(invShortName, grenadeTypes[j]) == 0)
                            {
                                grenadeSlots[grenadeCount] = i;
                                grenadeCount++;
                                break;
                            }
                        }
                    }

                    for (int i = 0; i < grenadeCount && i < 4; i++)
                    {
                        C_BaseCombatWeapon* pGrenade = dynamic_cast<C_BaseCombatWeapon*>(CBaseEntity::Instance(pCSPlayer->m_hMyWeapons[grenadeSlots[i]]));
                        if (!pGrenade)
                            continue;

                        const char* grenadeName = pGrenade->GetClassname();
                        const char* grenadeShortName = Q_strstr(grenadeName, "weapon_") ? grenadeName + 7 : "";
                        char grenadePath[128];
                        Q_snprintf(grenadePath, sizeof(grenadePath), "materials/vgui/weapons/svg/%s.svg", grenadeShortName);

                        FileHandle_t gf = g_pFullFileSystem->Open(grenadePath, "rt");
                        if (!gf)
                        {
                            Warning("CPlayerNamePanel: Failed to open SVG file %s\n", grenadePath);
                            m_pGrenadeIcons[i]->SetVisible(false);
                            continue;
                        }

                        int gSize = g_pFullFileSystem->Size(gf);
                        int gBufSize = gSize + 1;
                        char* gMem = (char*)malloc(gBufSize);
                        int gBytesRead = g_pFullFileSystem->ReadEx(gMem, gBufSize, gSize, gf);
                        gMem[gBytesRead] = 0;
                        g_pFullFileSystem->Close(gf);

                        std::unique_ptr<lunasvg::Document> gDocument = lunasvg::Document::loadFromData(gMem);
                        free(gMem);

                        int gOriginalWide, gOriginalTall;
                        if (gDocument)
                        {
                            gOriginalWide = static_cast<int>(std::ceil(gDocument->width()));
                            gOriginalTall = static_cast<int>(std::ceil(gDocument->height()));
                        }
                        else
                        {
                            Warning("CPlayerNamePanel: Failed to load SVG %s\n", grenadePath);
                            gOriginalWide = 16;
                            gOriginalTall = 16;
                        }

                        int gScaledWide = static_cast<int>(gOriginalWide * scale);
                        int gScaledTall = static_cast<int>(gOriginalTall * scale);

                        currentX -= (gScaledWide + SPACING); 
                        m_pGrenadeIcons[i]->SetRenderSize(gScaledWide, gScaledTall);
                        m_pGrenadeIcons[i]->SetTexture(grenadePath);
                        m_pGrenadeIcons[i]->SetSize(gScaledWide, gScaledTall);
                        m_pGrenadeIcons[i]->SetPos(currentX, -gScaledTall);
                        m_pGrenadeIcons[i]->SetFgColor(Color(255, 255, 255, 200));
                        m_pGrenadeIcons[i]->SetVisible(true);
                        m_pGrenadeIcons[i]->SetMirrorX(true);

                        DevMsg("Grenade %d icon size: original (%d, %d), scaled (%d, %d), pos (%d, %d)\n",
                               i, gOriginalWide, gOriginalTall, gScaledWide, gScaledTall, currentX, -gScaledTall);
                    }
                }

                if (grenadeCount == 0)
                {
                    m_pWeaponIcon->SetPos((PANEL_WIDTH - scaledWide) / 2, -scaledTall);
                }
                else
                {
                    m_pWeaponIcon->SetPos(currentX + SPACING, -scaledTall);
                }

                DevMsg("Weapon icon size: original (%d, %d), scaled (%d, %d), pos (%d, %d)\n",
                       originalWide, originalTall, scaledWide, scaledTall, grenadeCount == 0 ? (PANEL_WIDTH - scaledWide) / 2 : currentX + SPACING, -scaledTall);
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
    static const int ARROW_SIZE = 12;

    m_pArrowLabel->SetBounds((PANEL_WIDTH - ARROW_SIZE) / 2, PANEL_HEIGHT + 2, ARROW_SIZE, ARROW_SIZE);
    m_pArrowLabel->SetVisible(true);
}

void CPlayerNamePanel::UpdatePosition(const Vector& screenPos)
{
    static const int PANEL_WIDTH = 160;
    static const int PANEL_HEIGHT = 32;

    m_targetPos.x = screenPos.x - PANEL_WIDTH / 2;
    m_targetPos.y = screenPos.y - PANEL_HEIGHT - 20;

    if (!IsVisible())
    {
        m_currentPos = m_targetPos; 
    }
    else
    {
        m_flLerpFactor = cl_hud_namepanel_lerp.GetFloat();
        float distance = (m_targetPos - m_currentPos).Length();
        float dynamicLerp = m_flLerpFactor;
        if (distance > 50.0f) 
            dynamicLerp = MIN(1.0f, m_flLerpFactor * 2.0f);
        m_currentPos.x = Lerp(dynamicLerp, m_currentPos.x, m_targetPos.x);
        m_currentPos.y = Lerp(dynamicLerp, m_currentPos.y, m_targetPos.y);
    }

    int screenWide, screenTall;
    vgui::surface()->GetScreenSize(screenWide, screenTall);
    m_currentPos.x = clamp(m_currentPos.x, 0.0f, static_cast<float>(screenWide - PANEL_WIDTH));
    m_currentPos.y = clamp(m_currentPos.y, 0.0f, static_cast<float>(screenTall - PANEL_HEIGHT));

    SetPos(m_currentPos.x, m_currentPos.y);
}

void CPlayerNamePanel::ApplyVisibilitySettings(bool isFreezetime)
{
    int teamIdMode = cl_teamid_overhead.GetInt();

    m_pNameLabel->SetVisible(true);
    m_pHPLabel->SetVisible(true);
    m_pWeaponIcon->SetVisible(m_hLastWeapon != nullptr);
    m_pArrowLabel->SetVisible(true);
    for (int i = 0; i < 4; i++)
        m_pGrenadeIcons[i]->SetVisible(m_pGrenadeIcons[i]->IsVisible()); 

    if (teamIdMode == 1)
    {
        m_pHPLabel->SetVisible(isFreezetime);
        m_pArrowLabel->SetVisible(false);
        m_pWeaponIcon->SetVisible(isFreezetime && m_hLastWeapon != nullptr);
        for (int i = 0; i < 4; i++)
            m_pGrenadeIcons[i]->SetVisible(isFreezetime && m_pGrenadeIcons[i]->IsVisible());
    }
    else if (teamIdMode == 2)
    {
        m_pHPLabel->SetVisible(true);
        m_pArrowLabel->SetVisible(true);
        m_pWeaponIcon->SetVisible(m_hLastWeapon != nullptr);
        for (int i = 0; i < 4; i++)
            m_pGrenadeIcons[i]->SetVisible(m_pGrenadeIcons[i]->IsVisible());
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
    for (int i = 0; i < 4; i++)
        m_pGrenadeIcons[i]->SetVisible(false);
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
    const int iBone = pPlayer->LookupBone("head_0");
    if (iBone == -1)
        return false;

    Vector vecOrigin;
    QAngle dummy;
    pPlayer->GetBonePosition(iBone, vecOrigin, dummy);
    vecOrigin.z += 12.0f;

    Vector screen;
    if (ScreenTransform(vecOrigin, screen) != 0)
        return false;

    int screenWide, screenTall;
    vgui::surface()->GetScreenSize(screenWide, screenTall);

    screenPos.x = 0.5f * (1.0f + screen.x) * screenWide;
    screenPos.y = 0.5f * (1.0f - screen.y) * screenTall;
    return true;
}