// $Id$

#ifndef _FLAGSVIEWER_H
#define _FLAGSVIEWER_H

#include <QFrame>


class FlagsViewer : public QFrame
{
	Q_OBJECT;
public:
	FlagsViewer( QWidget* parent = 0 );

public slots:
	void setFlags(quint8 newFlags);

protected:
	void resizeEvent(QResizeEvent *e);
	void paintEvent(QPaintEvent *e);

private:
	int frameL, frameR, frameT, frameB;

	unsigned char flags;
	unsigned char flagsChanged;

	void setSizes();
	void drawValue(QPainter& p, const int x, const int y, const QString& str, const bool changed);
};

#endif    // _FLAGSVIEWER_H
