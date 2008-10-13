#ifndef VDPREGVIEWER_H
#define VDPREGVIEWER_H

#include <QDialog>
#include <QColor>
#include "OpenMSXConnection.h"
#include "CommClient.h"
#include "Settings.h"
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

class VDPRegViewer : public QDialog, private Ui::VDPRegisters
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

public slots:
	void refresh();
	void on_VDPDataStore_dataRefreshed();
};

#endif /* VDPSTATUSREGVIEWER_H */
