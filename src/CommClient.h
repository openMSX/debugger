// $Id$

#ifndef _COMMCLIENT_H
#define _COMMCLIENT_H

#include <QString>
#include <QObject>

class OpenMSXConnection;
class Command;

class CommClient : public QObject
{
	Q_OBJECT
public:
	static CommClient& instance();

	void sendCommand(Command* command);

public slots:
	void connectToOpenMSX(OpenMSXConnection* conn);
	void closeConnection();

signals:
	void connectionReady();
	void connectionTerminated();

	void logParsed(const QString& level, const QString& message);
	void updateParsed(const QString& type, const QString& name, const QString& message);

private:
	CommClient();
	~CommClient();

	OpenMSXConnection* connection;
};

#endif    // _COMMCLIENT_H
