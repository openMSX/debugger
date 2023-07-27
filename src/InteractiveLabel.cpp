#include "InteractiveLabel.h"
#include <QApplication>
#include <QPalette>

InteractiveLabel::InteractiveLabel(QWidget *parent)
	: QLabel(parent)
{
	setEnabled(true);
	setAutoFillBackground(true);
}

void InteractiveLabel::highlight(bool state)
{
	// I first tried with style sheets, but then I found in the
	// documentation that Qt style sheets are currently not supported for
	// QMacStyle (the default style on Mac OS X).
	// So I use a QPalette for now.
	if (state) {
		QPalette fiddle = QApplication::palette();
		fiddle.setColor(QPalette::Active, QPalette::Window, Qt::yellow);
		setPalette(fiddle);
        } else {
		setPalette(QApplication::palette());
	}
	update();
}

void InteractiveLabel::enterEvent(QEnterEvent* /*event*/)
{
	emit mouseOver(true);
}

void InteractiveLabel::leaveEvent(QEvent* /*event*/)
{
	emit mouseOver(false);
}
