#include "SlotViewer.h"
#include "DebuggerData.h"
#include "OpenMSXConnection.h"
#include "CommClient.h"
#include <QPainter>
#include <QPaintEvent>
#include <QStyleOptionHeader>


class DebugMemMapperHandler : public SimpleCommand
{
public:
	DebugMemMapperHandler(SlotViewer& viewer_)
		: SimpleCommand("debug_memmapper")
		, viewer(viewer_)
	{
	}

	void replyOk(const QString& message) override
	{
		viewer.updateSlots(message);
		delete this;
	}
private:
	SlotViewer& viewer;
};


SlotViewer::SlotViewer(QWidget* parent)
	: QFrame(parent)
{
	setFrameStyle(WinPanel | Sunken);
	setFocusPolicy(Qt::StrongFocus);
	setBackgroundRole(QPalette::Base);
	setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum));

	memLayout = nullptr;
	for (int p = 0; p < 4; ++p) {
		slotsChanged[p] = false;
		segmentsChanged[p] = false;
	}

	frameR = frameL = frameT = frameB = frameWidth();

	headerSize1  = 8 + fontMetrics().horizontalAdvance("Page");
	headerSize2  = 8 + fontMetrics().horizontalAdvance("Address");
	headerSize3  = 8 + fontMetrics().horizontalAdvance("Slot");
	headerSize4  = 8 + fontMetrics().horizontalAdvance("Segment");
	headerHeight = 8 + fontMetrics().height();
}

void SlotViewer::resizeEvent(QResizeEvent* e)
{
	QFrame::resizeEvent(e);
}

void SlotViewer::paintEvent(QPaintEvent* e)
{
	// call parent for drawing the actual frame
	QFrame::paintEvent(e);

	QPainter p(this);

	// calc and set drawing bounds
	QRect r(e->rect());
	if (r.left() < frameL) r.setLeft(frameL);
	if (r.top()  < frameT) r.setTop (frameT);
	if (r.right()  > width()  - frameR - 1) r.setRight (width()  - frameR - 1);
	if (r.bottom() > height() - frameB - 1) r.setBottom(height() - frameB - 1);
	p.setClipRect(r);

	// redraw background
	p.fillRect(r, palette().color(QPalette::Base));

	QStyleOptionHeader so;
	so.init(this);
	so.state |= QStyle::State_Raised;
	so.orientation = Qt::Horizontal;
	so.position = QStyleOptionHeader::Beginning;
	so.sortIndicator = QStyleOptionHeader::None;
	so.textAlignment = Qt::AlignHCenter;
	so.rect.setTop(frameT);
	so.rect.setHeight(headerHeight);
	so.rect.setLeft(frameL);
	so.rect.setWidth(headerSize1);
	so.section = 0;
	so.text = "Page";
	style()->drawControl(QStyle::CE_Header, &so, &p, this);
	so.rect.setLeft(so.rect.left() + headerSize1);
	so.rect.setWidth(headerSize2);
	so.section = 1;
	so.text = "Address";
	style()->drawControl(QStyle::CE_Header, &so, &p, this);
	so.rect.setLeft(so.rect.left() + headerSize2);
	so.rect.setWidth(headerSize3);
	so.section = 2;
	so.text = "Slot";
	style()->drawControl(QStyle::CE_Header, &so, &p, this);
	so.rect.setLeft(so.rect.left() + headerSize3);
	so.rect.setWidth(headerSize4);
	so.section = 3;
	so.text = "Segment";
	style()->drawControl(QStyle::CE_Header, &so, &p, this);

	int mid1 = frameL + headerSize1 / 2;
	int mid2 = frameL + headerSize1 + headerSize2 / 2;
	int mid3 = frameL + headerSize1 + headerSize2 + headerSize3 / 2;
	int mid4 = frameL + headerSize1 + headerSize2 + headerSize3 + headerSize4 / 2;
	int dy = (height() - frameT - frameB - headerHeight) / 4;
	int y = frameT + headerHeight + dy / 2 + fontMetrics().height() / 2 -
	        fontMetrics().descent();

	int isOn = isEnabled() && memLayout != nullptr;
	for (int i = 0; i < 4; ++i) {
		QString str;
		p.setPen(palette().color(QPalette::Text));

		// print page nr
		str = QString("$%1").arg(i);
		p.drawText(mid1 - fontMetrics().horizontalAdvance(str) / 2, y, str);

		// print address
		str = QString("$%1").arg(i * 0x4000, 4, 16, QChar('0')).toUpper();
		p.drawText(mid2 - fontMetrics().horizontalAdvance(str) / 2, y, str);

		// print slot
		if (isOn) {
			if (memLayout->isSubslotted[memLayout->primarySlot[i]]) {
				str = QString("%1-%2").arg(memLayout->primarySlot[i]).arg(memLayout->secondarySlot[i]);
			} else {
				str = QString::number(memLayout->primarySlot[i]);
			}
		} else {
			str = "-";
		}
		// set pen colour to red if slot was recently changed
		if (slotsChanged[i] && isOn) {
			p.setPen(Qt::red);
		} else {
			p.setPen(palette().color(QPalette::Text));
		}

		p.drawText(mid3 - fontMetrics().horizontalAdvance(str) / 2, y, str);

		// print segment
		if (isOn) {
			int ms;
			if (memLayout->isSubslotted[memLayout->primarySlot[i] & 3]) {
				ms = memLayout->mapperSize[memLayout->primarySlot[i] & 3]
				                          [memLayout->secondarySlot[i] & 3];
			} else {
				ms = memLayout->mapperSize[memLayout->primarySlot[i] & 3][0];
			}
			if (ms > 0) {
				str = QString("%1").arg(memLayout->mapperSegment[i]);
			} else if (memLayout->romBlock[2*i] >= 0) {
				if (memLayout->romBlock[2*i] == memLayout->romBlock[2*i+1]) {
					str = QString("R%1").arg(memLayout->romBlock[2*i]);
				} else {
					str = QString("R%1/%2").arg(memLayout->romBlock[2*i]).arg(memLayout->romBlock[2*i+1]);
				}
			} else {
				str = "-";
			}
		} else {
			str = "-";
		}
		// set pen colour to red if slot was recently changed
		if (segmentsChanged[i] && isOn) {
			p.setPen(Qt::red);
		} else {
			p.setPen(palette().color(QPalette::Text));
		}
		p.drawText(mid4 - fontMetrics().horizontalAdvance(str) / 2, y, str);

		y += dy;
	}
}

