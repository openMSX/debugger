#ifndef FLAGSVIEWER_H
#define FLAGSVIEWER_H

#include <QFrame>

class QPainter;
class QResizeEvent;
class QPaintEvent;
class QString;

class FlagsViewer : public QFrame
{
	Q_OBJECT;
public:
	FlagsViewer(QWidget* parent = nullptr);

	QSize sizeHint() const override;

public slots:
	void setFlags(quint8 newFlags);

private:
	void resizeEvent(QResizeEvent* e) override;
	void paintEvent(QPaintEvent* e) override;

	void drawValue(QPainter& p, int x, int y, const QString& str,
	               bool changed);

	int frameL, frameR, frameT, frameB;
	unsigned char flags;
	unsigned char flagsChanged;
};

#endif // FLAGSVIEWER_H
