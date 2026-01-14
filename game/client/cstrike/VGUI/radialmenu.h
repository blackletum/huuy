#ifndef VGUI_RADIALMENU_H
#define VGUI_RADIALMENU_H

#include <vgui_controls/EditablePanel.h>
#include <vgui/ISurface.h>
#include "VGuiMatSurface/IMatSystemSurface.h"
#include "cs_weapon_parse.h"

#define RADIAL_SEGMENTS 64

namespace vgui
{

class RadialMenuSection : public Panel
{
    DECLARE_CLASS_SIMPLE( RadialMenuSection, Panel );

public:
    RadialMenuSection( Panel *parent, const char *name );
    ~RadialMenuSection();

    void SetAngles( float startAngle, float endAngle );
    void SetRadii( int innerRadius, int outerRadius );
    void SetIcon( const char *pszTexturePath, int wide, int tall );
    void SetLabel( const wchar_t *text );
    void SetPrice( int price );
    void SetWeaponID( CSWeaponID id ) { m_nWeaponID = id; }
    void SetAvailable( bool available ) { m_bAvailable = available; }
    void SetCommand( const char *cmd );
    const char* GetCommand() { return m_szCommand; }
    CSWeaponID GetWeaponID() { return m_nWeaponID; }
    int GetPrice() { return m_iPrice; }

    virtual void Paint();
    virtual void ApplySettings( KeyValues *inResourceData );
    virtual void ApplySchemeSettings( IScheme *pScheme );
    virtual void OnCursorEntered();
    virtual void OnCursorExited();
    virtual void OnMousePressed( MouseCode code );
    virtual void OnMouseReleased( MouseCode code );

    bool IsPointInSection( int x, int y, int centerX, int centerY );
    bool IsHovered() { return m_bHovered; }

    void DrawFilledArc( int centerX, int centerY, int outerRadius, int innerRadius,
                        float startAngle, float endAngle, Color color );
    void DrawTexturedArc( int centerX, int centerY, int outerRadius, int innerRadius,
                          float startAngle, float endAngle, int textureID, Color color );

    // Углы и радиусы
    float m_flStartAngle;
    float m_flEndAngle;
    int m_iInnerRadius;
    int m_iOuterRadius;
    
    // Иконка
    int m_nIconTextureID;
    int m_iIconSize[2];
    
    // Текст
    wchar_t m_wszLabel[64];
    wchar_t m_wszPrice[16];
    char m_szCommand[128];
    
    // Данные оружия
    CSWeaponID m_nWeaponID;
    int m_iPrice;

    // Состояние
    bool m_bHovered;
    bool m_bPressed;
    bool m_bAvailable;

    // Текстуры фона
    int m_nBgTextureID;
    int m_nBgTextureArmedID;
    int m_nBgTextureDepressedID;

    // Цвета
    Color m_clrFg;
    Color m_clrBg;
    Color m_clrFgArmed;
    Color m_clrBgArmed;
    Color m_clrFgDepressed;
    Color m_clrBgDepressed;
    Color m_clrFgDisabled;
    Color m_clrBgDisabled;
    Color m_clrPriceAvailable;
    Color m_clrPriceUnavailable;

    // Шрифты
    HFont m_hFont;
    HFont m_hPriceFont;

    // Параметры расположения
    float m_flNumberInsetRadius;
    float m_flTextInsetRadius;
    float m_flInfoInsetRadius;
    int m_iPriceInsetX;
    int m_iPriceInsetY;

    // Звуки
    char m_szSoundArmed[128];
    char m_szSoundReleased[128];
};

class RadialMenu : public EditablePanel
{
    DECLARE_CLASS_SIMPLE( RadialMenu, EditablePanel );

public:
    RadialMenu( Panel *parent, const char *name );
    ~RadialMenu();

    void AddSection( const char *command, const wchar_t *label, const char *iconPath, 
                     int price = 0, CSWeaponID weaponID = WEAPON_NONE );
    void ClearSections();
    void SetRadii( int inner, int outer );
    
    RadialMenuSection* GetHoveredSection();
    RadialMenuSection* GetSection( int index );
    int GetSectionCount() { return m_Sections.Count(); }

    virtual void Paint();
    virtual void ApplySettings( KeyValues *inResourceData );
    virtual void ApplySchemeSettings( IScheme *pScheme );
    virtual void OnCursorMoved( int x, int y );
    virtual void OnMousePressed( MouseCode code );
    virtual void OnThink();

    bool m_bIsItemMenu;

protected:
    void UpdateSectionAngles();
    void ApplySectionSettings( RadialMenuSection *section );

    CUtlVector<RadialMenuSection*> m_Sections;
    int m_iInnerRadius;
    int m_iOuterRadius;
    int m_iIconWide;
    int m_iIconTall;
    float m_flGapAngle;
    
    HFont m_hLabelFont;
    HFont m_hPriceFont;
    
    Color m_clrCenterBg;

    KeyValues *m_pItemButtonSettings;
    KeyValues *m_pCategoryButtonSettings;
};

} // namespace vgui

#endif // VGUI_RADIALMENU_H