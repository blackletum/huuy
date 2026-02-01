#include "cbase.h"
#include "vgui_endgame_panel.h"
#include "c_cs_playerresource.h"
#include "cs_gamerules.h"
#include <vgui/IVGui.h>
#include <ienginevgui.h>
#include "c_team.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/Button.h>
#include "GameEventListener.h"
#include "viewpostprocess.h"

using namespace vgui;

CGameResultPanel *g_pGameResultPanel = NULL;

static CGameResultPanel* GetEndgamePanel()
{
    static CGameResultPanel *s_pPanel = NULL;
    
    if (!s_pPanel)
    {
        s_pPanel = new CGameResultPanel(enginevgui->GetPanel(PANEL_CLIENTDLL));
        
        if (s_pPanel)
        {
            vgui::ivgui()->AddTickSignal(s_pPanel->GetVPanel(), 100);
            Msg("[ENDGAME] Panel created successfully!\n");
        }
        else
        {
            Warning("[ENDGAME] Failed to create panel!\n");
        }
    }
    
    return s_pPanel;
}

CGameResultPanel::CGameResultPanel(vgui::VPANEL parent) : BaseClass(NULL, "GameResultPanel")
{
    SetParent(parent);
    int wide, tall;
    surface()->GetScreenSize(wide, tall);
    SetSize(wide, tall);
    SetTitle("", true);
    SetMoveable(false);
    SetSizeable(false);
    SetCloseButtonVisible(false);
    
    m_pResultLabel = new Label(this, "ResultLabel", "");
    m_pContinueButton = new Button(this, "ContinueButton", "Continue", this, "continue");
    
    SetVisible(false);
    SetScheme("ClientScheme");
    LoadControlSettings("resource/UI/GameResultPanel.res");
    
    ListenForGameEvent( "announce_phase_end" );
    
    g_pGameResultPanel = this;
    m_gameOver = false;
    
    Msg("[ENDGAME] Panel created\n");
}

CGameResultPanel::~CGameResultPanel()
{
    g_pGameResultPanel = NULL;
}

void CGameResultPanel::ShowResult(bool bWin)
{
    Msg("[ENDGAME] ShowResult called: %s\n", bWin ? "WIN" : "LOSE");
    
    if (bWin)
    {
        m_pResultLabel->SetText("#Game_Win");
        SetBgColor(Color(0, 128, 0, 220));
    }
    else
    {
        m_pResultLabel->SetText("#Game_Lose");
        SetBgColor(Color(128, 0, 0, 220));
    }
    
    SetVisible(true);
    MoveToFront();
    RequestFocus();
    
    int wide, tall;
    surface()->GetScreenSize(wide, tall);
    SetPos((wide - GetWide()) / 2, (tall - GetTall()) / 2);
    
    m_bShownResult = true;
    
    Msg("[ENDGAME] Panel should be visible now\n");
}

void CGameResultPanel::OnCommand(const char *command)
{
    if (!Q_strcmp(command, "continue"))
    {
        SetVisible(false);
        engine->ClientCmd("disconnect");
    }
    
    BaseClass::OnCommand(command);
}

void CGameResultPanel::ApplySchemeSettings(vgui::IScheme *pScheme)
{
    BaseClass::ApplySchemeSettings(pScheme);
    
    if (m_pResultLabel)
    {
        m_pResultLabel->SetFont(pScheme->GetFont("HudNumbers", true));
        m_pResultLabel->SetFgColor(Color(255, 255, 255, 255));
    }
}

void CGameResultPanel::OnThink()
{
    BaseClass::OnThink();
}

void CGameResultPanel::UpdateWinLose()
{
    BaseClass::OnThink();
    
    if (m_bShownResult)
        return;
    
    if (!CSGameRules())
        return;
    
    if (CSGameRules()->GetGamePhase() == GAMEPHASE_MATCH_ENDED)
    {
        C_CSPlayer *pLocalPlayer = C_CSPlayer::GetLocalCSPlayer();
        if (!pLocalPlayer)
            return;
        
        bool bLocalPlayerWon = false;
        
        int iLocalTeam = pLocalPlayer->GetTeamNumber();
        
        C_Team *pLocalTeam = GetGlobalTeam(iLocalTeam);
        if (pLocalTeam)
        {
            int iLocalTeamScore = pLocalTeam->Get_Score();
            
            int iEnemyTeam = (iLocalTeam == TEAM_CT) ? TEAM_TERRORIST : TEAM_CT;
            C_Team *pEnemyTeam = GetGlobalTeam(iEnemyTeam);
            
            if (pEnemyTeam)
            {
                int iEnemyScore = pEnemyTeam->Get_Score();
                bLocalPlayerWon = (iLocalTeamScore > iEnemyScore);
            }
        }
        
        ShowResult(bLocalPlayerWon);
    }
}

void CGameResultPanel::FireGameEvent( IGameEvent *event )
{
	if ( event == NULL )
		return;

    const char *pEventName = event->GetName();
	if ( pEventName == NULL )
		return;

    if ( Q_strcmp( pEventName, "cs_win_panel_match" ) == 0 )
    {
        if ( Q_strcmp( pEventName, "announce_phase_end" ) == 0 )
        {
            m_gameOver = true;
            UpdateWinLose();
            ShowResult(true);
        }
    }
	//BaseClass::FireGameEvent( event );
}

extern ConVar mat_blur_strength;
extern ConVar mat_blur_desaturate;
void CGameResultPanel::PaintBackground()
{
    if ( engine->GetDXSupportLevel() < 90 )
		BaseClass::PaintBackground();
	else
	{
		// do the blur here instead of clientmode because it needs to render over VGUI elements
		int x, y, w, h;
		GetBounds( x, y, w, h );
		DoBlurFade( mat_blur_strength.GetFloat(), mat_blur_desaturate.GetFloat(), x, y, w, h );
	}
}

void CGameResultPanel::Reset()
{
    m_bShownResult = false;
    SetVisible(false);
}

CON_COMMAND(show_endgame_win, "Test endgame panel - WIN")
{
    if (g_pGameResultPanel)
    {
        g_pGameResultPanel->ShowResult(true);
    }
    else
    {
        Warning("Panel not created!\n");
    }
}

CON_COMMAND(show_endgame_lose, "Test endgame panel - LOSE")
{
    if (g_pGameResultPanel)
    {
        g_pGameResultPanel->ShowResult(false);
    }
    else
    {
        Warning("Panel not created!\n");
    }
}

CON_COMMAND(hide_endgame, "Hide endgame panel")
{
    if (g_pGameResultPanel)
    {
        g_pGameResultPanel->SetVisible(false);
    }
}
