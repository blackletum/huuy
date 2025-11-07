//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: a small piece of HUD that shows alive counter and win counter for each team
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef CS_HUD_TEAMCOUNTER_H
#define CS_HUD_TEAMCOUNTER_H
#ifdef _WIN32
#pragma once
#endif

#include "hudelement.h"
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/VectorImagePanel.h>
#include <vgui/ILocalize.h>
#include "utlvector.h"
#include "vgui_avatarimage_nonsteam.h"

enum VIEW_MODE
{
    VIEW_MODE_NORMAL = 0,
    VIEW_MODE_GUN_GAME_PROGRESSIVE,
    VIEW_MODE_GUN_GAME_BOMB,
    VIEW_MODE_NUM
};

struct MiniStatus
{
    int nPlayerIdx;
    int nEntIdx;
    int nHealth;
    int nArmor;
    int nPoints;
    int nGunGameLevel;
    int nGGProgressiveRank;
    bool bIsCT;
    bool bIsLocalPlayer;
    bool bDead;
    bool bTeamLeader;

    MiniStatus() { Reset(); }

    void Reset()
    {
        nPlayerIdx = -1;
        nEntIdx = -1;
        nHealth = 0;
        nArmor = 0;
        nPoints = 0;
        nGunGameLevel = -1;
        nGGProgressiveRank = -1;
        bIsCT = false;
        bIsLocalPlayer = false;
        bDead = true;
        bTeamLeader = false;
    }

    bool Update(int idx, int entIdx, int health, int armor, bool isCT, bool isLocal, bool dead, bool leader, int points, int ggLevel, int team)
    {
        bool bChanged = (nPlayerIdx != idx || nEntIdx != entIdx || nHealth != health || nArmor != armor ||
                         bIsCT != isCT || bIsLocalPlayer != isLocal || bDead != dead || bTeamLeader != leader ||
                         nPoints != points || nGunGameLevel != ggLevel);
        nPlayerIdx = idx;
        nEntIdx = entIdx;
        nHealth = health;
        nArmor = armor;
        bIsCT = isCT;
        bIsLocalPlayer = isLocal;
        bDead = dead;
        bTeamLeader = leader;
        nPoints = points;
        nGunGameLevel = ggLevel;
        return bChanged;
    }
};

class CHudTeamCounter : public CHudElement, public vgui::EditablePanel
{
    DECLARE_CLASS_SIMPLE(CHudTeamCounter, vgui::EditablePanel);

public:
    CHudTeamCounter(const char *pElementName);
    virtual ~CHudTeamCounter();

    virtual void Init();
    virtual void Shutdown();
    virtual void OnScreenSizeChanged(int iOldWide, int iOldTall);
    virtual void ApplySettings(KeyValues *inResourceData);
    virtual void Reset();
    virtual bool ShouldDraw();
    virtual void OnThink();
    virtual void FireGameEvent(IGameEvent *event);

protected:
    void UpdateTimer();
    void UpdateScore();
    void UpdateMiniScoreboard();
    void SetViewMode(VIEW_MODE mode);
    void ResetLeader();
    
    static const int MAX_TEAM_SIZE = 16;
    static const int MAX_GGPROG_PLAYERS = 24;
    static const int kTimeRemainingToDisplayRed = 11;

private:
    vgui::Label *m_pCTWinCounterLabel;
    vgui::Label *m_pCTAliveCounterLabel;
    vgui::Label *m_pCTAliveTextLabel;
    vgui::Label *m_pTWinCounterLabel;
    vgui::Label *m_pTAliveCounterLabel;
    vgui::Label *m_pTAliveTextLabel;
    vgui::Label *m_pRoundTimerLabel;
    vgui::VectorImagePanel *m_pBombIcon;
    vgui::ImagePanel *m_pCTSkullImage;
    vgui::ImagePanel *m_pTSkullImage;
    vgui::Label *m_pProgressiveLeaderLabel;
    
    C_CSPlayer *pPlayer;

    CAvatarImagePanel *m_CTPlayerIcons[MAX_TEAM_SIZE];
CAvatarImagePanel *m_TPlayerIcons[MAX_TEAM_SIZE];
    vgui::ImagePanel *m_CTSkulls[MAX_TEAM_SIZE];
    vgui::Label *m_CTPlayerNames[MAX_TEAM_SIZE];
    vgui::Label *m_CTPlayerStatus[MAX_TEAM_SIZE];
    vgui::ImagePanel *m_TSkulls[MAX_TEAM_SIZE];
    vgui::Label *m_TPlayerNames[MAX_TEAM_SIZE];
    vgui::Label *m_TPlayerStatus[MAX_TEAM_SIZE];

    MiniStatus m_CTTeam[MAX_TEAM_SIZE];
    MiniStatus m_TTeam[MAX_TEAM_SIZE];
    MiniStatus m_GGProgressivePlayers[MAX_GGPROG_PLAYERS];
    CUtlVector<MiniStatus *> m_ggSortedList;

    VIEW_MODE m_Mode;
    bool m_bTimerAlertTriggered;
    bool m_bRoundStarted;
    bool m_bIsBombDefused;
    bool m_bTimerHidden;
    bool m_bIsAtTheBottom;
    bool m_bForceRefresh;
    int m_nTScoreLastUpdate;
    int m_nCTScoreLastUpdate;
    int m_nTerroristTeamCount;
    int m_nCTTeamCount;
    int m_nPreviousGGProgressiveTotalPlayers;
    int m_iOriginalXPos;
    int m_iOriginalYPos;
    float m_flPlayingTeamFadeoutTime;
    bool m_bActive;

    Color m_clrC4Planted;
    Color m_clrC4Defused;

private:
    int m_nLastAvatarPlayerIdx_CT[MAX_TEAM_SIZE];
    int m_nLastAvatarPlayerIdx_T[MAX_TEAM_SIZE];
    
};

#endif // CS_HUD_TEAMCOUNTER_H