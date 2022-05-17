#ifndef CONNECTDIALOG_HH
#define CONNECTDIALOG_HH

#include "OpenMSXConnection.h"
#include "ui_ConnectDialog.h"
#include <QDialog>
#include <QList>
#include <memory>
#include <vector>

class QString;
class ConnectionInfoRequest;

class ConnectDialog : public QDialog
{
	Q_OBJECT
public:
	static std::unique_ptr<OpenMSXConnection> getConnection(QWidget* parent = nullptr);

private:
	ConnectDialog(QWidget* parent);
	~ConnectDialog() override;

	void clear();
	void connectionOk(OpenMSXConnection& connection,
	                  const QString& title);
	void connectionBad(OpenMSXConnection& connection);

	void timerEvent(QTimerEvent *event) override;

	void on_connectButton_clicked();
	void on_rescanButton_clicked();

private:
	int delay;
	Ui::ConnectDialog ui;
	using OpenMSXConnections = std::vector<std::unique_ptr<OpenMSXConnection>>;
	OpenMSXConnections pendingConnections;
	OpenMSXConnections confirmedConnections;
	std::unique_ptr<OpenMSXConnection> result;
	QList<ConnectionInfoRequest*> connectionInfos;

	friend class ConnectionInfoRequest;
};

// Command handler to get initial info from new openMSX connections

class ConnectionInfoRequest : public QObject, private CommandBase
{
	Q_OBJECT
public:
	ConnectionInfoRequest(ConnectDialog& dialog, OpenMSXConnection& connection);

	QString getCommand() const override;
	void replyOk (const QString& message) override;
	void replyNok(const QString& message) override;
	void cancel() override;

private:
	void terminate();

private:
	ConnectDialog& dialog;
	OpenMSXConnection& connection;
	enum State { GET_MACHINE, GET_TITLE, DONE } state;
	QString title;
};

#endif
