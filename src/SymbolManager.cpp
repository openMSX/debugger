#include "SymbolManager.h"
#include "SymbolTable.h"
#include "Settings.h"
#include "Convert.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QHeaderView>

SymbolManager::SymbolManager(SymbolTable& symtable, QWidget *parent)
	: QDialog(parent), symTable(symtable)
{
	setupUi(this);

	// restore layout
	Settings &s = Settings::get();
	restoreGeometry( s.value( "SymbolManager/WindowGeometry", saveGeometry() ).toByteArray() );

	QHeaderView *header = treeFiles->header();
	header->resizeSection( 0, s.value( "SymbolManager/HeaderFileSize", 320 ).toInt() );
	header->resizeSection( 1, s.value( "SymbolManager/HeaderRefreshSize", header->sectionSize(1) ).toInt() );
	header = treeLabels->header();
	header->resizeSection( 0, s.value( "SymbolManager/HeaderSymbolSize", 150 ).toInt() );
	header->resizeSection( 1, s.value( "SymbolManager/HeaderTypeSize", header->sectionSize(1) ).toInt() );
	header->resizeSection( 2, s.value( "SymbolManager/HeaderValueSize", header->sectionSize(2) ).toInt() );
	header->resizeSection( 3, s.value( "SymbolManager/HeaderSlotsSize", header->sectionSize(3) ).toInt() );
	header->resizeSection( 4, s.value( "SymbolManager/HeaderSegmentsSize", header->sectionSize(4) ).toInt() );
	header->resizeSection( 5, s.value( "SymbolManager/HeaderRangeSize", header->sectionSize(5) ).toInt() );
	header->resizeSection( 6, s.value( "SymbolManager/HeaderSourceSize", header->sectionSize(6) ).toInt() );

	treeLabelsUpdateCount = 0;

	// put slot checkboxes in a convenience array
	chkSlots[ 0] = chk00; chkSlots[ 1] = chk01; chkSlots[ 2] = chk02; chkSlots[ 3] = chk03;
	chkSlots[ 4] = chk10; chkSlots[ 5] = chk11; chkSlots[ 6] = chk12; chkSlots[ 7] = chk13;
	chkSlots[ 8] = chk20; chkSlots[ 9] = chk21; chkSlots[10] = chk22; chkSlots[11] = chk23;
	chkSlots[12] = chk30; chkSlots[13] = chk31; chkSlots[14] = chk32; chkSlots[15] = chk33;

	connect( treeFiles, SIGNAL( itemSelectionChanged() ), this, SLOT( fileSelectionChange() ) );
	connect( btnAddFile, SIGNAL( clicked() ), this, SLOT( addFile() ) );
	connect( btnRemoveFile, SIGNAL( clicked() ), this, SLOT( removeFile() ) );
	connect( treeLabels, SIGNAL( itemSelectionChanged() ), this, SLOT( labelSelectionChanged() ) );
	connect( treeLabels, SIGNAL( itemDoubleClicked( QTreeWidgetItem *, int ) ),
	         this, SLOT( labelEdit( QTreeWidgetItem*, int ) ) );
	connect( treeLabels, SIGNAL( itemChanged( QTreeWidgetItem *, int ) ),
	         this, SLOT( labelChanged( QTreeWidgetItem*, int ) ) );
	connect( btnAddSymbol, SIGNAL( clicked() ), this, SLOT( addLabel() ) );
	connect( chk00, SIGNAL( stateChanged(int) ), this, SLOT( changeSlot00(int) ) );
	connect( chk01, SIGNAL( stateChanged(int) ), this, SLOT( changeSlot01(int) ) );
	connect( chk02, SIGNAL( stateChanged(int) ), this, SLOT( changeSlot02(int) ) );
	connect( chk03, SIGNAL( stateChanged(int) ), this, SLOT( changeSlot03(int) ) );
	connect( chk10, SIGNAL( stateChanged(int) ), this, SLOT( changeSlot10(int) ) );
	connect( chk11, SIGNAL( stateChanged(int) ), this, SLOT( changeSlot11(int) ) );
	connect( chk12, SIGNAL( stateChanged(int) ), this, SLOT( changeSlot12(int) ) );
	connect( chk13, SIGNAL( stateChanged(int) ), this, SLOT( changeSlot13(int) ) );
	connect( chk20, SIGNAL( stateChanged(int) ), this, SLOT( changeSlot20(int) ) );
	connect( chk21, SIGNAL( stateChanged(int) ), this, SLOT( changeSlot21(int) ) );
	connect( chk22, SIGNAL( stateChanged(int) ), this, SLOT( changeSlot22(int) ) );
	connect( chk23, SIGNAL( stateChanged(int) ), this, SLOT( changeSlot23(int) ) );
	connect( chk30, SIGNAL( stateChanged(int) ), this, SLOT( changeSlot30(int) ) );
	connect( chk31, SIGNAL( stateChanged(int) ), this, SLOT( changeSlot31(int) ) );
	connect( chk32, SIGNAL( stateChanged(int) ), this, SLOT( changeSlot32(int) ) );
	connect( chk33, SIGNAL( stateChanged(int) ), this, SLOT( changeSlot33(int) ) );

	groupSlots->setEnabled(false);
	groupSegments->setEnabled(false);
	btnRemoveFile->setEnabled(false);
	btnRemoveSymbol->setEnabled(false);

	initFileList();
	initSymbolList();
}


