#ifndef VDPREGVIEWER_H
#define VDPREGVIEWER_H

#include "SimpleHexRequest.h"
#include "ui_VDPRegistersExplained.h"
#include <QDialog>


class InteractiveButton;

/** See remarks for the highlightDispatcher in VDPStatusRegViewer.h :-)
  */
class buttonHighlightDispatcher : public QObject
{
	Q_OBJECT
public:
	buttonHighlightDispatcher();

public slots:
	void receiveState(bool state);

signals:
	void dispatchState(bool state);

private:
	int counter;
};


class VDPRegViewer : public QDialog, public SimpleHexRequestUser,
	             private Ui::VDPRegisters
{
	Q_OBJECT
public:
	VDPRegViewer(QWidget* parent = nullptr);
	~VDPRegViewer() override;

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

	unsigned char* regs;
	buttonHighlightDispatcher* modeBitsDispat;
	int vdpid;

public slots:
	void refresh();
	void registerBitChanged(int reg, int bit, bool state);

	//quick hack while no autodetection...
	void on_VDPcomboBox_currentIndexChanged(int index);
};

#endif /* VDPSTATUSREGVIEWER_H */
