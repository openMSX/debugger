// $Id$

#include "FlagsViewer.h"
#include <QPainter>
#include <QPaintEvent>


FlagsViewer::FlagsViewer(QWidget* parent)
	: QFrame(parent)
{
	flags = flagsChanged = 0; // avoid UMR

	setFrameStyle(WinPanel | Sunken);
	setFocusPolicy(Qt::StrongFocus);
	setBackgroundRole(QPalette::Base);
	setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));

	frameR = frameL = frameT = frameB = frameWidth();
}

void FlagsViewer::resizeEvent(QResizeEvent* e)
{
	QFrame::resizeEvent(e);
}

void FlagsViewer::paintEvent(QPaintEvent* e)
{
	const char* const flagNames[8] =
		{ "C"   , "N", "P"   , "", "H", "", "Z"   , "S"   };
	const char* const flagOn[8] =
		{ "(C)" , "" , "(PO)", "", "",  "", "(Z)" , "(M)" };
	const char* const flagOff[8] =
		{ "(NC)", "" , "(PE)", "", "",  "", "(NZ)", "(P)" };

	// call parent for drawing the actual frame
	QFrame::paintEvent(e);

	QPainter p(this);
	p.setPen(palette().color(QPalette::Text));

	// calc and set drawing bounds
	QRect r(e->rect());
	if (r.left() < frameL) r.setLeft(frameL);
	if (r.top()  < frameT) r.setTop (frameT);
	if (r.right()  > width()  - frameR - 1) r.setRight (width()  - frameR - 1);
	if (r.bottom() > height() - frameB - 1) r.setBottom(height() - frameB - 1);
	p.setClipRect(r);

	// redraw background
	p.fillRect(r, palette().color(QPalette::Base));

	int h = fontMetrics().height();
	int flagWidth = fontMetrics().width("ZW");
	int valWidth  = fontMetrics().width("0 ");
	int d = fontMetrics().descent();
	int y = frameT + h - 1 - d;
	for (int flag = 7; flag >= 0; --flag) {
		int x = frameL + 4;
		p.drawText(x, y, flagNames[flag]);
		x += flagWidth;
		int bit = 1 << flag;
		drawValue(p, x, y, (flags & bit) ? "1" : "0", flagsChanged & bit);
		x += valWidth;
		p.drawText(x, y, (flags & bit) ? flagOn[flag] : flagOff[flag]);
		y += h;
	}
}

QSize FlagsViewer::sizeHint() const
{
	return QSize(frameL + 4 + fontMetrics().width("ZW0 (PE) ") + 4 + frameR,
	             frameT + 8 * fontMetrics().height() + frameB);
}

void FlagsViewer::drawValue(QPainter& p, int x, int y, const QString& str,
                            bool changed)
{
	if (changed) {
		p.setPen(Qt::red);
	}
	p.drawText(x, y, str);
	if (changed) {
		p.setPen(palette().color(QPalette::Text));
	}
}

void FlagsViewer::setFlags(quint8 newFlags)
{
	flagsChanged = flags ^ newFlags;
	flags = newFlags;
	update();
}
