//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: a small piece of HUD that shows alive counter and win counter for each team
//
// $NoKeywords: $
//
//=============================================================================//

#include "cbase.h"
#include "iclientmode.h"
#include "hudelement.h"
#include "c_cs_player.h"
#include "c_cs_team.h"
#include "c_cs_playerresource.h"
#include "cs_gamerules.h"
#include <vgui_controls/AnimationController.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/VectorImagePanel.h>
#include <vgui/ILocalize.h>
#include "c_plantedc4.h"
#include "cs_hud_teamcounter.h"

using namespace vgui;

ConVar hud_playercount_pos("hud_playercount_pos", "0", FCVAR_ARCHIVE, "0 = default (top), 1 = bottom");
extern ConVar cl_draw_only_deathnotices;

DECLARE_HUDELEMENT(CHudTeamCounter);

CHudTeamCounter::CHudTeamCounter(const char *pElementName) : CHudElement(pElementName), EditablePanel(NULL, "HudTeamCounter")
{
    vgui::Panel *pParent = g_pClientMode->GetViewport();
    SetParent(pParent);

    SetHiddenBits(HIDEHUD_PLAYERDEAD);

    m_pCTWinCounterLabel = new Label(this, "CTWinCounterLabel", "0");
    m_pCTAliveCounterLabel = new Label(this, "CTAliveCounterLabel", "0");
    m_pCTAliveTextLabel = new Label(this, "CTAliveTextLabel", "#Cstrike_PlayerCount_Alive");
    m_pTWinCounterLabel = new Label(this, "TWinCounterLabel", "0");
    m_pTAliveCounterLabel = new Label(this, "TAliveCounterLabel", "0");
    m_pTAliveTextLabel = new Label(this, "TAliveTextLabel", "#Cstrike_PlayerCount_Alive");
    m_pRoundTimerLabel = new Label(this, "RoundTimerLabel", "0:00");
    m_pBombIcon = new VectorImagePanel(this, "BombIcon");
    m_pCTSkullImage = new ImagePanel(this, "CTSkullImage");
    m_pTSkullImage = new ImagePanel(this, "TSkullImage");
    m_pProgressiveLeaderLabel = new Label(this, "ProgressiveLeaderLabel", "");

    for (int i = 0; i < MAX_TEAM_SIZE; ++i)
{
    char panelName[32];
    Q_snprintf(panelName, sizeof(panelName), "CTPlayerIcon%d", i);
    m_CTPlayerIcons[i] = new CAvatarImagePanel(this, panelName);
    m_CTPlayerIcons[i]->SetShouldScaleImage(true); // Масштабировать изображение под размер панели
    m_CTPlayerIcons[i]->SetShouldDrawFriendIcon(false); // Отключить иконку друга, если не нужна

    Q_snprintf(panelName, sizeof(panelName), "CTSkull%d", i);
    m_CTSkulls[i] = new ImagePanel(this, panelName);
    m_CTSkulls[i]->SetImage("vgui/hud/skull_icon");

    Q_snprintf(panelName, sizeof(panelName), "TPlayerIcon%d", i);
    m_TPlayerIcons[i] = new CAvatarImagePanel(this, panelName);
    m_TPlayerIcons[i]->SetShouldScaleImage(true); // Масштабировать изображение под размер панели
    m_TPlayerIcons[i]->SetShouldDrawFriendIcon(false); // Отключить иконку друга, если не нужна

    Q_snprintf(panelName, sizeof(panelName), "TSkull%d", i);
    m_TSkulls[i] = new ImagePanel(this, panelName);
    m_TSkulls[i]->SetImage("vgui/hud/skull_icon");

    Q_snprintf(panelName, sizeof(panelName), "CTPlayerName%d", i);
    m_CTPlayerNames[i] = new Label(this, panelName, "");

    Q_snprintf(panelName, sizeof(panelName), "TPlayerName%d", i);
    m_TPlayerNames[i] = new Label(this, panelName, "");

    Q_snprintf(panelName, sizeof(panelName), "CTPlayerStatus%d", i);
    m_CTPlayerStatus[i] = new Label(this, panelName, "");

    Q_snprintf(panelName, sizeof(panelName), "TPlayerStatus%d", i);
    m_TPlayerStatus[i] = new Label(this, panelName, "");
    
    m_nLastAvatarPlayerIdx_CT[i] = -1;
    m_nLastAvatarPlayerIdx_T[i] = -1;
}

    m_bTimerAlertTriggered = false;
    m_bRoundStarted = true;
    m_bIsBombDefused = false;
    m_bTimerHidden = false;
    m_nTScoreLastUpdate = -1;
    m_nCTScoreLastUpdate = -1;
    m_nTerroristTeamCount = 0;
    m_nCTTeamCount = 0;
    m_flPlayingTeamFadeoutTime = -1;
    m_bActive = true; 
}

