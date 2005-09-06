// $Id$

#include "FlagsViewer.h"
#include <QPainter>
#include <QPixmap>
#include <QPaintEvent>


FlagsViewer::FlagsViewer( QWidget* parent )
	: QFrame( parent )
{
	flags = flagsChanged = 0; // avoid UMR
	
	setFrameStyle(WinPanel | Sunken);
	setFocusPolicy(Qt::StrongFocus);
	setBackgroundRole(QPalette::Base);

	frameL = frameT = frameB = frameWidth();
	frameR = frameL;

	setSizes();
}

void FlagsViewer::resizeEvent(QResizeEvent *e)
{
	QFrame::resizeEvent(e);
}

void FlagsViewer::paintEvent(QPaintEvent *e)
{
	// call parent for drawing the actual frame
	QFrame::paintEvent(e);
	
	QPainter p(this);
	p.setPen( palette().color(QPalette::Text) );
	
	int h = fontMetrics().height();
	int flagWidth = fontMetrics().width("ZW");
	int valWidth = fontMetrics().width("0 ");
	int d = fontMetrics().descent();
	

	// calc and set drawing bounds
	QRect r(e->rect());
	if(r.left()<frameL) r.setLeft(frameL);
	if(r.top()<frameT) r.setTop(frameT);
	if(r.right()>width()-frameR-1) r.setRight(width()-frameR-1);
	if(r.bottom()>height()-frameB-1) r.setBottom(height()-frameB-1);
	p.setClipRect(r);

	int	x = frameL + 4;
	int y = frameT + h - 1 - d;
	
	QString hexStr;

	p.drawText(x, y, "S");
	x += flagWidth;
	hexStr.sprintf("%1i", flags&128?1:0);
	drawValue(p, x, y, hexStr, flagsChanged&128);
	x += valWidth;
	if(flags&128) 
		hexStr = "(M)";
	else
		hexStr = "(P)";
	p.drawText(x, y, hexStr);
	
	x = frameL+4;
	y += h;
	
	p.drawText(x, y, "Z");
	x += flagWidth;
	hexStr.sprintf("%1i", flags&64?1:0);
	drawValue(p, x, y, hexStr, flagsChanged&64);
	x += valWidth;
	if(flags&64) 
		hexStr = "(Z)";
	else
		hexStr = "(NZ)";
	p.drawText(x, y, hexStr);
	
	x = frameL+4;
	y += h;

	p.drawText(x, y, " ");
	x += flagWidth;
	hexStr.sprintf("%1i", flags&32?1:0);
	drawValue(p, x, y, hexStr, flagsChanged&32);
	
	x = frameL+4;
	y += h;

	p.drawText(x, y, "H");
	x += flagWidth;
	hexStr.sprintf("%1i", flags&16?1:0);
	drawValue(p, x, y, hexStr, flagsChanged&16);
	
	x = frameL+4;
	y += h;
	
	p.drawText(x, y, " ");
	x += flagWidth;
	hexStr.sprintf("%1i", flags&8?1:0);
	drawValue(p, x, y, hexStr, flagsChanged&8);
	
	x = frameL+4;
	y += h;
	
	p.drawText(x, y, "P");
	x += flagWidth;
	hexStr.sprintf("%1i", flags&4?1:0);
	drawValue(p, x, y, hexStr, flagsChanged&4);
	x += valWidth;
	if(flags&64) 
		hexStr = "(PO)";
	else
		hexStr = "(PE)";
	p.drawText(x, y, hexStr);
	
	x = frameL+4;
	y += h;
	
	p.drawText(x, y, "N");
	x += flagWidth;
	hexStr.sprintf("%1i", flags&2?1:0);
	drawValue(p, x, y, hexStr, flagsChanged&2);
	
	x = frameL+4;
	y += h;
	
	p.drawText(x, y, "C");
	x += flagWidth;
	hexStr.sprintf("%1i", flags&1);
	drawValue(p, x, y, hexStr, flagsChanged&1);
	x += valWidth;
	if(flags&1)
		hexStr = "(C)";
	else
		hexStr = "(NC)";
	p.drawText(x, y, hexStr);
}

void FlagsViewer::setSizes()
{
	int v = frameT + 8*fontMetrics().height() + frameB;
	setMinimumHeight(v);
	setMaximumHeight(v);
	
	v = frameL + 4 + fontMetrics().width("ZW0 (PE) ") + 4 + frameR;
	setMinimumWidth(v);
	setMaximumWidth(v);
}

void FlagsViewer::drawValue(QPainter& p, const int x, const int y, const QString& str, const bool changed)
{
	if(changed)
		p.setPen( Qt::red );
	p.drawText(x, y, str);
	if(changed)
		p.setPen( palette().color(QPalette::Text) );
}

void FlagsViewer::setFlags(quint8 newFlags)
{
	flagsChanged = flags ^ newFlags;
	flags = newFlags;
	
	update();
}
