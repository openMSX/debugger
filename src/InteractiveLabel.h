#ifndef INTERACTIVELABEL
#define INTERACTIVELABEL

#include <QLabel>

class InteractiveLabel : public QLabel
{
	Q_OBJECT
public:
	InteractiveLabel(QWidget* parent = nullptr);

	void highlight(bool state);

signals:
	void mouseOver(bool state);

protected:
	void enterEvent(QEvent* event) override;
	void leaveEvent(QEvent* event) override;
};

#endif // INTERACTIVELABEL
