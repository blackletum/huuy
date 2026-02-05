#pragma once

#include "hudelement.h"
#include "vgui_controls/EditablePanel.h"
#include "vgui_controls/Label.h"
#include "utlvector.h"
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <vgui_controls/TextEntry.h>
#include "vgui_controls/VectorImagePanel.h"  
#include "lunasvg/lunasvg.h"
#include "c_cs_player.h"


class CPlayerNamePanel : public vgui::EditablePanel
{
    DECLARE_CLASS_SIMPLE(CPlayerNamePanel, vgui::EditablePanel);

public:
    CPlayerNamePanel(vgui::Panel* parent);
    virtual void Update(C_BasePlayer* pPlayer, const Vector& screenPos);
    virtual void Reset();

protected:
    virtual void UpdateNameLabel(C_BasePlayer* pPlayer);
    virtual void UpdateHealthLabel(C_BasePlayer* pPlayer);
    virtual void UpdateWeaponIcon(C_BaseCombatWeapon* pWeapon, C_CSPlayer* pCSPlayer);
    virtual void UpdateArrowLabel();
    virtual void UpdatePosition(const Vector& screenPos);
    virtual void ApplyVisibilitySettings(bool isFreezetime);

private:
    vgui::Label* m_pNameLabel;
    vgui::Label* m_pHPLabel;
    vgui::VectorImagePanel* m_pWeaponIcon;
    vgui::VectorImagePanel* m_pGrenadeIcons[4]; 
    vgui::Label* m_pArrowLabel;
    vgui::HFont m_hFont;

    Vector2D m_currentPos;
    Vector2D m_targetPos;
    float m_flLerpFactor;
    int m_iLastTeam;
    Color m_teamColor;
    int m_iLastHealth;
    CHandle<C_BaseCombatWeapon> m_hLastWeapon;
};

class CHudPlayerName : public CHudElement, public vgui::EditablePanel
{
    DECLARE_CLASS_SIMPLE(CHudPlayerName, vgui::EditablePanel);

public:
    CHudPlayerName(const char* pElementName);
    virtual void Init();
    virtual void Reset();
    virtual void OnThink();
    void Update(C_BasePlayer* pPlayer, const Vector& screenPos);

private:
    bool GetHeadScreenPosition(C_BasePlayer* pPlayer, Vector& screenPos);

    CUtlVector<CPlayerNamePanel*> m_PlayerPanels;
};