void SymbolManager::closeEvent( QCloseEvent *e )
{
	// store layout
	Settings &s = Settings::get();
	s.setValue( "SymbolManager/WindowGeometry", saveGeometry() );
	QHeaderView *header = treeFiles->header();
	s.setValue( "SymbolManager/HeaderFileSize", header->sectionSize(0) );
	s.setValue( "SymbolManager/HeaderRefreshSize", header->sectionSize(1) );
	header = treeLabels->header();
	s.setValue( "SymbolManager/HeaderSymbolSize", header->sectionSize(0) );
	s.setValue( "SymbolManager/HeaderTypeSize", header->sectionSize(1) );
	s.setValue( "SymbolManager/HeaderValueSize", header->sectionSize(2) );
	s.setValue( "SymbolManager/HeaderSlotsSize", header->sectionSize(3) );
	s.setValue( "SymbolManager/HeaderSegmentsSize", header->sectionSize(4) );
	s.setValue( "SymbolManager/HeaderRangeSize", header->sectionSize(5) );
	s.setValue( "SymbolManager/HeaderSourceSize", header->sectionSize(6) );

	QDialog::closeEvent(e);
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
		item->setText( 1, symTable.symbolFileRefresh(i).toString( Qt::LocaleDate ) );
	}
}

void SymbolManager::addFile()
{
	// create dialog
	QFileDialog *d = new QFileDialog(this);
	QStringList types;
	types << "All supported files (*.sym *.map)"
	      << "tniASM 0.x symbol files (*.sym)"
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
		if( f.startsWith( "tniASM 0" ) ) {
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
			initFileList();
			initSymbolList();
		}
	}
	// store last used path
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
	// only symbol name and value are editable
	if( column == 0 || column == 2 ) {;
		// open editor if manually added symbol
		Symbol *sym = (Symbol *)(item->data(0, Qt::UserRole).value<quintptr>());
		if( sym->source() == 0 ) {
			treeLabels->openPersistentEditor( item, column );
		}
		if( column == 0 )
			updateItemName( item );
		else
			updateItemValue( item );
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
		// attach a pointer to the symbol object to the tree item
		item->setData(0L, Qt::UserRole, quintptr(sym) );
		// update columns
		updateItemName( item );
		updateItemType( item );
		updateItemValue( item );
		updateItemSlots( item );
		updateItemSegments( item );
		updateItemCodeRange( item );
		
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
	item->setIcon( 0, QIcon(":/icons/symman.png") );
	item->setText( 1, tr("Active") );
	item->setText( 2, QString("$%1").arg(sym->value(), 4, 16, QChar('0')) );
	endTreeLabelsUpdate();
	treeLabels->setFocus();
	treeLabels->setCurrentItem( item, 0 );
	treeLabels->openPersistentEditor( item, 0 );
	treeLabels->scrollToItem(item);
}

