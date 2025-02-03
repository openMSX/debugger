#ifndef MAINMEMORYVIEWER_H
#define MAINMEMORYVIEWER_H

#include <QWidget>

class HexViewer;
class CPURegsViewer;
class SymbolTable;
class QComboBox;
class QLineEdit;
class QCompleter;
struct MemoryLayout;

class MainMemoryViewer : public QWidget
{
	Q_OBJECT
public:
	MainMemoryViewer(QWidget* parent = nullptr);

	void setDebuggable(const QString& name, int size);
	void setRegsView(CPURegsViewer* viewer);
	void setSymbolTable(SymbolTable* symtable);
	void setMemoryLayout(MemoryLayout* ml);

	void setLocation(int addr);
	void settingsChanged();
	void refresh();
	void registerChanged(int id, int value);

	void hexViewChanged(int addr);
	void updateCompleter();
	void addressValueChanging();
	void addressValueChanged();
	void addressSourceListChanged(int index);

private:
	HexViewer* hexView;
	QComboBox* addressSourceList;
	QLineEdit* addressValue;
	QCompleter* completer;

	CPURegsViewer* regsViewer;
	SymbolTable* symTable;
	MemoryLayout* memLayout;
	int linkedId;
	bool isLinked;
};

#endif // MAINMEMORYVIEWER_H
