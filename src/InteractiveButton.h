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
    // this one is specific for the VDPRegViewer and depends on the name of the button also
    void newBitValue(int reg, int bit, bool state);

private:
    	//TODO this bool doesn't serve any purpose atm.
	bool isHighlighted;

private slots:
    // this one is specific for the VDPRegViewer
    void newBitValueSlot(bool);

};

#endif // INTERACTIVEBUTTON

