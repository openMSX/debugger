#include "InteractiveButton.h"
#include <QApplication>
#include <QPalette>

InteractiveButton::InteractiveButton(QWidget* parent)
	: QPushButton(parent)
{
	setEnabled(true);
	setAutoFillBackground(true);
	_mustBeSet = false;
	_highlight = false;
	connect(this, &InteractiveButton::toggled, this, &InteractiveButton::newBitValueSlot);
}

void InteractiveButton::highlight(bool state)
{
	if (_highlight == state) return;

	_highlight = state;
	setColor();
}

void InteractiveButton::setColor()
{
	// No style sheet since this will not work for Mac OS X atm.
	QPalette fiddle = QApplication::palette();
	int colorset = (_mustBeSet ? 4 : 0) + (_highlight ? 2 : 0) + ((_mustBeSet & !_state) ? 1 : 0);
	switch (colorset) {
		case 6:
			fiddle.setColor(QPalette::Active, QPalette::Button, Qt::green);
			break;
		case 4:
			fiddle.setColor(QPalette::Active, QPalette::Button, Qt::darkGreen);
			break;
		case 3:
		case 7:
			fiddle.setColor(QPalette::Active, QPalette::Button, Qt::red);
			break;
		case 2:
			fiddle.setColor(QPalette::Active, QPalette::Button, Qt::yellow);
			break;
		case 5:
		case 1:
			fiddle.setColor(QPalette::Active, QPalette::Button, Qt::darkRed);
			break;
	}
	setPalette(fiddle);
	update();
}

void InteractiveButton::enterEvent(QEvent* /*event*/)
{
	emit mouseOver(true);
}

void InteractiveButton::leaveEvent(QEvent* /*event*/)
{
	emit mouseOver(false);
}

void InteractiveButton::newBitValueSlot(bool state)
{
	_state=state;
	QString name = objectName();
	int bit = name.right(1).toInt();
	int reg = name.mid(
		1 + name.indexOf('_'),
		name.lastIndexOf('_') - name.indexOf('_') - 1).toInt();
	emit newBitValue(reg, bit, state);
	setColor();
}

void InteractiveButton::mustBeSet(bool state)
{
	if (state == _mustBeSet) return;
	_mustBeSet = state;
	setColor();
}
