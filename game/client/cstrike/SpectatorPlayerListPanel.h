#ifdef _WIN32
#pragma once
#endif

#include "hudelement.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include "vgui/IVGui.h"
#include "vgui_borderprogress.h"
#include "viewpostprocess.h"
#include "vgui_controls/Panel.h"
#include "vgui_avatarimage_nonsteam.h"
#include "vgui_controls/Label.h"
#include "vgui_controls/EditablePanel.h"
#include "c_cs_player.h"
#include "vgui_controls/VectorImagePanel.h"

using namespace vgui;

class SpectatorPlayerPanel : public vgui::EditablePanel
{
    DECLARE_CLASS_SIMPLE(SpectatorPlayerPanel, vgui::EditablePanel);

public:
    SpectatorPlayerPanel(vgui::Panel* parent, const char* name);

    virtual void PaintBackground();
    virtual void OnThink() override;
    
    C_CSPlayer* pPlayer;

private:
    CAvatarImagePanel      *m_pAvatar;
    vgui::ContinuousProgressBar *m_pHPBar;
    vgui::Label            *m_pHPLabel;
    VectorImagePanel       *m_pWeaponIcon;
    vgui::Label            *m_pWeaponName;
};