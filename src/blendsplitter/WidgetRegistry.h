#pragma once

#include <QLabel>

class RegistryItem;
class SwitchingBar;

/** \brief A registry of all widgets that can be displayed in a SwitchingWidget
 *
 * This singleton-class acts as a registry of widgets that can be displayed in a SwitchingWidget by selecting from a combo box in the SwitchingBar. Each item is represented as one RegistryItem. The Registry also contains a pointer to the default RegistryItem, which is shown when a new SwitchingWidget is created.
 */
class WidgetRegistry : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(WidgetRegistry)
public:
    /** \brief Registry getter
     *
     * This is a singleton class, i. e. you can't construct any object yourself. To get the one-and-only instance of this class, you need to call WidgetRegistry::getRegistry(). The function will create the object if neccessary (= when called for the first time) and return a pointer to it.
     * \return A pointer to the one-and-only instance of WidgetRegistry
     */
    static WidgetRegistry* getRegistry();
    /** \brief Get the item at position i.
     *
     * This function gives you the item at position i (counting starts at 0).
     * \param i Index of the item to be returned
     * \return A pointer to the RegistryItem at position i
     */
    RegistryItem* item(int i) const;
    /** \brief Get the position of an item in WidgetRegistry
     *
     * Get the index (counting starts at 0) of an item. Often used together with item(int i) const.
     * \param item A pointer to the item whose index is to be returned
     * \return Index of the item
     */
    int indexOf(RegistryItem* item) const;
    /** \brief Get the default RegistryItem
     *
     * This function gives you the default RegistryItem. Note that if no item was set as default, the currently first item is set as default by this function. If the registry is empty, a RegistryItem is added to the registry (using the default constructor) and set as default.
     * \return A pointer to the default RegistryItem
     */
    RegistryItem* getDefault();
    /** \brief Set the default RegistryItem
     *
     * This function sets the default RegistryItem. Note that if the item is not in WidgetRegistry, it is added as the last entry. The default item is used when a new SwitchingWidget is created as the displayed widget.
     * \param item A pointer to the RegistryItem to be set as default
     */
    void setDefault(RegistryItem* item);
    /** \brief Set the default RegistryItem
     *
     * This function sets the default RegistryItem to be the RegistryItem at given index. This is equal to calling setDefault(item(index)).
     * \param index Index of the RegistryItem to be set as default (counting starts at 0)
     */
    void setDefault(int index = 0);
    /** \brief Add an item to WidgetRegistry
     *
     * Adds a given RegistryItem at the end of WidgetRegistry.
     * \param item A pointer to the RegistryItem to be added
     */
    void addItem(RegistryItem* item);
    /** \brief Add an item to WidgetRegistry
     *
     * Adds a RegistryItem constructed with the given parameters at the end of WidgetRegistry. This is equal to calling addItem(new RegistryItem{name, widget, populateBar}).
     * \param name The name of the widget, used in the SwitchingBar combo box
     * \param widget A pointer to a function constructing the widget
     * \param populateBar A pointer to a function populating the SwitchingBar
     */
    void addItem(QString name = "Default", QWidget* (*widget) () = []()->QWidget* {return new QLabel{"Default widget"};}, void (*populateBar) (SwitchingBar*, QWidget*) = [](SwitchingBar*, QWidget*)->void {});
    /** \brief Insert an item into WidgetRegistry
     *
     * Inserts a given RegistryItem into WidgetRegistry at a given index.
     * \param index The desired index of the inserted RegistryItem (counting starts at 0)
     * \param item A pointer to the RegistryItem to be added
     */
    void insertItem(int index, RegistryItem* item);
    /** \brief Insert an item into widgetRegistry
     *
     * Inserts a RegistryItem constructed with the given parameters into WidgetRegistry at a given index. This is equal to calling insertItem(index, new RegistryItem{name, widget, populateBar}).
     * \param index The desired index of the inserted RegistryItem (counting starts at 0)
     * \param name The name of the widget, used in the SwitchingBar combo box
     * \param widget A pointer to a function constructing the widget
     * \param populateBar A pointer to a function populating the SwitchingBar
     */
    void insertItem(int index, QString name = "Default", QWidget* (*widget) () = []()->QWidget* {return new QLabel{"Default widget"};}, void (*populateBar) (SwitchingBar*, QWidget*) = [](SwitchingBar*, QWidget*)->void {});
    /** \brief Remove a RegistryItem from WidgetRegistry
     *
     * Removes a given RegistryItem from WidgetRegistry. If this is also the default RegistryItem, the first RegistryItem is set as default if it exists. This is equal to calling removeItem(indexOf(item)).
     * \param item
     */
    void removeItem(RegistryItem* item);
    /** \brief Remove a RegistryItem from WidgetRegistry
     *
     * Removes the RegistryItem at position index (counting starts at 0) from WidgetRegistry. If this is also the default RegistryItem, the first RegistryItem is set as default if it exists.
     * \param index Index of the RegistryItem to be removed
     */
    void removeItem(int index);
    /** \brief Get the size of WidgetRegistry
     *
     * This function returns the number of RegistryItems currently in WidgetRegistry. Note that these are indexed as 0, ..., (size() - 1).
     * \return Number of RegistryItems in WidgetRegistry
     */
    int size() const;
signals:
    /** \brief Signal emited when WidgetRegistry changes its contents
     *
     * This signal is emited when a RegistryItem is added, inserted or removed from WidgetRegistry. It is NOT emited when the default item is changed unless this change requires adding a RegistryItem.
     */
    void registryChanged();
private:
    static WidgetRegistry* theRegistry;
    QList<RegistryItem*> list;
    RegistryItem* defaultItem;
    WidgetRegistry();
};
