#ifndef PREFERENCESDIALOG_OPENMSX_H
#define PREFERENCESDIALOG_OPENMSX_H

#include "ui_PreferencesDialog.h"
#include <QDialog>

class PreferencesDialog : public QDialog, private Ui::PreferencesDialog
{
	Q_OBJECT
public:
	PreferencesDialog(QWidget* parent = nullptr);

private:
	void initConfig();
	void initFontList();
	void setFontPreviewColor(const QColor& c);

	void fontSelectionChange(int row);
	void fontTypeChanged(bool state);
	void fontSelectCustom();
	void fontSelectColor();

	void autoReloadSymbols(int state);
	void preserveLostSymbols(int state);
	void preserveBreakpointSymbol(int state);

private:
	bool updating;
};

#endif // PREFERENCESDIALOG_OPENMSX_H
