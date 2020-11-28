#ifndef CONNECTDIALOG_HH
#define CONNECTDIALOG_HH

#include "OpenMSXConnection.h"
#include "ui_ConnectDialog.h"
#include <QDialog>
#include <QList>

class QString;
class ConnectionInfoRequest;

class ConnectDialog : public QDialog
{
	Q_OBJECT
public:
	static OpenMSXConnection* getConnection(QWidget* parent = nullptr);

private slots:
	void on_connectButton_clicked();
	void on_rescanButton_clicked();

private:
	ConnectDialog(QWidget* parent);
	~ConnectDialog() override;

	void clear();
	void connectionOk(OpenMSXConnection& connection,
	                  const QString& title);
	void connectionBad(OpenMSXConnection& connection);

	void timerEvent(QTimerEvent *event) override;

	int delay;
	Ui::ConnectDialog ui;
	using OpenMSXConnections = QList<OpenMSXConnection*>;
	OpenMSXConnections pendingConnections;
	OpenMSXConnections confirmedConnections;
	OpenMSXConnection* result;
	QList<ConnectionInfoRequest*> connectionInfos;

	friend class ConnectionInfoRequest;
};

// Command handler to get initial info from new openmsx connections

class ConnectionInfoRequest : public QObject, Command
{
	Q_OBJECT
public:
	ConnectionInfoRequest(ConnectDialog& dialog, OpenMSXConnection& connection);

	QString getCommand() const override;
	void replyOk (const QString& message) override;
	void replyNok(const QString& message) override;
	void cancel() override;

private:
	enum State { GET_MACHINE, GET_TITLE, DONE };

	ConnectDialog& dialog;
	OpenMSXConnection& connection;
	State state;
	QString title;

private slots:
	void terminate();
};

#endif
