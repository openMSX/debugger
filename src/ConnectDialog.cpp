#include "ConnectDialog.h"
#include <QProcess>
#include <QString>
#include <QDir>
#include <QFileInfo>
#include <QTcpSocket>
#include <QtAlgorithms>
#include <cassert>

#ifdef _WIN32
#include "SspiNegotiateClient.h"
#include "QAbstractSocketStreamWrapper.h"
#include <QHostAddress>
#include <fstream>
#include <winsock2.h>
using namespace openmsx;
#else
#include <pwd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <unistd.h>
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
	std::ifstream in(info.absoluteFilePath().toLatin1().data());
	in >> port;
	if (port != -1) {
		QHostAddress localhost(QHostAddress::LocalHost);
		socket = new QTcpSocket();
		socket->connectToHost(localhost, port);

		QAbstractSocketStreamWrapper stream(socket);
		SspiNegotiateClient client(stream);

		if (!socket->waitForConnected(1000) ||
			!client.Authenticate()) {
			delete socket;
			socket = NULL;
		}
	}
#else
	int sd = ::socket(AF_UNIX, SOCK_STREAM, 0);
	if (sd != -1) {
		sockaddr_un addr;
		addr.sun_family = AF_UNIX;
		strcpy(addr.sun_path, info.absoluteFilePath().toLatin1().data());
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
#ifdef _WIN32
	DWORD len = GetTempPathW(0, nullptr);
	assert(len > 0); // nothing we can do to recover this
	//VLA(wchar_t, bufW, (len+1));
	//wchar_t bufW[len+1];
	auto bufW = static_cast<wchar_t*>(_alloca(sizeof(wchar_t) * (len+1)));

	len = GetTempPathW(len, bufW);
	assert(len > 0); // nothing we can do to recover this
	QDir dir(QString::fromWCharArray(bufW, len));
#else
	QDir dir((getenv("TMPDIR")) ? getenv("TMPDIR") : QDir::tempPath());
#endif
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


// ConnectionInfoRequest class

ConnectionInfoRequest::ConnectionInfoRequest(
		ConnectDialog& dialog_, OpenMSXConnection& connection_)
	: dialog(dialog_), connection(connection_)
{
	state = GET_MACHINE;
	connection.sendCommand(this);
}

QString ConnectionInfoRequest::getCommand() const
{
	switch (state) {
	case GET_MACHINE:
		return "machine_info config_name";
	case GET_TITLE:
		return "guess_title";
	default:
		assert(false);
		return "";
	}
}

void ConnectionInfoRequest::replyOk(const QString& message)
{
	switch (state) {
	case GET_MACHINE:
		title = message;
		state = GET_TITLE;
		connection.sendCommand(this);
		break;
	case GET_TITLE: {
		if (!message.isEmpty()) {
			title += " (" + message + ")";
		}
		dialog.connectionOk(connection, title);
		state = DONE;
		connect(&connection, SIGNAL(disconnected()),
		        this, SLOT(terminate()));
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
}

void ConnectionInfoRequest::terminate()
{
	cancel();
}

// class ConnectDialog

OpenMSXConnection* ConnectDialog::getConnection(QWidget* parent)
{
	ConnectDialog dialog(parent);

	// delay for at most 500ms while checking the connections
	dialog.delay = 1;
	dialog.startTimer(500);
	while (!dialog.pendingConnections.empty() && dialog.delay) {
		qApp->processEvents(QEventLoop::AllEvents, 200);
	}

	// if there is only one valid connection, use it immediately,
	// otherwise execute the dialog.
	if (dialog.pendingConnections.empty() && dialog.confirmedConnections.size() == 1 ) {
		dialog.on_connectButton_clicked();
	} else {
		dialog.exec();
	}
	return dialog.result;
}

ConnectDialog::ConnectDialog(QWidget* parent)
	: QDialog(parent)
	, result(NULL)
{
	ui.setupUi(this);
	on_rescanButton_clicked();
}

ConnectDialog::~ConnectDialog()
{
	clear();
}

void ConnectDialog::timerEvent(QTimerEvent* event)
{
	killTimer(event->timerId());
	delay = 0;
}

void ConnectDialog::clear()
{
	ui.listConnections->clear();

	// First kill the infos. That will remove the disconnect signals.
	qDeleteAll(connectionInfos);
	connectionInfos.clear();
	qDeleteAll(pendingConnections);
	pendingConnections.clear();
	qDeleteAll(confirmedConnections);
	confirmedConnections.clear();
}

void ConnectDialog::on_connectButton_clicked()
{
	int row = ui.listConnections->currentRow();
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
		connectionInfos.append(new ConnectionInfoRequest(*this, *connection));
	}
}

void ConnectDialog::connectionOk(OpenMSXConnection& connection,
                                 const QString& title)
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

	new QListWidgetItem(title, ui.listConnections);

	if (ui.listConnections->count() == 1) {
		// automatically select first row
		ui.listConnections->setCurrentRow(0);
	}
}

void ConnectDialog::connectionBad(OpenMSXConnection& connection)
{
	OpenMSXConnections::iterator it = qFind(pendingConnections.begin(),
	                                        pendingConnections.end(),
	                                        &connection);
	if (it == pendingConnections.end()) {
		// was this connection established but terminated?
		int id = confirmedConnections.indexOf(&connection);
		if (id >= 0) {
			OpenMSXConnection* conn = confirmedConnections.takeAt(id);
			QListWidgetItem* item = ui.listConnections->takeItem(id);
			delete item;
			conn->deleteLater();
                }
		return;
	}
	pendingConnections.erase(it);
	connection.deleteLater();
}
