#include "ScopedAssign.h"
#include "CommandDialog.h"
#include <QStandardItemModel>


enum CommandColumns {
    ICON = 0, NAME, DESCRIPTION, SOURCE, MAX
};


CommandDialog::CommandDialog(QList<CommandRef>& commands, QWidget* parent)
    : QDialog(parent), commands(commands)
{
    setupUi(this);

    connect(btnAdd, &QPushButton::clicked, this, &CommandDialog::onAddButtonClicked);
    connect(btnRemove, &QPushButton::clicked, this, &CommandDialog::onRemoveButtonClicked);

    connect(okButton, &QPushButton::clicked, this, &CommandDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &CommandDialog::reject);

    refresh();
}

void CommandDialog::refresh()
{
    for (const auto& command : commands) {
        createItem();
        writeCommand(command);
    }
    twCommands->resizeColumnsToContents();
}

void CommandDialog::onAddButtonClicked()
{
    auto sa = ScopedAssign(userMode, false);
    createCommand();
    twCommands->resizeColumnsToContents();
}

void CommandDialog::onRemoveButtonClicked()
{
    if (twCommands->currentRow() == -1) return;

    auto sa = ScopedAssign(userMode, false);
    twCommands->removeRow(twCommands->currentRow());
}

int CommandDialog::createItem()
{
    int row = twCommands->rowCount();
    twCommands->setRowCount(row + 1);
    twCommands->selectRow(row);

    for (auto column : {ICON, NAME, DESCRIPTION, SOURCE}) {
        auto* item = new QTableWidgetItem();
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
        twCommands->setItem(row, column, item);
    }

    return row;
}

void CommandDialog::createCommand()
{
    // get next available command name
    QString cmdName = {};
    bool found = false;
    do {
        cmdName = QString(tr("Command_%1")).arg(counter++);
        found = false;
        for (auto& command : commands) {
            if (command.name == cmdName) {
                found = true;
                break;
            }
        }
    } while (found);

    int index = createItem();
    writeCommand({cmdName, {}, {}, ":/icons/gear.png", index});
}

void CommandDialog::writeCommand(const CommandRef& command)
{
    auto* item0 = twCommands->item(command.index, ICON);
    item0->setText(command.icon);

    auto* item1 = twCommands->item(command.index, NAME);
    item1->setText(command.name);

    auto* item2 = twCommands->item(command.index, DESCRIPTION);
    item2->setText(command.description);

    auto* item3 = twCommands->item(command.index, SOURCE);
    item3->setText(command.source);
}

void CommandDialog::accept()
{
    commands.clear();
    for (int row = 0; row < twCommands->rowCount(); ++row) {
        auto icon = twCommands->item(row, ICON)->text();
        auto name = twCommands->item(row, NAME)->text();
        auto description = twCommands->item(row, DESCRIPTION)->text();
        auto source = twCommands->item(row, SOURCE)->text();
        const CommandRef command{name, description, source, icon, row};
        commands.append(command);
    }
    done(QDialog::Accepted);
}

void CommandDialog::reject()
{
    done(QDialog::Rejected);
}
