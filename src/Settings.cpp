#include "Settings.h"
#include <QApplication>
#include <QDebug>

static const char* DebuggerConfigNames[Settings::CONFIG_END] = {
	"AutoReloadSymbols", "PreserveLostSymbols", "PreserveBreakpointSymbol"
};

static const char* DebuggerFontNames[Settings::FONT_END] = {
	"Application Font", "Default Fixed Font", "Code Font",
	"Label Font", "Hex viewer font"
};

static QString configLocation(Settings::DebuggerConfig c)
{
	return QString("Config/").append(DebuggerConfigNames[c]);
}

static QString fontLocation(Settings::DebuggerFont f)
{
	return QString("Fonts/").append(DebuggerFontNames[f]);
}

static QString fontColorLocation(Settings::DebuggerFont f)
{
	return QString("Fonts/%1 Color").arg(DebuggerFontNames[f]);
}

Settings::Settings()
	: QSettings("openMSX", "debugger")
{
	getConfigFromSettings();
	getFontsFromSettings();
}

Settings& Settings::get()
{
	static Settings instance;
	return instance;
}

void Settings::getBoolFromSetting(DebuggerConfig c, bool defaultValue)
{
	QVariant v = value(configLocation(c));

	if (v.canConvert<bool>()) {
		config[c] = v;
	} else {
		// default value
		config[c] = defaultValue;
	}
}

void Settings::getConfigFromSettings()
{
	getBoolFromSetting(AUTO_RELOAD_SYMBOLS, false);
	getBoolFromSetting(PRESERVE_LOST_SYMBOLS, true);
	getBoolFromSetting(PRESERVE_BREAKPOINT_SYMBOL, false);
}

void Settings::setConfig(DebuggerConfig c, const QVariant& v)
{
	config[c] = v;
	setValue(configLocation(c), config[c]);
}

void Settings::getFontsFromSettings()
{
	// first get application default
	QVariant v = value(fontLocation(APP_FONT));
	if (v.type() == QVariant::Font) {
		fonts[APP_FONT] = v.value<QFont>();
		fontTypes[APP_FONT] = CUSTOM;
	} else {
		fontTypes[APP_FONT] = APPLICATION_DEFAULT;
		fonts[APP_FONT] = qApp->font();
	}

	// then get default fixed spacing font
	v = value(fontLocation(FIXED_FONT));
	if (v.type() == QVariant::Font) {
		fonts[FIXED_FONT] = v.value<QFont>();
		fontTypes[FIXED_FONT] = CUSTOM;
	} else {
		fontTypes[FIXED_FONT] = APPLICATION_DEFAULT;
		fonts[FIXED_FONT] = fonts[APP_FONT];
	}

	// then the rest
	for (int f = CODE_FONT; f < FONT_END; ++f) {
		v = value(fontLocation((DebuggerFont)f));
		if (v.type() == QVariant::Font) {
			fonts[f] = v.value<QFont>();
			fontTypes[f] = CUSTOM;
		} else if (v.toString() == "FixedDefault") {
			fonts[f] = fonts[FIXED_FONT];
			fontTypes[f] = FIXED_DEFAULT;
		} else {
			fonts[f] = fonts[APP_FONT];
			fontTypes[f] = CUSTOM;
		}
	}

	// read colors
	for (int f =CODE_FONT; f < FONT_END; ++f) {
		fontColors[f] =
			value(fontColorLocation((DebuggerFont)f), QColor(Qt::black))
				.value<QColor>();
	}
}

QString Settings::fontName(DebuggerFont f) const
{
	return DebuggerFontNames[f];
}

const QFont& Settings::font(DebuggerFont f) const
{
	return fonts[f];
}

void Settings::setFont(DebuggerFont f, const QFont& ft)
{
	fontTypes[f] = CUSTOM;
	fonts[f] = ft;
	setValue(fontLocation(f), fonts[f]);
	if (f <= FIXED_FONT) updateFonts();
}

Settings::DebuggerFontType Settings::fontType(DebuggerFont f) const
{
	return fontTypes[f];
}

void Settings::setFontType(DebuggerFont f, DebuggerFontType t)
{
	if (fontTypes[f] == t) return;

	fontTypes[f] = t;
	switch (t) {
	case APPLICATION_DEFAULT:
		setValue(fontLocation(f), "AppDefault");
		if (f == APP_FONT) {
			fonts[f] = qApp->font();
		} else {
			fonts[f] = fonts[APP_FONT];
		}
		break;
	case FIXED_DEFAULT:
		if (f > FIXED_FONT) {
			setValue(fontLocation(f), "FixedDefault");
			fonts[f] = fonts[FIXED_FONT];
		}
		break;
	case CUSTOM:
		setValue(fontLocation(f), fonts[f]);
		break;
	}
	if (f > FIXED_FONT) updateFonts();
}

const QColor& Settings::fontColor(DebuggerFont f) const
{
	return fontColors[f];
}

void Settings::setFontColor(DebuggerFont f, const QColor& c)
{
	if (f > FIXED_FONT) {
		fontColors[f] = c;
		setValue(fontColorLocation(f), c);
	}
}

bool Settings::autoReloadSymbols() const
{
	return config[AUTO_RELOAD_SYMBOLS].value<bool>();
}

void Settings::setAutoReloadSymbols(bool b)
{
	setConfig(AUTO_RELOAD_SYMBOLS, b);
}

bool Settings::preserveLostSymbols() const
{
	return config[PRESERVE_LOST_SYMBOLS].value<bool>();
}

void Settings::setPreserveLostSymbols(bool b)
{
	setConfig(PRESERVE_LOST_SYMBOLS, b);
}

bool Settings::preserveBreakpointSymbol() const
{
	return config[PRESERVE_BREAKPOINT_SYMBOL].value<bool>();
}

void Settings::setPreserveBreakpointSymbol(bool b)
{
	setConfig(PRESERVE_BREAKPOINT_SYMBOL, b);
}

void Settings::updateFonts()
{
	for (int f = CODE_FONT; f < FONT_END; ++f) {
		switch (fontTypes[f]) {
		case APPLICATION_DEFAULT:
			fonts[f] = fonts[APP_FONT];
			break;
		case FIXED_DEFAULT:
			fonts[f] = fonts[FIXED_FONT];
			break;
		default:
			break;
		}
	}
}
