#ifndef FLAGSVIEWER_H
#define FLAGSVIEWER_H

#include <QFrame>

class QPainter;
class QResizeEvent;
class QPaintEvent;
class QString;

class FlagsViewer : public QFrame
{
	Q_OBJECT;
public:
	FlagsViewer(QWidget* parent = 0);

	QSize sizeHint() const;

public slots:
	void setFlags(quint8 newFlags);

private:
	void resizeEvent(QResizeEvent* e);
	void paintEvent(QPaintEvent* e);

	void drawValue(QPainter& p, int x, int y, const QString& str,
	               const bool changed);

	int frameL, frameR, frameT, frameB;
	unsigned char flags;
	unsigned char flagsChanged;
};

#endif // FLAGSVIEWER_H
