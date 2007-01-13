// $Id$

#include "CPURegsViewer.h"
#include <QPainter>
#include <QPaintEvent>


CPURegsViewer::CPURegsViewer(QWidget* parent)
	: QFrame(parent)
{
	// avoid UMR
	memset(&regs,        0, sizeof(regs));
	memset(&regsChanged, 0, sizeof(regsChanged));
	
	setFrameStyle(WinPanel | Sunken);
	setFocusPolicy(Qt::StrongFocus);
	setBackgroundRole(QPalette::Base);
	setSizePolicy( QSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed ) );

	frameL = frameT = frameB = frameWidth();
	frameR = frameL;
}

void CPURegsViewer::resizeEvent(QResizeEvent* e)
{
	QFrame::resizeEvent(e);
}

void CPURegsViewer::paintEvent(QPaintEvent* e)
{
	// call parent for drawing the actual frame
	QFrame::paintEvent(e);
	
	QPainter p(this);
	p.setPen(palette().color(QPalette::Text));
	
	int h = fontMetrics().height();
	int regWidth = fontMetrics().width("HLW");
	int valWidth = fontMetrics().width("FFFFWW");
	int d = fontMetrics().descent();
	
	// calc and set drawing bounds
	QRect r(e->rect());
	if (r.left() < frameL) r.setLeft(frameL);
	if (r.top()  < frameT) r.setTop (frameT);
	if (r.right()  > (width()  - frameR - 1)) r.setRight (width()  - frameR - 1);
	if (r.bottom() > (height() - frameB - 1)) r.setBottom(height() - frameB - 1);
	p.setClipRect(r);

	// redraw background
	p.fillRect( r, palette().color(QPalette::Base) );

	int x = frameL + 4;
	int y = frameT + h - 1 - d;

	QString hexStr;

	p.drawText(x, y, "AF");
	x += regWidth;
	hexStr.sprintf("%04X", regs.AF);
	drawValue(p, x, y, hexStr, regsChanged.AF);
	x += valWidth;

	p.drawText(x, y, "AF'");
	x += regWidth;
	hexStr.sprintf("%04X", regs.AF2);
	drawValue(p, x, y, hexStr, regsChanged.AF2);

	x = frameL+4;
	y += h;
	
	p.drawText(x, y, "BC");
	x += regWidth;
	hexStr.sprintf("%04X", regs.BC);
	drawValue(p, x, y, hexStr, regsChanged.BC);
	x += valWidth;

	p.drawText(x, y, "BC'");
	x += regWidth;
	hexStr.sprintf("%04X", regs.BC2);
	drawValue(p, x, y, hexStr, regsChanged.BC2);

	x = frameL+4;
	y += h;

	p.drawText(x, y, "DE");
	x += regWidth;
	hexStr.sprintf("%04X", regs.DE);
	drawValue(p, x, y, hexStr, regsChanged.DE);
	x += valWidth;

	p.drawText(x, y, "DE'");
	x += regWidth;
	hexStr.sprintf("%04X", regs.DE2);
	drawValue(p, x, y, hexStr, regsChanged.DE2);

	x = frameL+4;
	y += h;
	
	p.drawText(x, y, "HL");
	x += regWidth;
	hexStr.sprintf("%04X", regs.HL);
	drawValue(p, x, y, hexStr, regsChanged.HL);
	x += valWidth;

	p.drawText(x, y, "HL'");
	x += regWidth;
	hexStr.sprintf("%04X", regs.HL2);
	drawValue(p, x, y, hexStr, regsChanged.HL2);

	x = frameL+4;
	y += h;

	p.drawText(x, y, "IX");
	x += regWidth;
	hexStr.sprintf("%04X", regs.IX);
	drawValue(p, x, y, hexStr, regsChanged.IX);
	x += valWidth;

	p.drawText(x, y, "IY");
	x += regWidth;
	hexStr.sprintf("%04X", regs.IY);
	drawValue(p, x, y, hexStr, regsChanged.IY);

	x = frameL+4;
	y += h;

	p.drawText(x, y, "PC");
	x += regWidth;
	hexStr.sprintf("%04X", regs.PC);
	drawValue(p, x, y, hexStr, regsChanged.PC);
	x += valWidth;

	p.drawText(x, y, "SP");
	x += regWidth;
	hexStr.sprintf("%04X", regs.SP);
	drawValue(p, x, y, hexStr, regsChanged.SP);

	x = frameL+4;
	y += h;

	p.drawText(x, y, "I");
	x += regWidth;
	hexStr.sprintf("%02X", regs.I);
	drawValue(p, x, y, hexStr, regsChanged.I);
	x += valWidth;

	p.drawText(x, y, "R");
	x += regWidth;
	hexStr.sprintf("%02X", regs.R);
	drawValue(p, x, y, hexStr, regsChanged.R);

	x = frameL+4;
	y += h;

	p.drawText(x, y, "IM");
	x += regWidth;
	hexStr.sprintf("%i", regs.IM);
	drawValue(p, x, y, hexStr, regsChanged.IM);
	x += valWidth;

	p.drawText(x, y, "IFF");
	x += regWidth;
	hexStr.sprintf("%02X", regs.IFF);
	drawValue(p, x, y, hexStr, regsChanged.IFF);
}

