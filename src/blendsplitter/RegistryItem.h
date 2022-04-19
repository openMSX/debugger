#pragma once

#include <QLabel>

class SwitchingBar;

/** \brief An item intended to be put into WidgetRegistry
 *
 * Each RegistryItem corresponds to one widget that can be displayed in a BlendSplitter. It describes how this widget should be constructed, what is its name and what items should be in the SwitchingBar when this widget is selected.
 */
class RegistryItem
{
public:
    QString name; /**< The name of the widget, used in the SwitchingBar combo box. */
    /** \brief A function constructing the widget
     *
     * A pointer to a function returning QWidget*. This function is called to construct the widget each time it is selected in any SwitchingWidget. Usually in this function the widget is dynamically created using `new` operator and the pointer is returned.
     * \return A pointer to the newly-created QWidget
     */
    QWidget* (*widget) ();
    /** \brief A function populating the SwitchingBar
     *
     * A pointer to a function populating the SwitchingBar. This function is called each time this widget is selected in any SwitchingWidget. Usually this function makes use of the interface provided by SwitchingBar to populate it.
     * \param A pointer to the SwitchingBar to be populated
     * \param A pointer to the newly-created widget in the SwitchingWidget
     */
    void (*populateBar) (SwitchingBar*, QWidget*);
    /** \brief A constructor setting all the internal values
     *
     * This constructor takes 3 parameters corresponding to the 3 members of the RegistryItem class. See their desription for more details.
     * \param name The name of the widget, used in the SwitchingBar combo box
     * \param widget A pointer to a function constructing the widget
     * \param populateBar A pointer to a function populating the SwitchingBar
     */
    RegistryItem(QString name = "Default", QWidget* (*widget) () = []()->QWidget* {return new QLabel{"Default Widget"};}, void (*populateBar) (SwitchingBar*, QWidget*) = [](SwitchingBar*, QWidget*)->void {});
};
