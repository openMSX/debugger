#ifndef DEBUGGABLEVIEWER_H
#define DEBUGGABLEVIEWER_H

#include "SavesJsonInterface.h"
#include <QWidget>

class HexViewer;
class QComboBox;

class DebuggableViewer : public QWidget, public SavesJsonInterface
{
	Q_OBJECT
public:
	DebuggableViewer(QWidget* parent = nullptr);

	QJsonObject save2json() override;
	bool loadFromJson(const QJsonObject& obj) override;

	void settingsChanged();
	void setDebuggables(const QMap<QString, int>& list);
	void refresh();

private:
	void debuggableSelected(int index);
	void locationChanged(int loc);

	HexViewer* hexView;
	QComboBox* debuggableList;
	QString lastSelected;
	int lastLocation;

	friend class DebuggerForm; // to get to debuggableSelected from the widgetFactory
};

#endif // DEBUGGABLEVIEWER_H
