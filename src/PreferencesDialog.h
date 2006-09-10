#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include "ui_PreferencesDialog.h"

class PreferencesDialog : public QDialog, private Ui::PreferencesDialog
{
	Q_OBJECT
public:
	PreferencesDialog(QWidget *parent = 0);

private:
	void initFontList();
	void setFontPreviewColor( const QColor& c );

	bool updating;

private slots:
	void fontSelectionChange( int row );
	void fontTypeChanged( bool state );
	void fontSelectCustom();
	void fontSelectColor();
};

#endif /* PREFERENCESDIALOG_H */
