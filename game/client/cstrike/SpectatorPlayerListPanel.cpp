#include "cbase.h"
#include "SpectatorPlayerListPanel.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include "vgui/IVGui.h"
#include "vgui_controls/Label.h"
#include "c_cs_player.h"
#include "weapon_csbase.h"
#include "vgui/IVGui.h"

SpectatorPlayerPanel::SpectatorPlayerPanel(vgui::Panel* parent, const char* name)
    : BaseClass(parent, name)
{
    SetMouseInputEnabled(false);
    SetPaintBackgroundEnabled(true);
    SetPaintBackgroundType(0);
    SetVisible(true);
    
    LoadControlSettings("resource/ui/SpectatorUIListPanel.res");

    m_pAvatar     = dynamic_cast<CAvatarImagePanel*>(FindChildByName("Avatar"));
    m_pHPBar      = dynamic_cast<vgui::ContinuousProgressBar*>(FindChildByName("HPBar"));
    m_pHPLabel    = dynamic_cast<vgui::Label*>(FindChildByName("HPLabel"));
    m_pWeaponIcon = dynamic_cast<VectorImagePanel*>(FindChildByName("WeaponIcon"));
    m_pWeaponName = dynamic_cast<vgui::Label*>(FindChildByName("WeaponName"));
}

extern ConVar mat_blur_strength;
extern ConVar mat_blur_desaturate;

void SpectatorPlayerPanel::PaintBackground()
{
	int x, y, w, h;
	GetSize(w, h);
	
	surface()->DrawSetColor(Color(0, 0, 0, 200));
    surface()->DrawFilledRect(0, 0, w, h);
	
        GetBounds( x, y, w, h );
        DoBlurFade( mat_blur_strength.GetFloat(), mat_blur_desaturate.GetFloat(), x, y, w, h );
}

void SpectatorPlayerPanel::OnThink()
{
    if (!pPlayer || !pPlayer->IsAlive())
    {
        SetVisible(false);
        return;
    }
    
    SetVisible(true);

    int hp = pPlayer->GetHealth();
    if (m_pHPBar)   m_pHPBar->SetProgress(hp / 100.0f);
    if (m_pHPLabel)
    {
        wchar_t buf[8];
        V_swprintf_safe(buf, L"%d", hp);
        m_pHPLabel->SetText(buf);
    }

    if (m_pAvatar)
        m_pAvatar->SetPlayer(pPlayer, k_EAvatarSize64x64);

    if (C_WeaponCSBase *w = static_cast<C_WeaponCSBase*>(pPlayer->GetActiveWeapon()))
    {
        const char *szIcon = w->GetClassname() + 7;
        char path[MAX_PATH];
        Q_snprintf(path, sizeof(path), "vgui/hud/weapons/%s.svg", szIcon);

        if (m_pWeaponIcon) m_pWeaponIcon->SetTexture(path);
        if (m_pWeaponName)
        {
            wchar_t name[64];
            g_pVGuiLocalize->ConvertANSIToUnicode(w->GetPrintName() + 1, name, sizeof(name));
            m_pWeaponName->SetText(name);
        }
    }
}