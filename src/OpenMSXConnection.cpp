// $Id$

#include "OpenMSXConnection.h"
#include <QXmlInputSource>
#include <QXmlSimpleReader>
#include <cassert>


SimpleCommand::SimpleCommand(const QString& command_)
	: command(command_)
{
}

QString SimpleCommand::getCommand() const
{
	return command;
}

void SimpleCommand::replyOk (const QString& message)
{
	emit replyStatusOk(true);
	delete this;
}

void SimpleCommand::replyNok(const QString& message)
{
	emit replyStatusOk(false);
	cancel();
}

void SimpleCommand::cancel()
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
		unsigned offset, unsigned size, unsigned char *data )
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
	, reader(new QXmlSimpleReader())
	, connected(true)
{
	assert(socket->isValid());
	reader->setContentHandler(this);
	reader->setErrorHandler(this);

	connect(socket, SIGNAL(readyRead()), this, SLOT(processData()));
	connect(socket, SIGNAL(stateChanged(QAbstractSocket::SocketState)),
	        this, SLOT(socketStateChanged(QAbstractSocket::SocketState)));
	connect(socket, SIGNAL(error(QAbstractSocket::SocketError)),
	        this, SLOT(socketError(QAbstractSocket::SocketError)));

	socket->write("<openmsx-control>\n");
}

OpenMSXConnection::~OpenMSXConnection()
{
	cleanup();
	assert(commands.empty());
	assert(!connected);
	socket->deleteLater();
}

void OpenMSXConnection::sendCommand(Command* command)
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
		socket->disconnect(this, SLOT(processData()));
		socket->disconnect(this, SLOT(socketStateChanged(QAbstractSocket::SocketState)));
		socket->disconnect(this, SLOT(socketError(QAbstractSocket::SocketError)));
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
		Command* command = commands.dequeue();
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
	if (input.get()) {
		// continue
		input->setData(socket->readAll());
		reader->parseContinue();
	} else {
		// first time
		input.reset(new QXmlInputSource());
		input->setData(socket->readAll());
		reader->parse(input.get(), true); // incremental parsing
	}
}

bool OpenMSXConnection::fatalError(const QXmlParseException& exception)
{
	qWarning("Fatal error on line %i, column %i: %s",
	         exception.lineNumber(), exception.columnNumber(),
	         exception.message().toLatin1().data());
	cleanup();
	return false;
}

bool OpenMSXConnection::startElement(
		const QString& /*namespaceURI*/, const QString& /*localName*/,
		const QString& /*qName*/, const QXmlAttributes& atts)
{
	xmlAttrs = atts;
	xmlData.clear();
	return true;
}

bool OpenMSXConnection::endElement(
		const QString& /*namespaceURI*/, const QString& /*localName*/,
		const QString& qName)
{
	if (qName == "openmsx-output") {
		// ignore
	} else if (qName == "reply") {
		if (connected) {
			Command* command = commands.dequeue();
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
		emit logParsed(xmlAttrs.value("level"), xmlData);
	} else if (qName == "update") {
		emit updateParsed(xmlAttrs.value("type"), xmlAttrs.value("name"), xmlData);
	} else {
		qWarning("Unknown XML tag: %s", qName.toLatin1().data());
	}
	return true;
}

bool OpenMSXConnection::characters(const QString& ch)
{
	xmlData += ch;
	return true;
}
