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
	void initFontList();
	void setFontPreviewColor(const QColor& c);

	void fontSelectionChange(int row);
	void fontTypeChanged(bool state);
	void fontSelectCustom();
	void fontSelectColor();

    void layoutTypeChanged(bool state);
    void on_btnBrowseLayout_clicked();
    void openMSXConnectionChanged();

    void on_leFileName_textChanged(const QString &arg1);
    void on_btnSaveLayout_clicked();

    void createCLI();
    void testOpenMSXCommandLine();

    bool updating;
};

#endif // PREFERENCESDIALOG_OPENMSX_H
