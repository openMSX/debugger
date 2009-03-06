#include "PreferencesDialog.h"
#include "Settings.h"
#include <QListWidgetItem>
#include <QFontDialog>
#include <QColorDialog>

PreferencesDialog::PreferencesDialog(QWidget* parent)
	: QDialog(parent)
{
	setupUi(this);

	connect(listFonts, SIGNAL(currentRowChanged(int)),
	        this, SLOT(fontSelectionChange(int)));
	connect(rbUseAppFont,    SIGNAL(toggled(bool)),
	        this, SLOT(fontTypeChanged(bool)));
	connect(rbUseFixedFont,  SIGNAL(toggled(bool)),
	        this, SLOT(fontTypeChanged(bool)));
	connect(rbUseCustomFont, SIGNAL(toggled(bool)),
	        this, SLOT(fontTypeChanged(bool)));
	connect(btnSelectFont, SIGNAL(clicked()),
	        this, SLOT(fontSelectCustom()));
	connect(btnFontColor,  SIGNAL(clicked()),
	        this, SLOT(fontSelectColor()));

	initFontList();
	listFonts->setCurrentRow(0);
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

	switch (Settings::get().fontType( (Settings::DebuggerFont)row)) {
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

	Settings::DebuggerFont f = (Settings::DebuggerFont)(listFonts->currentRow());
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
	Settings::DebuggerFont f = (Settings::DebuggerFont)(listFonts->currentRow());
	QFont newFont = QFontDialog::getFont(&ok, Settings::get().font(f));
	if (ok) {
		lblPreview->setFont(newFont);
		Settings::get().setFont(f, newFont);
	}
}

void PreferencesDialog::fontSelectColor()
{
	Settings::DebuggerFont f = (Settings::DebuggerFont)(listFonts->currentRow());
	QColor newColor = QColorDialog::getColor(Settings::get().fontColor(f), this);
	if (newColor.isValid()) {
		Settings::get().setFontColor(f, newColor);
		setFontPreviewColor(newColor);
	}
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
