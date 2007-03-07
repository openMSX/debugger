#ifndef SYMBOLMANAGER_H
#define SYMBOLMANAGER_H

#include "ui_SymbolManager.h"

class SymbolTable;
class QTreeWidgetItem;

class SymbolManager : public QDialog, private Ui::SymbolManager
{
	Q_OBJECT
public:
	SymbolManager(SymbolTable& symtable, QWidget *parent = 0);

private:
	SymbolTable& symTable;
	int treeLabelsUpdateCount;

	void initFileList();
	void initSymbolList();

	void beginTreeLabelsUpdate();
	void endTreeLabelsUpdate();

private slots:
	void fileSelectionChange();
	void addFile();
	void removeFile();
	void addLabel();
	void labelEdit( QTreeWidgetItem * item, int column );
	void labelChanged( QTreeWidgetItem *item, int column );
	void labelSelectionChanged();
};

#endif /* SYMBOLMANAGER_H */
