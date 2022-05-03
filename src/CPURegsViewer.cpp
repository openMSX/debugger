#include "CPURegsViewer.h"
#include "CPURegs.h"
#include "CommClient.h"
#include "OpenMSXConnection.h"
#include <QPainter>
#include <QPaintEvent>
#include <QMessageBox>
#include <QToolTip>

CPURegsViewer::CPURegsViewer(QWidget* parent)
	: QFrame(parent)
{
    setObjectName("CPURegsViewer");
	// avoid UMR
	memset(&regs,         0, sizeof(regs));
	memset(&regsChanged,  0, sizeof(regsChanged));
	memset(&regsModified, 0, sizeof(regsModified));

	setFrameStyle(WinPanel | Sunken);
	setFocusPolicy(Qt::StrongFocus);
	setBackgroundRole(QPalette::Base);
	setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));

	frameR = frameL = frameT = frameB = frameWidth();
	cursorLoc = -1;
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

	rowHeight = fontMetrics().height();
	int regWidth = fontMetrics().horizontalAdvance("HLW");
	int valWidth = fontMetrics().horizontalAdvance("FFFFWW");
	int d = fontMetrics().descent();

	leftRegPos = frameL + 4;
	leftValuePos = leftRegPos + regWidth;
	rightRegPos = leftValuePos + valWidth;
	rightValuePos = rightRegPos + regWidth;

	// calc and set drawing bounds
	QRect r(e->rect());
	if (r.left() < frameL) r.setLeft(frameL);
	if (r.top()  < frameT) r.setTop (frameT);
	if (r.right()  > (width()  - frameR - 1)) r.setRight (width()  - frameR - 1);
	if (r.bottom() > (height() - frameB - 1)) r.setBottom(height() - frameB - 1);
	p.setClipRect(r);

	// redraw background
	p.fillRect(r, palette().color(QPalette::Base));

	int y = frameT + rowHeight - 1 - d;
	for (int r = 0; r < 14; r += 2) {
		p.drawText(leftRegPos,  y, CpuRegs::regNames[r + 0]);
		drawValue(p, r + 0, leftValuePos,  y);
		p.drawText(rightRegPos, y, CpuRegs::regNames[r + 1]);
		drawValue(p, r + 1, rightValuePos, y);
		y += rowHeight;
	}

	// draw interrupt mode
	p.drawText(leftRegPos, y, "IM");
	drawValue(p, 14, leftValuePos, y);

	// draw interrupt state
	if (regsChanged[CpuRegs::REG_IFF] & 1) {
		p.setPen(Qt::red);
	}
	p.drawText(rightRegPos, y, regs[CpuRegs::REG_IFF] ? "EI" : "DI");
}

QSize CPURegsViewer::sizeHint() const
{
	return {frameL + 4 + fontMetrics().horizontalAdvance("HLWFFFFWWHLWFFFFW") + 4 + frameR,
	        frameT + 8 * fontMetrics().height() + frameB};
}

void CPURegsViewer::drawValue(QPainter& p, int id, int x, int y)
{
	// determine value colour
	QColor penClr = palette().color(QPalette::Text);
	if (regsModified[id]) {
		penClr = Qt::darkGreen;
	} else if (regsChanged[id]) {
		penClr = Qt::red;
	}
	// print (edit) value
	if ((cursorLoc >> 2) == id) {
		// cursor is in this value, print separate digits
		int digit = 3;
		if (id < CpuRegs::REG_I) {
			digit = 0;
		} else if (id < CpuRegs::REG_IM) {
			digit = 2;
		}
		// write all digit
		for (/**/; digit < 4; ++digit) {
			// create string with a single digit
			QString digitTxt = QString("%1")
			                   .arg((regs[id] >> (4 * (3 - digit))) & 15, 0, 16)
			                   .toUpper();
			// if digit has cursor, draw as cursor
			if ((cursorLoc & 3) == digit) {
				// draw curser background
				QBrush b(palette().color(QPalette::Highlight));
				p.fillRect(x, frameT + (cursorLoc >> 3) * rowHeight,
				           fontMetrics().horizontalAdvance(digitTxt), rowHeight, b);
				p.setPen(palette().color(QPalette::HighlightedText));
			} else {
				p.setPen(penClr);
			}
			p.drawText(x, y, digitTxt);
			x += fontMetrics().horizontalAdvance(digitTxt);
		}
	} else {
		// regular value print
		p.setPen(penClr);
		// create string
		QString str;
		if (id < CpuRegs::REG_I) {
			str = QString("%1").arg(regs[id], 4, 16, QChar('0')).toUpper();
		} else if (id < CpuRegs::REG_IM) {
			str = QString("%1").arg(regs[id], 2, 16, QChar('0')).toUpper();
		} else {
			str = QString("%1").arg(regs[id], 1, 16, QChar('0')).toUpper();
		}
		// draw
		p.drawText(x, y, str);
	}
	// reset pen
	p.setPen(palette().color(QPalette::Text));
}

