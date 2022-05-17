#ifndef HEXVIEWER_H
#define HEXVIEWER_H

#include <QFrame>
#include <cstdint>
#include <vector>

class HexRequest;
class QScrollBar;
class QPaintEvent;

class HexViewer : public QFrame
{
	Q_OBJECT
public:
	HexViewer(QWidget* parent = nullptr);

	enum Mode { FIXED, FILL_WIDTH, FILL_WIDTH_POWEROF2 };

	void setDebuggable(const QString& name, int size);
	void setIsInteractive(bool enabled);
	void setUseMarker(bool enabled);
	void setIsEditable(bool enabled);

	void setDisplayMode(Mode mode);
	void setDisplayWidth(short width);

	QSize sizeHint() const override;

	void setLocation(int addr);
	void setTopLocation(int addr);
	void scrollBarChanged(int addr);
	void settingsChanged();
	void refresh();

signals:
	void locationChanged(int addr);

private:
	void wheelEvent(QWheelEvent* e) override;
	void resizeEvent(QResizeEvent* e) override;
	void paintEvent(QPaintEvent* e) override;
	void mousePressEvent(QMouseEvent* e) override;
	bool event(QEvent* e) override;
	void keyPressEvent(QKeyEvent* e) override;
	void focusInEvent(QFocusEvent* e) override;
	void focusOutEvent(QFocusEvent* e) override;

	void createActions();

	void setSizes();
	void hexdataTransfered(HexRequest* r);
	void transferCancelled(HexRequest* r);
	int coorToOffset(int x, int y) const;

	void changeWidth();

private:
	QScrollBar* vertScrollBar;
	QAction* fillWidthAction;
	QAction* fillWidth2Action;
	QAction* setWith8Action;
	QAction* setWith16Action;
	QAction* setWith32Action;

	int wheelRemainder = 0;

	// layout
	int frameL, frameR, frameT, frameB;
	int leftCharPos, leftValuePos, rightValuePos, rightCharPos;
	Mode displayMode = FILL_WIDTH;
	short horBytes = 16;
	int visibleLines, partialBottomLine;
	int lineHeight, xAddr, xData, xChar, dataWidth, charWidth, hexCharWidth;
	int addressLength = 4;

	// data
	QString debuggableName;
	std::vector<uint8_t> hexData;
	std::vector<uint8_t> previousHexData;
	int debuggableSize = 0;
	int hexTopAddress = 0;
	int hexMarkAddress = 0;
	bool waitingForData = false;
	bool highlitChanges = true;
	bool useMarker = false;
	bool isInteractive = false;
	bool isEditable = false;
	bool beingEdited = false;
	bool editedChars = false;
	bool hasFocus = false;
	int cursorPosition, editValue;

	friend class HexRequest;
};

#endif // HEXVIEWER_H
