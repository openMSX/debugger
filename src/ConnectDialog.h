// $Id$

#ifndef CONNECTDIALOG_HH
#define CONNECTDIALOG_HH

#include "ui_ConnectDialog.h"
#include "OpenMSXConnection.h"
#include <QDialog>
#include <QList>

class QString;
class ConnectionInfoRequest;

class ConnectDialog : public QDialog
{
	Q_OBJECT
public:
	static OpenMSXConnection* getConnection(QWidget* parent = 0);

protected:
	void timerEvent( QTimerEvent *event );

private slots:
	void on_connectButton_clicked();
	void on_rescanButton_clicked();

private:
	ConnectDialog(QWidget* parent);
	~ConnectDialog();

	void clear();
	void connectionOk(OpenMSXConnection& connection,
	                  const QString& title);
	void connectionBad(OpenMSXConnection& connection);
	friend class ConnectionInfoRequest;

	int delay;
	Ui::ConnectDialog ui;
	typedef QList<OpenMSXConnection*> OpenMSXConnections;
	OpenMSXConnections pendingConnections;
	OpenMSXConnections confirmedConnections;
	OpenMSXConnection* result;
	QList<ConnectionInfoRequest*> connectionInfos;
};

// Command handler to get initial info from new openmsx connections

class ConnectionInfoRequest : public QObject, Command
{
	Q_OBJECT
public:
	ConnectionInfoRequest(ConnectDialog& dialog, OpenMSXConnection& connection);

	virtual QString getCommand() const;
	virtual void replyOk (const QString& message);
	virtual void replyNok(const QString& message);
	virtual void cancel();

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
