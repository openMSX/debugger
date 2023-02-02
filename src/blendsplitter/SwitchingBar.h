#ifndef SWITCHINGBAR_H
#define SWITCHINGBAR_H

#include <QWidget>
#include <QMenu>
#include <QHBoxLayout>

#include <functional>

class SwitchingWidget;
class SwitchingCombo;

/** \brief A menu bar which is always found on the bottom of SwitchingWidget
 *
 * This menu bar is similar to the built-in QMenuBar, but can also contain plain QWidgets. The first item on the left is always a combo box for selecting which widget should be displayed in the SwitchingWidget.
 */
class SwitchingBar : public QWidget
{
    friend SwitchingWidget;

    Q_OBJECT

public:
    SwitchingBar(const SwitchingBar&) = delete;
    SwitchingBar& operator=(const SwitchingBar&) = delete;

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
    explicit SwitchingBar(QWidget* parent = nullptr);
    void reconstruct(std::function<void(SwitchingBar*, QWidget*)> populateBar, QWidget* widget);
    ~SwitchingBar();

private:
    QHBoxLayout* layout;
    SwitchingCombo* combo;
};

#endif
