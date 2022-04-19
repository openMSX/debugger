#ifndef DEBUGGABLEVIEWER_H
#define DEBUGGABLEVIEWER_H

#include <QWidget>

class HexViewer;
class QComboBox;

class DebuggableViewer : public QWidget
{
	Q_OBJECT
public:
	DebuggableViewer(QWidget* parent = nullptr);

public slots:
	void settingsChanged();
	void setDebuggables(const QMap<QString, int>& list);
	void refresh();

private:
	HexViewer* hexView;
	QComboBox* debuggableList;
	QString lastSelected;
	int lastLocation;

private slots:
	void debuggableSelected(int index);
	void locationChanged(int loc);

friend class DebuggerForm; // to get to  debuggableSelected  from the widgetFactory
};

#endif // DEBUGGABLEVIEWER_H
