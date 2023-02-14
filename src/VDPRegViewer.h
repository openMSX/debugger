#ifndef VDPREGVIEWER_H
#define VDPREGVIEWER_H

#include "SimpleHexRequest.h"
#include "ui_VDPRegViewer.h"
#include <QDialog>
#include <cstdint>


class InteractiveButton;

/** See remarks for the highlightDispatcher in VDPStatusRegViewer.h :-)
  */
class buttonHighlightDispatcher : public QObject
{
	Q_OBJECT
public:
	buttonHighlightDispatcher();

	void receiveState(bool state);

signals:
	void stateDispatched(bool state);

private:
	int counter;
};


class VDPRegViewer : public QDialog, public SimpleHexRequestUser,
	             private Ui::VDPRegisters
{
	Q_OBJECT
public:
	VDPRegViewer(QWidget* parent = nullptr);

	void refresh();
	void registerBitChanged(int reg, int bit, bool state);

	//quick hack while no auto-detection...
	void on_VDPcomboBox_currentIndexChanged(int index);

private:
	void decodeVDPRegs();
	void decodeStatusVDPRegs();
	void setRegisterVisible(int r, bool visible);

	void connectHighLights();

	void doConnect(InteractiveButton* lab, buttonHighlightDispatcher* dis);
	buttonHighlightDispatcher* makeGroup(
		const QList<InteractiveButton*>&, InteractiveLabel*);
	void reGroup(InteractiveButton*, buttonHighlightDispatcher*);
	void monoGroup(InteractiveButton*, InteractiveLabel*);

	void DataHexRequestReceived() override;

private:
	uint8_t regs[64 + 16 + 2 + 4] = {};
	buttonHighlightDispatcher* modeBitsDispat;
	int vdpId;
};

#endif /* VDPSTATUSREGVIEWER_H */
