// $Id$

#include "ConnectDialog.h"
#include "OpenMSXConnection.h"
#include <QProcess>
#include <QHeaderView>
#include <QString>
#include <QDir>
#include <QFileInfo>
#include <QTcpSocket>
#include <QtAlgorithms>
#include <cassert>

#ifdef _WIN32
#include <fstream>
#else
#include <pwd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#endif


// Helper functions to setup a connection

static QString getUserName()
{
#ifdef _WIN32
	return "default";
#else
	struct passwd* pw = getpwuid(getuid());
	return pw->pw_name ? pw->pw_name : "";
#endif
}

static bool checkSocketDir(const QDir& dir)
{
	if (!dir.exists()) {
		return false;
	}
#ifndef _WIN32
	// only do permission and owner checks on *nix
	QFileInfo info(dir.absolutePath());
	if (info.ownerId() != getuid()) {
		return false;
	}
	int all = QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner |
	          QFile::ReadGroup | QFile::WriteGroup | QFile::ExeGroup |
	          QFile::ReadOther | QFile::WriteOther | QFile::ExeOther;
	int needed = QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner;
	if ((info.permissions() & all) != needed) {
		return false;
	}
#endif
	return true;
}

static bool checkSocket(const QFileInfo& info)
{
	if (!info.fileName().startsWith("socket.")) {
		// wrong name
		return false;
	}
#ifndef _WIN32
	// only do permission and owner checks on *nix
	int all = QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner |
	          QFile::ReadGroup | QFile::WriteGroup | QFile::ExeGroup |
	          QFile::ReadOther | QFile::WriteOther | QFile::ExeOther;
	int needed = QFile::ReadOwner | QFile::WriteOwner;
	if ((info.permissions() & all) != needed) {
		return false;
	}
	if (info.ownerId() != getuid()) {
		return false;
	}
#endif
	return true;
}

static void deleteSocket(const QFileInfo& info)
{
	QFile::remove(info.absoluteFilePath()); // ignore errors
	QDir dir;
	dir.rmdir(info.absolutePath()); // ignore errors
}

static OpenMSXConnection* createConnection(const QDir& dir, const QString& socketName)
{
	QFileInfo info(dir, socketName);
	if (!checkSocket(info)) {
		// invalid socket
		return NULL;
	}

	QAbstractSocket* socket = NULL;
#ifdef _WIN32
	int port = -1;
	std::ifstream in(info.absoluteFilePath().toAscii().data());
	in >> port;
	if (port != -1) {
		socket = new QTcpSocket();
		socket->connectToHost("localhost", port);
		if (!socket->waitForConnected(1000)) {
			delete socket;
			socket = NULL;
		}
	}
#else
	int sd = ::socket(AF_UNIX, SOCK_STREAM, 0);
	if (sd != -1) {
		sockaddr_un addr;
		addr.sun_family = AF_UNIX;
		strcpy(addr.sun_path, info.absoluteFilePath().toAscii().data()); 
		if (connect(sd, (sockaddr*)&addr, sizeof(addr)) != -1) {
			socket = new QTcpSocket();
			if (!socket->setSocketDescriptor(sd)) {
				// failed to wrap socket in QTcpSocket
				delete socket;
				socket = NULL;
				close(sd);
			}
		} else {
			// failed to connect to UNIX socket
			close(sd);
		}
	}

#endif
	if (socket) {
		return new OpenMSXConnection(socket);
	} else {
		// cannot connect, must be a stale socket, try to clean it up
		deleteSocket(socketName);
		return NULL;
	}
}

static void collectServers(QList<OpenMSXConnection*>& servers)
{
	QDir dir(QDir::tempPath());
	dir.cd("openmsx-" + getUserName());
	if (!checkSocketDir(dir)) {
		// no correct socket directory
		return;
	}
	QDir::Filters filters =
#ifdef _WIN32
		QDir::Files;  // regular files for win32
#else
		QDir::System; // sockets for *nix
#endif
	foreach (QString name, dir.entryList(filters)) {
		if (OpenMSXConnection* connection = createConnection(dir, name)) {
			servers.push_back(connection);
		}
	}
}


// Command handler to get initial info from new openmsx connections

class ConnectionInfoRequest : public Command
{
public:
	ConnectionInfoRequest(ConnectDialog& dialog, OpenMSXConnection& connection);

