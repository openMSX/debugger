#ifndef VDPCOMMANDREGVIEWER_H
#define VDPCOMMANDREGVIEWER_H

#include "SimpleHexRequest.h"
#include "ui_VDPCommandRegisters.h"
#include <QDialog>


class view88to16 : public QObject
{
	Q_OBJECT
public:
	view88to16(QWidget* a, QWidget* b, QWidget* c)
	{
		rl = rh = rw = -1;
		disp_rl = disp_rh = 1;
		disp_rw = 2;
		w_rl = a;
		w_rh = b;
		w_rw = c;
	}

	int getRL() const { return rl; }
	int getRH() const { return rh; }
	int getRW() const { return rw; }

	void setDispRL(int mode) { disp_rl = mode; }
	void setDispRH(int mode) { disp_rh = mode; }
	void setDispRW(int mode) { disp_rw = mode; }

	void setWidgetRL(QWidget* wdgt) { w_rl = wdgt; }
	void setWidgetRH(QWidget* wdgt) { w_rh = wdgt; }
	void setWidgetRW(QWidget* wdgt) { w_rw = wdgt; }

public slots:
	void finishRH() { setRH(getWidgetText(w_rh)); }
	void finishRL() { setRL(getWidgetText(w_rl)); }
	void finishRW() { setRW(getWidgetText(w_rw)); }

	void setRH(const QString& newval)
	{
		bool ok;
		int val = newval.toInt(&ok, 0) & 0xFF;
		if (!ok || (val == rh)) return;

		rh = val;
		updaterw();
		updaterh();
	}
	void setRL(const QString& newval)
	{
		bool ok;
		int val = newval.toInt(&ok, 0) & 0xFF;
		if (!ok || (val == rl)) return;

		rl = val;
		updaterw();
		updaterl();
	}
	void setRW(const QString& newval)
	{
		//TODO: build a split-in-two method
		bool ok;
		int val = newval.toInt(&ok, 0) & 0xFFFF;
		if (!ok || (val == rw)) return;

		rw = val;
		updaterl();
		updaterh();
		updaterw();
	}

private:
	int rh;
	int rl;
	int rw;
	int disp_rw;
	int disp_rl;
	int disp_rh;
	QWidget* w_rh;
	QWidget* w_rl;
	QWidget* w_rw;

	QString convert(int val, int mode)
	{
		if (mode & 1) {
			return QString("0x%1").arg(val, 2, 16, QChar('0'));
		} else {
			return QString("%1").arg(val);
		}
	}

	QString getWidgetText(QWidget* wdg)
	{
		if (wdg == nullptr) return QString();
		if (auto* ql = dynamic_cast<QLabel*>(wdg)) {
			return ql->text();
		} else {
			auto* qe = dynamic_cast<QLineEdit*>(wdg);
			return qe->text();
		}
	}

	void updateWidget(QWidget* wdg, int val, int mode)
	{
		if (auto* ql = dynamic_cast<QLabel*>(wdg)) {
			ql->setText(convert(val, mode));
		} else {
			auto* qe = dynamic_cast<QLineEdit*>(wdg);
			qe->setText(convert(val, mode));
		}
	}

	void updaterw()
	{
		if (w_rw == nullptr) return;
		rw = rl + 256 * rh;
		updateWidget(w_rw, rw, disp_rw);
	}
	void updaterl()
	{
		if (w_rl == nullptr) return;
		rl = rw & 255;
		updateWidget(w_rl, rl, disp_rl);
	}
	void updaterh()
	{
		if (w_rh == nullptr) return;
		rh = (rw >> 8) & 255;
		updateWidget(w_rh, rh, disp_rh);
	}
};


class VDPCommandRegViewer : public QDialog, public SimpleHexRequestUser,
	                    private Ui::VDPCmdRegs
{
	Q_OBJECT
public:
	VDPCommandRegViewer(QWidget* parent = nullptr);
	~VDPCommandRegViewer() override;

private:
	void DataHexRequestReceived() override;
	void decodeR46(int val);
	void syncRegToCmd();

	unsigned char* regs;
	unsigned char* statusregs;
	view88to16* grp_l_sx;
	view88to16* grp_l_sy;
	view88to16* grp_l_dx;
	view88to16* grp_l_dy;
	view88to16* grp_l_nx;
	view88to16* grp_l_ny;
	view88to16* grp_sx;
	view88to16* grp_sy;
	view88to16* grp_dx;
	view88to16* grp_dy;
	view88to16* grp_nx;
	view88to16* grp_ny;
	int R46;

public slots:
	void refresh();
	void R45BitChanged(int);
	void on_lineEdit_r44_editingFinished();
	void on_lineEdit_r45_editingFinished();
	void on_lineEdit_r46_editingFinished();
	void on_comboBox_cmd_currentIndexChanged(int index);
	void on_comboBox_operator_currentIndexChanged(int index);
	void on_syncPushButton_clicked();
	void on_launchPushButton_clicked();
};

#endif /* VDPCOMMANDREGVIEWER_H */
