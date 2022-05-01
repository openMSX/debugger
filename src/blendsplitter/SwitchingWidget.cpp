#include "SwitchingWidget.h"

#include "WidgetRegistry.h"
#include "RegistryItem.h"
#include "SwitchingBar.h"
#include "SwitchingCombo.h"
#include <QScrollArea>
#include <QJsonObject>
#include <QDebug>

SwitchingWidget::SwitchingWidget(RegistryItem* item, QWidget* parent,bool menuAtTop) : QSplitter(Qt::Vertical, parent),
    bar{new SwitchingBar{}}, widgetEnabled(true),isWidgetAlwaysEnabled(false),barAtTop(menuAtTop)
{
    setChildrenCollapsible(true);
    setHandleWidth(1);
    setStyleSheet("QSplitter::handle{background: grey;}");
    if (barAtTop){
        addWidget(bar);
        addWidget(wrapInScrollArea( (*WidgetRegistry::getRegistry()->getDefault()->widget) () ) );
    } else {
        addWidget(wrapInScrollArea( (*WidgetRegistry::getRegistry()->getDefault()->widget) () ) );
        addWidget(bar);

    }
    bar->reconstruct(*WidgetRegistry::getRegistry()->getDefault()->populateBar, widget(widgetIndex()));
    connect(bar->combo, static_cast<void(QComboBox::*)(int)>(&QComboBox::activated), this, &SwitchingWidget::changeCurrentWidget);
    setCurrentWidget(item);
}

void SwitchingWidget::setCurrentWidget(RegistryItem *item)
{
    if(item == nullptr)
    {
        item = WidgetRegistry::getRegistry()->getDefault();
    }
    if(WidgetRegistry::getRegistry()->indexOf(item) >= 0)
    {
        delete widget(widgetIndex());
        insertWidget(widgetIndex(),wrapInScrollArea(  (*item->widget) ()));
        bar->reconstruct(*item->populateBar, widget(widgetIndex()));
        bar->combo->setCurrentIndex(bar->combo->findText(item->name));
        widget(widgetIndex())->setEnabled(widgetEnabled);
    }
    //hack to have manual always enabled
    //qDebug() << "WidgetRegistry::getRegistry()->indexOf(item) " << WidgetRegistry::getRegistry()->indexOf(item);
    setWidgetAlwaysEnabled(WidgetRegistry::getRegistry()->indexOf(item)==14);
}

bool SwitchingWidget::getEnableWidget()
{
    return widgetEnabled;
}

void SwitchingWidget::setCurrentIndex(int index)
{
    setCurrentWidget(WidgetRegistry::getRegistry()->item(index));
}

int SwitchingWidget::getCurrentIndex()
{
    return bar->combo->currentIndex();
}

QJsonObject SwitchingWidget::save2json()
{
    QJsonObject obj;
    obj["type"]="SwitchingWidget";
    obj["item"]=bar->combo->currentIndex();
    obj["size_width"]=size().width();
    obj["size_height"]=size().height();
    return obj;
}

SwitchingWidget *SwitchingWidget::createFromJson(const QJsonObject &obj)
{
    int i=obj["item"].toInt();
    SwitchingWidget* wdgt= new SwitchingWidget{WidgetRegistry::getRegistry()->item(i)};
    wdgt->resize(obj["size_width"].toInt(),obj["size_height"].toInt());

    return wdgt;
}

void SwitchingWidget::setEnableWidget(bool enable)
{
    widgetEnabled=enable;

    QWidget* wdgt=widget(widgetIndex());

    if (isWrappedInScrollArea){
        static_cast<QScrollArea*>(wdgt)->setAutoFillBackground(true);
//        static_cast<QScrollArea*>(wdgt)->setBackgroundRole(QPalette::Text); //trying to force repaint of background
//        static_cast<QScrollArea*>(wdgt)->update();
        /*static_cast<QScrollArea*>(wdgt)->setBackgroundRole(enable ? QPalette::Window : QPalette::Dark);*/
        if (static_cast<QScrollArea*>(wdgt)->viewport()){
            static_cast<QScrollArea*>(wdgt)->viewport()->update();
        };
        wdgt=static_cast<QScrollArea*>(wdgt)->widget();
    };

    if (wdgt != nullptr){
        bool finalstatus=enable||isWidgetAlwaysEnabled;
//        qDebug() << "wdgt->setEnabled(" << enable << "||" << isWidgetAlwaysEnabled << "= "<< finalstatus<< " )  ";

        wdgt->setEnabled(finalstatus);
        wdgt->update();
    }

}

void SwitchingWidget::setWidgetAlwaysEnabled(bool enable)
{
    isWidgetAlwaysEnabled=enable;
//    qDebug() << "SwitchingWidget::setWidgetAlwaysEnabled(bool "<<enable<<")";
    setEnableWidget(widgetEnabled);
}

void SwitchingWidget::changeCurrentWidget(int index)
{
    if(index >= 0)
    {
        setCurrentWidget(WidgetRegistry::getRegistry()->item(index));
    }
}

QWidget *SwitchingWidget::wrapInScrollArea(QWidget *wdgt, bool dowrap)
{
    isWrappedInScrollArea=dowrap;
    if (!dowrap){
        return wdgt;
    };

    QScrollArea* scrollArea = new QScrollArea;
    //scrollArea->setBackgroundRole(QPalette::Dark);
    scrollArea->setSizePolicy(QSizePolicy::Expanding , QSizePolicy::Expanding);
    scrollArea->setContentsMargins(0,0,0,0);
    scrollArea->setWidget(wdgt);
    scrollArea->setWidgetResizable(true);
    return scrollArea;
}

int SwitchingWidget::barIndex()
{
    return barAtTop?0:1;
}

int SwitchingWidget::widgetIndex()
{
    return barAtTop?1:0;
}
