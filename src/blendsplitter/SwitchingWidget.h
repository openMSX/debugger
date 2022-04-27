#pragma once

#include <QSplitter>

class SwitchingBar;
class RegistryItem;
class QJsonObject;

/** \brief A widget whose actual content can be selected from a combo box
 *
 * This widget displays a Widget with a SwitchingBar on the bottom. The widget displayed is one from WidgetRegistry and it can be selected using a combo box in the SwitchingBar.
 *
 * Note that constructing an object of this class when WidgetRegistry is empty will cause a default RegistryItem to be added to it. The height of the SwitchingBar can be modified by changing BlendSplitter::switchingBarHeight.
 */
class SwitchingWidget : public QSplitter
{
    Q_OBJECT
    Q_DISABLE_COPY(SwitchingWidget)
public:
    /** \brief A default constructor similar to that of QWidget
     *
     * Creates a SwitchingWidget containg the default widget specified in WidgetRegistry
     * \param item A RegistryItem to display in the widget. If nullptr, then WidgetRegistry::getDefault() is used.
     * \param parent A parent widget
     */
    SwitchingWidget(RegistryItem* item = nullptr, QWidget* parent = nullptr,bool menuAtTop=true);
    /** \brief Set the current Widget displayed.
     *
     * Sets the current widget to be the item
     * \param item A RegistryItem to display in the widget. If nullptr, then WidgetRegistry::getDefault() is used.
     */
    void setCurrentWidget(RegistryItem* item = nullptr);
    bool getEnableWidget();
    void setCurrentIndex(int index);
    int getCurrentIndex();

    QJsonObject save2json();
    static SwitchingWidget* createFromJson(const QJsonObject& obj);


public slots:
    void setEnableWidget(bool enable = true);
private slots:
    void changeCurrentWidget(int index);
private:
    QWidget* wrapInScrollArea(QWidget* wdgt, bool dowrap=true);
    int barIndex();
    int widgetIndex();
    SwitchingBar* bar;
    bool widgetEnabled;
    bool barAtTop;
    bool isWrappedInScrollArea;
};
