#include "PreferencesDialog.h"
#include "Settings.h"
#include <QListWidgetItem>
#include <QFontDialog>
#include <QColorDialog>

PreferencesDialog::PreferencesDialog(QWidget* parent)
	: QDialog(parent)
{
	setupUi(this);

	connect(listFonts, &QListWidget::currentRowChanged,
	        this, &PreferencesDialog::fontSelectionChange);
	connect(rbUseAppFont,    &QRadioButton::toggled,
	        this, &PreferencesDialog::fontTypeChanged);
	connect(rbUseFixedFont,  &QRadioButton::toggled,
	        this, &PreferencesDialog::fontTypeChanged);
	connect(rbUseCustomFont, &QRadioButton::toggled,
	        this, &PreferencesDialog::fontTypeChanged);
	connect(btnSelectFont, &QPushButton::clicked,
	        this, &PreferencesDialog::fontSelectCustom);
	connect(btnFontColor,  &QPushButton::clicked,
	        this, &PreferencesDialog::fontSelectColor);

	initConfig();
	initFontList();
	listFonts->setCurrentRow(0);
}

/*
 * Config settings
 */
void PreferencesDialog::initConfig()
{
	Settings& s = Settings::get();
	int status1 = s.autoReloadSymbols() ? Qt::Checked : Qt::Unchecked;
	cbAutoReloadSymbols->setChecked(status1);
	connect(cbAutoReloadSymbols, &QCheckBox::stateChanged,
			this, &PreferencesDialog::autoReloadSymbols);

	int status2 = s.preserveLostSymbols() ? Qt::Checked : Qt::Unchecked;
	cbPreserveLostSymbols->setChecked(status2);
	connect(cbPreserveLostSymbols, &QCheckBox::stateChanged,
			this, &PreferencesDialog::preserveLostSymbols);

	int status3 = s.preserveBreakpointSymbol() ? Qt::Checked : Qt::Unchecked;
	cbPreserveBreakpointSymbol->setChecked(status3);
	connect(cbPreserveBreakpointSymbol, &QCheckBox::stateChanged,
			this, &PreferencesDialog::preserveBreakpointSymbol);
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

void PreferencesDialog::autoReloadSymbols(int state)
{
	Settings::get().setAutoReloadSymbols(state == Qt::Checked);
}

void PreferencesDialog::preserveLostSymbols(int state)
{
	Settings::get().setPreserveLostSymbols(state == Qt::Checked);
}

void PreferencesDialog::preserveBreakpointSymbol(int state)
{
	Settings::get().setPreserveBreakpointSymbol(state == Qt::Checked);
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