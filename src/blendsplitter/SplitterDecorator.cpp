#include "SplitterDecorator.h"

#include "BlendSplitter.h"

SplitterDecorator::SplitterDecorator(BlendSplitter* splitter) : QWidget{}, splitter{splitter}
{
    QHBoxLayout* layout{new QHBoxLayout{}};
    layout->addWidget(splitter);
    layout->setMargin(0);
    setLayout(layout);
}

SplitterDecorator::~SplitterDecorator()
{
    delete layout();
}
