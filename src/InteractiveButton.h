#ifndef INTERACTIVEBUTTON
#define INTERACTIVEBUTTON

#include <QPushButton>

class InteractiveButton : public QPushButton
{
	Q_OBJECT
public:
	InteractiveButton(QWidget* parent = nullptr);

	void highlight(bool state);
	void mustBeSet(bool state);

signals:
	void mouseOver(bool state);
	// this one is specific for the VDPRegViewer and depends on the
	// name of the button also
	void newBitValue(int reg, int bit, bool state);

protected:
	void enterEvent(QEnterEvent* event) override;
	void leaveEvent(QEvent* event) override;

private:
	// this one is specific for the VDPRegViewer
	void newBitValueSlot(bool);

	void setColor();

private:
	bool _mustBeSet;
	bool _highlight;
	bool _state;
};

#endif // INTERACTIVEBUTTON
