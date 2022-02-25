#ifndef OPENMSXCONNECTION_HH
#define OPENMSXCONNECTION_HH

#include <QObject>
#include <QAbstractSocket>
#include <QXmlStreamAttributes>
#include <QQueue>
#include <memory>
#include <functional>

class QXmlStreamReader;

class CommandBase
{
public:
	virtual ~CommandBase() = default;

	virtual QString getCommand() const = 0;
	virtual void replyOk (const QString& message) = 0;
	virtual void replyNok(const QString& message) = 0;
	virtual void cancel() = 0;
};

class SimpleCommand : public QObject, public CommandBase
{
	Q_OBJECT
public:
	SimpleCommand(const QString& command);
	QString getCommand() const override;
	void replyOk (const QString& message) override;
	void replyNok(const QString& message) override;
	void cancel() override;

signals:
	void replyStatusOk(bool status);

private:
	QString command;
};

class Command : public CommandBase
{
public:
	Command(const QString& command,
		std::function <void (const QString&)> okCallback,
		std::function <void (const QString&)> errorCallback = nullptr);

	QString getCommand() const override;

	void replyOk (const QString& message) override;
	void replyNok(const QString& message) override;
	void cancel() override;

private:
	QString command;
	std::function <void (const QString&)> okCallback;
	std::function <void (const QString&)> errorCallback;
};

class ReadDebugBlockCommand : public SimpleCommand
{
public:
	ReadDebugBlockCommand(const QString& commandString, unsigned size,
	                      unsigned char* target);
	ReadDebugBlockCommand(const QString& debuggable, unsigned offset, unsigned size,
	                      unsigned char* target);
protected:
	void copyData(const QString& message);
private:
	unsigned size;
	unsigned char* target;
};

class WriteDebugBlockCommand : public SimpleCommand
{
public:
	WriteDebugBlockCommand(const QString& debuggable, unsigned offset, unsigned size,
	                      unsigned char* source);
};

class OpenMSXConnection : public QObject
{
	Q_OBJECT
public:
	/** require: socket must be in connected state */
	OpenMSXConnection(QAbstractSocket* socket);
	~OpenMSXConnection() override;

	void sendCommand(CommandBase* command);

signals:
	void disconnected();
	void logParsed(const QString& level, const QString& message);
	void updateParsed(const QString& type, const QString& name, const QString& message);

private slots:
	void processData();
	void socketStateChanged(QAbstractSocket::SocketState state);
	void socketError(QAbstractSocket::SocketError state);

private:
	void cleanup();
	void cancelPending();

	bool startElement(const QStringRef& qName, const QXmlStreamAttributes& atts);
	bool endElement(const QStringRef& qName);
	bool characters(const QStringRef& ch);

	//std::unique_ptr<QAbstractSocket> socket;
	QAbstractSocket* socket;
	std::unique_ptr<QXmlStreamReader> reader;

	QString xmlData;
	QXmlStreamAttributes xmlAttrs;
	QQueue<CommandBase*> commands;
	bool connected;
};

#endif