void SymbolManager::labelChanged( QTreeWidgetItem *item, int column )
{
	if( !treeLabelsUpdateCount ) {
		Symbol *sym = (Symbol *)(item->data(0, Qt::UserRole).value<quintptr>());
		// Set symbol text from tree item if any
		QString symText = item->text(0).trimmed();
		if( symText.isEmpty() ) symText = "[unnamed]";
		sym->setText( symText );
		// set value TODO: proper decoding of value
		int value = stringToValue( item->text(2) );
		if( value >= 0 && value < 65536 ) sym->setValue( value );
		treeLabels->closePersistentEditor( item, column );
		// update item name and value
		beginTreeLabelsUpdate();
		updateItemName(item);
		updateItemValue(item);
		endTreeLabelsUpdate();
	}
}

void SymbolManager::labelSelectionChanged()
{
	QList<QTreeWidgetItem *> selection = treeLabels->selectedItems();
	// check if is available at all
	if( !selection.size() ) {
		// disable everything
		btnRemoveSymbol->setEnabled(false);
		groupSlots->setEnabled(false);
		groupSegments->setEnabled(false);
		return;
	}
	// check selection for "manual insertion", identical slot mask
	bool removeButActive = true;
	int slotMask, slotMaskMultiple = 0;
	QList<QTreeWidgetItem *>::iterator selit = selection.begin();
	while( selit != selection.end() ) {
		// get symbol
		Symbol *sym = (Symbol *)((*selit)->data(0, Qt::UserRole).value<quintptr>());
		// check if symbol is from symbol file
		if( sym->source() ) removeButActive = false;
		
		if( selit == selection.begin() ) {
			// first item, reference for slotMask
			slotMask = sym->validSlots();
		} else {
			// other, set all different bits
			slotMaskMultiple |= slotMask ^ sym->validSlots();
		}
		
		// next
		selit++;
	}

	btnRemoveSymbol->setEnabled(removeButActive);
	groupSlots->setEnabled(true);
	beginTreeLabelsUpdate();
	for( int i = 0; i < 16; i++ ) {
		chkSlots[i]->setTristate(false);
		if( slotMaskMultiple & 1 )
			chkSlots[i]->setCheckState(Qt::PartiallyChecked);
		else if( slotMask & 1 )
			chkSlots[i]->setCheckState(Qt::Checked);
		else
			chkSlots[i]->setCheckState(Qt::Unchecked);
		slotMask >>= 1;
		slotMaskMultiple >>= 1;
	}
	endTreeLabelsUpdate();
}

void SymbolManager::changeSlot( int id, int state )
{
	if( !treeLabelsUpdateCount ) {
		// disallow another tristate selection
		chkSlots[id]->setTristate(false);
		// get selected items
		QList<QTreeWidgetItem *> selection = treeLabels->selectedItems();

		// update items		
		beginTreeLabelsUpdate();
		QList<QTreeWidgetItem *>::iterator selit = selection.begin();
		int bit = 1<<id;
		while( selit != selection.end() ) {
			// get symbol
			Symbol *sym = (Symbol *)((*selit)->data(0, Qt::UserRole).value<quintptr>());
			// set or clear bit
			if( state == Qt::Checked )
				sym->setValidSlots( sym->validSlots() | bit );
			else
				sym->setValidSlots( sym->validSlots() & (~bit) );
			// update item in treewidget
			updateItemSlots( *selit );
			// next
			selit++;
		}
		endTreeLabelsUpdate();
	}
}

void SymbolManager::changeSlot00( int state )
{
	changeSlot( 0, state );
}

void SymbolManager::changeSlot01( int state )
{
	changeSlot( 1, state );
}

void SymbolManager::changeSlot02( int state )
{
	changeSlot( 2, state );
}

void SymbolManager::changeSlot03( int state )
{
	changeSlot( 3, state );
}

void SymbolManager::changeSlot10( int state )
{
	changeSlot( 4, state );
}

void SymbolManager::changeSlot11( int state )
{
	changeSlot( 5, state );
}

