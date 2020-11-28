#ifndef INTERACTIVEBUTTON
#define INTERACTIVEBUTTON

#include <QPushButton>

class InteractiveButton : public QPushButton
{
	Q_OBJECT
public:
	InteractiveButton(QWidget* parent = nullptr);

protected:
	void enterEvent(QEvent* event) override;
	void leaveEvent(QEvent* event) override;

signals:
	void mouseOver(bool state);
	// this one is specific for the VDPRegViewer and depends on the
	// name of the button also
	void newBitValue(int reg, int bit, bool state);

public slots:
	void highlight(bool state);
	void mustBeSet(bool state);

private slots:
	// this one is specific for the VDPRegViewer
	void newBitValueSlot(bool);

private:
	void setColor();
	bool _mustBeSet;
	bool _highlight;
	bool _state;
};

#endif // INTERACTIVEBUTTON
