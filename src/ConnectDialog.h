#ifndef CONNECTDIALOG_HH
#define CONNECTDIALOG_HH

#include "ui_ConnectDialog.h"
#include <QDialog>
#include <QList>

class OpenMSXConnection;
class QString;

class ConnectDialog : public QDialog
{
	Q_OBJECT
public:
	static OpenMSXConnection* getConnection(QWidget* parent = 0);

private slots:
	void on_connectButton_clicked();
	void on_rescanButton_clicked();
	void on_launchButton_clicked();

private:
	ConnectDialog(QWidget* parent);
	~ConnectDialog();

	void clear();
	void connectionOk(OpenMSXConnection& connection,
	                  const QString& pid, const QString& title);
	void connectionBad(OpenMSXConnection& connection);
	friend class ConnectionInfoRequest;

	Ui::ConnectDialog ui;
	typedef QList<OpenMSXConnection*> OpenMSXConnections;
	OpenMSXConnections pendingConnections;
	OpenMSXConnections confirmedConnections;
	OpenMSXConnection* result;
};

#endif
