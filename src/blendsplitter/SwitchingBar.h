#pragma once

#include <QWidget>
#include <QMenu>
#include <QHBoxLayout>

class SwitchingWidget;
class SwitchingCombo;

/** \brief A menu bar which is always found on the bottom of SwitchingWidget
 *
 * This menu bar is similar to the built-in QMenuBar, but can also contain plain QWidgets. The first item on the left is always a combo box for selecting which widget should be displayed in the SwitchingWidget.
 */
class SwitchingBar : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY(SwitchingBar)
public:
    /** \brief Add a QMenu
     *
     * This function adds a QMenu to the very right of the SwitchingBar. The menu is wrapped in an invisible QMenuBar.
     * \param menu A pointer to the QMenu to be added
     */
    void addMenu(QMenu* menu);
    /** \brief Add a QWidget
     *
     * This function adds a QWidget to the very right of the SwitchingBar. The widget is placed in a QHBoxLayout.
     * @param widget A pointer to the QWidget to be added
     */
    void addWidget(QWidget* widget);
private:
    friend SwitchingWidget;
    QHBoxLayout* layout;
    SwitchingCombo* combo;
    explicit SwitchingBar(QWidget* parent = nullptr);
    void reconstruct(void (*populateBar) (SwitchingBar*, QWidget*), QWidget* widget);
    ~SwitchingBar();
};
