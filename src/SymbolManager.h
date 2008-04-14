// $Id$
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

protected:
	void closeEvent( QCloseEvent *e );

private:
	SymbolTable& symTable;
	int treeLabelsUpdateCount;
	QCheckBox *chkSlots[16], *chkRegs[18];
	QTreeWidgetItem *editItem;
	int editColumn;

	void initFileList();
	void initSymbolList();

	void closeEditor();

	// formatting functions
	void updateItemName( QTreeWidgetItem *item );
	void updateItemType( QTreeWidgetItem *item );
	void updateItemValue( QTreeWidgetItem *item );
	void updateItemSlots( QTreeWidgetItem *item );
	void updateItemSegments( QTreeWidgetItem *item );
	void updateItemRegisters( QTreeWidgetItem *item );

	void beginTreeLabelsUpdate();
	void endTreeLabelsUpdate();

private slots:
	void fileSelectionChange();
	void addFile();
	void removeFile();
	void reloadFiles();
	void addLabel();
	void removeLabel();
	void labelEdit( QTreeWidgetItem * item, int column );
	void labelChanged( QTreeWidgetItem *item, int column );
	void labelSelectionChanged();
	void changeType( bool checked );
	void changeSlot( int id, int state );
	void changeSlot00( int state );
	void changeSlot01( int state );
	void changeSlot02( int state );
	void changeSlot03( int state );
	void changeSlot10( int state );
	void changeSlot11( int state );
	void changeSlot12( int state );
	void changeSlot13( int state );
	void changeSlot20( int state );
	void changeSlot21( int state );
	void changeSlot22( int state );
	void changeSlot23( int state );
	void changeSlot30( int state );
	void changeSlot31( int state );
	void changeSlot32( int state );
	void changeSlot33( int state );
	void changeRegister( int id, int state );
	void changeRegisterA( int state );
	void changeRegisterB( int state );
	void changeRegisterC( int state );
	void changeRegisterD( int state );
	void changeRegisterE( int state );
	void changeRegisterH( int state );
	void changeRegisterL( int state );
	void changeRegisterBC( int state );
	void changeRegisterDE( int state );
	void changeRegisterHL( int state );
	void changeRegisterIX( int state );
	void changeRegisterIY( int state );
	void changeRegisterIXL( int state );
	void changeRegisterIXH( int state );
	void changeRegisterIYL( int state );
	void changeRegisterIYH( int state );
	void changeRegisterOffset( int state );
	void changeRegisterI( int state );
	
};

#endif /* SYMBOLMANAGER_H */