CHudTeamCounter::~CHudTeamCounter()
{
}

void CHudTeamCounter::Init()
{
    m_bTimerAlertTriggered = false;
    m_bRoundStarted = true;
    m_bIsBombDefused = false;
    m_bTimerHidden = false;
    m_nTScoreLastUpdate = -1;
    m_nCTScoreLastUpdate = -1;

    gameeventmanager->AddListener(this, "round_start", false);
    gameeventmanager->AddListener(this, "round_announce_warmup", false);
    gameeventmanager->AddListener(this, "round_end", false);
    gameeventmanager->AddListener(this, "cs_match_end_restart", false);
    gameeventmanager->AddListener(this, "bomb_planted", false);
    gameeventmanager->AddListener(this, "bomb_defused", false);
    gameeventmanager->AddListener(this, "player_spawn", false);
    gameeventmanager->AddListener(this, "player_death", false);
    gameeventmanager->AddListener(this, "player_team", false);
    gameeventmanager->AddListener(this, "bot_takeover", false);

    LoadControlSettings("resource/hud/teamcounter.res");

    for (int i = 0; i < MAX_TEAM_SIZE; ++i)
    {
        m_CTTeam[i].Reset();
        m_TTeam[i].Reset();
    }
}

void CHudTeamCounter::Shutdown()
{
    gameeventmanager->RemoveListener(this);
}

void CHudTeamCounter::OnScreenSizeChanged(int iOldWide, int iOldTall)
{
    LoadControlSettings("resource/hud/teamcounter.res");
}

void CHudTeamCounter::ApplySettings(KeyValues *inResourceData)
{
    BaseClass::ApplySettings(inResourceData);
    GetPos(m_iOriginalXPos, m_iOriginalYPos);
}

void CHudTeamCounter::Reset()
{
    g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("RoundTimerReset");
    m_bTimerAlertTriggered = false;
    m_bRoundStarted = false;
    m_bIsBombDefused = false;
    m_bTimerHidden = false;
    m_nTScoreLastUpdate = -1;
    m_nCTScoreLastUpdate = -1;

    if (CSGameRules())
        m_bRoundStarted = (CSGameRules()->GetRoundStartTime() < gpGlobals->curtime);
}

bool CHudTeamCounter::ShouldDraw()
{
    if (cl_draw_only_deathnotices.GetBool())
        return false;

    C_CSPlayer *pPlayer = C_CSPlayer::GetLocalCSPlayer();
    if (!pPlayer || pPlayer->IsObserver())
        return false;

    return CHudElement::ShouldDraw();
}

void CHudTeamCounter::OnThink()
{
    UpdateTimer();
    UpdateScore();
    UpdateMiniScoreboard();

    if (m_bIsAtTheBottom != hud_playercount_pos.GetBool())
    {
        m_bIsAtTheBottom = hud_playercount_pos.GetBool();
        int ypos = m_bIsAtTheBottom ? (ScreenHeight() - m_iOriginalYPos - GetTall()) : m_iOriginalYPos;
        SetPos(m_iOriginalXPos, ypos);
    }

    if (m_flPlayingTeamFadeoutTime > -1 && m_flPlayingTeamFadeoutTime <= gpGlobals->curtime)
    {
        g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("FadeOutSelectedTeam");
        m_flPlayingTeamFadeoutTime = -1;
    }
}

