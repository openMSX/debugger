// $Id$
#ifndef _SETTINGS_H
#define _SETTINGS_H

#include <QSettings>
#include <QFont>
#include <QColor>

class Settings : public QSettings
{
	Q_OBJECT
public:
	static Settings& get();

	enum DebuggerFont { APP_FONT, FIXED_FONT, CODE_FONT, LABEL_FONT, HEX_FONT, FONT_END };
	enum DebuggerFontType { APPLICATION_DEFAULT, FIXED_DEFAULT, CUSTOM };

	QString fontName( DebuggerFont f ) const;
	const QFont& font( DebuggerFont f ) const;
	void setFont( DebuggerFont f, const QFont& ft );
	DebuggerFontType fontType( DebuggerFont f ) const;
	void setFontType( DebuggerFont f, DebuggerFontType t );
	const QColor& fontColor( DebuggerFont f ) const;
	void setFontColor( DebuggerFont f, const QColor& c );

private:
	Settings();
	~Settings();

	QFont fonts[FONT_END];
	DebuggerFontType fontTypes[FONT_END];
	QColor fontColors[FONT_END];

	void getFontsFromSettings();
	void updateFonts();
};

#endif // _SETTINGS_H
