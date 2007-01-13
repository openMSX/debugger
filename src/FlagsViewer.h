// $Id$

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

protected:
	void resizeEvent(QResizeEvent* e);
	void paintEvent(QPaintEvent* e);

private:
	int frameL, frameR, frameT, frameB;

	unsigned char flags;
	unsigned char flagsChanged;

	void drawValue(QPainter& p, int x, int y, const QString& str,
	               const bool changed);
};

#endif // FLAGSVIEWER_H