void CPURegsViewer::setRegister(int id, int value)
{
	regsChanged[id] = regs[id] != value;
	regs[id] = value;
	if (regsChanged[id]) {
		emit registerChanged(id, value);
	}
}

void CPURegsViewer::setData(unsigned char* datPtr)
{
	setRegister(CpuRegs::REG_AF , datPtr[ 0] * 256 + datPtr[ 1]);
	setRegister(CpuRegs::REG_BC , datPtr[ 2] * 256 + datPtr[ 3]);
	setRegister(CpuRegs::REG_DE , datPtr[ 4] * 256 + datPtr[ 5]);
	setRegister(CpuRegs::REG_HL , datPtr[ 6] * 256 + datPtr[ 7]);
	setRegister(CpuRegs::REG_AF2, datPtr[ 8] * 256 + datPtr[ 9]);
	setRegister(CpuRegs::REG_BC2, datPtr[10] * 256 + datPtr[11]);
	setRegister(CpuRegs::REG_DE2, datPtr[12] * 256 + datPtr[13]);
	setRegister(CpuRegs::REG_HL2, datPtr[14] * 256 + datPtr[15]);
	setRegister(CpuRegs::REG_IX , datPtr[16] * 256 + datPtr[17]);
	setRegister(CpuRegs::REG_IY , datPtr[18] * 256 + datPtr[19]);
	setRegister(CpuRegs::REG_PC , datPtr[20] * 256 + datPtr[21]);
	setRegister(CpuRegs::REG_SP , datPtr[22] * 256 + datPtr[23]);
	setRegister(CpuRegs::REG_I  , datPtr[24]);
	setRegister(CpuRegs::REG_R  , datPtr[25]);
	setRegister(CpuRegs::REG_IM , datPtr[26]);

	// IFF separately to only check bit 0 for change
	regsChanged[CpuRegs::REG_IFF] = (regs[CpuRegs::REG_IFF] & 1) != (datPtr[27] & 1);
	regs[CpuRegs::REG_IFF] = datPtr[27];

	// reset modifications
	cursorLoc = -1;
	memset(&regsModified, 0, sizeof(regsModified));
	memcpy(&regsCopy, &regs, sizeof(regs));

	update();

	emit pcChanged(regs[CpuRegs::REG_PC]);
	emit spChanged(regs[CpuRegs::REG_SP]);
	emit flagsChanged(regs[CpuRegs::REG_AF] & 0xFF);
}

void CPURegsViewer::focusOutEvent(QFocusEvent* e)
{
	if (e->lostFocus()) {
		cancelModifications();
		cursorLoc = -1;
		update();
	}
}

void CPURegsViewer::mousePressEvent(QMouseEvent* e)
{
	if (e->button() == Qt::LeftButton) {
		int pos = -1;
		if (e->x() >= leftValuePos && e->x() < rightRegPos) {
			pos = 0;
		}
		if (e->x() >= rightValuePos) {
			pos = 4;
		}
		if (pos >= 0 && e->y() < frameT + 7 * rowHeight) {
			int row = (e->y() - frameT) / rowHeight;
			cursorLoc = pos + 8 * row;
			if (row == 6) cursorLoc += 2;
		}
		update();
	}
}

