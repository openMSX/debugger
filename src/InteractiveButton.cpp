#include "InteractiveButton.h"
#include <QApplication>
#include <QPalette>

InteractiveButton::InteractiveButton(QWidget* parent)
	: QPushButton(parent)
{
	setEnabled(true);
	setAutoFillBackground(true);
	connect(this, SIGNAL(toggled(bool)), this, SLOT(newBitValueSlot(bool)));
}

void InteractiveButton::highlight(bool state)
{
	if (state) {
		// No style sheet since this will not work for Mac OS X atm.
		QPalette fiddle = QApplication::palette();
		fiddle.setColor(QPalette::Active, QPalette::Button, Qt::yellow);
		setPalette(fiddle);
        } else {
		setPalette(QApplication::palette());
	}
	update();
}

void InteractiveButton::enterEvent(QEvent* event)
{
	emit mouseOver(true);
}

void InteractiveButton::leaveEvent(QEvent* event)
{
	emit mouseOver(false);
}

void InteractiveButton::newBitValueSlot(bool state)
{
	QString name = objectName();
	int bit = name.right(1).toInt();
	int reg = name.mid(
		1 + name.indexOf('_'),
		name.lastIndexOf('_') - name.indexOf('_') - 1).toInt();
	emit newBitValue(reg, bit, state);
}
