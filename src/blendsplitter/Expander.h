#ifndef EXPANDER_H
#define EXPANDER_H

#include "Global.h"

class Overlay;
class WidgetDecorator;

class Expander : public QLabel
{
    Q_OBJECT

public:
    Expander(const Expander&) = delete;
    Expander& operator=(const Expander&) = delete;

protected:
    explicit Expander(WidgetDecorator* parent);
    virtual void reposition(); //= 0;
    ~Expander();

protected slots:
    //void enterEvent(QEvent* event) final;
    //void leaveEvent(QEvent* event) final;
    //void mousePressEvent(QMouseEvent* event) override;
    //void mouseMoveEvent(QMouseEvent* event) override = 0;
    //void mouseReleaseEvent(QMouseEvent* event) override;

protected:
    QPixmap* pixmap;
    Overlay* overlay;
};

#endif
