#include "SymbolManager.h"
#include <QFileDialog>
#include <QSettings>
#include <QMessageBox>

SymbolManager::SymbolManager(SymbolTable& symtable, QWidget *parent)
	: QDialog(parent), symTable(symtable)
{
	setupUi(this);
	
	treeLabelsUpdateCount = 0;
	
	connect( treeFiles, SIGNAL( itemSelectionChanged() ), this, SLOT( fileSelectionChange() ) );
	connect( btnAddFile, SIGNAL( clicked() ), this, SLOT( addFile() ) );
	connect( btnRemoveFile, SIGNAL( clicked() ), this, SLOT( removeFile() ) );
	connect( treeLabels, SIGNAL( itemDoubleClicked( QTreeWidgetItem *, int ) ),
	         this, SLOT( labelEdit( QTreeWidgetItem*, int ) ) );
	connect( treeLabels, SIGNAL( itemChanged( QTreeWidgetItem *, int ) ),
	         this, SLOT( labelChanged( QTreeWidgetItem*, int ) ) );
	connect( btnAddSymbol, SIGNAL( clicked() ), this, SLOT( addLabel() ) );

	groupSlots->setEnabled(false);
	groupSegments->setEnabled(false);
	btnRemoveFile->setEnabled(false);
	btnRemoveSymbol->setEnabled(false);
	
	initFileList();
	initAddressSymbolList();
}

void SymbolManager::beginTreeLabelsUpdate()
{
	treeLabelsUpdateCount++;
}

void SymbolManager::endTreeLabelsUpdate()
{
	if( treeLabelsUpdateCount > 0 )
		treeLabelsUpdateCount--;
}

void SymbolManager::initFileList()
{
	treeFiles->clear();
	for( int i = 0; i < symTable.symbolFilesSize(); i++ ) {
		QTreeWidgetItem *item = new QTreeWidgetItem(treeFiles);
		item->setText( 0, symTable.symbolFile(i) );
	}
}

void SymbolManager::initAddressSymbolList()
{
	treeLabels->clear();
	beginTreeLabelsUpdate();
	AddressSymbol *sym = symTable.findFirstAddressSymbol( 0 );
	while( sym != 0 ) {
		QTreeWidgetItem *item = new QTreeWidgetItem(treeLabels);
		item->setData(0, Qt::UserRole, int(sym) );
		item->setText( 0, sym->getText() );
		item->setText( 1, QString("$%1").arg(sym->getAddress(), 4, 16, QChar('0')) );
		switch( sym->status() ) {
			case AddressSymbol::HIDDEN:
				item->setTextColor(0, QColor(128, 128, 128) );
				break;
			case AddressSymbol::LOST:
				item->setTextColor(0, QColor(128, 0, 0) );
				break;
		}
		if( sym->getSource() )
			item->setIcon( 0, QIcon(":/icons/symfil.png") );
		else
			item->setIcon( 0, QIcon(":/icons/symman.png") );
		sym = symTable.findNextAddressSymbol();
	}
	endTreeLabelsUpdate();
}

void SymbolManager::fileSelectionChange()
{
	btnRemoveFile->setEnabled( treeFiles->selectedItems().size() );
}

void SymbolManager::addFile()
{
	// create dialog
	QFileDialog *d = new QFileDialog(this);
	QStringList types;
	types << "Symbol files (*.sym)"
	      << "TNIASM 0.x symbol files (*.sym)";
	d->setFilters( types );
	d->setAcceptMode( QFileDialog::AcceptOpen );
	d->setFileMode( QFileDialog::ExistingFile );
	// set default directory
	QSettings settings("openMSX", "debugger");
	d->setDirectory( settings.value("SymbolManager/OpenDir", QDir::currentPath() ).toString() );
	// run
	if( d->exec() ) {
		QString f = d->selectedFilter();
		QString n = d->selectedFiles().at(0);
		// load file from the correct type
		bool read = false;
		if( f.startsWith( "Symbol files" ) ) {
			read = symTable.readTNIASM0File( n );
		} else if( f.startsWith( "TNIASM 0" ) ) {
			read = symTable.readTNIASM0File( n );
		}
		// if read succesful, add it to the list
		if( read ) {
			QTreeWidgetItem *item = new QTreeWidgetItem(treeFiles);
			item->setText( 0, d->selectedFiles().at(0) );
			initAddressSymbolList();
		}
	}
	// story last used path
	settings.setValue( "SymbolManager/OpenDir", d->directory().absolutePath() );
}

void SymbolManager::removeFile()
{
	int r = QMessageBox::question(this, tr("Remove symbol file(s)"), 
		tr("When removing the symbol file(s), do you want keep or delete the attached symbols?"),
		"Keep symbols", "Delete symbols", "Cancel", 1, 2);
	
	if( r == 2 ) return;
	
	for( int i = 0; i < treeFiles->selectedItems().size(); i++ )
		symTable.unloadFile( treeFiles->selectedItems().at(i)->text(0), r == 0);
	
	initFileList();
	initAddressSymbolList();
}

void SymbolManager::labelEdit( QTreeWidgetItem * item, int column )
{
	if( column > 1 ) return;
	
	AddressSymbol *sym = (AddressSymbol *)(item->data(0, Qt::UserRole).toInt());
	if( sym->getSource() == 0 ) {
		treeLabels->openPersistentEditor( item, column );
	}
}

void SymbolManager::addLabel()
{
	// create an empty symbol
	AddressSymbol *sym = new AddressSymbol( tr("New symbol"), 0 );
	symTable.addAddressSymbol( sym );

	beginTreeLabelsUpdate();
	QTreeWidgetItem *item = new QTreeWidgetItem(treeLabels);
	item->setData(0, Qt::UserRole, int(sym) );
	item->setText( 0, sym->getText() );
	item->setText( 1, QString("$%1").arg(sym->getAddress(), 4, 16, QChar('0')) );
	item->setIcon( 0, QIcon(":/icons/symman.png") );
	endTreeLabelsUpdate();
	treeLabels->scrollToItem(item);
	treeLabels->openPersistentEditor( item, 0 );
}

void SymbolManager::labelChanged( QTreeWidgetItem *item, int column )
{
	if( !treeLabelsUpdateCount ) {
		AddressSymbol *sym = (AddressSymbol *)(item->data(0, Qt::UserRole).toInt());
		// Todo: add validity checks
		sym->setText( item->text(0) );
		sym->setAddress( item->text(1).toInt() );
		treeLabels->closePersistentEditor( item, column );
	}
}

void SymbolManager::labelSelectionChanged()
{
}
