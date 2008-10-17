#ifndef VDPREGVIEWER_H
#define VDPREGVIEWER_H

#include <QDialog>
#include <QColor>
#include "OpenMSXConnection.h"
#include "CommClient.h"
#include "Settings.h"
#include "SimpleHexRequest.h"

#include "ui_VDPRegistersExplained.h"


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

class VDPRegViewer : public QDialog, public SimpleHexRequestUser, private Ui::VDPRegisters
{
	Q_OBJECT
public:
	VDPRegViewer( QWidget *parent = 0);
	~VDPRegViewer();

private:
	unsigned char* regs;

	void decodeVDPRegs();

	void connectHighLights();

	void doConnect( InteractiveButton* lab, buttonHighlightDispatcher* dis);
	void makeGroup(QList<InteractiveButton*>, InteractiveLabel*);
	void monoGroup(InteractiveButton*, InteractiveLabel*);

protected:
        virtual void DataHexRequestReceived();

public slots:
	void refresh();
	void registerBitChanged(int reg, int bit, bool state);
};

#endif /* VDPSTATUSREGVIEWER_H */
