#ifndef SLOTVIEWER_H
#define SLOTVIEWER_H

#include <QFrame>

struct MemoryLayout;
class QString;

class SlotViewer : public QFrame
{
	Q_OBJECT
public:
	SlotViewer(QWidget* parent = nullptr);

	void setMemoryLayout(MemoryLayout* ml);

	QSize sizeHint() const override;

	void refresh();

signals:
	void slotsUpdated(bool slotsChanged);
	void contentsChanged();

private:
	void resizeEvent(QResizeEvent* e) override;
	void paintEvent(QPaintEvent* e) override;

private:
	int frameL, frameR, frameT, frameB;
	int headerSize1, headerSize2, headerSize3, headerSize4;
	int headerHeight;

	MemoryLayout* memLayout;

	bool slotsChanged[4];
	bool segmentsChanged[4];

signals:
//	void slotsUpdated(bool slotsChanged);
//	void contentsChanged();
};

#endif // SLOTVIEWER_H