void SymbolManager::changeSlot12( int state )
{
	changeSlot( 6, state );
}

void SymbolManager::changeSlot13( int state )
{
	changeSlot( 7, state );
}

void SymbolManager::changeSlot20( int state )
{
	changeSlot( 8, state );
}

void SymbolManager::changeSlot21( int state )
{
	changeSlot( 9, state );
}

void SymbolManager::changeSlot22( int state )
{
	changeSlot( 10, state );
}

void SymbolManager::changeSlot23( int state )
{
	changeSlot( 11, state );
}

void SymbolManager::changeSlot30( int state )
{
	changeSlot( 12, state );
}

void SymbolManager::changeSlot31( int state )
{
	changeSlot( 13, state );
}

void SymbolManager::changeSlot32( int state )
{
	changeSlot( 14, state );
}

void SymbolManager::changeSlot33( int state )
{
	changeSlot( 15, state );
}


/*
 * Symbol tree layout functions
 */

void SymbolManager::updateItemName( QTreeWidgetItem *item )
{
	Symbol *sym = (Symbol *)(item->data(0, Qt::UserRole).value<quintptr>());
	// set text and icon
	item->setText( 0, sym->text() );
	if( sym->source() ) {
		item->setIcon( 0, QIcon(":/icons/symfil.png") );
		item->setText( 6, *sym->source() );
	} else
		item->setIcon( 0, QIcon(":/icons/symman.png") );
	// set colour based on status as well as status in 2nd column
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
}

void SymbolManager::updateItemType( QTreeWidgetItem *item )
{
	Symbol *sym = (Symbol *)(item->data(0, Qt::UserRole).value<quintptr>());
	// set symbol type in 2nd column
	switch( sym->type() ) {
		case Symbol::JUMPLABEL:
			item->setText( 1, tr("Jump label") );
			break;
		case Symbol::VARIABLELABEL:
			item->setText( 1, tr("Variabel label") );
			break;
		case Symbol::VALUE:
			item->setText( 1, tr("Value") );
			break;
	}
}

void SymbolManager::updateItemValue( QTreeWidgetItem *item )
{
	Symbol *sym = (Symbol *)(item->data(0, Qt::UserRole).value<quintptr>());

	// symbol value in 3rd column
	// TODO: Custom prefix/postfix
	item->setText( 2, QString("$%1").arg(sym->value(), 4, 16, QChar('0')) );
}

void SymbolManager::updateItemSlots( QTreeWidgetItem *item )
{
	Symbol *sym = (Symbol *)(item->data(0, Qt::UserRole).value<quintptr>());

	QString slotText;
	int slotmask = sym->validSlots();
	// value represents 16 bits for 4 subslots in 4 slots
	if( slotmask == 0xFFFF ) {
		slotText = tr("All");
	} else if( slotmask == 0 ) {
		slotText = tr("None");
	} else {
		// create a list of valid slots
		// loop over all primary slots
		for( int ps = 0; ps < 4; ps++ ) {
			QString subText;
			int subslots = (slotmask >> (4*ps)) & 15;
			if( subslots == 15 ) {
				// all subslots are ok
				subText = QString("%1-*").arg(ps);
			} else if( subslots ) {
				// some subslots are ok
				if( subslots & 1 )
					subText += "/0";
				if( subslots & 2 )
					subText += "/1";
				if( subslots & 4 )
					subText += "/2";
				if( subslots & 8 )
					subText += "/3";
				subText = QString("%1-").arg(ps) + subText.mid(1);
			}
			// add to string if any subslots were ok
			if( !subText.isEmpty() ) {
				if( !slotText.isEmpty() ) slotText += ", ";
				slotText += subText;
			}
		}
	}

	// valid slots in 4th column
	item->setText( 3, slotText );
}

void SymbolManager::updateItemSegments( QTreeWidgetItem *item )
{
	Symbol *sym = (Symbol *)(item->data(0, Qt::UserRole).value<quintptr>());
}

void SymbolManager::updateItemCodeRange( QTreeWidgetItem *item )
{
	Symbol *sym = (Symbol *)(item->data(0, Qt::UserRole).value<quintptr>());
}

