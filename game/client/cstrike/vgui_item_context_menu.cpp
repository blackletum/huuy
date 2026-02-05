#include "cbase.h"
#include "vgui_item_context_menu.h"
#include <vgui/IInput.h>
#include <KeyValues.h>

using namespace vgui;

CItemContextMenu::CItemContextMenu(Panel *parent, const char *panelName)
    : BaseClass(parent, panelName)
{
    m_iCurrentPaintKit = 0;
    m_CurrentWeaponID = WEAPON_NONE;
    m_szWeaponName[0] = '\0';
    m_szSkinName[0] = '\0';
    
    AddMenuItem("Inspect", new KeyValues("MenuCommand", "command", "inspect"), parent);
    AddMenuItem("Equip", new KeyValues("MenuCommand", "command", "equip"), parent);
}

CItemContextMenu::~CItemContextMenu()
{
}

void CItemContextMenu::PerformLayout()
{
    BaseClass::PerformLayout();
    int menuwight = 100;
    int menutall = 60;
    SetSize(menuwight, menutall);
}
void CItemContextMenu::ShowForItem(int iPaintKit, CSWeaponID weaponID, const char *pszWeaponName, const char *pszSkinName, int x, int y)
{
    m_iCurrentPaintKit = iPaintKit;
    m_CurrentWeaponID = weaponID;
    int screenWidth, screenHeight;
    vgui::surface()->GetScreenSize(screenWidth, screenHeight);
    Q_strncpy(m_szWeaponName, pszWeaponName, sizeof(m_szWeaponName));
    Q_strncpy(m_szSkinName, pszSkinName, sizeof(m_szSkinName));
    
    SetVisible(true);
    SetPos(x, y);
    MoveToFront();
    RequestFocus();
}
