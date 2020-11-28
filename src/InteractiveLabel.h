#ifndef INTERACTIVELABEL
#define INTERACTIVELABEL

#include <QLabel>

class InteractiveLabel : public QLabel
{
	Q_OBJECT
public:
	InteractiveLabel(QWidget* parent = nullptr);

protected:
	void enterEvent(QEvent* event) override;
	void leaveEvent(QEvent* event) override;

signals:
	void mouseOver(bool state);

public slots:
	void highlight(bool state);
};

#endif // INTERACTIVELABEL
