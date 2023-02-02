#ifndef REGISTRYITEM_H
#define REGISTRYITEM_H

#include <QLabel>

#include <functional>

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
    std::function<QWidget*()> widget;

    /** \brief A function populating the SwitchingBar
     *
     * A pointer to a function populating the SwitchingBar. This function is called each time this widget is selected in any SwitchingWidget. Usually this function makes use of the interface provided by SwitchingBar to populate it.
     * \param A pointer to the SwitchingBar to be populated
     * \param A pointer to the newly-created widget in the SwitchingWidget
     */
    std::function<void(SwitchingBar*, QWidget*)> populateBar;

    /** \brief A constructor setting all the internal values
     *
     * This constructor takes 3 parameters corresponding to the 3 members of the RegistryItem class. See their description for more details.
     * \param name The name of the widget, used in the SwitchingBar combo box
     * \param widget A pointer to a function constructing the widget
     * \param populateBar A pointer to a function populating the SwitchingBar
     */
    RegistryItem(const QString& name,
                 std::function<QWidget*()> widget,
                 std::function<void(SwitchingBar*, QWidget*)> populateBar = {});
};

#endif
