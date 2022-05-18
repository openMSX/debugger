#ifndef MAINMEMORYVIEWER_H
#define MAINMEMORYVIEWER_H

#include "SavesJsonInterface.h"
#include <QWidget>

class HexViewer;
class CPURegsViewer;
class SymbolTable;
class QComboBox;
class QLineEdit;
class QJsonObject;

class MainMemoryViewer : public QWidget, public SavesJsonInterface
{
	Q_OBJECT
public:
	MainMemoryViewer(QWidget* parent = nullptr);

	void setDebuggable(const QString& name, int size);
//	void setRegsView(CPURegsViewer* viewer);
	void setSymbolTable(SymbolTable* newtable);

    QJsonObject save2json() final;
    bool loadFromJson(const QJsonObject& obj) final;

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

//	CPURegsViewer* regsViewer;
	SymbolTable* symTable;
	int linkedId;
	bool isLinked;
};

#endif // MAINMEMORYVIEWER_H
