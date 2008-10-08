#ifndef INTERACTIVELABEL
#define INTERACTIVELABEL

#include <QEvent>
#include <QLabel>
class InteractiveLabel : public QLabel
{
	Q_OBJECT

public:
    InteractiveLabel(QWidget *parent = 0);

public slots:
    void highlight(bool state);
    
protected:
	virtual void enterEvent(QEvent *event);
	virtual void leaveEvent(QEvent *event);

signals:
    void mouseOver(bool state);

private:
	bool isHighlighted;
};

#endif // INTERACTIVELABEL