QSize SlotViewer::sizeHint() const
{
	return {headerSize1 + headerSize2 + headerSize3 + headerSize4 + frameL + frameR,
	        headerHeight + 4*fontMetrics().height()};
}

void SlotViewer::refresh()
{
	CommClient::instance().sendCommand(new DebugMemMapperHandler(*this));
}

void SlotViewer::setMemoryLayout(MemoryLayout* ml)
{
	memLayout = ml;
}

void SlotViewer::updateSlots(const QString& message)
{
	QStringList lines = message.split('\n');
	bool changed = false;

	// parse page slots and segments
	for (int p = 0; p < 4; ++p) {
		bool subSlotted = (lines[p * 2][1] != 'X');
		slotsChanged[p] = (memLayout->primarySlot  [p] != lines[p * 2][0].toLatin1()-'0') ||
		                  (memLayout->secondarySlot[p] != (subSlotted ? lines[p * 2][1].toLatin1() - '0' : -1));
		changed |= slotsChanged[p];
		memLayout->primarySlot  [p] = lines[p * 2][0].toLatin1()-'0';
		memLayout->secondarySlot[p] = subSlotted
			? lines[p * 2][1].toLatin1() - '0' : -1;
		segmentsChanged[p] = memLayout->mapperSegment[p] !=
		                     lines[p * 2 + 1].toInt();
		memLayout->mapperSegment[p] = lines[p * 2 + 1].toInt();
	}
	// parse slot layout
	int l = 8;
	for (int ps = 0; ps < 4; ++ps) {
		memLayout->isSubslotted[ps] = lines[l++][0] == '1';
		if (memLayout->isSubslotted[ps]) {
			for (int ss = 0; ss < 4; ++ss) {
				memLayout->mapperSize[ps][ss] = lines[l++].toUShort();
			}
		} else {
			memLayout->mapperSize[ps][0] = lines[l++].toUShort();
		}
	}
	// parse rom blocks
	for (int i = 0; i < 8; ++i, ++l) {
		if (lines[l][0] == 'X')
			memLayout->romBlock[i] = -1;
		else
			memLayout->romBlock[i] = lines[l].toInt();
	}
	update();
	if (changed) emit slotsUpdated();
}
