#include "CommClient.h"
#include "OpenMSXConnection.h"
#include <QDebug>

CommClient::~CommClient()
{
	closeConnection();
}

CommClient& CommClient::instance()
{
	static CommClient oneInstance;
	return oneInstance;
}

void CommClient::connectToOpenMSX(std::unique_ptr<OpenMSXConnection> conn)
{
	closeConnection();
	connection = std::move(conn);
	connect(connection.get(), &OpenMSXConnection::disconnected, this, &CommClient::closeConnection);
	connect(connection.get(), &OpenMSXConnection::logParsed,    this, &CommClient::logParsed);
	connect(connection.get(), &OpenMSXConnection::updateParsed, this, &CommClient::updateParsed);
	emit connectionReady();
}

void CommClient::closeConnection()
{
	if (connection) {
		connection.reset();
		emit connectionTerminated();
	}
}

void CommClient::sendCommand(CommandBase* command)
{
	if (connection) {
		//qDebug() << "CommClient::sendCommand(CommandBase* " << command << ")  connection available";
		connection->sendCommand(command);
	} else {
		//qDebug() << "CommClient::sendCommand(CommandBase* " << command << ")  connection NOT available";
		command->cancel();
	}
}
