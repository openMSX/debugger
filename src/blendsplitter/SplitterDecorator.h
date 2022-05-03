#ifndef SPLITTERDECORATOR_H
#define SPLITTERDECORATOR_H

#include "Global.h"

class BlendSplitter;

class SplitterDecorator : public QWidget
{
    Q_OBJECT

public:
    SplitterDecorator(const SplitterDecorator&) = delete;
    SplitterDecorator& operator=(const SplitterDecorator&) = delete;
    explicit SplitterDecorator(BlendSplitter* splitter);
    ~SplitterDecorator();

private:
    BlendSplitter* splitter;
};

#endif
