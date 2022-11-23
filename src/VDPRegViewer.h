#ifndef VDPREGVIEWER_H
#define VDPREGVIEWER_H

#include "SimpleHexRequest.h"
#include "ui_VDPRegistersExplained.h"
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

	//override auto-detection or in case of older openMSX...
	void on_VDPcomboBox_currentIndexChanged(int index);
	void on_VDPVersionChanged(QString VDPversion);

private slots:
	void on_cb_useOpenMSXVDP_stateChanged(int arg1);

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
	uint8_t regs[64 + 16 + 2];
	buttonHighlightDispatcher* modeBitsDispat;
	int vdpId;
	void VDPVersion_to_combobox(QString VDPversion);

};

#endif /* VDPSTATUSREGVIEWER_H */