void CHudTeamCounter::UpdateTimer()
{
    C_CSGameRules *pRules = CSGameRules();
    if (!pRules)
        return;

    bool bBeginTimerAlert = false;
    bool bCancelTimerAlert = false;

    if (g_PlantedC4s.Count() > 0)
    {
        if (!m_bTimerHidden)
        {
            m_bTimerHidden = true;
            m_pRoundTimerLabel->SetText(L"");
            m_pBombIcon->SetVisible(true);
        }

        C_PlantedC4 *pC4 = g_PlantedC4s[0];
        if (m_bIsBombDefused)
        {
            m_pBombIcon->SetAlpha(255);
            m_pBombIcon->SetFgColor(m_clrC4Defused);
        }
        else
        {
            int alpha = (gpGlobals->curtime + 0.1f >= pC4->m_flNextGlow) ? 128 : 255;
            m_pBombIcon->SetAlpha(alpha);
            m_pBombIcon->SetFgColor(m_clrC4Planted);
            m_pBombIcon->SetVisible(!pC4->m_bExplodeWarning);
        }
        return;
    }
    else
    {
        m_bTimerHidden = false;
        m_pBombIcon->SetVisible(false);
    }

    if (pRules->IsWarmupPeriod())
    {
        m_pRoundTimerLabel->SetText(L"");
        return;
    }
    else if (pRules->IsFreezePeriod() && pRules->IsMatchWaitingForResume())
    {
        m_pRoundTimerLabel->SetText(L"❚❚");
        g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("RoundTimerAlert");
        return;
    }

    int nTimer = static_cast<int>(ceil(pRules->GetRoundRemainingTime()));
    if (pRules->IsFreezePeriod())
        nTimer = static_cast<int>(ceil(pRules->GetRoundStartTime() - gpGlobals->curtime));

    if (m_bRoundStarted)
    {
        if (!m_bTimerAlertTriggered && nTimer < kTimeRemainingToDisplayRed)
        {
            m_bTimerAlertTriggered = true;
            bBeginTimerAlert = true;
        }
        else if (m_bTimerAlertTriggered && nTimer >= kTimeRemainingToDisplayRed)
        {
            m_bTimerAlertTriggered = false;
            bCancelTimerAlert = true;
        }
    }

    if (nTimer < 0)
        nTimer = 0;

    int nMinutes = nTimer / 60;
    int nSeconds = nTimer % 60;

    wchar_t szTime[32];
    if (m_bRoundStarted)
    {
        V_snwprintf(szTime, ARRAYSIZE(szTime), L"%d:%.2d", nMinutes, nSeconds);
        m_pRoundTimerLabel->SetText(szTime);
    }

    if (bCancelTimerAlert)
        g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("RoundTimerNormal");
    if (bBeginTimerAlert)
        g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("RoundTimerLow");
}

void CHudTeamCounter::UpdateScore()
{
    if (!g_PR || !CSGameRules() || !m_bActive)
        return;

    int nCTScore = 0;
    int nTScore = 0;

    C_CSTeam *CT_team = GetGlobalCSTeam(TEAM_CT);
    if (CT_team)
        nCTScore = CT_team->Get_Score();

    C_CSTeam *T_team = GetGlobalCSTeam(TEAM_TERRORIST);
    if (T_team)
        nTScore = T_team->Get_Score();

    wchar_t unicode[16];
    if (nCTScore != m_nCTScoreLastUpdate)
    {
        m_nCTScoreLastUpdate = nCTScore;
        V_snwprintf(unicode, ARRAYSIZE(unicode), L"%d", nCTScore);
        m_pCTWinCounterLabel->SetText(unicode);
    }

    if (nTScore != m_nTScoreLastUpdate)
    {
        m_nTScoreLastUpdate = nTScore;
        V_snwprintf(unicode, ARRAYSIZE(unicode), L"%d", nTScore);
        m_pTWinCounterLabel->SetText(unicode);
    }

    int iCTCounter = 0, iTCounter = 0;
    for (int playerIndex = 1; playerIndex <= MAX_PLAYERS; playerIndex++)
    {
        if (g_PR->IsConnected(playerIndex) && g_PR->IsAlive(playerIndex))
        {
            if (g_PR->GetTeam(playerIndex) == TEAM_CT)
                iCTCounter++;
            if (g_PR->GetTeam(playerIndex) == TEAM_TERRORIST)
                iTCounter++;
        }
    }

    V_snwprintf(unicode, ARRAYSIZE(unicode), L"%d", iCTCounter);
    m_pCTAliveCounterLabel->SetText(unicode);
    V_snwprintf(unicode, ARRAYSIZE(unicode), L"%d", iTCounter);
    m_pTAliveCounterLabel->SetText(unicode);

    m_pCTAliveCounterLabel->SetVisible(iCTCounter > 0);
    m_pCTAliveTextLabel->SetVisible(iCTCounter > 0);
    m_pTAliveCounterLabel->SetVisible(iTCounter > 0);
    m_pTAliveTextLabel->SetVisible(iTCounter > 0);
    m_pCTSkullImage->SetVisible(iCTCounter < 1);
    m_pTSkullImage->SetVisible(iTCounter < 1);
}

static C_CSPlayer* GetPlayerByIndex(int iIndex)
{
    if (iIndex < 1 || iIndex > gpGlobals->maxClients)
        return nullptr;
    return static_cast<C_CSPlayer*>(UTIL_PlayerByIndex(iIndex));
}

