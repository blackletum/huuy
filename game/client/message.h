#ifndef MESSAGE_H
#define MESSAGE_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "hudelement.h"
#include "hud_macros.h"
#include "itextmessage.h"
#include "iclientmode.h"
#include "vgui_controls/Controls.h"
#include "vgui_controls/Panel.h"
#include "vgui/ILocalize.h"
#include "vgui/IScheme.h"
#include "vgui/ISurface.h"
#include "client_textmessage.h"
#include "VGuiMatSurface/IMatSystemSurface.h"

using namespace vgui;

// Константа для максимального количества HUD сообщений
#define maxHUDMessages 16

// Структура параметров сообщения
struct message_parms_t
{
	client_textmessage_t	*pMessage;
	float	time;
	int		x, y;
	int		totalWidth, totalHeight;
	int		width;
	int		lines;
	int		lineLength;
	int		length;
	int		r, g, b;
	int		text;
	int		fadeBlend;
	float	charTime;
	float	fadeTime;
	const char *vguiFontName;
	vgui::HFont	font;
};

class CHudMessage: public vgui::Panel, public CHudElement, public ITextMessage 
{
	DECLARE_CLASS_SIMPLE( CHudMessage, vgui::Panel );
public:

	enum
	{
		TYPE_UNKNOWN = 0,
		TYPE_POSITION,
		TYPE_CHARACTER,
		TYPE_FONT,
	};

	struct message_t
	{
		vgui::HFont	font;
		short		x, y;
		wchar_t		ch;
		byte		type;
		byte		r, g, b, a;
	};

	CHudMessage( const char *pElementName );
	~CHudMessage();

	void Init( void );
	void VidInit( void );
	bool ShouldDraw( void );
	virtual void Paint();
	void MsgFunc_HudText(bf_read &msg);
	void MsgFunc_GameTitle(bf_read &msg);
	void MsgFunc_HudMsg(bf_read &msg);

	float FadeBlend( float fadein, float fadeout, float hold, float localTime );
	int	XPosition( float x, int width, int lineWidth );
	int YPosition( float y, int height );

	void MessageAdd( const char *pName );
	void MessageDrawScan( client_textmessage_t *pMessage, float time );
	void MessageScanStart( void );
	void MessageScanNextChar( void );
	void Reset( void );

	virtual void ApplySchemeSettings( IScheme *scheme );

	void SetFont( HScheme scheme, const char *pFontName );

public: // ITextMessage
	virtual void		SetPosition( int x, int y );
	virtual void		AddChar( int r, int g, int b, int a, wchar_t ch );

	virtual void		GetLength( int *wide, int *tall, const char *string );
	virtual int			GetFontInfo( FONTABC *pABCs, vgui::HFont hFont );

	virtual void		SetFont( vgui::HFont hCustomFont );
	virtual void		SetDefaultFont( void );

private:

	message_t			*AllocMessage( void );
	void				ResetCharacters( void );
	void				PaintCharacters();
	virtual void		GetTextExtents( int *wide, int *tall, const char *string );


	client_textmessage_t		*m_pMessages[maxHUDMessages];
	float						m_startTime[maxHUDMessages];
	message_parms_t				m_parms;
	float						m_gameTitleTime;
	client_textmessage_t		*m_pGameTitle;
	bool						m_bHaveMessage;

	CHudTexture *m_iconTitleLife;
	CHudTexture *m_iconTitleHalf;

	vgui::HFont					m_hFont;
	vgui::HFont					m_hDefaultFont;
	CUtlVector< message_t >		m_Messages;
};
#endif // MESSAGE_H
