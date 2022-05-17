#include "OpenMSXConnection.h"
#include <QXmlStreamReader>
#include <cassert>


void SimpleCommand::replyOk (const QString& /*message*/)
{
	delete this;
}

void SimpleCommand::replyNok(const QString& /*message*/)
{
	cancel();
}

void SimpleCommand::cancel()
{
	delete this;
}


Command::Command(QString command_,
                 std::function<void(const QString&)> okCallback_,
                 std::function<void(const QString&)> errorCallback_)
	: command(std::move(command_))
	, okCallback(std::move(okCallback_))
	, errorCallback(std::move(errorCallback_))
{
}

void Command::replyOk(const QString& message)
{
	okCallback(message);
	delete this;
}

void Command::replyNok(const QString& message)
{
	if (errorCallback != nullptr)
		errorCallback(message);
	cancel();
}

QString Command::getCommand() const
{
	return command;
}

void Command::cancel()
{
	delete this;
}

static QString createDebugCommand(const QString& debuggable,
		unsigned offset, unsigned size)
{
	return QString("debug_bin2hex [ debug read_block %1 %2 %3 ]")
	               .arg(debuggable).arg(offset).arg(size);
}

ReadDebugBlockCommand::ReadDebugBlockCommand(const QString& commandString,
		unsigned size_, unsigned char* target_)
	: SimpleCommand(commandString)
	, size(size_), target(target_)
{
}

ReadDebugBlockCommand::ReadDebugBlockCommand(const QString& debuggable,
		unsigned offset, unsigned size_, unsigned char* target_)
	: SimpleCommand(createDebugCommand(debuggable, offset, size_))
	, size(size_), target(target_)
{
}

static QString createDebugWriteCommand(const QString& debuggable,
		unsigned offset, unsigned size, unsigned char *data)
{
	QString cmd = QString("debug write_block %1 %2 [ debug_hex2bin \"")
	                  .arg(debuggable).arg(offset);
	for (unsigned i = offset; i < offset + size; ++i) {
		cmd += QString("%1").arg(int(data[i]), 2, 16, QChar('0')).toUpper();
	}
	cmd += "\" ]";
	return cmd;
}
WriteDebugBlockCommand::WriteDebugBlockCommand(const QString& debuggable,
		unsigned offset, unsigned size_, unsigned char* source_)
	: SimpleCommand(createDebugWriteCommand(debuggable, offset, size_, source_))
{
}


static unsigned char hex2val(char c)
{
	return (c <= '9') ? (c - '0') : (c - 'A' + 10);
}
void ReadDebugBlockCommand::copyData(const QString& message)
{
	assert(static_cast<unsigned>(message.size()) == 2 * size);
	for (unsigned i = 0; i < size; ++i) {
		target[i] = (hex2val(message[2 * i + 0].toLatin1()) << 4) +
		            (hex2val(message[2 * i + 1].toLatin1()) << 0);
	}
}


OpenMSXConnection::OpenMSXConnection(QAbstractSocket* socket_)
	: socket(socket_)
	, reader(new QXmlStreamReader())
	, connected(true)
{
	assert(socket->isValid());

	connect(socket, &QAbstractSocket::readyRead,
	        this, &OpenMSXConnection::processData);
	connect(socket, &QAbstractSocket::stateChanged,
	        this, &OpenMSXConnection::socketStateChanged);
	connect(socket, &QAbstractSocket::errorOccurred,
	        this, &OpenMSXConnection::socketError);

	socket->write("<openmsx-control>\n");
}

OpenMSXConnection::~OpenMSXConnection()
{
	cleanup();
	assert(commands.empty());
	assert(!connected);
	socket->deleteLater();
}

void OpenMSXConnection::sendCommand(CommandBase* command)
{
	assert(command);
	if (connected && socket->isValid()) {
		commands.enqueue(command);
		QString cmd = "<command>" + command->getCommand() + "</command>";
		socket->write(cmd.toUtf8());
	} else {
		command->cancel();
	}
}

void OpenMSXConnection::cleanup()
{
	if (!connected) return;

	connected = false;
	if (socket->isValid()) {
		socket->disconnect(this);
		socket->write("</openmsx-control>\n");
		socket->disconnectFromHost();
	}
	cancelPending();
	emit disconnected();
}

void OpenMSXConnection::cancelPending()
{
	assert(!connected);
	while (!commands.empty()) {
		CommandBase* command = commands.dequeue();
		command->cancel();
	}
}

void OpenMSXConnection::socketStateChanged(QAbstractSocket::SocketState state)
{
	if (state != QAbstractSocket::ConnectedState) {
		cleanup();
	}
}

void OpenMSXConnection::socketError(QAbstractSocket::SocketError /*state*/)
{
	cleanup();
}


void OpenMSXConnection::processData()
{
	if (!reader->device()) {
		reader->setDevice(socket);
	}

	while (!reader->atEnd()) {
		reader->readNext();
		if (reader->isStartElement()) {
			startElement(reader->name(), reader->attributes());
		} else if (reader->isEndElement()) {
			endElement(reader->name());
		} else if (reader->isCharacters()) {
			characters(reader->text());
		}
	}

	if (reader->hasError() && reader->error() != QXmlStreamReader::PrematureEndOfDocumentError) {
		qWarning("Fatal error on line %lli, column %lli: %s",
				reader->lineNumber(), reader->columnNumber(),
				reader->errorString().toLatin1().data());
		cleanup();
	}
}

bool OpenMSXConnection::startElement(const QStringRef& /*qName*/, const QXmlStreamAttributes& atts)
{
	xmlAttrs = atts;
	xmlData.clear();
	return true;
}

bool OpenMSXConnection::endElement(const QStringRef& qName)
{
	if (qName == "openmsx-output") {
		// ignore
	} else if (qName == "reply") {
		if (connected) {
			CommandBase* command = commands.dequeue();
			if (xmlAttrs.value("result") == "ok") {
				command->replyOk (xmlData);
			} else {
				command->replyNok(xmlData);
			}
		} else {
			// still receive a reply while we're already closing
			// the connection, ignore it
		}
	} else if (qName == "log") {
		emit logParsed(xmlAttrs.value("level").toString(), xmlData);
	} else if (qName == "update") {
		emit updateParsed(xmlAttrs.value("type").toString(), xmlAttrs.value("name").toString(), xmlData);
	} else {
		qWarning("Unknown XML tag: %s", qName.toLatin1().data());
	}
	return true;
}

bool OpenMSXConnection::characters(const QStringRef& ch)
{
	xmlData += ch;
	return true;
}
