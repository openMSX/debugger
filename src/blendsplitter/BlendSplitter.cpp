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

BlendSplitter::BlendSplitter(Qt::Orientation orientation, std::function<QWidget*()> defaultWidget)
    : QSplitter{orientation, nullptr}
    , defaultWidget{defaultWidget}
{
    //Q_INIT_RESOURCE(BlendSplitterResources);
    setChildrenCollapsible(false);
    setHandleWidth(1);
    setStyleSheet("QSplitter::handle{background: black;}");
}

void BlendSplitter::addWidget()
{
    addWidget(defaultWidget());
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
    insertWidget(index, defaultWidget());
}

void BlendSplitter::insertWidget(int index, QWidget* widget)
{
    if (widget->inherits("SwitchingWidget")) {
        auto* sw = static_cast<SwitchingWidget*>(widget);
        connect(SignalDispatcher::getDispatcher(), &SignalDispatcher::enableWidget,
                sw, &SwitchingWidget::setEnableWidget);
        sw->setEnableWidget(SignalDispatcher::getDispatcher()->getEnableWidget());
    }
    auto* decorator = new WidgetDecorator{widget};
    QSplitter::insertWidget(index, decorator);
}

void BlendSplitter::insertWidget(int index, RegistryItem* item)
{
    auto* wdg = new SwitchingWidget{item};
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

void BlendSplitter::addSplitter(BlendSplitter* splitter)
{
    insertSplitter(-1, splitter);
}

void BlendSplitter::insertSplitter(int index, BlendSplitter* splitter)
{
    auto* decorator = new SplitterDecorator{splitter};
    QSplitter::insertWidget(index, decorator);
}

QWidget* BlendSplitter::getNestedWidget(int index)
{
    return widget(index)->layout()->itemAt(0)->widget();
}

QJsonObject BlendSplitter::save2json() const
{
    QJsonObject obj;
    obj["type"] = "BlendSplitter";
    obj["orientation"] = orientation();
    obj["size_width"] = size().width();
    obj["size_height"] = size().height();

    QJsonArray ar_sizes;
    for (auto i : sizes()) {
        ar_sizes.append(i);
    }
    obj["sizes"] = ar_sizes;

    QJsonArray ar_subwidgets;
    for (int i = 0; i < count(); ++i) {
        //qDebug() << " i " << i << " : " << widget(i);
        if (widget(i)->inherits("WidgetDecorator")) {
            auto* wdg = widget(i)->layout()->itemAt(0)->widget();
            if (wdg->inherits("SwitchingWidget")) {
                ar_subwidgets.append(static_cast<SwitchingWidget*>(wdg)->save2json());
            }
        } else if (widget(i)->inherits("SplitterDecorator")) {
            auto* wdg = widget(i)->layout()->itemAt(0)->widget();
            ar_subwidgets.append(static_cast<BlendSplitter*>(wdg)->save2json());
        } else {
            QJsonObject o;
            ar_subwidgets.append(o);
        }
    }
    obj["subs"] = ar_subwidgets;

    return obj;
}

BlendSplitter* BlendSplitter::createFromJson(const QJsonObject &obj)
{
    auto* split = new BlendSplitter(obj["orientation"].toInt(1) == 1 ? Qt::Horizontal : Qt::Vertical);

    for (const QJsonValue& value : obj["subs"].toArray()) {
        QJsonObject obj = value.toObject();
        if (obj["type"].toString() == "SwitchingWidget") {
            split->addWidget(SwitchingWidget::createFromJson(obj));
        } else if (obj["type"].toString() == "BlendSplitter") {
            split->addSplitter(BlendSplitter::createFromJson(obj));
        } else {
            QMessageBox::warning(nullptr, tr("Open workspaces ..."),
                                 tr("Unknown subs type:%1").arg(obj["type"].toString()));
        }
    }

    QList<int> sizes;
    split->resize(obj["size_width"].toInt(), obj["size_height"].toInt());
    for (const auto& value : obj["sizes"].toArray()) {
        sizes.append(value.toInt());
    }
    split->setSizes(sizes);
    //qDebug() << "split->size" << split->size();
    return split;
}

QSplitterHandle* BlendSplitter::createHandle()
{
    return new SplitterHandle(orientation(), this);
}