void CPURegsViewer::keyPressEvent(QKeyEvent* e)
{
	// don't accept when not editting
	if (cursorLoc < 0) {
		QFrame::keyPressEvent(e);
		return;
	}

	int move = e->key();
	if ((e->key() >= Qt::Key_0 && e->key() <= Qt::Key_9) ||
	    (e->key() >= Qt::Key_A && e->key() <= Qt::Key_F)) {
		// calculate numercial value
		int v = e->key() - Qt::Key_0;
		if (v > 9) v -= Qt::Key_A - Qt::Key_0 - 10;
		// modify value
		int id = cursorLoc >> 2;
		int digit = cursorLoc & 3;
		regs[id] &= ~(0xF000 >> (4 * digit));
		regs[id] |= v << (12 - 4 * digit);
		regsModified[id] = true;
		move = Qt::Key_Right;
	}

	if (move == Qt::Key_Right) {
		++cursorLoc;
		if (cursorLoc == 4 * CpuRegs::REG_I ||
		    cursorLoc == 4 * CpuRegs::REG_R) {
			cursorLoc += 2;
		} else if (cursorLoc == 4 * CpuRegs::REG_IM) {
			cursorLoc = 0;
		}
	} else if (move == Qt::Key_Left) {
		--cursorLoc;
		if (cursorLoc == -1) {
			cursorLoc = 4 * CpuRegs::REG_R + 3;
		} else if (cursorLoc == 4 * CpuRegs::REG_R + 1 ||
		           cursorLoc == 4 * CpuRegs::REG_I + 1) {
			cursorLoc -= 2;
		}
	} else if (move == Qt::Key_Up) {
		cursorLoc -= 8;
		if (cursorLoc < 0) {
			cursorLoc += 4 * CpuRegs::REG_IM;
			// move to lowest row
			if        (cursorLoc == 4 * CpuRegs::REG_I) {
				cursorLoc = 4 * CpuRegs::REG_I + 2;
			} else if (cursorLoc <  4 * CpuRegs::REG_R) {
				cursorLoc = 4 * CpuRegs::REG_I + 3;
			} else if (cursorLoc == 4 * CpuRegs::REG_R) {
				cursorLoc = 4 * CpuRegs::REG_R + 2;
			} else {
				cursorLoc = 4 * CpuRegs::REG_R + 3;
			}
		} else if (cursorLoc >= 4 * CpuRegs::REG_PC) {
			// move from lowest row
			cursorLoc -= 2;
		}
	} else if (move == Qt::Key_Down) {
		cursorLoc += 8;
		if (cursorLoc >= 4 * CpuRegs::REG_IM) {
			// move from lowest row
			cursorLoc -= 4 * CpuRegs::REG_IM + 2;
		} else if (cursorLoc >= 4 * CpuRegs::REG_I) {
			// move to lowest row
			if        (cursorLoc == 4 * CpuRegs::REG_I) {
				cursorLoc = 4 * CpuRegs::REG_I + 2;
			} else if (cursorLoc <  4 * CpuRegs::REG_R) {
				cursorLoc = 4 * CpuRegs::REG_I + 3;
			} else if (cursorLoc == 4 * CpuRegs::REG_R) {
				cursorLoc = 4 * CpuRegs::REG_R + 2;
			} else {
				cursorLoc = 4 * CpuRegs::REG_R + 3;
			}
		}
	} else if (move == Qt::Key_Escape) {
		// cancel changes
		cancelModifications();
		cursorLoc = -1;
	} else if (move == Qt::Key_Return || move == Qt::Key_Enter) {
		// apply changes
		applyModifications();
	} else {
		QFrame::keyPressEvent(e);
		return;
	}
	e->accept();
	update();
}

//void CPURegsViewer::mouseMoveEvent(QMouseEvent* e)
//{
//	QToolTip::hideText();
//	QFrame::mouseMoveEvent(e);
//}

int CPURegsViewer::readRegister(int id)
{
	return regs[id];
}

void CPURegsViewer::getRegister(int id, unsigned char* data)
{
	data[0] = regs[id] >> 8;
	data[1] = regs[id] & 255;
}

