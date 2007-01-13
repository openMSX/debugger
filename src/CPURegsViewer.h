// $Id$

#ifndef CPUREGSVIEWER_H
#define CPUREGSVIEWER_H

#include <QFrame>

class QPaintEvent;


struct Z80Registers {
	quint16 AF,BC,DE,HL;
	quint16 AF2,BC2,DE2,HL2;
	quint16 IX,IY,PC,SP;
	quint8  I,R,IM,IFF;
};

struct Z80RegisterChanges {
	bool AF,BC,DE,HL;
	bool AF2,BC2,DE2,HL2;
	bool IX,IY,PC,SP;
	bool I,R,IM,IFF;
};

class CPURegsViewer : public QFrame
{
	Q_OBJECT;
public:
	CPURegsViewer(QWidget* parent = 0);

	void setData(unsigned char* datPtr);

	QSize sizeHint() const;

protected:
	void resizeEvent(QResizeEvent* e);
	void paintEvent(QPaintEvent* e);

private:
	int frameL, frameR, frameT, frameB;

	Z80Registers regs;
	Z80RegisterChanges regsChanged;

	void drawValue(QPainter& p, int x, int y, const QString& str, bool changed);

signals:
	void pcChanged(quint16);
	void flagsChanged(quint8);
	void spChanged(quint16);
};

#endif // CPUREGSVIEWER_H
