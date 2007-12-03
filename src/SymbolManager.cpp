#include "SymbolManager.h"
#include "SymbolTable.h"
#include "Settings.h"
#include "Convert.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>

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
	initSymbolList();
}

/*
 * File list support functions
 */

void SymbolManager::initFileList()
{
	treeFiles->clear();
	for( int i = 0; i < symTable.symbolFilesSize(); i++ ) {
		QTreeWidgetItem *item = new QTreeWidgetItem(treeFiles);
		item->setText( 0, symTable.symbolFile(i) );
	}
}

void SymbolManager::addFile()
{
	// create dialog
	QFileDialog *d = new QFileDialog(this);
	QStringList types;
	types << "Symbol files (*.sym)"
	      << "TNIASM 0.x symbol files (*.sym)"
	      << "asMSX 0.x symbol files (*.sym)"
	      << "HiTech link map files (*.map)";
	d->setFilters( types );
	d->setAcceptMode( QFileDialog::AcceptOpen );
	d->setFileMode( QFileDialog::ExistingFile );
	// set default directory
	d->setDirectory( Settings::get().value("SymbolManager/OpenDir", QDir::currentPath() ).toString() );
	// run
	if( d->exec() ) {
		QString f = d->selectedFilter();
		QString n = d->selectedFiles().at(0);
		// load file from the correct type
		bool read = false;
		if( f.startsWith( "TNIASM 0" ) ) {
			read = symTable.readTNIASM0File( n );
		} else if( f.startsWith( "asMSX" ) ) {
			read = symTable.readASMSXFile( n );
		} else {
			if( n.endsWith(".sym") ) {
				// auto detect which sym file
				QFile file( n );
				if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
					QTextStream in(&file);
					QString line = in.readLine();
					file.close();
					if( line[0] == ';' )
						read = symTable.readASMSXFile( n );
					else
						read = symTable.readTNIASM0File( n );
				}
			} else if ( n.endsWith(".map") ) {
				// HiTech link map file
				read = symTable.readLinkMapFile( n );
			} 
		}
		// if read succesful, add it to the list
		if( read ) {
			QTreeWidgetItem *item = new QTreeWidgetItem(treeFiles);
			item->setText( 0, d->selectedFiles().at(0) );
			initSymbolList();
		}
	}
	// story last used path
	Settings::get().setValue( "SymbolManager/OpenDir", d->directory().absolutePath() );
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
	initSymbolList();
}

void SymbolManager::fileSelectionChange()
{
	btnRemoveFile->setEnabled( treeFiles->selectedItems().size() );
}

void SymbolManager::labelEdit( QTreeWidgetItem * item, int column )
{
	if( column > 1 ) return;
	
	Symbol *sym = (Symbol *)(item->data(0, Qt::UserRole).value<quintptr>());
	if( sym->source() == 0 ) {
		treeLabels->openPersistentEditor( item, column );
	}
}

/*
 * Symbol support functions
 */

void SymbolManager::initSymbolList()
{
	treeLabels->clear();
	beginTreeLabelsUpdate();
	Symbol *sym = symTable.findFirstAddressSymbol( 0 );
	while( sym != 0 ) {
		QTreeWidgetItem *item = new QTreeWidgetItem(treeLabels);
		item->setData(0L, Qt::UserRole, quintptr(sym) );
		item->setText( 0, sym->text() );
		item->setText( 1, QString("$%1").arg(sym->value(), 4, 16, QChar('0')) );
		switch( sym->status() ) {
			case Symbol::HIDDEN:
				item->setTextColor(0, QColor(128, 128, 128) );
				break;
			case Symbol::LOST:
				item->setTextColor(0, QColor(128, 0, 0) );
				break;
			default:
				break;
		}
		if( sym->source() )
			item->setIcon( 0, QIcon(":/icons/symfil.png") );
		else
			item->setIcon( 0, QIcon(":/icons/symman.png") );
		sym = symTable.findNextAddressSymbol();
	}
	endTreeLabelsUpdate();
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

void SymbolManager::addLabel()
{
	// create an empty symbol
	Symbol *sym = new Symbol( tr("New symbol"), 0 );
	symTable.add( sym );

	beginTreeLabelsUpdate();
	QTreeWidgetItem *item = new QTreeWidgetItem(treeLabels);
	item->setData(0, Qt::UserRole, quintptr(sym) );
	item->setText( 0, sym->text() );
	item->setText( 1, QString("$%1").arg(sym->value(), 4, 16, QChar('0')) );
	item->setIcon( 0, QIcon(":/icons/symman.png") );
	endTreeLabelsUpdate();
	treeLabels->scrollToItem(item);
	treeLabels->openPersistentEditor( item, 0 );
}

void SymbolManager::labelChanged( QTreeWidgetItem *item, int column )
{
	if( !treeLabelsUpdateCount ) {
		Symbol *sym = (Symbol *)(item->data(0, Qt::UserRole).value<quintptr>());
		// Todo: add validity checks
		sym->setText( item->text(0) );
		int value = stringToValue( item->text(1) );
		if( value >= 0 ) sym->setValue( value );
		treeLabels->closePersistentEditor( item, column );
	}
}

void SymbolManager::labelSelectionChanged()
{
	//QList<QTreeWidgetItem *>
}
