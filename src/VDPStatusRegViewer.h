#ifndef VDPSTATUSREGVIEWER_H
#define VDPSTATUSREGVIEWER_H

#include "SimpleHexRequest.h"
#include "ui_VDPStatusRegisters.h"
#include <QList>
#include <QDialog>
#include <cstdint>

/** The highlightDispatcher serves 2 purposes for the InteractiveLabel widgets
  * a) keep a correct state: Assume widget A and B are related (both are
  * highlighted at the same time) and they are touching each other. I don't
  * think that there is a guaranteed order in the enterEvent,leaveEvent, so if
  * we use the MouseOver signal to connect to the highlight slots of A and B
  * then this sequence might arise if our mouse moves from A to B and produce
  * a wrong result:
  * 	B.mouseOver(true); A.mouseOver(false) -> highlights are false for both A and B.
  *
  * b) serve as a central hub (star toplogy) to dispatch events from one widget
  * to all others. This way all InteractiveLabel widgets only need to connect
  * to this highlightDispatcher
  */
class highlightDispatcher : public QObject
{
	Q_OBJECT
public:
	highlightDispatcher();

	void receiveState(bool state);

signals:
	void stateDispatched(bool state);

private:
	int counter;
};

class VDPStatusRegViewer : public QDialog, public SimpleHexRequestUser,
                           private Ui::VDPStatusRegisters
{
	Q_OBJECT
public:
	VDPStatusRegViewer(QWidget* parent = nullptr);

	void refresh();

private:
	void decodeVDPStatusRegs();
	void connectHighLights();
	void doConnect(InteractiveLabel* lab, highlightDispatcher* dis);
	void makeGroup(QList<InteractiveLabel*> list, InteractiveLabel* explained);

	void DataHexRequestReceived() override;

private:
	uint8_t statusregs[16];
};

#endif // VDPSTATUSREGVIEWER_H
