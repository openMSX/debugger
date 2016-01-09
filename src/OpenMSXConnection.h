#ifndef OPENMSXCONNECTION_HH
#define OPENMSXCONNECTION_HH

#include <QObject>
#include <QAbstractSocket>
#include <QXmlDefaultHandler>
#include <QQueue>
#include <memory>

class QXmlInputSource;
class QXmlSimpleReader;

class Command
{
public:
	virtual ~Command() {}

	virtual QString getCommand() const = 0;
	virtual void replyOk (const QString& message) = 0;
	virtual void replyNok(const QString& message) = 0;
	virtual void cancel() = 0;
};

class SimpleCommand : public QObject, public Command
{
	Q_OBJECT
public:
	SimpleCommand(const QString& command);
	virtual QString getCommand() const;
	virtual void replyOk (const QString& message);
	virtual void replyNok(const QString& message);
	virtual void cancel();
	
signals:
	void replyStatusOk(bool status);

private:
	QString command;
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

class OpenMSXConnection : public QObject, private QXmlDefaultHandler
{
	Q_OBJECT
public:
	/** require: socket must be in connected state */
	OpenMSXConnection(QAbstractSocket* socket);
	~OpenMSXConnection();

	void sendCommand(Command* command);

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

	// QXmlDefaultHandler
	bool fatalError(const QXmlParseException& exception);
	bool startElement(const QString& namespaceURI, const QString& localName,
	                  const QString& qName, const QXmlAttributes& atts);
	bool endElement(const QString& namespaceURI, const QString& localName,
	                const QString& qName);
	bool characters(const QString& ch);

	//std::unique_ptr<QAbstractSocket> socket;
	QAbstractSocket* socket;
	std::unique_ptr<QXmlInputSource> input;
	std::unique_ptr<QXmlSimpleReader> reader;

	QString xmlData;
	QXmlAttributes xmlAttrs;
	QQueue<Command*> commands;
	bool connected;
};

#endif
