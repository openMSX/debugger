#ifndef INTERACTIVEBUTTON
#define INTERACTIVEBUTTON

#include <QEvent>
#include <QPushButton>
class InteractiveButton : public QPushButton
{
	Q_OBJECT

public:
    InteractiveButton(QWidget *parent = 0);

public slots:
    void highlight(bool state);
    
protected:
	virtual void enterEvent(QEvent *event);
	virtual void leaveEvent(QEvent *event);

signals:
    void mouseOver(bool state);

private:
    	//TODO this bool doesn't serve any purpose atm.
	bool isHighlighted;
};

#endif // INTERACTIVEBUTTON

