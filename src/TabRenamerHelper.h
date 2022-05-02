#ifndef TABRENAMERHELPER_H
#define TABRENAMERHELPER_H

#include <QObject>

class QLineEdit;
class QTabWidget;

class TabRenamerHelper : public QObject
{
    Q_OBJECT
public:
    explicit TabRenamerHelper(QTabWidget *parent = nullptr);

public slots:
    void tabBarDoubleClicked(int index);
    void tabNameEditingFinished();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

    QTabWidget* workspaces;
    QLineEdit* tabLineEdit;
    int editedTab;

};

#endif // TABRENAMERHELPER_H
