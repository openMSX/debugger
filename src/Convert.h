#ifndef CONVERT_H
#define CONVERT_H

#include <QString>
#include <cstdint>
#include <optional>
#include <limits>

QString hexValue(int value, int width = 0);
QString escapeXML(QString str);
QString unescapeXML(QString str);

// Create optional<T> if boolean b is true.
template <typename T>
std::optional<T> make_if(bool b, T value)
{
	return b ? value : std::optional<T>();
}

template <typename T>
std::optional<T> make_positive_optional(int value)
{
    return make_if<T>(value >= 0, T(value));
}

template <typename T>
std::optional<T> stringToValue(const QString& str)
{
        QString s = str.trimmed();
        int base = 10;

        // find base (prefix or postfix)
        if (s.startsWith("&") && s.size() >= 2) {
                switch (s[1].toUpper().toLatin1()) {
                case 'H':
                        base = 16;
                        break;
                case 'B':
                        base = 2;
                        break;
                case 'O':
                        base = 8;
                        break;
                }
                s = s.remove(0, 2);
        } else if (s.startsWith("#") || s.startsWith("$")) {
                base = 16;
                s = s.remove(0, 1);
        } else if (s.startsWith("0x")) {
                base = 16;
                s = s.remove(0, 2);
        } else if (s.startsWith("%")) {
                base = 2;
                s = s.remove(0, 1);
        } else if (!s.isEmpty()) {
                switch (s.right(1)[0].toUpper().toLatin1()) {
                        case 'H':
                        case '#':
                                base = 16;
                                break;
                        case 'B':
                                base = 2;
                                break;
                        case 'O':
                                base = 8;
                                break;
                }
                if (base != 10) s.chop(1);
        }

        // convert value
        bool ok;
        int value = s.toInt(&ok, base);
        if (!ok) return {};

        // reject {under|over}flow
        T max = std::numeric_limits<T>::max();
        T min = std::numeric_limits<T>::min();
        if (value > max || value < min) return {};

        return value;
}

#endif // CONVERT_H
