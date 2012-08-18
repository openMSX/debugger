#ifndef CONVERT_H
#define CONVERT_H

class QString;

int stringToValue(const QString& str);
QString hexValue(int value, int width = 0);
QString& escapeXML(QString& str);
QString& unescapeXML(QString& str);

#endif // CONVERT_H