QSize CPURegsViewer::sizeHint() const
{
	return QSize( frameL + 4 + fontMetrics().width("HLWFFFFWWHLWFFFFW") + 4 + frameR,
	              frameT + 8 * fontMetrics().height() + frameB );
}

void CPURegsViewer::drawValue(QPainter& p, const int x, const int y,
                              const QString& str, const bool changed)
{
	if (changed) {
		p.setPen(Qt::red);
	}
	p.drawText(x, y, str);
	if (changed) {
		p.setPen(palette().color(QPalette::Text));
	}
}

void CPURegsViewer::setData(unsigned char* datPtr)
{
	for (int i = 0; i < 24; i += 2) {
		*((quint16*)(datPtr + i)) = datPtr[i + 0] * 256 + datPtr[i + 1];
	}
	Z80Registers* newRegs;
	newRegs = (Z80Registers*)datPtr;

	regsChanged.AF = regs.AF != newRegs->AF;
	regs.AF = newRegs->AF;
	regsChanged.BC = regs.BC != newRegs->BC;
	regs.BC = newRegs->BC;
	regsChanged.DE = regs.DE != newRegs->DE;
	regs.DE = newRegs->DE;
	regsChanged.HL = regs.HL != newRegs->HL;
	regs.HL = newRegs->HL;
	regsChanged.AF2 = regs.AF2 != newRegs->AF2;
	regs.AF2 = newRegs->AF2;
	regsChanged.BC2 = regs.BC2 != newRegs->BC2;
	regs.BC2 = newRegs->BC2;
	regsChanged.DE2 = regs.DE2 != newRegs->DE2;
	regs.DE2 = newRegs->DE2;
	regsChanged.HL2 = regs.HL2 != newRegs->HL2;
	regs.HL2 = newRegs->HL2;
	regsChanged.IX = regs.IX != newRegs->IX;
	regs.IX = newRegs->IX;
	regsChanged.IY = regs.IY != newRegs->IY;
	regs.IY = newRegs->IY;
	regsChanged.PC = regs.PC != newRegs->PC;
	regs.PC = newRegs->PC;
	regsChanged.SP = regs.SP != newRegs->SP;
	regs.SP = newRegs->SP;
	regsChanged.I = regs.I != newRegs->I;
	regs.I = newRegs->I;
	regsChanged.R = regs.R != newRegs->R;
	regs.R = newRegs->R;
	regsChanged.IM = regs.IM != newRegs->IM;
	regs.IM = newRegs->IM;
	regsChanged.IFF = regs.IFF != newRegs->IFF;
	regs.IFF = newRegs->IFF;

	update();

	emit pcChanged(regs.PC);
	emit spChanged(regs.SP);
	emit flagsChanged(newRegs->AF & 0xFF);
}
