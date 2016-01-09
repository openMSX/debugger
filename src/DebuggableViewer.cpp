// $Id$

#include "DebuggableViewer.h"
#include "HexViewer.h"
#include <QComboBox>
#include <QVBoxLayout>

DebuggableViewer::DebuggableViewer(QWidget* parent)
	: QWidget(parent)
{
	// create selection list and viewer
	debuggableList = new QComboBox();
	debuggableList->setEditable(false);

	hexView = new HexViewer();
	hexView->setIsInteractive(true);
	hexView->setIsEditable(true);

	QVBoxLayout* vbox = new QVBoxLayout();
	vbox->setMargin(0);
	vbox->addWidget(debuggableList);
	vbox->addWidget(hexView);
	setLayout(vbox);
}

void DebuggableViewer::settingsChanged()
{
	hexView->settingsChanged();
}

void DebuggableViewer::refresh()
{
	hexView->refresh();
}

void DebuggableViewer::debuggableSelected(int index)
{
	QString name = debuggableList->itemText(index);
	int size = debuggableList->itemData(index).toInt();

	if (index >= 0)
		lastSelected = name;
	// add braces when the name contains a space
	if (name.contains(QChar(' '))) {
		name.append(QChar('}'));
		name.prepend(QChar('{'));
	}
	hexView->setDebuggable(name, size);
}

void DebuggableViewer::setDebuggables(const QMap<QString, int>& list)
{
	int select = 0;

	// disconnect signal to prevent updates
	debuggableList->disconnect(this, SLOT(debuggableSelected(int)));

	debuggableList->clear();
	for (QMap<QString, int>::const_iterator it = list.begin();
	     it != list.end(); ++it) {
		// set name and strip braces if necessary
		QString name = it.key();
		if (name.contains(QChar(' '))) {
			name = name.mid(1, name.size() - 2);
		}
		// check if this was the previous selection
		if (name == lastSelected)
			select = debuggableList->count();

		debuggableList->addItem(name, it.value());
	}

	// reconnect signal before selecting item
	connect(debuggableList, SIGNAL(currentIndexChanged(int)),
	        this, SLOT(debuggableSelected(int)));

	if (!list.empty()) 
		debuggableList->setCurrentIndex(select);
}
