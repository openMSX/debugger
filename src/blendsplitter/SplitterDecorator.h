#pragma once

#include "Global.h"

class BlendSplitter;

class SplitterDecorator : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY(SplitterDecorator)
public:
    SplitterDecorator() = delete;
    explicit SplitterDecorator(BlendSplitter* splitter);
    ~SplitterDecorator();
private:
    BlendSplitter* splitter;
};