void CHudTeamCounter::UpdateMiniScoreboard()
{
    if (!CSGameRules() || !g_PR)
        return;

    C_CSPlayer *pLocalPlayer = C_CSPlayer::GetLocalCSPlayer();
    if (!pLocalPlayer)
        return;

    int nCTTeamCount = 0, nTerroristTeamCount = 0;

    // === Сброс всех слотов ===
    for (int i = 0; i < MAX_TEAM_SIZE; ++i)
    {
        m_CTTeam[i].Reset();
        m_TTeam[i].Reset();
    }

    // === Заполняем слоты игроками ===
    for (int playerIndex = 1; playerIndex <= MAX_PLAYERS; ++playerIndex)
    {
        if (!g_PR->IsConnected(playerIndex))
            continue;

        int teamId = g_PR->GetTeam(playerIndex);
        if (teamId != TEAM_CT && teamId != TEAM_TERRORIST)
            continue;

        C_CSPlayer *pPlayer = GetPlayerByIndex(playerIndex);
        if (!pPlayer)
            continue;

        bool bIsCT = (teamId == TEAM_CT);
        bool bDead = !g_PR->IsAlive(playerIndex);
        bool bIsLocal = (playerIndex == pLocalPlayer->entindex());

        int entIdx = pPlayer->entindex();
        int health = pPlayer->GetHealth();
        int armor = pPlayer->ArmorValue();
        int points = 0;
        int ggLevel = -1;

        MiniStatus *ms = nullptr;
        int slotIdx = -1;

        if (bIsCT && nCTTeamCount < MAX_TEAM_SIZE)
        {
            slotIdx = nCTTeamCount++;
            ms = &m_CTTeam[slotIdx];
        }
        else if (!bIsCT && nTerroristTeamCount < MAX_TEAM_SIZE)
        {
            slotIdx = nTerroristTeamCount++;
            ms = &m_TTeam[slotIdx];
        }

        if (!ms)
            continue;

        // === Обновляем статус ===
        bool bChanged = ms->Update(
            playerIndex, entIdx, health, armor,
            bIsCT, bIsLocal, bDead, false, points, ggLevel, teamId
        );

        // === Проверяем, сменился ли игрок в слоте ===
        bool bAvatarChanged = false;
        if (bIsCT)
            bAvatarChanged = (ms->nPlayerIdx != m_nLastAvatarPlayerIdx_CT[slotIdx]);
        else
            bAvatarChanged = (ms->nPlayerIdx != m_nLastAvatarPlayerIdx_T[slotIdx]);

        // === Обновляем аватарку и имя только при необходимости ===
        if (bChanged || m_bForceRefresh || bAvatarChanged)
        {
            if (bIsCT)
            {
                if (bAvatarChanged)
                {
                    m_CTPlayerIcons[slotIdx]->SetPlayer(playerIndex, k_EAvatarSize32x32);
                    m_CTPlayerIcons[slotIdx]->SetDefaultAvatar(GetDefaultAvatarImage(pPlayer));
                    m_nLastAvatarPlayerIdx_CT[slotIdx] = ms->nPlayerIdx; // кэшируем
                }

                char szName[MAX_PLAYER_NAME_LENGTH];
V_strncpy(szName, g_PR->GetPlayerName(playerIndex), sizeof(szName));

wchar_t wszName[MAX_PLAYER_NAME_LENGTH];
g_pVGuiLocalize->ConvertANSIToUnicode(szName, wszName, sizeof(wszName));

m_CTPlayerNames[slotIdx]->SetText(wszName);
            }
            else
            {
                if (bAvatarChanged)
                {
                    m_TPlayerIcons[slotIdx]->SetPlayer(playerIndex, k_EAvatarSize32x32);
                    m_TPlayerIcons[slotIdx]->SetDefaultAvatar(GetDefaultAvatarImage(pPlayer));
                    m_nLastAvatarPlayerIdx_T[slotIdx] = ms->nPlayerIdx;
                }

                char szName[MAX_PLAYER_NAME_LENGTH];
V_strncpy(szName, g_PR->GetPlayerName(playerIndex), sizeof(szName));

wchar_t wszName[MAX_PLAYER_NAME_LENGTH];
g_pVGuiLocalize->ConvertANSIToUnicode(szName, wszName, sizeof(wszName));

                m_TPlayerNames[slotIdx]->SetText(wszName);
            }
        }

        // === Видимость аватара / черепа ===
        if (bIsCT)
        {
            m_CTPlayerIcons[slotIdx]->SetVisible(!bDead);
            m_CTSkulls[slotIdx]->SetVisible(bDead);
        }
        else
        {
            m_TPlayerIcons[slotIdx]->SetVisible(!bDead);
            m_TSkulls[slotIdx]->SetVisible(bDead);
        }
    }

    // === Скрываем пустые слоты ===
    for (int i = nCTTeamCount; i < MAX_TEAM_SIZE; ++i)
    {
        m_CTPlayerIcons[i]->SetVisible(false);
        m_CTSkulls[i]->SetVisible(false);
        m_CTPlayerNames[i]->SetText(L"");
        m_CTPlayerStatus[i]->SetText(L"");
    }
    for (int i = nTerroristTeamCount; i < MAX_TEAM_SIZE; ++i)
    {
        m_TPlayerIcons[i]->SetVisible(false);
        m_TSkulls[i]->SetVisible(false);
        m_TPlayerNames[i]->SetText(L"");
        m_TPlayerStatus[i]->SetText(L"");
    }

    m_nCTTeamCount = nCTTeamCount;
    m_nTerroristTeamCount = nTerroristTeamCount;
    m_bForceRefresh = false;
}

