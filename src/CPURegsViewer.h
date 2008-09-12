// $Id$

#ifndef CPUREGSVIEWER_H
#define CPUREGSVIEWER_H

#include <QFrame>

class QPaintEvent;

class CPURegsViewer : public QFrame
{
	Q_OBJECT;
public:
	CPURegsViewer(QWidget* parent = 0);

	void setData(unsigned char* datPtr);
	int readRegister(int id);

	QSize sizeHint() const;

protected:
	void resizeEvent(QResizeEvent* e);
	void paintEvent(QPaintEvent* e);
	void mousePressEvent(QMouseEvent *e);
	//void mouseMoveEvent(QMouseEvent *e);
	//void mouseReleaseEvent(QMouseEvent *e);
	void keyPressEvent(QKeyEvent *e);
	void focusOutEvent(QFocusEvent *e);
	bool event(QEvent *e);
	
private:
	// layout
	int frameL, frameR, frameT, frameB;
	int leftRegPos, leftValuePos, rightRegPos, rightValuePos;
	int rowHeight;

	int regs[16], regsCopy[16];
	bool regsModified[16];
	bool regsChanged[16];
	int cursorLoc;

	void drawValue( QPainter& p, int id, int x, int y );
	void setRegister( int id, int value );
	void getRegister( int id, unsigned char* data );
	void applyModifications();
	void cancelModifications();

signals:
	void registerChanged(int id, int value);
	void pcChanged(quint16);
	void flagsChanged(quint8);
	void spChanged(quint16);
};

#endif // CPUREGSVIEWER_H
