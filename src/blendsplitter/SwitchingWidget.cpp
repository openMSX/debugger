#include "SwitchingWidget.h"

#include "WidgetRegistry.h"
#include "RegistryItem.h"
#include "SwitchingBar.h"
#include "SwitchingCombo.h"
#include "qscrollarea.h"
#include "SavesJsonInterface.h"

#include <QScrollArea>
#include <QJsonObject>
#include <QDebug>

SwitchingWidget::SwitchingWidget(RegistryItem* item, QWidget* parent, bool menuAtTop)
    : QSplitter(Qt::Vertical, parent)
    , bar{new SwitchingBar{}}
    , widgetEnabled(true), isWidgetAlwaysEnabled(false)
    , barAtTop(menuAtTop)
{
    setChildrenCollapsible(true);
    setHandleWidth(1);
    setStyleSheet("QSplitter::handle{background: grey;}");

    auto* defaultItem = WidgetRegistry::instance().getDefault();
    if (barAtTop) {
        addWidget(bar);
        addWidget(wrapInScrollArea((defaultItem->widget)()));
    } else {
        addWidget(wrapInScrollArea((defaultItem->widget)()));
        addWidget(bar);
    }
    bar->reconstruct(defaultItem->populateBar, widget(widgetIndex()));
    connect(bar->combo, static_cast<void(QComboBox::*)(int)>(&QComboBox::activated), this, &SwitchingWidget::changeCurrentWidget);
    setCurrentWidget(item);
}

void SwitchingWidget::setCurrentWidget(RegistryItem* item)
{
    auto& registry = WidgetRegistry::instance();
    if (item == nullptr) {
        item = registry.getDefault();
    }
    if (registry.indexOf(item) >= 0) {
        delete widget(widgetIndex());
        insertWidget(widgetIndex(), wrapInScrollArea((item->widget)()));
        bar->reconstruct(item->populateBar, widget(widgetIndex()));
        bar->combo->setCurrentIndex(bar->combo->findText(item->name));
//        widget(widgetIndex())->setEnabled(widgetEnabled);
        setEnableWidget(widgetEnabled);
        //qDebug() << widget(widgetIndex());
    }
    //hack to have manual always enabled
    //qDebug() << "WidgetRegistry::getRegistry()->indexOf(item) " << registry->indexOf(item);
    setWidgetAlwaysEnabled(registry.indexOf(item) == 14);
}

bool SwitchingWidget::getEnableWidget()
{
    return widgetEnabled;
}

void SwitchingWidget::setCurrentIndex(int index)
{
    setCurrentWidget(WidgetRegistry::instance().item(index));
}

int SwitchingWidget::getCurrentIndex()
{
    return bar->combo->currentIndex();
}

QJsonObject SwitchingWidget::save2json()
{
    QJsonObject obj;
    obj["type"] = "SwitchingWidget";
    obj["item"] = bar->combo->currentIndex();
    obj["size_width"] = size().width();
    obj["size_height"] = size().height();
    QJsonObject childobj = getWidgetSettings();
    if (!childobj.isEmpty()) {
        obj["childwidget"] = childobj;
    }
    return obj;
}

SwitchingWidget* SwitchingWidget::createFromJson(const QJsonObject &obj)
{
    int i = obj["item"].toInt();
    auto* wdgt = new SwitchingWidget{WidgetRegistry::instance().item(i)};
    wdgt->resize(obj["size_width"].toInt(), obj["size_height"].toInt());

    if (auto* childWdgt = dynamic_cast<SavesJsonInterface*>(wdgt->getWidget())) {
        if (auto cw = obj["childwidget"]; cw != QJsonValue::Undefined) {
            childWdgt->loadFromJson(cw.toObject());
        }
    }

    return wdgt;
}

QWidget *SwitchingWidget::getWidget()
{
    auto* wdgt = widget(widgetIndex());

    if (isWrappedInScrollArea) {
        auto* sa = static_cast<QScrollArea*>(wdgt);
        wdgt = sa->widget();
    }
    return wdgt;
}

QJsonObject SwitchingWidget::getWidgetSettings()
{
    QJsonObject obj;
    if (auto* wd = dynamic_cast<SavesJsonInterface*>(getWidget())) {
        obj = wd->save2json();
    }
    return obj;
}

bool SwitchingWidget::setWidgetSettings(const QJsonObject &obj)
{
    if (auto* wd = dynamic_cast<SavesJsonInterface*>(getWidget())) {
        return wd->loadFromJson(obj);
    }
    return false;
}

void SwitchingWidget::setEnableWidget(bool enable)
{
    widgetEnabled = enable;

    auto* wdgt = widget(widgetIndex());

    if (isWrappedInScrollArea) {
        auto* sa = static_cast<QScrollArea*>(wdgt);
        sa->setAutoFillBackground(true);
        //sa->setBackgroundRole(QPalette::Text); //trying to force repaint of background
        //sa->update();
        //sa->setBackgroundRole(enable ? QPalette::Window : QPalette::Dark);
        if (auto* vp = sa->viewport()) {
            vp->update();
        }
        wdgt = sa->widget();
    }

    if (wdgt != nullptr) {
        bool finalstatus = enable || isWidgetAlwaysEnabled;
        //qDebug() << "wdgt->setEnabled(" << enable << "||" << isWidgetAlwaysEnabled << "= " << finalstatus << " )  " << wdgt->objectName();
        wdgt->setEnabled(finalstatus);
        wdgt->update();
    }
}

void SwitchingWidget::setWidgetAlwaysEnabled(bool enable)
{
    isWidgetAlwaysEnabled = enable;
    //qDebug() << "SwitchingWidget::setWidgetAlwaysEnabled(bool " << enable << ")";
    setEnableWidget(widgetEnabled);
}

void SwitchingWidget::changeCurrentWidget(int index)
{
    if (index >= 0) {
        setCurrentWidget(WidgetRegistry::instance().item(index));
    }
}

QWidget* SwitchingWidget::wrapInScrollArea(QWidget* wdgt, bool dowrap)
{
    isWrappedInScrollArea = dowrap;
    if (!dowrap) {
        return wdgt;
    }

    auto* scrollArea = new QScrollArea;
    //scrollArea->setBackgroundRole(QPalette::Dark);
    scrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    scrollArea->setContentsMargins(0, 0, 0, 0);
    scrollArea->setWidget(wdgt);
    scrollArea->setWidgetResizable(true);
    return scrollArea;
}

int SwitchingWidget::barIndex()
{
    return barAtTop ? 0 : 1;
}

int SwitchingWidget::widgetIndex()
{
    return barAtTop ? 1 : 0;
}
