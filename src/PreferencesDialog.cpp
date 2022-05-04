#include "PreferencesDialog.h"
#include "Settings.h"
#include "DebuggerForm.h"

#include <QListWidgetItem>
#include <QFontDialog>
#include <QColorDialog>
#include <QFileDialog>


PreferencesDialog::PreferencesDialog(QWidget* parent)
	: QDialog(parent)
{
	setupUi(this);

    //font stuff
    connect(listFonts, &QListWidget::currentRowChanged,
            this, &PreferencesDialog::fontSelectionChange);
    connect(rbUseAppFont,&QRadioButton::toggled,
            this, &PreferencesDialog::fontTypeChanged);
    connect(rbUseFixedFont,  &QRadioButton::toggled,
            this, &PreferencesDialog::fontTypeChanged);
    connect(rbUseCustomFont, &QRadioButton::toggled,
            this, &PreferencesDialog::fontTypeChanged);
    connect(btnSelectFont, &QPushButton::clicked,
            this, &PreferencesDialog::fontSelectCustom);
    connect(btnFontColor, &QPushButton::clicked,
            this, &PreferencesDialog::fontSelectColor);

	initFontList();
	listFonts->setCurrentRow(0);

    //layout stuff
    QList<QRadioButton*> rblayouttypes;
    rblayouttypes << rbFirstTimeUser << rbDefaultWorkspaces << rbLayoutFromFile;
    foreach(auto rb, rblayouttypes){
        connect(rb, &QRadioButton::toggled,
                this, &PreferencesDialog::layoutTypeChanged);
    };
//    connect(rbDefaultWorkspaces, &QRadioButton::toggled,
//            this, &PreferencesDialog::layoutTypeChanged);
//    connect(rbLayoutFromFile, &QRadioButton::toggled,
//            this, &PreferencesDialog::layoutTypeChanged);
    updating=true;
    Settings& s = Settings::get();
    rblayouttypes.at(s.value("creatingWorkspaceType",0).toInt())->setChecked(true);
    leFileName->setText(s.value("creatingWorkspaceFile","").toString());
    updating=false;
}

/*
 * Font settings
 */
void PreferencesDialog::initFontList()
{
	Settings& s = Settings::get();
	listFonts->clear();
	for (int f = Settings::APP_FONT; f < Settings::FONT_END; ++f) {
		new QListWidgetItem(s.fontName((Settings::DebuggerFont)f), listFonts);
	}
}

void PreferencesDialog::fontSelectionChange(int row)
{
	updating = true;
	if (row == 0) {
		rbUseAppFont->setText(tr("Use system font"));
	} else {
		rbUseAppFont->setText(tr("Use application font"));
	}

	rbUseAppFont->setEnabled(row >= 0);
	rbUseFixedFont->setEnabled(row > 1);
	rbUseCustomFont->setEnabled(row >= 0);

	switch (Settings::get().fontType((Settings::DebuggerFont)row)) {
	case Settings::APPLICATION_DEFAULT:
		rbUseAppFont->setChecked(true);
		break;
	case Settings::FIXED_DEFAULT:
		rbUseFixedFont->setChecked(true);
		break;
	case Settings::CUSTOM:
		rbUseCustomFont->setChecked(true);
		break;
	}

	lblPreview->setFont(Settings::get().font     ((Settings::DebuggerFont)row));
	setFontPreviewColor(Settings::get().fontColor((Settings::DebuggerFont)row));
	btnSelectFont->setEnabled(rbUseCustomFont->isChecked());
	btnFontColor->setEnabled(row > 1);
	updating = false;
}

void PreferencesDialog::fontTypeChanged(bool state)
{
	if (!state || updating) return;

	auto f = (Settings::DebuggerFont)(listFonts->currentRow());
	Settings& s = Settings::get();

	if (rbUseAppFont->isChecked()) {
		s.setFontType(f, Settings::APPLICATION_DEFAULT);
	} else if (rbUseFixedFont->isChecked()) {
		s.setFontType(f, Settings::FIXED_DEFAULT);
	} else {
		s.setFontType(f, Settings::CUSTOM);
	}
	lblPreview->setFont(s.font(f));
	btnSelectFont->setEnabled(rbUseCustomFont->isChecked());
}

void PreferencesDialog::fontSelectCustom()
{
	bool ok;
	auto f = (Settings::DebuggerFont)(listFonts->currentRow());
	QFont newFont = QFontDialog::getFont(&ok, Settings::get().font(f));
	if (ok) {
		lblPreview->setFont(newFont);
		Settings::get().setFont(f, newFont);
	}
}

void PreferencesDialog::fontSelectColor()
{
	auto f = (Settings::DebuggerFont)(listFonts->currentRow());
	QColor newColor = QColorDialog::getColor(Settings::get().fontColor(f), this);
	if (newColor.isValid()) {
		Settings::get().setFontColor(f, newColor);
		setFontPreviewColor(newColor);
    }
}

void PreferencesDialog::layoutTypeChanged(bool state)
{
    if (!state || updating) return;

    Settings& s = Settings::get();

    int wst=2;
    if (rbFirstTimeUser->isChecked()){
        wst=0;
    } else if (rbDefaultWorkspaces->isChecked()){
        wst=1;
    } else {
        s.setValue("creatingWorkspaceFile",leFileName->text());
    }
    s.setValue("creatingWorkspaceType",wst);
}

void PreferencesDialog::setFontPreviewColor(const QColor& c)
{
	if (listFonts->currentRow() > 1) {
		QPalette pal = lblPreview->palette();
		pal.setColor(QPalette::WindowText, c);
		lblPreview->setPalette(pal);
	} else {
		lblPreview->setPalette(QPalette());
	}
}

void PreferencesDialog::on_btnBrowseLayout_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(
        this, tr("Select workspace layout"),
        QDir::currentPath(), tr("Debug Workspace Layout Files (*.omdl)"));

    if (!fileName.isEmpty()){
        leFileName->setText(fileName);
        rbLayoutFromFile->setChecked(true); //not sure if setText with already string in lineEdit will trigger the on_leFileName_textChanged
    }
}

void PreferencesDialog::on_leFileName_textChanged(const QString &arg1)
{
    if (updating) return;
    Settings& s = Settings::get();
    s.setValue("creatingWorkspaceFile",arg1);

    rbLayoutFromFile->setChecked(true);
}


void PreferencesDialog::on_btnSaveLayout_clicked()
{
    Settings& s = Settings::get();

    QString savefilename=leFileName->text();
    if (savefilename.isEmpty()){
        //maybe the user cleared the lineEdit or first time launch or something went wrong :-)
         savefilename=s.value("creatingWorkspaceFile","").toString();
    }
    if (savefilename.isEmpty()) {
        savefilename=static_cast<DebuggerForm*>(parent())->fileSaveWorkspaceAs();
    } else {
        static_cast<DebuggerForm*>(parent())->saveWorkspacesAs(savefilename);
    }
    //update filename in case of fileSaveWorkspaceAs
    if (!savefilename.isEmpty()) {
       leFileName->setText(savefilename);
       s.setValue("creatingWorkspaceFile",savefilename);
    }
}

