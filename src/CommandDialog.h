#ifndef CONTROLDIALOG_OPENMSX_H
#define CONTROLDIALOG_OPENMSX_H

#include "ui_CommandDialog.h"
#include <QDialog>


struct CommandRef
{
    QString name;
    QString description;
    QString source;
    QString icon;
    int index;
};


class CommandDialog : public QDialog, private Ui::CommandDialog
{
    Q_OBJECT
public:
    CommandDialog(QList<CommandRef>& commands, QWidget* parent = nullptr);
    void refresh();

private:
    int createItem();
    void createCommand();
    void writeCommand(const CommandRef& command);

    // void onSelectedCommandChanged(int currentRow, int currentCol, int previousRow, int previousCol);
    void onAddButtonClicked();
    void onRemoveButtonClicked();
    void accept() override;
    void reject() override;

    int counter = 0;
    bool userMode = true;
    QList<CommandRef>& commands;
};

#endif // CONTROLDIALOG_OPENMSX_H
