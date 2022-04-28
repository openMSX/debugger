#include "BlendSplitter.h"

#include "WidgetRegistry.h"
#include "RegistryItem.h"
#include "SplitterDecorator.h"
#include "SplitterHandle.h"
#include "WidgetDecorator.h"

#include "SignalDispatcher.h"

#include <QDebug>
#include <QMessageBox>
#include <QJsonObject>
#include <QJsonArray>

int BlendSplitter::expanderSize{12};
int BlendSplitter::switchingBarHeight{32};
QString BlendSplitter::expanderImage{":/BlendSplitter/Expander"};

BlendSplitter::BlendSplitter(QWidget* (*defaultWidget) (), Qt::Orientation orientation) : QSplitter{orientation, nullptr}, defaultWidget{defaultWidget}
{
//    Q_INIT_RESOURCE(BlendSplitterResources);
    setChildrenCollapsible(false);
    setHandleWidth(1);
    setStyleSheet("QSplitter::handle{background: black;}");
}

void BlendSplitter::addWidget()
{
    addWidget((*defaultWidget) ());
}

void BlendSplitter::addWidget(QWidget* widget)
{
    insertWidget(-1, widget);
}

void BlendSplitter::addWidget(RegistryItem* item)
{
    insertWidget(-1, item);
}

void BlendSplitter::insertWidget(int index)
{
    insertWidget(index, (*defaultWidget) ());
}

void BlendSplitter::insertWidget(int index, QWidget* widget)
{
    if (widget->inherits("SwitchingWidget")){
        connect(SignalDispatcher::getDispatcher(), &SignalDispatcher::enableWidget,
                static_cast<SwitchingWidget*>(widget), &SwitchingWidget::setEnableWidget);
        static_cast<SwitchingWidget*>(widget)->setEnableWidget(SignalDispatcher::getDispatcher()->getEnableWidget());
    };
    WidgetDecorator* decorator{new WidgetDecorator{widget}};
    QSplitter::insertWidget(index, decorator);
}

void BlendSplitter::insertWidget(int index, RegistryItem* item)
{
    SwitchingWidget* wdg=new SwitchingWidget{item};
    insertWidget(index, wdg);
}

void BlendSplitter::addDecoratedWidget(WidgetDecorator* widget)
{
    insertDecoratedWidget(-1, widget);
}

void BlendSplitter::insertDecoratedWidget(int index, WidgetDecorator* widget)
{
    QSplitter::insertWidget(index, widget);
}

void BlendSplitter::addSplitter(BlendSplitter *splitter)
{
    insertSplitter(-1, splitter);
}

void BlendSplitter::insertSplitter(int index, BlendSplitter* splitter)
{
    SplitterDecorator* decorator{new SplitterDecorator{splitter}};
    QSplitter::insertWidget(index, decorator);
}

QWidget *BlendSplitter::getNestedWidget(int index)
{
    return widget(index)->layout()->itemAt(0)->widget();
}

QJsonObject BlendSplitter::save2json() const
{
    QJsonObject obj;
    QJsonArray ar_sizes,ar_subwidgets;

    obj["type"]="BlendSplitter";
    obj["orientation"]=orientation();
    obj["size_width"]=size().width();
    obj["size_height"]=size().height();
    foreach(int i,sizes()){
        ar_sizes.append(i);
    };
    obj["sizes"]=ar_sizes;
    for(int i=0 ; i<count() ; i++){
//        qDebug() << " i " << i << " : " << widget(i);
        if (widget(i)->inherits("WidgetDecorator")){
            QWidget* wdg = widget(i)->layout()->itemAt(0)->widget();
            if (wdg->inherits("SwitchingWidget")){
                ar_subwidgets.append(static_cast<SwitchingWidget*>(wdg)->save2json());
            }
        } else if (widget(i)->inherits("SplitterDecorator")){
            QWidget* wdg = widget(i)->layout()->itemAt(0)->widget();
            ar_subwidgets.append(static_cast<BlendSplitter*>(wdg)->save2json());
        } else {
            QJsonObject o;
            ar_subwidgets.append(o);
        };
    };
    obj["subs"]=ar_subwidgets;


    return obj;
}

BlendSplitter *BlendSplitter::createFromJson(const QJsonObject &obj)
{

    BlendSplitter *split= new BlendSplitter([]()->QWidget* {return new SwitchingWidget{};},
                                            obj["orientation"].toInt(1)==1?Qt::Horizontal:Qt::Vertical);

    foreach (const QJsonValue & value, obj["subs"].toArray()) {
        QJsonObject obj = value.toObject();
        if (obj["type"].toString()=="SwitchingWidget"){
            split->addWidget(SwitchingWidget::createFromJson(obj));
        } else if (obj["type"].toString()=="BlendSplitter"){
            split->addSplitter(BlendSplitter::createFromJson(obj));
        } else {
            QMessageBox::warning(nullptr, tr("Open workspaces ..."),
                                 tr("Unknown subs type:%1").arg(obj["type"].toString())
                                 );
        }

    }

    QList<int> sizes;
    split->resize(obj["size_width"].toInt(),obj["size_height"].toInt());
    foreach (const QJsonValue & value, obj["sizes"].toArray()) {
        sizes.append(value.toInt());
    }
    split->setSizes(sizes);
//    qDebug() << "split->size" << split->size();

    return split;
}

QSplitterHandle* BlendSplitter::createHandle()
{
    return new SplitterHandle(orientation(), this);
}
