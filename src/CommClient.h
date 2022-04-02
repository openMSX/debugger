#ifndef COMMCLIENT_H
#define COMMCLIENT_H

#include "OpenMSXConnection.h"
#include <QObject>
#include <memory>

class CommandBase;
class QString;

class CommClient : public QObject
{
	Q_OBJECT
public:
	static CommClient& instance();

	void sendCommand(CommandBase* command);
	void connectToOpenMSX(std::unique_ptr<OpenMSXConnection> conn);

public slots:
	void closeConnection();

signals:
	void connectionReady();
	void connectionTerminated();

	void logParsed(const QString& level, const QString& message);
	void updateParsed(const QString& type, const QString& name, const QString& message);

private:
	CommClient() = default;
	~CommClient() override;

	std::unique_ptr<OpenMSXConnection> connection;
};

#endif // COMMCLIENT_H
