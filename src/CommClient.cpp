// $Id$

#include "CommClient.h"
#include "OpenMSXConnection.h"

CommClient::CommClient()
	: connection(NULL)
{
}

CommClient::~CommClient()
{
	closeConnection();
}

CommClient& CommClient::instance()
{
	static CommClient oneInstance;
	return oneInstance;
}

void CommClient::connectToOpenMSX(OpenMSXConnection* conn)
{
	closeConnection();
	connection = conn;
	connect(connection, SIGNAL(disconnected()), SLOT(closeConnection()));
	connect(connection,
	        SIGNAL(logParsed(const QString&, const QString&)),
	        SIGNAL(logParsed(const QString&, const QString&)));
	connect(connection,
	        SIGNAL(updateParsed(const QString&, const QString&, const QString&)),
	        SIGNAL(updateParsed(const QString&, const QString&, const QString&)));
	emit connectionReady();
}

void CommClient::closeConnection()
{
	if (connection) {
		OpenMSXConnection* tmp = connection;
		connection = NULL;
		delete tmp; // triggers closeConnection()
		emit connectionTerminated();
	}
}

void CommClient::sendCommand(Command* command)
{
	if (connection) {
		connection->sendCommand(command);
	} else {
		command->cancel();
	}
}
