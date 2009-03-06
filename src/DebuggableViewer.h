// $Id$

#ifndef DEBUGGABLEVIEWER_H
#define DEBUGGABLEVIEWER_H

#include <QWidget>

class HexViewer;
class QComboBox;

class DebuggableViewer : public QWidget
{
	Q_OBJECT
public:
	DebuggableViewer(QWidget* parent = 0);

public slots:
	void settingsChanged();
	void setDebuggables(const QMap<QString, int>& list);
	void refresh();

private:
	HexViewer* hexView;
	QComboBox* debuggableList;

private slots:
	void debuggableSelected(int index);
};

#endif // DEBUGGABLEVIEWER_H
