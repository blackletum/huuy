#ifndef CS_HUD_PLAYERPANEL_H
#define CS_HUD_PLAYERPANEL_H
#pragma once

#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/ProgressBar.h>
#include "vgui_avatarimage_nonsteam.h"
#include "c_cs_player.h"

class CCSPlayerPanel : public vgui::EditablePanel
{
    DECLARE_CLASS_SIMPLE(CCSPlayerPanel, vgui::EditablePanel);

public:
    CCSPlayerPanel(vgui::Panel *parent, const char *panelName);
    virtual ~CCSPlayerPanel();

    virtual void ApplySchemeSettings(vgui::IScheme *pScheme) OVERRIDE;
    virtual void OnThink() OVERRIDE;
    virtual void PerformLayout() OVERRIDE;

    void SetPlayer(C_CSPlayer *pPlayer);
    void UpdatePlayerData();
    void Clear();

    C_CSPlayer* GetPlayer() const { return m_pPlayer; }

private:
    C_CSPlayer *m_pPlayer;
    
    CAvatarImagePanel *m_pAvatarImage;  
    vgui::Label *m_pPlayerName;
    vgui::ContinuousProgressBar *m_pHealthBar;
    vgui::Panel *m_pHealthBarBG;

    Color m_HealthColor;
    Color m_HealthColorLow;
    Color m_BackgroundColor;
};

#endif // CS_HUD_PLAYERPANEL_H