	virtual QString getCommand() const;
	virtual void replyOk (const QString& message);
	virtual void replyNok(const QString& message);
	virtual void cancel();

private:
	enum State { GET_PID, GET_TITLE };

	ConnectDialog& dialog;
	OpenMSXConnection& connection;
	State state;
	QString pid;
};

ConnectionInfoRequest::ConnectionInfoRequest(
		ConnectDialog& dialog_, OpenMSXConnection& connection_)
	: dialog(dialog_), connection(connection_)
{
	state = GET_PID;
	connection.sendCommand(this);
}

QString ConnectionInfoRequest::getCommand() const
{
	switch (state) {
	case GET_PID:
		return "pid";
	case GET_TITLE:
		return "openmsx_info time"; // TODO fix this
	default:
		assert(false);
		return "";
	}
}

void ConnectionInfoRequest::replyOk(const QString& message)
{
	switch (state) {
	case GET_PID:
		pid = message;
		state = GET_TITLE;
		connection.sendCommand(this);
		break;
	case GET_TITLE: {
		QString title = message;
		dialog.connectionOk(connection, pid, title);
		delete this;
		break;
	}
	default:
		assert(false);
	}
}

void ConnectionInfoRequest::replyNok(const QString& /*message*/)
{
	cancel();
}

void ConnectionInfoRequest::cancel()
{
	dialog.connectionBad(connection);
	delete this;
}


// class ConnectDialog

OpenMSXConnection* ConnectDialog::getConnection(QWidget* parent)
{
	ConnectDialog dialog(parent);
	dialog.exec();
	return dialog.result;
}

ConnectDialog::ConnectDialog(QWidget* parent)
	: QDialog(parent)
	, result(NULL)
{
	ui.setupUi(this);
	ui.table->verticalHeader()->hide();
	ui.table->horizontalHeader()->setStretchLastSection(true);
	ui.table->setColumnCount(2);
	on_rescanButton_clicked();
}

ConnectDialog::~ConnectDialog()
{
	clear();
}

void ConnectDialog::clear()
{
	ui.table->setRowCount(0);

	OpenMSXConnections pendingCopy(pendingConnections);
	OpenMSXConnections confirmedCopy(confirmedConnections);
	pendingConnections.clear();
	confirmedConnections.clear();
	qDeleteAll(pendingCopy);
	qDeleteAll(confirmedCopy);
}

void ConnectDialog::on_connectButton_clicked()
{
	int row = ui.table->currentRow();
	if (row != -1) {
		assert((0 <= row) && (row < confirmedConnections.size()));
		result = confirmedConnections[row];
		confirmedConnections.removeAt(row);
	}
	accept();
}

void ConnectDialog::on_rescanButton_clicked()
{
	clear();
	collectServers(pendingConnections);
	foreach (OpenMSXConnection* connection, pendingConnections) {
		new ConnectionInfoRequest(*this, *connection);
	}
}

void ConnectDialog::on_launchButton_clicked()
{
	QProcess::startDetached("openmsx");
}

void ConnectDialog::connectionOk(OpenMSXConnection& connection,
                                 const QString& pid, const QString& title)
{
	OpenMSXConnections::iterator it = qFind(pendingConnections.begin(),
	                                        pendingConnections.end(),
	                                        &connection);
	if (it == pendingConnections.end()) {
		// connection is already beiing destoyed
		return;
	}
	pendingConnections.erase(it);
	confirmedConnections.push_back(&connection);

	int row = ui.table->rowCount();
	QTableWidgetItem* pidItem = new QTableWidgetItem(pid);
	QTableWidgetItem* titleItem = new QTableWidgetItem(title);
	ui.table->insertRow(row);
	ui.table->setItem(row, 0, pidItem);
	ui.table->setItem(row, 1, titleItem);

	if (row == 0) {
		// automatically select first row
		ui.table->setCurrentItem(pidItem);
	}
}

void ConnectDialog::connectionBad(OpenMSXConnection& connection)
{
	OpenMSXConnections::iterator it = qFind(pendingConnections.begin(),
	                                        pendingConnections.end(),
	                                        &connection);
	if (it == pendingConnections.end()) {
		// connection is already beiing destoyed
		return;
	}
	pendingConnections.erase(it);
	delete &connection;
}
