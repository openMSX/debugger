#ifndef CPUREGSVIEWER_H
#define CPUREGSVIEWER_H

#include <QFrame>
#include <array>

class QPaintEvent;

class CPURegsViewer : public QFrame
{
	Q_OBJECT
public:
	CPURegsViewer(QWidget* parent = nullptr);

	void setData(unsigned char* datPtr);
	int readRegister(int id);

	QSize sizeHint() const override;

private:
	void resizeEvent(QResizeEvent* e) override;
	void paintEvent(QPaintEvent* e) override;
	void mousePressEvent(QMouseEvent* e) override;
	//void mouseMoveEvent(QMouseEvent* e) override;
	//void mouseReleaseEvent(QMouseEvent* e) override;
	void keyPressEvent(QKeyEvent* e) override;
	void focusOutEvent(QFocusEvent* e) override;
	bool event(QEvent* e) override;

	// layout
	int frameL, frameR, frameT, frameB;
	int leftRegPos, leftValuePos, rightRegPos, rightValuePos;
	int rowHeight;

	std::array<int, 16> regs;
	std::array<int, 16> regsCopy;
	std::array<bool, 16> regsModified;
	std::array<bool, 16> regsChanged;
	int cursorLoc;

	void drawValue(QPainter& p, int id, int x, int y);
	void setRegister(int id, int value);
	void getRegister(int id, unsigned char* data);
	void applyModifications();
	void cancelModifications();

signals:
	void registerChanged(int id, int value);
	void pcChanged(uint16_t);
	void flagsChanged(quint8);
	void spChanged(quint16);
};

#endif // CPUREGSVIEWER_H
