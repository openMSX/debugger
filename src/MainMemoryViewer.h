// $Id: $

#ifndef MAINMEMORYVIEWER_H
#define MAINMEMORYVIEWER_H

#include <QWidget>
#include "HexViewer.h"
#include <QString>
#include <QComboBox>
#include <QLineEdit>

class CPURegsViewer;

class MainMemoryViewer : public QWidget
{
	Q_OBJECT
public:
	MainMemoryViewer(QWidget* parent = 0);
	~MainMemoryViewer();

	void setDebuggable( const QString& name, int size );
	void setRegsView( CPURegsViewer* viewer );


public slots:
	void setLocation(int addr);
	void settingsChanged();
	void refresh();
	void registerChanged(int id, int value);

	void hexViewChanged(int addr);
	void addressValueChanged();
	void addressSourceListChanged(int index);

private:
	HexViewer* hexView;
	QComboBox* addressSourceList;
	QLineEdit* addressValue;

	bool isLinked;
	int linkedId;
	static const int linkRegisters[];
	CPURegsViewer* regsViewer;
};


#endif // MAINMEMORYVIEWER_H
