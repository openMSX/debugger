#ifndef STACKVIEWER_H
#define STACKVIEWER_H

#include <QFrame>

class StackRequest;
class QScrollBar;
class QPaintEvent;

class StackViewer : public QFrame
{
	Q_OBJECT
public:
	StackViewer(QWidget* parent = nullptr);

	void setData(unsigned char* memPtr, int memLength);
	QSize sizeHint() const override;

public slots:
	void setLocation(int addr);
	void setStackPointer(quint16 addr);

private:
	void wheelEvent(QWheelEvent* e) override;
	void resizeEvent(QResizeEvent* e) override;
	void paintEvent(QPaintEvent* e) override;

	void setScrollBarValues();
	void memdataTransfered(StackRequest* r);
	void transferCancelled(StackRequest* r);

	QScrollBar* vertScrollBar;

	int wheelRemainder;

	int frameL, frameR, frameT, frameB;
	double visibleLines;
	bool waitingForData;

	int stackPointer;
	int topAddress;
	unsigned char* memory;
	int memoryLength;

	friend class StackRequest;
};

#endif // STACKVIEWER_H
