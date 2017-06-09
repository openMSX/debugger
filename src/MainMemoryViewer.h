#ifndef MAINMEMORYVIEWER_H
#define MAINMEMORYVIEWER_H

#include <QWidget>

class HexViewer;
class CPURegsViewer;
class SymbolTable;
class QComboBox;
class QLineEdit;

class MainMemoryViewer : public QWidget
{
	Q_OBJECT
public:
	MainMemoryViewer(QWidget* parent = 0);

	void setDebuggable(const QString& name, int size);
	void setRegsView(CPURegsViewer* viewer);
	void setSymbolTable(SymbolTable* symtable);

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

	static const int linkRegisters[];
	CPURegsViewer* regsViewer;
	SymbolTable* symTable;
	int linkedId;
	bool isLinked;
};

#endif // MAINMEMORYVIEWER_H
