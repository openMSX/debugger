#ifndef PREFERENCESDIALOG_OPENMSX_H
#define PREFERENCESDIALOG_OPENMSX_H

#include "ui_PreferencesDialog.h"
#include <QDialog>

class PreferencesDialog : public QDialog, private Ui::PreferencesDialog
{
	Q_OBJECT
public:
	PreferencesDialog(QWidget* parent = 0);

private:
	void initFontList();
	void setFontPreviewColor(const QColor& c);

	bool updating;

private slots:
	void fontSelectionChange(int row);
	void fontTypeChanged(bool state);
	void fontSelectCustom();
	void fontSelectColor();
};

#endif // PREFERENCESDIALOG_OPENMSX_H