void CPURegsViewer::applyModifications()
{
	unsigned char data[26];
	getRegister(CpuRegs::REG_AF,  &data[ 0]);
	getRegister(CpuRegs::REG_BC,  &data[ 2]);
	getRegister(CpuRegs::REG_DE,  &data[ 4]);
	getRegister(CpuRegs::REG_HL,  &data[ 6]);
	getRegister(CpuRegs::REG_AF2, &data[ 8]);
	getRegister(CpuRegs::REG_BC2, &data[10]);
	getRegister(CpuRegs::REG_DE2, &data[12]);
	getRegister(CpuRegs::REG_HL2, &data[14]);
	getRegister(CpuRegs::REG_IX,  &data[16]);
	getRegister(CpuRegs::REG_IY,  &data[18]);
	getRegister(CpuRegs::REG_PC,  &data[20]);
	getRegister(CpuRegs::REG_SP,  &data[22]);
	data[24] = regs[CpuRegs::REG_I];
	data[25] = regs[CpuRegs::REG_R];

	// send new data to openmsx
	auto* req = new WriteDebugBlockCommand("{CPU regs}", 0, 26, data);
	CommClient::instance().sendCommand(req);
	// turn off editing
	cursorLoc = -1;
	// update copy
	for (int i = 0; i < 14; ++i) {
		if (regsModified[i]) {
			regsModified[i] = false;
			regsCopy[i] = regs[i];
			regsChanged[i] = true;
		}
	}
	// update screen
	update();
}

void CPURegsViewer::cancelModifications()
{
	bool mod = false;
	for (int i = 0; i < 14; ++i) mod |= regsModified[i];
	if (!mod) return;

	int ret = QMessageBox::warning(
		this,
		tr("CPU registers changes"),
		tr("You made changes to the CPU registers.\n"
		   "Do you want to apply your changes or ignore them?"),
		QMessageBox::Apply | QMessageBox::Ignore,
		QMessageBox::Ignore);
	if (ret == QMessageBox::Ignore) {
		memcpy(&regs, &regsCopy, sizeof(regs));
		memset(&regsModified, 0, sizeof(regsModified));
	} else {
		applyModifications();
	}
}

bool CPURegsViewer::event(QEvent* e)
{
	if (e->type() != QEvent::ToolTip) {
		return QFrame::event(e);
	}

	auto* helpEvent = static_cast<QHelpEvent*>(e);
	// calc register number
	int pos = -1;
	if (helpEvent->x() >= leftValuePos &&
	    helpEvent->x() <  rightRegPos) {
		pos = 0;
	}
	if (helpEvent->x() >= rightValuePos) {
		pos = 1;
	}
	if (pos >= 0 && helpEvent->y() < frameT + 7 * rowHeight) {
		pos += 2 * ((helpEvent->y() - frameT) / rowHeight);
	}
	if (pos >= 0 && pos != 10 && pos != 11) {
		// create text with binary and decimal values
		QString text(CpuRegs::regNames[pos]);
		text += "\nBinary: ";
		if (pos < 12) {
			text += QString("%1 ")
				.arg((regs[pos] & 0xF000) >> 12, 4, 2, QChar('0'));
			text += QString("%1 ")
				.arg((regs[pos] & 0x0F00) >>  8, 4, 2, QChar('0'));
		}
		text += " ";
		text += QString("%1 ").arg((regs[pos] & 0x00F0) >> 4, 4, 2, QChar('0'));
		text += QString("%1") .arg((regs[pos] & 0x000F) >> 0, 4, 2, QChar('0'));
		text += "\nDecimal: ";
		text += QString::number(regs[pos]);
		// print 8 bit values
		if (pos < 8) {
			text += QString("\n%1: %2  %3: %4")
				.arg(CpuRegs::regNames[pos][0])
				.arg(regs[pos] >> 8)
				.arg(CpuRegs::regNames[pos][1])
				.arg(regs[pos] & 255);
		} else if (pos < 10) {
			text += QString("\nI%1H: %2  I%3L: %4")
				.arg(CpuRegs::regNames[pos][1])
				.arg(regs[pos] >> 8)
				.arg(CpuRegs::regNames[pos][1])
				.arg(regs[pos] & 255);
		}
		QToolTip::showText(helpEvent->globalPos(), text);
	} else {
		QToolTip::hideText();
	}
	return QFrame::event(e);
}
