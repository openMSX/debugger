#ifndef SETTINGS_H
#define SETTINGS_H

#include <QSettings>
#include <QFont>
#include <QColor>

class Settings : public QSettings
{
	Q_OBJECT
public:
	enum DebuggerConfig {
		AUTO_RELOAD_SYMBOLS, PRESERVE_LOST_SYMBOLS, PRESERVE_BREAKPOINT_SYMBOL,
		CONFIG_END
	};
	enum DebuggerFont {
		APP_FONT, FIXED_FONT, CODE_FONT, LABEL_FONT, HEX_FONT, FONT_END
	};
	enum DebuggerFontType {
		APPLICATION_DEFAULT, FIXED_DEFAULT, CUSTOM
	};

	static Settings& get();

	QString fontName(DebuggerFont f) const;
	const QFont& font(DebuggerFont f) const;
	void setFont(DebuggerFont f, const QFont& ft);
	DebuggerFontType fontType(DebuggerFont f) const;
	void setFontType(DebuggerFont f, DebuggerFontType t);
	const QColor& fontColor(DebuggerFont f) const;
	void setFontColor(DebuggerFont f, const QColor& c);

	bool autoReloadSymbols() const;
	void setAutoReloadSymbols(bool b);
	bool preserveLostSymbols() const;
	void setPreserveLostSymbols(bool b);
	bool preserveBreakpointSymbol() const;
	void setPreserveBreakpointSymbol(bool b);
	void setConfig(DebuggerConfig c, const QVariant& v);

private:
	Settings();
	~Settings() override = default;

	QFont fonts[FONT_END];
	DebuggerFontType fontTypes[FONT_END];
	QColor fontColors[FONT_END];

	QVariant config[CONFIG_END];

	void getBoolFromSetting(DebuggerConfig c, bool defaultValue);
	void getConfigFromSettings();
	void getFontsFromSettings();
	void updateFonts();
};

#endif // SETTINGS_H
