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

	auto* vbox = new QVBoxLayout();
	vbox->setMargin(0);
	vbox->addWidget(debuggableList);
	vbox->addWidget(hexView);
	setLayout(vbox);

	connect(hexView, &HexViewer::locationChanged,
	        this, &DebuggableViewer::locationChanged);
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

void DebuggableViewer::locationChanged(int loc)
{
	lastLocation = loc;
}

void DebuggableViewer::setDebuggables(const QMap<QString, int>& list)
{
	int select = -1;

	// disconnect signal to prevent updates
	debuggableList->disconnect(this);

	debuggableList->clear();
	for (auto it = list.begin(); it != list.end(); ++it) {
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
	connect(debuggableList, qOverload<int>(&QComboBox::currentIndexChanged),
	        this, &DebuggableViewer::debuggableSelected);

	if (!list.empty() && select >= 0) {
		debuggableList->setCurrentIndex(select);
		hexView->setLocation(lastLocation);
	}
}