void CHudTeamCounter::FireGameEvent(IGameEvent *event)
{
    const char *type = event->GetName();
    CBasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
    int EventUserID = event->GetInt("userid", -1);
    int LocalPlayerID = pLocalPlayer ? pLocalPlayer->GetUserID() : -2;

    if (!V_strcmp(type, "round_start"))
    {
        m_bRoundStarted = true;
        m_bIsBombDefused = false;
        g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("RoundTimerNormal");
    }
    else if (!V_strcmp(type, "round_announce_warmup"))
    {
        m_pRoundTimerLabel->SetText(L"");
    }
    else if (!V_strcmp(type, "round_end"))
    {
        C_CSGameRules *pRules = CSGameRules();
        if (pRules)
        {
            int nTimer = static_cast<int>(floor(pRules->GetRoundRemainingTime()));
            if (nTimer < 0)
                nTimer = 0;
            wchar_t szTime[32];
            V_snwprintf(szTime, ARRAYSIZE(szTime), L"%d:%.2d", nTimer / 60, nTimer % 60);
            m_pRoundTimerLabel->SetText(szTime);

            int iReason = event->GetInt("reason", -1);
            if (iReason == Bomb_Defused)
                m_bIsBombDefused = true;
        }
        m_bTimerAlertTriggered = false;
        m_bRoundStarted = false;
        g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("HideTeamPanels");
    }
    else if (!V_strcmp(type, "cs_match_end_restart"))
    {
    }
    else if (!V_strcmp(type, "bomb_planted"))
    {
        m_pRoundTimerLabel->SetText(L"");
        m_pBombIcon->SetVisible(true);
        m_pBombIcon->SetAlpha(100);
        m_pBombIcon->SetFgColor(m_clrC4Planted);
    }
    else if (!V_strcmp(type, "bomb_defused"))
    {
        m_bIsBombDefused = true;
    }
    else if (!V_strcmp(type, "player_spawn"))
    {
        UpdateMiniScoreboard();
        if (EventUserID == LocalPlayerID)
        {
            g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("HideTeamPanels");
            int nTeam = event->GetInt("teamnum", -1);
            if (nTeam > 0)
            {
                g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("ShowSelectedTeam");
                m_flPlayingTeamFadeoutTime = gpGlobals->curtime + 10.0f;
            }
            m_bRoundStarted = true;
        }
    }
    else if (!V_strcmp(type, "player_death") && EventUserID == LocalPlayerID)
    {
        g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("HideTeamPanels");
    }
    else if (!V_strcmp(type, "player_team") && EventUserID == LocalPlayerID)
    {
        m_bForceRefresh = true;
        UpdateMiniScoreboard();
    }
    else if (!V_strcmp(type, "bot_takeover") && EventUserID == LocalPlayerID)
    {
        C_BasePlayer *pBot = UTIL_PlayerByUserId(event->GetInt("botid"));
        if (pBot)
        {
            wchar_t wszLocalized[100];
            wchar_t wszPlayerName[MAX_PLAYER_NAME_LENGTH];
            g_pVGuiLocalize->ConvertANSIToUnicode(pBot->GetPlayerName(), wszPlayerName, sizeof(wszPlayerName));
            g_pVGuiLocalize->ConstructString(wszLocalized, sizeof(wszLocalized), g_pVGuiLocalize->Find("#SFUI_Notice_Hint_Bot_Takeover"), 1, wszPlayerName);
            g_pClientMode->GetViewportAnimationController()->StartAnimationSequence("ShowBotTakeover");
            m_flPlayingTeamFadeoutTime = -1;
        }
    }
}

void CHudTeamCounter::ResetLeader()
{
    if (m_pProgressiveLeaderLabel)
        m_pProgressiveLeaderLabel->SetText(L"");
}