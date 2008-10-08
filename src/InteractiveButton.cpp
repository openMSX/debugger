#include "InteractiveButton.h"
#include <QApplication>
#include <QFrame>
#include <QPalette>

InteractiveButton::InteractiveButton(QWidget *parent) 
	: QPushButton(parent)
{
	isHighlighted=false;
	setEnabled(true);
	setAutoFillBackground(true);
}

void InteractiveButton::highlight(bool state)
{
	isHighlighted=state;
	if (state == true){
		// No style sheet since this will not work for Mac OS X atm.
		QPalette fiddle = QApplication::palette();
		fiddle.setColor(QPalette::Active, QPalette::Button, Qt::yellow);
		setPalette(fiddle);
        } else {
		setPalette(QApplication::palette());
	};
	update();
}


void InteractiveButton::enterEvent(QEvent *event)
{
	emit mouseOver(true);
}

void InteractiveButton::leaveEvent(QEvent *event)
{
	emit mouseOver(false);
}



