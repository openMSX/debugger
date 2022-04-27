#pragma once

#include <QSplitter>

#include "RegistryItem.h"
#include "SwitchingBar.h"
#include "SwitchingWidget.h"
#include "WidgetRegistry.h"

class Expander;
class ExpanderCorner;
class ExpanderBottom;
class ExpanderTop;
class WidgetDecorator;
class QJsonObject;

/** \brief A user-defined Splitter
 *
 * This widget implements the functionality of Blender (Open-source 3D modelling software) widget management. This widget displays a splitter similar to QSplitter. However, each widget in BlendSplitter has a pair of Expanders (one in top right and one in bottom left corner). By dragging from these Expanders inwards a new widget is created in the direction of the drag. If the direction is different to that of the BlendSplitter, a new BlendSplitter with parallel direction is created in place of the widget with the widget and the new widget in it. By dragging from these expanders outwards, a neighbouring widget (or a collection of widgets) can be closed. While the mouse is held, the widgets to be closed are marked with black overlay. When the mouse is released, they are closed. BlendSplitter can be used like any other QWidget, although setting one as the central widget is recommended. A BlendSplitter can contain any QWidget, but to achieve best results, use it together with SwitchingWidget.
 *
 * BlendSplitter provides 3 static variables that allow some customization of the library design. These are expanderSize, switchingBarHeight and expanderImage. These are all initialized with default values. The default Expander image is provided by the library.
 *
 * The default Expander image:
 *
 * ![The default Expander](resources/expander.png)
 */
class BlendSplitter : public QSplitter
{
    Q_OBJECT
    Q_DISABLE_COPY(BlendSplitter)
public:
    static int expanderSize; /**< Size of the expanders in the corners. Default value: 12 */
    static int switchingBarHeight; /**< Height of the SwitchingBar. Default value: 36 */
    static QString expanderImage; /**< The image to be used for the top left expander. The bottom right one will rotate this by pi (180 degrees). Default value: \":/BlendSplitter/Expander\" */
    /** \brief BlendSplitter class constructor
     * \param defaultWidget A pointer to function constructing the default widget. This function is called when a new widget is added to BlendSplitter.
     * \param orientation Orientation of the main BlendSplitter
     */
    BlendSplitter(QWidget* (*defaultWidget) () = []()->QWidget* {return new SwitchingWidget{};}, Qt::Orientation orientation = Qt::Horizontal);
    /** \brief Add a widget to the BlendSplitter
     *
     * Adds the default widget to the very bottom/right of the BlendSplitter.
     */
    void addWidget();
    /** \brief Add a widget to the BlendSplitter
     *
     * Adds the specified widget to the very bottom/right of the BlendSplitter
     * \param widget A pointer to the widget to be added
     */
    void addWidget(QWidget* widget);
    /** \brief Add a widget to the BlendSplitter
     *
     * Adds the specified widget from the WidgetRegistry to the very bottom/right of the BlendSplitter
     * \param item A RegistryItem to be added (inside a SwitchingWidget).
     */
    void addWidget(RegistryItem* item);
    /** \brief Insert a widget into the BlendSplitter
     *
     * Inserts the default widget into the BlendSplitter at the given position counting from top/left (counting starts at 0). This function should NOT be called with a BlendSplitter as a parameter.
     * \param index The desired position
     */
    void insertWidget(int index);
    /** \brief Insert a widget into the BlendSplitter
     *
     * Inserts the specified widget into the BlendSplitter at the given position counting from top/left (counting starts at 0). This function should NOT be called with a BlendSplitter as a parameter.
     * \param index The desired position
     * \param widget A pointer to the widget to be inserted
     */
    void insertWidget(int index, QWidget* widget);
    /** \brief Insert a widget into the BlendSplitter
     *
     * Inserts the specified widget from WidgetRegistry into the BlendSplitter at the given position counting from top/left (counting starts at 0). This function should NOT be called with a BlendSplitter as a parameter.
     * \param index The desired position
     * \param item A RegistryItem to be added (inside a SwitchingWidget).
     */
    void insertWidget(int index, RegistryItem* item);
    /** \brief Add another BlendSplitter to this BlendSplitter
     *
     * Adds a BlendSplitter (usually with parallel orientation) to the BlendSplitter
     * \param splitter A pointer to the BlendSplitter to be added
     */
    void addSplitter(BlendSplitter* splitter);
    /** \brief Insert another BlendSplitter into this BlendSplitter
     *
     * Inserts a BlendSplitter (usually with parallel orientation) into the BlendSplitter at the given position counting from top/left (counting starts at 0).
     * \param index The desired position
     * \param splitter A pointer to the BlendSplitter to be inserted
     */
    void insertSplitter(int index, BlendSplitter* splitter);
    /**
     * @brief getNestedWidget A helper function to get the widget that is in the splitter at the given position. The actual widget is a WidgetDecorator or SplitterDecorator so we return the widget that is a direct child of this decorator.
     * @param index The desired position
     * @return A pointer to widget that is decorated
     */
    QWidget* getNestedWidget(int index);

    QJsonObject save2json() const;
    static BlendSplitter* createFromJson(const QJsonObject& obj);
private:
    friend Expander;
    friend ExpanderCorner;
    QWidget* (*defaultWidget) ();
    virtual QSplitterHandle* createHandle() override;
    void addDecoratedWidget(WidgetDecorator* widget);
public:
    void insertDecoratedWidget(int index, WidgetDecorator* widget);

};
