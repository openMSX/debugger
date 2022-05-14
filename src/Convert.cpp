#include "Convert.h"
#include "Settings.h"

QString hexValue(int value, int width)
{
	Settings& s = Settings::get();
	return QString("%1%2%3").arg(s.value("Preferences/HexPrefix", "$").toString())
	                        .arg(QString("%1").arg(value, width, 16, QChar('0')).toUpper())
	                        .arg(s.value("Preferences/HexPostfix", "").toString());
	                      
}

QString escapeXML(QString str)
{
	return str.replace('&', "&amp;").replace('<', "&lt;").replace('>', "&gt;");
}

QString unescapeXML(QString str)
{
	return str.replace("&amp;", "&").replace("&lt;", "<").replace("&gt;", ">");
}
