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

	void settingsChanged();
	void setDebuggables(const QMap<QString, int>& list);
	void refresh();

private:
	void debuggableSelected(int index);
	void locationChanged(int loc);

private:
	HexViewer* hexView;
	QComboBox* debuggableList;
	QString lastSelected;
	int lastLocation;
};

#endif // DEBUGGABLEVIEWER_H
