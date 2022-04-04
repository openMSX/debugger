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
	void updateSlots(const QString& message);

	QSize sizeHint() const override;

public slots:
	void refresh();

private:
	void resizeEvent(QResizeEvent* e) override;
	void paintEvent(QPaintEvent* e) override;

	int frameL, frameR, frameT, frameB;
	int headerSize1, headerSize2, headerSize3, headerSize4;
	int headerHeight;

	MemoryLayout* memLayout;

	bool slotsChanged[4];
	bool segmentsChanged[4];

signals:
	void slotsUpdated(bool slotsChanged);
	void contentsChanged();
};

#endif // SLOTVIEWER_H
